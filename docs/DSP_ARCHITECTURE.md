# ARC — DSP Architecture

ARC has no oscillator → filter → amplifier chain. Each voice is a small physical system:
an **exciter** puts energy into a network of five coupled **waveguide resonators**, the
network rings, and the sound is picked up from the resonators. The network *is* the
oscillator.

```
ENERGY ──────────► MATTER ─────────────► INTERACTION ──────────► RESONANCE
exciter            material shapes        orthogonal scattering    stereo pickup of
(strike / pluck /  every loop (ratios,    between CORE, A, B,      the five loops →
 bow / air)        decay, dispersion)     C, D (coupling)          output stage
```

Detailed design documents: RESONATOR_DESIGN (one loop), NETWORK_COUPLING (the
scattering network and its compensation), EXCITERS, MATERIAL_SYSTEM, MOTION_SYSTEM
(drift, gestures, CHAOS, FREEZE), VOICE_ARCHITECTURE (allocation, MIDI, MPE).

## 1. Signal flow

```
                         ┌──────────────────────── ArcVoice (× up to 16 + 4 steal slots) ─┐
 MIDI ─► ArcEngine ──────┤                                                                 │
   (sample accurate)     │   Exciter ──force──► CORE loop ◄──┐                             │
                         │     ▲  (strike /                  │  Q(n): 5×5 orthogonal       │
                         │     │   pluck also                ▼  Cayley scattering          │
                         │     │   drive A–D)     A ◄──► B ◄──► D ◄──► C  (topology edges) │
                         │     └── CORE output (bow friction / air column feedback)       │
                         │                                                                 │
                         │   pickup: Σ level_i · pan_i · y_i  ──► voice gain / steal fade  │
                         └──────────────────────────────────────────┬──────────────────────┘
                                                                    ▼  Σ voices
                         OutputStage: width (M/S) → small FDN space → ADAA drive → master
                                      → DC blocker → safety soft-clip (≤ 0 dBFS) + NaN guard
```

Per sample and voice: every loop is read (`y_i`), the scattering matrix redistributes
the five signals (`u = Q y`), the exciter force is added at the CORE (and, for the
transient exciters, spread to the nodes by the injection weights) and the loops are
written. The bowed and blown exciters are *feedback* exciters: their friction and
air-column forces are computed from the CORE output and injected only at the CORE
(collocated), so they cannot pump energy into the other nodes. AIR's feed-forward
turbulence is still spread to the nodes. An earlier non-collocated version let a
resting bow make node B self-oscillate (EXCITERS "Collocation").

Each loop (`WaveguideResonator`) is a delay line with an exactly tuned first-order
allpass fractional delay, a two-point one-pole loss filter (frequency-dependent T60 with
a DC bound) and a chain of 2–6 dispersion allpasses whose phase delay is subtracted
from the loop length, so partials stretch while the fundamental stays exact.

## 2. Rates

| Rate | What runs | Where |
|---|---|---|
| Sample | loops, scattering, exciters, pickup, output stage | `ArcVoice::renderSpan`, `ResonantNetwork`, `OutputStage` |
| Control (QUALITY: 0.33 / 0.67 / 1.33 ms) | network redesign (frequencies, T60s, dispersion, selectivity), coupling generators → `Q`, coupling compensation (Schur complement per node), material morph, motion / gesture / CHAOS offsets, glide, bend, FREEZE governor, envelopes | `ArcEngine::updateGlobalControl`, `ArcVoice::updateControl` |
| Block | parameter snapshot (`ParamCache` → `EngineParams`), transport, gesture hand-over, telemetry publication | `ArcAudioProcessor::processBlock`, `ArcEngine::publishTelemetry` |
| Message thread | presets, RANDOM, state save / restore, gesture recording, editor (60 fps active / 15 fps idle) | `PresetManager`, `PluginEditor`, `ResonanceField` |

`Q(n)` is interpolated linearly between two orthogonal matrices across each control
period. A convex combination of orthogonal matrices has spectral norm ≤ 1, so
automation or motion can never make the scattering add energy (NETWORK_COUPLING §3).

Every voice keeps its **own control clock**: the processor splits the host block at
each MIDI event, and a split never causes an extra redesign. Consequence (measured,
`host` tests): the output is **bit-identical for host block sizes 1, 7, 64, 333, 1024
and 4096**.

## 3. QUALITY = control period

Profiling (callgrind, 8 bowed voices) showed per-sample DSP at ~40 % of the cost and
control-rate work (network redesign, compensation, per-voice scattering design) at ~55 %
with a 0.33 ms control period. QUALITY therefore sets the control period, and with it
the dispersion detail:

| QUALITY | control period | dispersion stages | 16 bowed voices, 48 kHz | spectrum vs HIGH | pitch vs HIGH |
|---|---|---|---|---|---|
| HIGH | 0.33 ms | 6 | 18.7 % of a core | — | — |
| NORMAL (default) | 0.67 ms | 4 | 11.8 % | 1.17 dB LSD | 0.011 cents |
| ECO | 1.33 ms | 2 | 8.3 % | 1.74 dB LSD | 0.033 cents |

(LSD = log-spectral distance over 1/6-octave bands of a bowed chord.) Control-period
constants are converted from seconds, so envelopes, smoothing and silence detection
behave the same in every mode. QUALITY switches live, at each voice's next control
boundary, without re-preparing: the largest sample step while switching every 250 ms
under a bowed chord is 0.0123 against 0.0121 for the steady chord (no click).

## 4. Threading model

| Data | From → to | Mechanism |
|---|---|---|
| Parameters | host / UI → audio | APVTS atomics, read once per block into `EngineParams` (plain copy) |
| Gestures (4 × 128 points) | message → audio | SpinLock-protected store + per-node serial; the audio thread only `tryLock`s (never waits) and picks the gesture up on the next block if the lock is busy |
| CHAOS / motion seed | message → audio | atomic value + flag |
| Telemetry (node energy, edge flux and strength, node geometry and ratios, meters, freeze, voices, strikes, tempo, heartbeat) | audio → UI | `std::atomic` with relaxed ordering, written once per block; meter peaks accumulate with `max` and are consumed with `exchange` so no peak is missed between frames |
| Preset / modified state | message → UI | published snapshot under a lock, message thread only |

No audio buffers cross threads, and the UI never calls into the engine.

## 5. Realtime safety

The claims below are instrumented, not assumed (`Tests/RealtimeTests.cpp`, TESTING.md):

* **No allocation on the audio thread.** The test binary replaces global `operator new`
  and counts allocations made while inside `processBlock`: **0 allocations in 640
  blocks**. The run includes MIDI notes, bend, pedal and pressure, preset changes,
  gestures, and automation of every structural parameter (topology, exciter, material,
  quality, voice mode, freeze, sync).
* **No blocking.** The only lock the audio thread touches is the gesture store, and only
  through a try-lock. There is no I/O and no logging.
* **No data races.** An audio thread renders continuously while the message thread loads
  presets, randomises, records gestures, saves and restores state and runs the editor at
  60 fps. Built with ThreadSanitizer this reports **0 warnings**, and the full suite
  under AddressSanitizer + UndefinedBehaviorSanitizer reports no errors.
* **Bounded output.** Loops are passive and scattering is orthogonal. On top of that,
  compensation authority is clamped to ±30 %, driven voices soft-limit edge rotations,
  and FREEZE has an energy governor. A non-finite voice is reset and counted; the output
  stage zeroes non-finite blocks and soft-clips below 0 dBFS. Denormals are disabled per
  block (`ScopedNoDenormals`) and the loop filters are DC bounded.

## 6. Stability strategy (summary)

1. **Passive loops.** The fractional-delay and dispersion allpasses have unit magnitude,
   and the loss filter has gain < 1 at every frequency.
2. **Lossless coupling.** Scattering is a Cayley transform of a skew-symmetric
   generator: orthogonal for any coupling, topology or edge order (measured
   ‖QᵀQ − I‖ ≤ 1.2e-7).
3. **Safe automation.** Changes are interpolated between orthogonal matrices (norm ≤ 1).
4. **Bounded compensation.** Tuning and damping compensation is bounded: ±30 %
   authority, under-relaxation 0.25, a coincidence taper near unison. This is the fix
   for the limit cycle the preset audit found.
5. **Limited feedback exciters.** Feedback forces are collocated at the CORE, and
   driven voices soft-limit their edge rotations (the "wolf-tone" limit).
6. **Fuzz tested.** 2000 random networks under abusive and realistic modulation stay
   bounded; so does maximum coupling with every topology.

## 7. CPU

Release build, % of one core of the build machine (4 vCPU cloud container) in real time,
NORMAL quality, COUPLING 0.35, CHAOS 0.1, held (bow) or re-struck (strike) notes
(`docs/measurements/release/cpu_profile.csv`, `Tests/PerformanceTests.cpp`):

| voices | 44.1 kHz strike / bow | 48 kHz strike / bow | 96 kHz strike / bow |
|---|---|---|---|
| 1 | 0.8 % / 0.8 % | 0.8 % / 0.9 % | 1.2 % / 1.3 % |
| 4 | 3.6 % / 2.8 % | 2.8 % / 3.2 % | 4.1 % / 4.7 % |
| 8 | 4.8 % / 6.2 % | 5.4 % / 6.2 % | 8.0 % / 8.6 % |
| 16 | 9.4 % / 10.8 % | 10.6 % / 12.3 % | 16.5 % / 20.0 % |

Single runs on a shared cloud machine vary by about ±10 % (e.g. 4 strike voices at
44.1 kHz).

Extremes at 48 kHz, 16 voices (strike / bow): COUPLING 0 → 9.0 % / 10.7 %, COUPLING 1 →
9.8 % / 11.2 %, CHAOS 1 → 12.2 % / 12.6 %, MOTION 1 → 12.2 % / 11.7 %. CHAOS and
MOTION cost up to ~2 points because their offsets change the network every control
period. The worst case measured anywhere is 20.0 % (16 bowed voices at 96 kHz).

Inactive systems cost nothing: idle voices are skipped, voices end ~21 ms after they
fall below −100 dB, the space and drive stages are bypassed at 0, and the editor drops
to 15 fps when the engine is silent. The editor does not slow the audio thread (8
bowed voices: 8.8 % of a core with the editor closed, 8.9 % with it open and animating —
within run-to-run noise; see UI_SYSTEM §7).

# ARC — Motion System

MOTION makes the *network itself* move: the four outer nodes drift around the Resonance
Field, recorded gestures replay node paths, CHAOS adds bounded irregularity and FREEZE
suspends the network's energy. Everything here runs at control rate (every 16 samples at
48 kHz) on the audio thread, allocation-free, and produces **offsets that are added to
the node parameters** — automation and motion never overwrite each other.

Sources: `Source/Motion/` (MotionEngine, Gesture, RandomWalk), `Source/Nonlinear/ChaosEngine.h`,
FREEZE in `Source/Engine/ArcEngine.cpp` / `ArcVoice.cpp`. Tests: `Tests/MotionTests.cpp`.

## 1. Autonomous drift (MOTION depth / rate)

Per node and axis (radius, angle): `0.55 · sine + 0.45 · random walk`.

* The eight sines run at **irrational rate ratios** (1, 0.7548, 1.3247, 0.5698, 1.1892,
  0.8409, 0.6180, 1.4142) with golden-ratio phase offsets, so the network drifts rather
  than wobbling in lock-step.
* The random walks (`RandomWalk`) are smoothed, bounded, seeded (deterministic recall).
* Depth scaling: radius ± 0.14 · depth (± 0.28 octave of node tuning at full depth),
  angle ± 0.45 rad · depth.

**SYNC.** The rate becomes one cycle per musical division (1/4, 1/2, 1, 2, 4, 8 bars) and
the eight sines use musical ratios (1, ½, 2, ¼ …). While the host plays, the phase is a
function of the song position (ppq), and the walk component is dropped — the drift is
purely periodic: **the same bar always moves the network the same way**, and relocating
the playhead lands on the same shape. Measured (`motion/synced drift is phase-locked…`):
identical radius trajectories 4 bars apart (max difference 0), 0.198 apart half a bar
later. When the host is stopped, synced motion free-runs at the host (or 120 bpm) tempo.

All offsets pass a 40 ms one-pole (angles along the shorter arc), so transport jumps,
SYNC toggles and gesture replacement never step a node's tuning.

## 2. Gestures (RECORD MOTION)

A gesture is the path a node was dragged along, relative to where the drag started.

* **Capture** (UI): raw pointer samples `(time, radius, angle)` while recording.
* **Build** (`buildGesture`): angle unwrapped → uniform resampling to 128 points over the
  recorded time → 5-tap binomial smoothing (pointer jitter) → **loop closure**: the last
  12 % cross-fades back to the start, *modulo whole turns* — a drag that circles the core
  becomes a seamless orbit instead of unwinding (measured: max step 0.0054 rad per
  sample across the wrap, exactly 2 turns over 2 cycles) → offsets bounded (radius ± 1,
  angle wrapped to (−π, π]).
* **Tempo awareness**: with SYNC on, the loop length snaps to the nearest musical
  division (in beats) and playback follows the host timeline (measured periods 2.000 s
  at 120 bpm, 2.667 s at 90 bpm for a 1-bar gesture); otherwise it free-runs at its
  recorded duration.
* **Playback** interpolates along the shorter arc, so orbits pass ±π without spinning
  back. Sample-rate independent (control-rate phase accumulation in seconds / beats).
* **Serialisation**: `"v1 <seconds> <beats> <dr0> <da0> …"` with 9 significant digits —
  the float round trip is exact, so DAW recall is bit-identical (see §6). Corrupt data
  (non-finite values, wrong version) is rejected.
* **Transport to audio**: the processor stores gestures on the message side and bumps a
  per-node serial; at the start of each block the audio thread copies changed nodes into
  the engine under a **try-lock** (never blocks; retried next block). No queue can
  overflow while the host is not processing.
* Procedural gestures for factory presets: `orbitGesture` (n turns + radius wobble) and
  `swayGesture` (pendulum).

## 3. CHAOS (controlled irregularity)

`ChaosEngine`, amount `a = chaos²` (gentle at low settings), rate `0.15 + 2.5 · chaos` Hz:

| target | range | why it is safe |
|---|---|---|
| node detune | CORE ± 12a cents, nodes ± 40a cents (+ per-note jitter ± 40·chaos cents on nodes) | moves loop lengths only |
| edge strength | × (1 + (0.15c + 0.5a)·walk), bounded 0.35 … 1.65 | scales rotation angles: Q stays orthogonal |
| extra routing | edges outside the topology get 0.5a·(0.5 + 0.5 walk) | adds rotations, never gain |

CHAOS never touches a feedback gain. Measured: CHAOS 0 is bit-identical across seeds;
the same seed reproduces exactly at CHAOS 0.8; CHAOS 1 peaks at 0.24 (bounded).
The seed belongs to the preset and the DAW state; RANDOM reseeds it.

## 4. FREEZE

* Engaging FREEZE increments a **freeze epoch**; only voices that were sounding at the
  epoch are captured (notes played while frozen decay normally — measured T60 4.96 s,
  same as unfrozen).
* A captured voice's decay times are multiplied by up to 10⁴ (smoothed 0.12 s), all
  dissipative selectivity fades out, and sustained drives (BOW/AIR) are blocked so a
  lossless network is not fed forever.
* **Energy governor**: a frozen, time-varying network can pump energy parametrically
  (CHAOS / MOTION move loop lengths). 16 blocks after capture the voice records its
  energy; above 1.15 × that reference it adds damping `(1.15/ratio)⁴`.
* Measured over 19 s: METAL static −17.76 → −17.69 dB; GLASS with CHAOS 1 + MOTION 1
  −18.92 → −18.89 dB; engage spike 2.7 dB (HF); level change of a frozen note 1 → 5.3 s:
  −0.015 dB.

## 5. Passive nonlinearities (Phase 9)

Only above normal playing levels (knees well above musical energy):

* **coupling saturation**: edge rotations × `1 / (1 + max(0, E − 0.5)/0.5)`;
* **dynamic damping**: decay × `1 / (1 + max(0, E − 1))`;
* energy-dependent tuning (material property, e.g. MEMBRANE +4 % at full energy)
  follows a 40 ms envelope.

Extreme test (`nonlinear/extreme energy…`): bounded (peak at the safety clip), no
non-finite events.

## 6. State and presets

Complete DAW state (`ArcAudioProcessor::getStateInformation`):

```
<ARC_STATE version="1" pluginVersion="1.0.0" seed="…">
  <ARC> … every parameter (APVTS) … </ARC>
  <GESTURES> <GESTURE node="0" data="v1 …"/> … </GESTURES>
  <PRESET name="Satellite String" category="PLUCKED" key="factory/Satellite String" modified="1"/>
</ARC_STATE>
```

Measured (`state/round trip reproduces the sound exactly`): a restored instance renders
the same MIDI **bit-identically** (max difference 0) — parameters, gestures (including
one recorded after loading the preset), seed and preset metadata. Pre-release states
(APVTS only) still load; garbage, foreign XML and corrupt gestures are ignored.

Presets: see `docs/PARAMETERS.md` (46 factory presets, user presets, favourites, RANDOM).

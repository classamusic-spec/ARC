# ARC — Development Log

Chronological engineering log. Every claim here is backed by a build, a test run,
or a measurement recorded in the repository (see `docs/measurements/`).

---

## Phase 0 — Environment inspection (2026-09-24)

| Item | Finding |
|---|---|
| Repository | Empty git repo, branch `claude/bold-tesla-evttof`, no prior commits |
| OS | Ubuntu 24.04.4 LTS, x86_64, Linux 6.18 (cloud container, 4 vCPU, 15 GiB RAM) |
| Compilers | GCC 13.3.0, Clang 18.1.3 |
| Build tools | CMake 3.28.3, Ninja, GNU Make |
| JUCE | Not installed. JUCE **8.0.12** fetched (pinned tag) from GitHub. Latest tags seen: 8.0.15, 9.0.2 — 8.0.12 pinned as a well-known stable 8.x API surface |
| Linux JUCE deps | Installed: ALSA, JACK, X11 (Xrandr/Xinerama/Xcursor/Xcomposite/Xext/Xrender), FreeType, Fontconfig, GL/GLU, curl |
| Display | No physical display. `Xvfb` / `xvfb-run` available → headless editor tests and snapshots |
| Validation | `pluginval 1.0.4` (Linux binary from Tracktion releases) |
| Sanitizers | GCC/Clang ASan, UBSan, TSan available |
| Audio analysis | Python 3 + numpy 2.4, scipy 1.17, matplotlib 3.11, Pillow 12.3 |
| MIDI infra | No hardware MIDI; MIDI is exercised through `juce::MidiBuffer` in offline test renders and pluginval |
| AU | Not buildable here (Linux). CMake enables AU automatically on macOS → **UNVERIFIED — ENVIRONMENT LIMITATION** |
| Fonts | System fonts only (DejaVu/Liberation). OFL fonts can be fetched from the google/fonts repository and embedded |

Locked visual reference stored at `design/reference/ARC_LOCKED_REFERENCE.png`
(converted losslessly from the supplied WebP, 1448×1086).

### Consequences for the plan

* Everything that needs a display (editor open/close, snapshots, pluginval editor
  tests) runs under Xvfb.
* Audio "listening" in this environment means rendering WAV files and analysing them
  numerically (pitch, T60, spectral centroid, inharmonicity, aliasing). The rendered
  WAVs are kept under `docs/measurements/audio/` so a human can listen.
* AU validation (`auval`) is impossible on Linux.

---

## Phase 1 — Build system + bootstrap MIDI synth

* CMake project with three layers:
  * `arc_engine` — static library, **pure C++20, no JUCE**. All audio-thread code lives
    here so it can be unit tested and inspected for realtime safety in isolation.
  * `arc_plugin_code` — INTERFACE library with the JUCE-facing code (processor, state,
    presets, editor); each consumer (plugin, tests) compiles its own copy.
  * `ARC` plugin (VST3 + Standalone, + AU on macOS) and `ARCTests` console app.
* JUCE resolution order: `-DARC_JUCE_PATH=…` → `external/JUCE` → FetchContent (tag 8.0.12).
* Bootstrap voice: 8-voice Karplus-Strong, sample-accurate MIDI splitting in `processBlock`.
* Results:
  * Clean configure + build (GCC 13, Release, LTO): VST3, Standalone and ARCTests link, 0 errors.
  * `smoke/processor renders MIDI note`: PASS (energy 31.4, finite).
  * `pluginval --strictness-level 5 --skip-gui-tests` on the VST3: **SUCCESS**.
* LTO made optional (`ARC_ENABLE_LTO`, default ON) because the serial LTRANS link costs
  ~1 min per iteration during development.

---

## Phase 2 — Resonator primitives (quality gate: PASS)

* Implemented `DelayLine` (+ linear / Hermite / Lagrange-3 / Thiran-1 readers),
  `OnePoleLoss` (two-point T60 match), `AllpassChain` dispersion, `WaveguideResonator`,
  `ModalBank` (complex one-pole modes).
* Built the measurement toolkit (`Tests/Analysis.*`): zero-padded FFT peak picking,
  phase-slope frequency estimation (sub-0.001-cent), demodulated-envelope T60 with
  harmonic-spacing windows, partial tracking, band-limited centroid, WAV output.
* First gate run: **7 of 11 failed**. Root causes (all fixed, see RESONATOR_DESIGN §6):
  unstable DC mode (loss filter G(0) > 1 when its pole was clamped), dispersion stage
  budget bug (up to −211 cents near Nyquist), Thiran refinement oscillating at C7/C8
  (7 cents), SR-dependent dispersion (18 % spread). Plus test-side issues: denormals in
  benchmarks (tests now set FTZ/DAZ like `processBlock`), T60 windows not isolating
  harmonics, inharmonicity search window catching the wrong partial, and an ill-posed
  glide criterion (a 30 ms octave glide adds Doppler HF for *every* interpolator).
* Decision: allpass fractional delay exactly matched at f0 (table in RESONATOR_DESIGN §3).
* Final: 11/11 tests, 326 checks pass. Worst pitch error C1–C6 at 5 sample rates:
  0.00007 cents. Loop cost 4.9 ns/sample.

---

## Phase 3 — Resonant network (quality gate: PASS)

* `CouplingMatrix`: Cayley-transform scattering (orthogonal for any generator), 10
  edges, 4 topologies. `ResonantNetwork`: 5 loops, per-sample Q interpolation, stereo
  pickup, node-energy/edge-flux telemetry, cached control-rate redesign.
* First run: 2 of 11 failed.
  * Fuzz: 3/1500 configs stored up to 1.76× injected energy. Investigation (scratch
    program) showed every case was near-lossless under abusive ±2.5 % delay jumps:
    bounded parametric pumping. Lossy + realistic modulation measured strictly passive
    (0.973). Test split into both regimes; FREEZE will need an energy governor.
  * Telemetry: matrix-entry flux showed ring energy in a STAR (indirect paths). Edge
    flux now comes from each edge's own rotation.
* Measured coupling-induced fundamental detune (5.4 cents at φ=0.35) → to be
  compensated in the tuning phase.
* 22/22 tests (resonator + network), 400 checks pass. Network cost 37–55 ns/sample/voice.

---

## Phases 4–6 — Exciters, tension/tuning, materials (quality gates: PASS)

Built `ExciterEngine` (STRIKE/PLUCK/BOW/AIR), `MaterialProfile` + `MaterialEngine`
(morphing, inspector modifiers, TENSION), `ArcVoice` (control-rate network design,
release semantics, per-node coupling compensation, intonation tracker),
`NetworkGeometry` (field → edge generators, shared with the UI). A "sound check"
renders all 16 exciter × material combinations to WAV with pitch / decay / centroid /
partial analysis; the design was iterated against it:

1. **First listen**: GLASS sounded like a bright string (the CORE's harmonic series
   dominated), METAL's fundamental split, bow/air locked onto high modes, AIR on GLASS
   peaked at 20.5 (fixed-amplitude jet into a high-Q loop).
2. **In-loop banded selectivity** gave correct modal spectra but a one-pole steep enough
   to band had G(0) ≫ G(f0): the DC cap shortened the fundamental and left sub-f0 modes
   ringing for 280 s. Replaced by a zero-phase `(1−s) + s·BP` stage.
3. That stage made selective nodes dissipative, so coupling became a damper (GLASS T60
   3.4 s → 0.6 s). Diagnosed with the new per-node perturbation estimator (predicted
   the damping exactly). **Selectivity moved to the excitation and radiation ports**;
   loops stay reactive. The CORE keeps in-loop selectivity only under BOW/AIR.
4. METAL −43 cents: the hum node's (0.5) loop overtone sat on f0 → avoided crossing
   (−50/+51 cents at coupling 0.35). METAL now uses tierce/quint/nominal/undeciem.
5. Nonlinear drives pull pitch → intonation tracker for BOW/AIR (≤ 0.4 cents).
6. AIR jet (half-period delay) pulled pitch 23 cents → in-phase negative resistance
   proportional to the CORE's measured loss + a pitch-independent speaking rate.
7. Coupling compensation: first-order formula under-corrected by 12 %. Exact Schur
   complement did not help; a bisection root of det(I − DQ) matched measurement to 0.01
   cents, isolating the bug to the **coincidence taper** (it counted short loops' DC
   modes as coincidences). Also reverted a wrong group-delay conversion. Now ≤ 0.37
   cents up to coupling 0.5.
8. Pluck-position notches were filled by FM sidebands: energy-dependent tuning tracked
   16-sample block energy (the waveform), modulating loops at f0. Now follows a 40 ms
   envelope (h4 notch 17 → 37 dB).
9. GLASS 4.93/7.9 and WOOD 3.93 nodes formed 25–35 Hz doublets with CORE harmonics →
   ratios moved away from integers (GLASS) / free-bar modes (WOOD).
10. Bow release damped GLASS 24 dB in 50 ms (bow stayed in contact) → contact force
    follows the envelope.
11. Material morph: exponential glide jumped HF 56 dB in one frame; adaptive dispersion
    stage count could change mid-note → stage count locked per note, all loop
    coefficients ramp per sample, smoothstep morph (worst spike 3.9 dB).

Gates: 41 tests / 620 checks pass (resonator, network, exciters, tuning, materials,
sound check, sustained-level stability). Key numbers: C1–C6 pitch ≤ 0.59 cents across
six exciter/material combos; chromatic ≤ 0.19; bowed pitch ≤ 0.4; coupling-compensated
pitch ≤ 0.37 cents up to coupling 0.5; all 16 combos balanced at −22 dB RMS (C4).

---

## Phase 7 — Polyphony and voice architecture (quality gate: PASS)

* Replaced the bootstrap synth with `ArcEngine`: 20 physical / ≤ 16 logical voices,
  fade-out stealing in spare slots, quietest-released victim, same-note re-strike,
  sustain pedal, pitch bend, poly/channel aftertouch, CC74, lower-zone MPE, mono/legato
  with glide. Voices now keep their own control clock so sample-accurate MIDI splits
  never cause extra redesigns.
* Added the complete parameter layout (65 parameters, stable versioned IDs), the
  parameter cache feeding `EngineParams` once per block, the output stage (width,
  small Householder FDN space, ADAA drive, master, soft safety clip, NaN guard) and
  lock-free telemetry.
* Gate: 10/10 voice tests passed on the first run (details in VOICE_ARCHITECTURE.md).
* CPU profile: ~1 % of a core per voice at 48 kHz; 16 voices 16–18 % (48 kHz), 22–25 %
  (96 kHz). Finding: quality modes differ by < 1 % — not yet a meaningful trade-off.


---

## Phases 8–10 — Motion, chaos, FREEZE, presets, RANDOM, state (quality gate: PASS)

**Phase 8 (motion / gestures / chaos / FREEZE)** — see MOTION_SYSTEM.md. MotionEngine
(drift + gestures + SYNC), ChaosEngine (bounded, seeded), FREEZE epochs + energy governor,
passive nonlinearities. Later refinements in this phase: gesture angles wrap and close
modulo whole turns (a drag around the core becomes an orbit), 9-digit serialisation
(exact float round trip — found by the bit-exact state test: 6 digits left a 2.8e-4
difference after reload), synced drift phase-locked to the host timeline and purely
periodic, motion offsets smoothed (40 ms).

**Phases 9–10 (presets / RANDOM / state).** 46 factory presets in 12 categories
(including all 25 names from the specification), `PresetManager` (factory + user XML
presets, favourites, prev/next, modified detection, host programs), musical `RANDOM`
(gentle mutation / SHIFT = regeneration from weighted, material-aware distributions,
reseeds CHAOS), complete DAW state (parameters + gestures + seed + preset metadata +
version). Preset tuning helper `radiusForRatio` places nodes exactly on harmonic ratios.

**The factory-preset audit found four real engine defects** (render every preset,
measure loudness / peak / tail / centroid / DC / distinctness):

1. *Coupling-compensation limit cycle.* "Black Bell" held its level after release with
   a 6.4 kHz centroid. Trace: node A 20 cents from unison with the CORE; the correction
   iteration fell into a period-3 cycle (CORE loop 126 / 129 / 132 Hz) — audio-rate
   loop FM pumping energy. Fix: static coincidence taper from intended frequencies,
   width scaled by pair coupling (4th-order edge), under-relaxation 0.25, authority
   ±12 %. Near-unison doublets now centre on the note (±3.4 cents). Regression test
   fails all three checks on the old estimator. (NETWORK_COUPLING §6.1)
2. *Non-collocated feedback forces.* BOW friction and the AIR column were also injected
   into the outer nodes; a resting bow made METAL node B self-oscillate (−60 → −11 dB
   in 0.8 s, "−13 dB of garbage, HNR −65 dB" in the playability map). Now only the
   feed-forward turbulence spreads. (EXCITERS "Collocation")
3. *Bow playability.* The stick width was absolute, so only pressure ≤ speed spoke
   (Black Monolith: −42 dB). The stick region is now a fraction of the bow velocity:
   every PRESSURE × SPEED speaks on every material, SPEED = amplitude (21 dB range),
   PRESSURE = brightness (+45 % centroid), HNR 25–48 dB.
4. *Wolf-tone limit.* AIR died at COUPLING ≥ 0.65 (CORE mode pulled > 12 %). Driven
   voices soft-limit edge rotations (tanh knees 0.35 / 0.6 rad). Cathedral Air −41.4 →
   −23.7 dB, Aurora Lattice −37.4 → −24.3 dB.
5. Also: sounding voices read the *global* EXCITER for dispersion, CORE selectivity and
   loudness normalisation, so switching EXCITER mid-note jumped their level (automation
   step ratio 3.05 → 0.79). They now use their own exciter.

Results: 46/46 presets finite, unclipped (worst peak −4.0 dBFS), no DC, momentary
loudness −18.4 … −28.1 dB (Mercury String is a mono legato preset), closest spectral
pair 2.2 dB apart; RANDOM: 24/24 regenerations speak, all four exciters and materials
drawn; state round trip bit-identical; every parameter automated under a ringing chord
without non-finite output (worst step ratio 2.96 = BRIGHTNESS brightening the tone);
preset switching every 130 ms while notes sound: bounded, no voice resets.
Full suite: 71 tests / 823 checks, all passing.


---

## Phases 11–12 — ARC silver UI and the audio-reactive Resonance Field (quality gate: PASS)

The engine was proven first (Phases 2–10); the machine was then built around it,
following the locked reference (UI_SYSTEM.md).

* **Design system.** Palette tokens (satin silver / graphite / ice cyan, amber only for
  recording) and embedded Jost (SIL OFL 1.1; static Light / Regular / Medium instances
  cut from the variable font with fontTools). Surfaces are drawn procedurally: chassis,
  raised plates, recessed tray, chrome bezel and brushed grain, cached at physical
  pixel density. Custom knobs (segmented cyan value ring, chrome ring, graphite cap),
  selector tiles, round buttons, segmented controls, chips, a vector icon set and the
  logotype.
* **Resonance Field.** Node positions come from the node parameters plus effective
  motion telemetry. The connections are the active topology's edges, weighted by the
  rotation actually in use. Halos, flow pulses, strike rings, frost, tremor and ring
  spacing are all driven by telemetry. Interaction: drag with Shift for fine control,
  double-click to reset, Alt-drag or REC to record gestures, click for inspectors. The
  chamber caption is the help system.
* **Contextual UX.** Inline exciter / material inspectors, node and CORE inspectors
  docked in the less crowded band, a settings card, a preset browser, keyboard
  shortcuts, and resizing from 75 to 150 % with the size persisted in the session.
* **Found by tests and snapshot review.**
  * The logotype read "ARU" (wrong arc angles).
  * A node jumped on grab: `dragOffset` was computed after the display switched to
    the drag values. The drag test measures landing error, now 0.04 px.
  * A pure-virtual call from a panel constructor (fixed with `finishInit()`).
  * Clipped round-button shadows, a translucent card, the crosshair glint over the
    title, readouts overlapping junctions, and the dock covering the CORE.
* **Performance.** The field frame at 2× took 22 ms. It now takes 5 ms, using an opaque
  static layer keyed by TENSION and device-resolution sprites blitted 1 : 1. The frame
  clock is display-synced and drops to 15 fps when idle.
* **pluginval at strictness 10 with GUI** found that boolean parameters could restore
  to 0.5-ish values. Fixed with `SnappingBool`, a parameter that snaps to 0 / 1. After
  that: SUCCESS.

## Phase 13 — Performance optimisation

* Profiling (callgrind, 8 bowed voices): per-sample DSP was ~40 % of the cost and
  control-rate work (network redesign, coupling compensation, per-voice scattering)
  ~55 % at a 0.33 ms control period.
* **QUALITY became a real trade-off.** It now sets the control period (HIGH 0.33 ms,
  NORMAL 0.67 ms default, ECO 1.33 ms) and the dispersion stages (6 / 4 / 2). 16 bowed
  voices at 48 kHz: 20.4 / 12.4 / 9.0 % of a core. All quality-gate tests pass at the
  new default and at ECO.
* **Modes sound alike.** Log-spectral distance to HIGH is 1.2 dB (NORMAL) and 1.7 dB
  (ECO); pitch is identical within 0.04 cents. Switching is live at each voice's next
  control boundary; the largest step while switching is 0.0123 vs 0.0121 steady.
* Block-counted time constants in the voice became time based (release damping, freeze
  capture, governor settle, silence detection), so every mode behaves the same.

## Phase 14 — Validation

* **Realtime instrumentation** (`Tests/RealtimeTests.cpp`). A global `operator new`
  hook counts audio-thread allocations: 0 in 640 blocks, with MIDI, preset / gesture
  changes and automation of every structural parameter. A concurrency stress test runs
  a live audio thread while the message thread loads presets, randomises, records
  gestures and saves / restores state.
* **Sanitizers.**
  * ASan + UBSan: full suite, no reports.
  * TSan: concurrency stress and editor-vs-audio test, no reports.
  * Timing thresholds are measured but not enforced in sanitizer builds
    (`ARC_SANITIZED`), and the allocation detector steps aside for the sanitizer's
    allocator.
* **Host behaviour** (`Tests/HostTests.cpp`).
  * Block sizes 1–4096 are bit-identical.
  * Sample-rate changes 22.05–192 kHz, each re-prepared mid-session, all sound.
  * Instances are isolated (difference 0).
  * Bypass, suspend / resume and editor open / close work while processing.
* **GUI load** (the matrix's GUI closed / open column).
  * An 8-voice bowed chord costs 9.2 % of a core with the editor closed and 8.9 %
    with it open at 60 fps.
  * The dirty regions the editor repaints per frame take 3.2 ms (18.5 % of the
    message thread); a full-window repaint takes 9.7 ms.
  * The frame-cost test now warms its caches first; the first-size number had
    included the one-off cache build.
* **CPU matrix extended.** Strike and bow at 8 and 16 voices, at coupling 0 / 1, CHAOS
  1 and MOTION 1. CHAOS and MOTION cost up to ~1.5 points at 16 voices, because the
  network is redesigned every control period.
* **Fixes.**
  * The editor's last-mouse position was a function-level `static`, so it was shared
    between plugin instances. It is now a member.
  * GCC LTO links now use `-flto=auto`: parallel, and without the "serial LTRANS"
    note.
* The parameter table in PARAMETERS.md is generated from the live layout by a test,
  so it cannot drift from the code.

## Phase 15 — Release candidate

* **UI contrast and legibility pass, measured against the reference.** The same pixel
  classifier on both images shows the silver share already matched (66.7 % vs 67.7 %).
  The rest was not a close match:
  * ARC's silver was lighter and nearly neutral against the reference's cool blue:
    side-panel face RGB (210, 213, 217) vs (189, 196, 206); strip (228, 230, 232) vs
    (169, 176, 186).
  * Label ink was far paler: darkest label pixels RGB 103 vs 32.
  * Knob bezels and tile outlines were weaker.

  Changes:
  * Cooler, darker silver tokens: panel face (198, 204, 211), strip (216, 220, 225);
    mean silver lightness 210 vs the reference's 182.
  * Label ink #1C2229–#353C45: the darkest label pixels are now (33, 39, 46), against
    the reference's (32, 40, 49).
  * Medium-weight macro labels; larger tile icons and labels.
  * Crisp tile outlines and a stronger neon glow on the selected tile.
  * Polished chrome knob rings with a dark outer edge; larger, visible value dots.
  * Button faces derived from the tokens instead of hard-coded greys.
* **COUPLING was out of tune at the top of its range. Found and fixed.** Writing the
  known-limitations list meant measuring pitch over the whole knob, not only up to 0.5
  as the tuning gate did. With the Phase 9 curve the compensated note was:
  * 12–40 cents off at 0.65;
  * 100–164 cents off at 0.8;
  * −186 cents off at 1.0.

  A trace showed the compensation's ±12 % authority clamp being reached at
  0.62–0.64 on every material. Beyond about 0.75 rad of rotation a physical limit
  also appears: the CORE leaks most of its energy per pass and dissolves into
  composite modes. Fix:
  * compensation authority ±30 %;
  * a new COUPLING curve (`0.7277 c^0.9432`) that ends at the in-tune limit, keeps
    the default 0.35 at the same rotation, and leaves the whole knob musical;
  * factory presets converted to keep their rotation (audit figures identical);
  * RANDOM's windows converted;
  * field connection weights renormalised.

  The tuning gate now covers all four materials and the whole range: ≤ 1.2 cents to
  0.65 and ≤ 6.3 cents at the maximum, with the note the dominant partial. The
  unison regression runs at maximum coupling. BOW and AIR speak everywhere, which
  retires the Phase 9 AIR-at-high-coupling limitation. (NETWORK_COUPLING §6.2)
* A clean rebuild surfaced four `-Wdouble-promotion` warnings in `Gesture.cpp` that
  incremental builds had not recompiled. The conversions are now explicit, with values
  unchanged.
* Documentation completed: PRODUCT_SPEC, DSP_ARCHITECTURE, UI_SYSTEM, CORE_QUALITY_GATE,
  README, RELEASE_CANDIDATE_REPORT.
* **Standalone launch check** under Xvfb: the app window appears in 3.8 s and renders
  the full editor. This check found a small UX bug: with no audio processed yet (no
  device, or a host that opens the editor first), the node readouts said "RATIO ×0.00".
  They now show "—" until the engine has published telemetry.
* **Steinberg VST3 validator** built from the VST3 SDK 3.8: 47 / 47 standard and
  537 / 537 extensive tests. pluginval's own validator hook passes the inner `.so`,
  which that validator rejects on Linux; a wrapper that passes the bundle makes the
  combined run pass.
* Final state:
  * 86 tests / 940 checks passing in Release (68.2 s).
  * Clean rebuild from an empty build directory: 4 min 18 s, 0 warnings.
  * pluginval strictness 10 with GUI: SUCCESS; VST3 validator: 47 / 47 and 537 / 537.
  * Sanitizer runs on the final code: see the release report.

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


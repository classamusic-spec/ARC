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


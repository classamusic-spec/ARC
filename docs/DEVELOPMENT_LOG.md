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


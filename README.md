# ARC — Resonant Network Synthesizer

*Matter shapes sound.*

ARC has no oscillators. Every note injects energy into a network of five coupled
virtual resonators: a central **CORE** and four nodes, **A–D**. The network rings, trades
energy between its bodies, and **is** the sound. The same network sits in the middle
of the instrument, where you can see it, touch it, retune it and make it move.

![ARC](docs/images/arc_obsidian_bloom.jpg)

* **Exciters:** STRIKE · PLUCK · BOW · AIR. Four physical mechanisms; bow and air are
  real feedback exciters (stick-slip friction, air column).
* **Materials:** GLASS · METAL · WOOD · MEMBRANE. Each reshapes the network itself
  (modal ratios, decay, dispersion, coupling) and morphs continuously.
* **Macros:** EXCITE · COUPLING · TENSION · CHAOS, plus FREEZE · RANDOM · SYNC, and
  MOTION drift with recorded, looping gestures.
* **Network:** energy-preserving orthogonal scattering, stable at any coupling, with
  per-node coupling compensation so coupled networks stay in tune.
* **Playing:** 16 voices (default 8), poly / mono / legato, MPE, sustain, aftertouch.
  46 factory presets in 12 categories. Full session recall.
* **Formats:** VST3 and Standalone (AU on macOS). C++20, JUCE 8.

## Build

Requirements: CMake ≥ 3.22, a C++20 compiler and JUCE 8. Built and tested with GCC
13.3 on Linux; Clang / Xcode / MSVC 2022 builds are expected to work but have not been
tried here. JUCE is found in this order: `-DARC_JUCE_PATH=<checkout>`, then
`external/JUCE`, then a pinned download (8.0.12). On Linux, install JUCE's usual
dependencies (ALSA, X11 / Xrandr / Xinerama / Xcursor, FreeType, fontconfig, curl).

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # [-DARC_JUCE_PATH=/path/to/JUCE]
cmake --build build
```

Artifacts:

| Target | Location |
|---|---|
| VST3 | `build/ARC_artefacts/Release/VST3/ARC.vst3` |
| Standalone | `build/ARC_artefacts/Release/Standalone/ARC` (`ARC.app` on macOS, `ARC.exe` on Windows) |
| AU (macOS only) | `build/ARC_artefacts/Release/AU/ARC.component` |
| Tests | `build/ARCTests_artefacts/Release/ARCTests` |

CMake options:

| Option | Effect |
|---|---|
| `ARC_BUILD_TESTS` | build the test suite (ON by default) |
| `ARC_ENABLE_LTO` | link-time optimisation (ON by default) |
| `ARC_ENABLE_ASAN`, `ARC_ENABLE_UBSAN`, `ARC_ENABLE_TSAN` | sanitizer builds |
| `ARC_COMPANY_NAME` | vendor name reported to hosts |

## Test and validate

```bash
./build/ARCTests_artefacts/Release/ARCTests            # 86 tests: DSP, network, exciters, materials,
                                                       # voices, motion, presets, state, realtime, host
xvfb-run -a ./build/ARCTests_artefacts/Release/ARCTests ui   # editor tests on a headless Linux machine
pluginval --strictness-level 10 --validate-in-process build/ARC_artefacts/Release/VST3/ARC.vst3
```

The suite renders audio and measures it (pitch, decay, spectra, loudness, stability,
CPU, allocations). Nothing is declared working without a measurement. See
[docs/TESTING.md](docs/TESTING.md).

## Documentation

| Document | Contents |
|---|---|
| [PRODUCT_SPEC](docs/PRODUCT_SPEC.md) | what ARC is, its controls, playing, presets |
| [DSP_ARCHITECTURE](docs/DSP_ARCHITECTURE.md) | signal flow, rates, QUALITY, threading, realtime safety, CPU |
| [RESONATOR_DESIGN](docs/RESONATOR_DESIGN.md) | the waveguide loop and its quality gate |
| [NETWORK_COUPLING](docs/NETWORK_COUPLING.md) | Cayley scattering, stability, coupling compensation |
| [EXCITERS](docs/EXCITERS.md) | strike, pluck, bow, air |
| [MATERIAL_SYSTEM](docs/MATERIAL_SYSTEM.md) | glass, metal, wood, membrane; TENSION |
| [MOTION_SYSTEM](docs/MOTION_SYSTEM.md) | drift, gestures, SYNC, CHAOS, FREEZE |
| [VOICE_ARCHITECTURE](docs/VOICE_ARCHITECTURE.md) | voices, stealing, MIDI, MPE |
| [UI_SYSTEM](docs/UI_SYSTEM.md) | design system, Resonance Field, inspectors, rendering |
| [PARAMETERS](docs/PARAMETERS.md) | stable IDs, parameter table, presets, RANDOM, state |
| [TESTING](docs/TESTING.md) | test suites, sanitizers, validation |
| [CORE_QUALITY_GATE](docs/CORE_QUALITY_GATE.md) | PASS / FAIL / UNVERIFIED per subsystem |
| [RELEASE_CANDIDATE_REPORT](docs/RELEASE_CANDIDATE_REPORT.md) | the V1.0 release candidate report |
| [DEVELOPMENT_LOG](docs/DEVELOPMENT_LOG.md) | what was built, measured and fixed, phase by phase |

## Repository

```
Source/
  PluginProcessor.*  PluginEditor.*
  Core/        parameters, factory presets, preset manager, RANDOM
  Engine/      engine, voices, telemetry, lock-free queues
  Synthesis/   waveguide loops, filters, fractional delay, Cayley scattering, network
  Exciters/    strike, pluck, bow, air
  Materials/   material profiles and morphing engine
  Motion/      drift, gestures      Nonlinear/  CHAOS      FX/  output stage
  UI/          theme, controls, panels, Resonance Field, inspectors, preset browser
  Graphics/    silver surfaces, icons, logotype
Resources/Fonts/   Jost (SIL Open Font License 1.1)
Tests/             test harness, analysis tools and suites
design/reference/  the locked UI reference
docs/              documentation, measurements, screenshots
```

## Licences

ARC's source is in this repository. ARC is built on JUCE, which is dual-licensed
(AGPLv3 or a commercial JUCE licence), and distributing ARC binaries requires
complying with one of them. The embedded Jost typeface is licensed under the SIL Open
Font License 1.1 (`Resources/Fonts/Jost-OFL.txt`).

# ARC — Core Quality Gate

Status of every gate at the V1.0 release candidate. Each verdict rests on tests that
were **run** on this build (`Tests/`, 90 tests / 1004 checks, all passing in Release),
on sanitizer builds, and on pluginval. Anything that could not be tested in this
environment is marked **UNVERIFIED — ENVIRONMENT LIMITATION**. Detailed measurements are
in the linked documents and in `docs/measurements/`.

| Gate | Verdict |
|---|---|
| RESONATORS | **PASS** |
| NETWORK | **PASS** |
| EXCITERS | **PASS** |
| MATERIALS | **PASS** |
| POLYPHONY | **PASS** |
| MOTION | **PASS** |
| UI | **PASS** (on-hardware visual review UNVERIFIED — ENVIRONMENT LIMITATION) |
| VALIDATION | **PASS** for VST3 / Standalone on Linux; AU, macOS and Windows **UNVERIFIED — ENVIRONMENT LIMITATION** |

---

## RESONATORS — PASS
(RESONATOR_DESIGN §4, `resonator` tests)

| Criterion | Measured |
|---|---|
| Pitch C1–C6 at 44.1–192 kHz, < 1 cent | worst 0.00007 cents harmonic, 0.003 dispersive |
| T60 at f0 within 10 % / at HF within 25 % | 0.004 % / 1.4 % |
| Sample-rate consistency | T60 spread 0.002 %, inharmonicity 0.7 % |
| Frequency automation (an octave in 200 ms / 30 ms) | no click; curvature 0.49× / 2.1× steady state |
| Extremes (1 Hz – 1 MHz, T60 0.1 ms – 1000 s, input × 1000) | all finite, bounded |
| Near Nyquist (8–19 kHz at 48 kHz) | stable, 0.00001 cents |
| Cost | 4.9 ns per loop-sample |

## NETWORK — PASS
(NETWORK_COUPLING §4 and §6, `network` and `tuning` tests)

| Criterion | Measured |
|---|---|
| Scattering orthogonal for any coupling | ‖QᵀQ − I‖ ≤ 9.4e-8 |
| Impulse into CORE reaches every node; nothing when uncoupled | all four nodes −9 … −15 dB; uncoupled exactly 0 |
| Stability: maximum coupling, 3000 random networks under abusive and realistic modulation | 0 non-finite samples; worst stored / injected energy 1.68 (near-lossless loops under abusive modulation, bounded), 0.49 after 1 s for lossy networks under smooth modulation |
| Lossless network conserves energy (10 s) | ratio 0.996 |
| Topologies audibly different | 1.5 … 16.7 dB log-spectral distance |
| Coupling automation while ringing | smooth (0.47× steady-state curvature) |
| Coupled fundamental in tune over the whole COUPLING range (compensation) | glass / metal / wood / membrane ≤ 1.2 cents to 0.65; ≤ 6.3 cents at the maximum (metal; others ≤ 0.3); the note stays the dominant partial; near-unison doublets centred ±3.4 cents |
| Telemetry equals DSP | node energy exact; absent edges read 0 |
| Block-size independence | bit-identical output for host blocks 1 … 4096 |

## EXCITERS — PASS
(EXCITERS, `exciters` and `soundcheck` tests)

| Criterion | Measured |
|---|---|
| STRIKE is a transient; hardness brightens | energy front-loaded (≥ 6 dB); centroid 862 → 2270 Hz |
| PLUCK position comb | ¼: h4 notch 37 dB; ½: even harmonics −37 … −40 dB |
| BOW is sustained friction, not noise | level drift < 0.7 dB, spectral flatness ≤ 1.2e-5, pitch ≤ 0.4 cents |
| BOW playable everywhere | every PRESSURE × SPEED speaks on every material (−40 … −19 dB, HNR 25–48 dB) |
| AIR is turbulence plus tone | breathy / tonal flatness ratio > 1.5; pitch within 0.6 cents (MEMBRANE +6 … +8 cents from its energy-dependent tuning: see the release report's known limitations) |
| Loudness normalised | 16 exciter × material pairs at −22 dB RMS ± 3 dB (AIR ± 5 dB), C2–C6 |
| Feedback exciters passive to the network | collocated at the CORE (a resting bow no longer excites node B) |

## MATERIALS — PASS
(MATERIAL_SYSTEM, `materials` tests)

| Criterion | Measured |
|---|---|
| Materials measurably different | distinct partial series (glass 2.63 / 4.44 …, metal doublets, wood free-bar, membrane Bessel); T60 0.86 … 8.2 s |
| Different for every exciter | minimum pairwise distance 28 dB (strike), 4.2 dB (bow) |
| Inspector modifiers act | LOSS 7.4 → 32.1 dB decay, BRIGHTNESS 454 → 1148 Hz, INHARMONICITY stretch 1.015 → 1.054, MASS T60 6.3 → 12.4 s |
| TENSION stretches modes, not pitch | fundamental within 0.27 cents (T 0.2–0.8) |
| Material morph continuous | worst HF event 3.9 dB |

## POLYPHONY — PASS
(VOICE_ARCHITECTURE, `voices`, `performance` and `host` tests)

| Criterion | Measured |
|---|---|
| ≥ 8 useful voices | 16 voices at 48 kHz: 10.6 % (strike) / 12.3 % (bow) of one core |
| Voice storms at 1 / 4 / 8 / 16 voices | max active = polyphony + 1 (the fading voice), 0 hard steals, 0 stuck voices |
| Stealing is click-free | worst HF event 4.7 dB above its neighbourhood (click criterion 12 dB); quietest released voice chosen |
| MIDI pitch / velocity / sustain / bend / MPE / legato | C1–C6 ≤ 0.66 cents; velocity +17 dB (strike); pedal holds; bend 0.004 cents; MPE per-note 0.005 cents; glide lands within 0.015 cents |
| Sample-accurate MIDI | identical onset at block offsets 0 / 17 / 101 / 255 |
| Realtime safe | 0 allocations in 640 blocks; no races (TSan); ASan / UBSan clean |

## MOTION — PASS
(MOTION_SYSTEM, `motion`, `gesture`, `chaos`, `freeze`, `random` and `state` tests)

| Criterion | Measured |
|---|---|
| Node movement changes real DSP | MOTION 1 moves node A's radius by 0.20 (0 at depth 0); the node radius sets its tuning |
| SYNC follows the host | drift identical at the same bar position (diff 0); gestures take 2.000 s at 120 BPM and 2.667 s at 90 BPM |
| Gesture recording | Alt-drag records a loop (UI test); serialisation exact; an orbit drag loops seamlessly (≤ 0.0054 rad per sample) |
| CHAOS | 0 = bit-identical across seeds; same seed = identical; CHAOS 1 bounded (peak 0.27) |
| FREEZE | level change ≤ 0.23 dB from 2 s to 19 s (metal, glass with CHAOS 1 and MOTION 1, wood); engage 2.8 dB HF event; new notes decay (T60 5.0 s) |
| RANDOM | gentle step ≤ 0.0995 (normalised); full regeneration 24 / 24 speak, all exciters and materials drawn, −35.8 … −21.8 dB |
| State | bit-identical render after restore (10 kB state); every parameter automated safely |

## UI — PASS
(UI_SYSTEM §10, `ui` tests, snapshots in `docs/images/`)

All fifteen UI quality gate criteria pass:

* Layout matches the reference, silver dominates (66.7 % vs 67.7 %), the chamber is
  dark, there is no keyboard, and controls are minimal.
* The field is the largest element. Nodes and connections come from DSP state and
  telemetry, and a drag lands within 0.04 px.
* There is no generic JUCE styling; rendering and typography are consistent.
* Animation is smooth: 60 fps, 3.0 ms per frame at 1×, 4.5 ms field frame at 2×.
* Resize (900–1800 px) and HiDPI (2× snapshots) work.
* An open, animating editor does not affect the audio thread (8.8 % vs 8.9 %).

UNVERIFIED — ENVIRONMENT LIMITATION: appearance and frame rate on a physical Retina
or 4K display inside a DAW. There is no display here; the editor ran under Xvfb and
was judged from rendered snapshots.

## VALIDATION — PASS (Linux) / UNVERIFIED (AU, macOS, Windows)

| Check | Result |
|---|---|
| pluginval 1.0.4, strictness 10, GUI tests on, VST3 | **SUCCESS**: all suites, including editor, automation, state restoration, thread safety, fuzzing, bus layouts and 44.1 / 48 / 96 kHz × 64–1024 blocks |
| Steinberg VST3 validator (VST3 SDK 3.8) | **47 / 47** standard tests; **537 / 537** extensive tests (`-e -l`); also passes as pluginval's validator step |
| Instantiation / destruction, editor open / close, MIDI, state, automation | pluginval + `host` tests |
| Sample-rate changes (22.05 – 192 kHz), block sizes 1 – 4096 | `host` tests: all sound; blocks bit-identical |
| Multiple instances | `host` tests: isolated (difference 0) |
| Bypass / suspend | `host` tests; pluginval |
| Preset switching | switching 94 presets while notes sound: peak −2.6 dB (the safety clip starts at −3 dBFS), no resets; each note keeps its own patch's level trim (a bell's tail moves −0.7 dB across a switch to a +18 dB patch) |
| Factory library | 396 presets audited: all clean (chord and hard single notes C2 / C4 / C6 below −1 dBFS), level-calibrated, true to category, 289 / 289 pitched presets in tune, closest pair 2.13 ([PRESETS](PRESETS.md)) |
| AddressSanitizer + UndefinedBehaviorSanitizer (full suite, 90 tests, Phase 16 code) | no reports |
| ThreadSanitizer (19 threaded tests: audio vs message thread, editor at 60 fps, host, state, preset switching, the library audit on four worker threads) | no reports |
| Allocation detector | 0 allocations on the audio thread |
| Clean rebuild from an empty build directory | succeeds, 0 warnings |
| AU build and `auval` | **UNVERIFIED — ENVIRONMENT LIMITATION** (Linux build machine; AU is enabled automatically on macOS) |
| macOS / Windows builds, real DAWs (Live, Logic, Reaper, Bitwig, Cubase) | **UNVERIFIED — ENVIRONMENT LIMITATION** |

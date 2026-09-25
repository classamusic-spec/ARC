# ARC — Testing

ARC is built with one rule: **no fake completion**. Nothing is "stable", "in tune" or
"like metal" until a test has rendered it and measured it. The suite renders audio, analyses
it (FFT, phase-refined pitch, partial tracking, T60 fits, harmonic-to-noise, log-spectral
distance), checks numeric criteria and records every measurement.

## Running

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DARC_JUCE_PATH=/path/to/JUCE
cmake --build build --target ARCTests -j4
./build/ARCTests_artefacts/Release/ARCTests                 # everything (~100 s)
./build/ARCTests_artefacts/Release/ARCTests tuning network  # groups or name fragments
./build/ARCTests_artefacts/Release/ARCTests --list
xvfb-run -a ./build/ARCTests_artefacts/Release/ARCTests ui  # UI tests on a headless Linux box
```

Outputs (`Tests/output/`, git-ignored): `measurements.csv` (every MEASURE of the run),
rendered WAVs (every factory preset, the exciter × material sound check, topology and
morph renders), UI snapshots (`ui/*.png`), CSV profiles. Curated copies of the
measurements live in `docs/measurements/phase*/`. A full (unfiltered) run of the library
audit also rewrites `docs/PRESETS.md` and `docs/measurements/presets/library.csv`.

The harness is self-contained (`Tests/ArcTest.h`): `TEST_CASE`, `CHECK` (continue),
`REQUIRE` (abort test), `MEASURE`. `Tests/Analysis.*` holds the signal analysis;
`Tests/VoiceRig.h` drives a single voice exactly like the engine does.

## Suites (90 tests)

| Group | What it proves |
|---|---|
| `resonator` (11) | pitch C1–C6 at 44.1–192 kHz (worst 0.00007 cents), decay accuracy at f0 and HF (0.004 % / 1.4 %), fractional-delay comparison, dispersion, click-free frequency automation, extremes finite, near-Nyquist tuning, modal bank, cost per resonator |
| `network` (11) | Cayley scattering orthogonal (1e-7), energy decays / bounded, lossless conservation (0.4 % over 10 s), impulse reaches every node, maximum coupling stable, 3000-configuration stability fuzz, topologies audibly different, coupling automation safe, telemetry matches DSP, compensated fundamental, cost per network |
| `exciters` (6) | strike is a transient, pluck-position comb notches, bow = sustained friction (not noise), air = continuous turbulence + tone, steady sustained level, velocity / EXCITE per exciter |
| `tuning` (8) | MIDI C1–C6 ≤ 0.66 cents, chromatic ≤ 0.2, TENSION is not master pitch, TENSION stretches modes, node radius / QUANTIZE, coupling compensation over the whole COUPLING range on all four materials (≤ 1.2 cents to 0.65, ≤ 6.3 at the maximum, the note stays the dominant partial), membrane glide settles, **estimator convergence near unison** up to maximum coupling (regression) |
| `materials` (4) | materials measurably different (decay, spectrum, inharmonicity) and different for every exciter, inspector modifiers act, material morph continuous (worst spike 3.9 dB) |
| `soundcheck` (1) | 16 exciter × material combinations: pitch, T60, centroid, partials, loudness balance |
| `voices` (10) | polyphony storms without stuck voices, click-free stealing, quietest-released victim, restrike, sustain pedal, pitch bend (0.004 cents), MPE per-note bend, legato glide, release semantics, sample-accurate MIDI |
| `motion` / `gesture` / `chaos` / `freeze` / `nonlinear` (10) | drift + tempo sync, host-timeline phase lock, gesture build / serialise / playback, drag-around-the-core orbit, synced gesture periods, procedural gestures (figure, breathe, steps, wander) closed, bounded and semitone-exact, CHAOS determinism and bounds, FREEZE holds energy (≤ 0.23 dB change over 19 s) and new notes still decay, extreme energy bounded |
| `presets` / `random` / `state` (9) | library complete (396, at least 29 per category, all spec names, unique names and descriptions, no clamped tuning, every preset calibrated), user presets / favourites / modified flag, preset switching under load, each note keeps its own patch's level trim (a ringing tail is not lifted by a quieter patch; a note in the same block as a switch plays at the new level), gentle vs full RANDOM, bit-identical state round trip, hostile state, every parameter automated under a ringing chord |
| `library` (2) | **every factory preset rendered and audited** (see below); loudness calibration (runs only with `ARC_CALIBRATE=1`) |
| `performance` (3) | CPU by voices × exciter × sample rate × coupling / chaos / motion, QUALITY trade-off, quality modes sound alike and switch live without clicks |
| `realtime` (2) | **zero allocations** in 640 `processBlock` calls with MIDI, preset / gesture changes and structural automation; audio + message threads concurrently (run under ThreadSanitizer) |
| `host` (4) | block sizes 1–4096 bit-identical, sample-rate changes 22.05–192 kHz, multiple instances independent, bypass / suspend / editor open-close while processing |
| `ui` (7) | editor snapshots (all main states, preset search, 2× HiDPI, smallest size), field frame cost, node drag lands under the pointer, Alt-drag gesture recording, tiles / FREEZE / SYNC / RANDOM drive parameters, audio cost with the editor closed vs open at 60 fps (also the ThreadSanitizer race test for editor frames against live audio), preset browser search over the whole library and clip-aware painting (10 rows drawn of 396, 4 ms for the whole sheet) |
| `smoke`, `docs` (2) | MIDI renders audio; parameter table generated from the live layout |

## The preset library audit

`library/every preset is clean, truthful and unique` renders every factory preset (four at a
time, one processor per thread) and measures a held C3-E3-G3 chord (loudness, peak, DC,
sustain, tail, centroid, stereo width, band spectrum, a 16-frame envelope, modulation), a
single A3 (phase-refined pitch, level against the strongest partial) and hard single notes
(C2, C4, C6 at full velocity: the loudest attacks). It fails on:

* **unclean**: non-finite output, a peak at or above −1 dBFS on the chord or on any hard
  note, DC above −30 dB, near silence;
* **off target**: momentary loudness more than 2 dB from −21 dB, unless the peak ceiling
  (−3 dBFS on the chord or a hard note, where the output's safety clip begins) held the
  trim back;
* **untruthful**: PLUCKED / STRUCK / BOWED / AIR not using their exciter, GLASS / METAL /
  WOOD / MEMBRANE not using their material, PADS / DRONES falling more than 10 dB in 1.5 s,
  PERCUSSION falling less than 18 dB;
* **out of tune**: the A3 more than 12 cents off or more than 30 dB below the strongest
  partial (PERCUSSION, EXPERIMENTAL and presets tagged inharmonic, unpitched, noise, detuned
  or chaos are exempt);
* **not unique**: two presets closer than 2.0 in a fingerprint distance
  √(S² + E²/4 + W²/16 + F²/4) of spectral shape S, envelope E, width W and modulation F (dB).

It prints the twelve closest pairs and every octave fold made by `chord()`.

```bash
ARC_PRESET_FILTER=glass ./build/ARCTests_artefacts/Release/ARCTests "clean, truthful"  # a category or name fragment
ARC_PRESET_WAVS=1 ./build/ARCTests_artefacts/Release/ARCTests "clean, truthful"        # also write every chord as a WAV
ARC_CALIBRATE=1 ./build/ARCTests_artefacts/Release/ARCTests "calibrate loudness"      # re-measure PATCH LEVEL trims
```

After changing a preset: calibrate it (the filter works here too; other entries are kept),
rebuild, and run the audit. The trims live in `Source/Core/Presets/Calibration.inc`.
Calibration measures with PATCH LEVEL at 0 dB and MASTER 12 dB below its default, so no
peak reaches the safety clip while it is measured, and adds the 12 dB back (MASTER comes
after DRIVE, so this is exact).

## Sanitizers

```bash
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DARC_ENABLE_ASAN=ON -DARC_ENABLE_UBSAN=ON -DARC_ENABLE_LTO=OFF ...
cmake -S . -B build-tsan -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DARC_ENABLE_TSAN=ON -DARC_ENABLE_LTO=OFF ...
```

Sanitizer builds define `ARC_SANITIZED`: timing thresholds (CPU budget, frame cost) are
still measured but not enforced, and the allocation detector (which replaces the global
allocator) steps aside for the sanitizer's own.

## Plugin validation

```bash
xvfb-run -a pluginval --strictness-level 10 --validate-in-process --timeout-ms 300000 \
    build/ARC_artefacts/Release/VST3/ARC.vst3
```

Runs with the editor (GUI tests on). It covers:
* open cold / warm, info and programs;
* editor, editor while processing, editor automation;
* audio processing at 44.1 / 48 / 96 kHz × 64–1024 blocks, and non-releasing processing;
* state, state restoration, background-thread state;
* automation, parameters, parameter thread safety;
* bus layouts and fuzzing.

**Steinberg VST3 validator** (the SDK's own conformance suite) is built from the VST3 SDK
(`vst3sdk`, target `validator`):

```bash
validator build/ARC_artefacts/Release/VST3/ARC.vst3          # 47 tests
validator -e -l build/ARC_artefacts/Release/VST3/ARC.vst3    # extensive: 537 tests
```

pluginval runs this validator too when given `--vst3validator <path>`. pluginval 1.0.4
passes it the inner `Contents/x86_64-linux/ARC.so`, which the VST3 SDK 3.8 validator
rejects on Linux ("not a module directory"). A two-line wrapper that passes the
`.vst3` bundle instead makes the combined run pass.

## How defects were found

The suite is not a formality: in Phase 9 alone, rendering every factory preset exposed a
coupling-compensation limit cycle, non-collocated feedback forces, a bow playability hole
and a wolf-tone limit (DEVELOPMENT_LOG, "Phases 8–10"); the state round-trip test found a
gesture precision loss; the UI drag test found a grab-offset jump; pluginval found the
boolean-parameter restore quirk.

# ARC — Parameters, Presets and State

## 1. Stable IDs

Every host-automatable parameter has a permanent, versioned ID of the form
`arc.<area>.<name>.v1` (nodes: `arc.node<A-D>.<field>.v1`). IDs are never display labels
and never change once released; a parameter whose meaning changes gets a new `.v2` ID
and the old one keeps working. Parameter *names* (what hosts show) may change freely.

Parameters are read on the audio thread through a cache of raw atomic pointers
(`params::ParameterCache::fill`, one snapshot per block, allocation-free).

**Boolean parameters** use `SnappingBool`, which snaps every write to 0 or 1. JUCE's
`AudioParameterBool` keeps any normalised value a host writes (e.g. 0.4) while the value
tree stores the snapped one, so a later restore of "0" could be skipped as unchanged —
found by pluginval's state-restoration test at strictness 10.

## 2. Parameter table

Generated from the live layout by the `docs/parameter table` test
(`docs/measurements/parameter_table.md`) — 66 parameters. "In presets" = stored in and
recalled by presets; performance / setup parameters (MASTER OUTPUT, POLYPHONY, BEND RANGE,
MPE, QUALITY, FREEZE) are never changed by loading a preset.

| ID | Name | Range | Default | In presets |
|---|---|---|---|---|
| `arc.network.excite.v1` | Excite | 0.00 ... 1.00 | 60 % | yes |
| `arc.network.coupling.v1` | Coupling | 0.00 ... 1.00 | 35 % | yes |
| `arc.network.tension.v1` | Tension | 0.00 ... 1.00 | 50 % | yes |
| `arc.network.chaos.v1` | Chaos | 0.00 ... 1.00 | 10 % | yes |
| `arc.network.topology.v1` | Topology | Star / Ring / Web / Chain | Ring | yes |
| `arc.network.quantize.v1` | Quantize Nodes | Off / On | Off | yes |
| `arc.exciter.type.v1` | Exciter | Strike / Pluck / Bow / Air | Strike | yes |
| `arc.exciter.strike.hardness.v1` | Strike Hardness | 0.00 ... 1.00 | 55 % | yes |
| `arc.exciter.strike.length.v1` | Strike Length | 0.00 ... 1.00 | 35 % | yes |
| `arc.exciter.strike.tone.v1` | Strike Tone | 0.00 ... 1.00 | 60 % | yes |
| `arc.exciter.pluck.position.v1` | Pluck Position | 0.00 ... 1.00 | 22 % | yes |
| `arc.exciter.pluck.damp.v1` | Pluck Damp | 0.00 ... 1.00 | 0 % | yes |
| `arc.exciter.pluck.tone.v1` | Pluck Tone | 0.00 ... 1.00 | 60 % | yes |
| `arc.exciter.bow.pressure.v1` | Bow Pressure | 0.00 ... 1.00 | 50 % | yes |
| `arc.exciter.bow.speed.v1` | Bow Speed | 0.00 ... 1.00 | 55 % | yes |
| `arc.exciter.bow.friction.v1` | Bow Friction | 0.00 ... 1.00 | 50 % | yes |
| `arc.exciter.air.flow.v1` | Air Flow | 0.00 ... 1.00 | 60 % | yes |
| `arc.exciter.air.turbulence.v1` | Air Turbulence | 0.00 ... 1.00 | 45 % | yes |
| `arc.exciter.air.tone.v1` | Air Tone | 0.00 ... 1.00 | 50 % | yes |
| `arc.material.type.v1` | Material | Glass / Metal / Wood / Membrane | Metal | yes |
| `arc.material.mass.v1` | Mass | 0.00 ... 1.00 | 50 % | yes |
| `arc.material.brightness.v1` | Brightness | 0.00 ... 1.00 | 50 % | yes |
| `arc.material.loss.v1` | Loss | 0.00 ... 1.00 | 50 % | yes |
| `arc.material.inharmonicity.v1` | Inharmonicity | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeA.radius.v1` | Node A Radius | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeA.angle.v1` | Node A Angle | 0.00 ... 1.00 | -45° | yes |
| `arc.nodeA.decay.v1` | Node A Decay | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeA.damp.v1` | Node A Damp | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeA.level.v1` | Node A Level | 0.00 ... 1.00 | 75 % | yes |
| `arc.nodeA.link.v1` | Node A Link | 0.00 ... 1.00 | 75 % | yes |
| `arc.nodeB.radius.v1` | Node B Radius | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeB.angle.v1` | Node B Angle | 0.00 ... 1.00 | 45° | yes |
| `arc.nodeB.decay.v1` | Node B Decay | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeB.damp.v1` | Node B Damp | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeB.level.v1` | Node B Level | 0.00 ... 1.00 | 75 % | yes |
| `arc.nodeB.link.v1` | Node B Link | 0.00 ... 1.00 | 75 % | yes |
| `arc.nodeC.radius.v1` | Node C Radius | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeC.angle.v1` | Node C Angle | 0.00 ... 1.00 | -135° | yes |
| `arc.nodeC.decay.v1` | Node C Decay | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeC.damp.v1` | Node C Damp | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeC.level.v1` | Node C Level | 0.00 ... 1.00 | 75 % | yes |
| `arc.nodeC.link.v1` | Node C Link | 0.00 ... 1.00 | 75 % | yes |
| `arc.nodeD.radius.v1` | Node D Radius | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeD.angle.v1` | Node D Angle | 0.00 ... 1.00 | 135° | yes |
| `arc.nodeD.decay.v1` | Node D Decay | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeD.damp.v1` | Node D Damp | 0.00 ... 1.00 | 50 % | yes |
| `arc.nodeD.level.v1` | Node D Level | 0.00 ... 1.00 | 75 % | yes |
| `arc.nodeD.link.v1` | Node D Link | 0.00 ... 1.00 | 75 % | yes |
| `arc.freeze.v1` | Freeze | Off / On | Off | no |
| `arc.sync.v1` | Sync | Off / On | Off | yes |
| `arc.motion.depth.v1` | Motion | 0.00 ... 1.00 | 0 % | yes |
| `arc.motion.rate.v1` | Motion Rate | 0.02 ... 4.00 Hz | 0.250 | yes |
| `arc.motion.division.v1` | Motion Division | 1/4 / 1/2 / 1 Bar / 2 Bars / 4 Bars / 8 Bars | 1 Bar | yes |
| `arc.motion.play.v1` | Gesture Play | Off / On | On | yes |
| `arc.voice.polyphony.v1` | Polyphony | 1.00 ... 16.00 | 8 | no |
| `arc.voice.mode.v1` | Voice Mode | Poly / Mono / Legato | Poly | yes |
| `arc.voice.bendrange.v1` | Bend Range | 0.00 ... 24.00 st | 2 | no |
| `arc.voice.release.v1` | Release Damping | 0.00 ... 1.00 | 25 % | yes |
| `arc.voice.glide.v1` | Glide | 0.00 ... 1.00 s | 0.060 | yes |
| `arc.voice.mpe.v1` | MPE | Off / On | Off | no |
| `arc.quality.v1` | Quality | Eco / Normal / High | Normal | no |
| `arc.master.output.v1` | Master Output | -60.00 ... 6.00 dB | -3.00 | no |
| `arc.fx.width.v1` | Width | 0.00 ... 1.50 | 1.000 | yes |
| `arc.fx.space.v1` | Space | 0.00 ... 1.00 | 12 % | yes |
| `arc.fx.drive.v1` | Drive | 0.00 ... 1.00 | 0 % | yes |


Node fields (A..D): `radius` (tuning offset, ±1 octave around the material ratio; with
QUANTIZE in semitones), `angle` (stereo position and neighbour proximity), `decay`
(T60 × 2^(±2)), `damp` (HF decay × 2^(∓2)), `level` (pickup), `link` (coupling weight).

## 3. Presets

A preset stores plain (denormalised) values of every *sound* parameter, the four node
gestures and the CHAOS / MOTION seed. Parameters missing from a preset take their
defaults, so presets from older versions load unchanged when parameters are added;
unknown IDs (from newer versions) are ignored (tested: `presets/modified flag,
favourites and user presets`).

* **Factory library**: 396 presets in 12 categories — PADS, PLUCKED, STRUCK, BOWED, AIR,
  GLASS, METAL, WOOD, MEMBRANE, DRONES, PERCUSSION, EXPERIMENTAL: the 46 signature sounds
  (including every example name from the product specification) and a 350-preset library.
  Catalogue: [PRESETS](PRESETS.md). Defined in code (`Core/FactoryPresets.cpp` and one file
  per category in `Core/Presets/`) with a builder (`Core/PresetBuilder.h`) that places nodes
  exactly on ratios (`radiusForRatio`: the inverse of the voice's tuning map, including
  material ratio, TENSION stretch and detune) or folds them by octaves into reach.
* **PATCH LEVEL** (`arc.fx.level.v1`, −24 … +18 dB) is a sound parameter: every factory
  preset stores its measured loudness trim (`Core/Presets/Calibration.inc`, generated by
  `library/calibrate loudness`). It is applied **per note**: a note keeps the trim of the
  patch it was played in (a preset load advances a patch epoch before its values arrive),
  so switching to a quiet patch never lifts the previous one's ringing tail; notes of the
  current patch follow automation. DRIVE sees the signal with the current trim taken out,
  so a trim never changes a patch's saturation. MASTER OUTPUT stays the player's
  monitoring level.
* **Audit** (`library/every preset is clean, truthful and unique`): each preset renders a
  C3-E3-G3 chord, a single A3 and hard single notes (C2, C4, C6 at full velocity). All 396
  are finite and DC-free, and peak below −1 dBFS on all of them. They sit at −21 dB ±2
  momentary loudness, or lower where a hard note's peak held the trim at −3 dBFS
  (short hits: 39 below −23 dB, the quietest −34.1). They play in tune where pitched
  (289 checked, within 12 cents), obey their category (exciter, material, sustain or
  decay) and are unique (closest pair 2.13, Reed Array / Sheng Cluster). Results: `docs/measurements/presets/library.csv` and `docs/PRESETS.md`
  (rewritten by a full, unfiltered run); WAVs in `Tests/output/presets/` with
  `ARC_PRESET_WAVS=1`.
* **User presets**: `~/Documents/ARC/Presets/*.arcpreset` (XML), favourites in
  `~/Documents/ARC/Favourites.xml` (keys `factory/<name>` / `user/<name>`).
* **Host programs** = factory presets (`getNumPrograms`, `setCurrentProgram`).
* **Modified detection**: the current values are compared with a snapshot taken when the
  preset was loaded (parameters and gestures); the header shows a cyan dot.

```xml
<ArcPreset format="1" pluginVersion="1.0.0" name="My Network" category="PADS"
           author="..." tags="dark, slow" description="..." seed="305419896">
  <Param id="arc.network.coupling.v1" value="0.52"/>
  ...
  <Gesture node="1" data="v1 7 0 0 0 ..."/>
</ArcPreset>
```

## 4. RANDOM

Never a uniform draw over every parameter (`Core/Randomiser.cpp`):

* **Click — gentle mutation**: small Gaussian steps (σ 0.03–0.06) on the macros, the
  *active* exciter's controls, the material modifiers and the nodes, inside musical
  windows that are widened (never forced) when the current value lies outside them;
  quantised nodes step by semitones; exciter, material and topology stay. Measured: the
  largest normalised step 0.0995, no discrete parameter changed.
* **SHIFT-click — regeneration**: weighted choices (strike 35 %, pluck 25 %, bow / air 20 %;
  ring 35 %, web 30 %, chain 20 %, star 15 %), COUPLING triangular around 0.5 over
  0.08–0.92 (the whole range is in tune since the RC coupling curve), TENSION 0.5 ± 0.07, CHAOS skewed low (0.45·u²), per-material inharmonicity
  windows, node tunings drawn from an interval grid (quantised), the harmonic series
  nearest the material's own ratios, or a small spread; 30 % chance of an orbit / sway
  gesture. Measured over 24 seeds: every patch speaks (−36 … −22 dB), none clips, all
  four exciters and materials appear.
* Both reseed CHAOS / MOTION.

## 5. DAW state

```xml
<ARC_STATE version="1" pluginVersion="1.0.0" seed="..." editorWidth="1080">
  <ARC> ... every parameter (APVTS tree) ... </ARC>
  <GESTURES> <GESTURE node="0" data="v1 ..."/> ... </GESTURES>
  <PRESET name="Satellite String" category="PLUCKED" key="factory/Satellite String" modified="1"/>
</ARC_STATE>
```

* A restored instance renders the same MIDI **bit-identically** (tested; gesture floats
  are serialised with 9 significant digits for an exact round trip).
* Pre-release states (APVTS tree only) load; garbage, foreign XML and corrupt gestures
  are ignored (tested).
* Preset metadata is published under a spin lock so hosts may save state from worker
  threads; restore from a non-message thread defers the metadata to the message thread.
* Gestures reach the audio thread through per-node serials and a never-blocking try-lock.

## 6. Preset morphing (future)

Presets are plain value maps plus gestures, so A/B morphing is a per-parameter
interpolation (log-domain for frequencies, as the material morph already does) — the
architecture supports it; V1 does not expose it.

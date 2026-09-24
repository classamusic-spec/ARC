# ARC — Material System

A material is a description of how the **network** behaves, not an EQ. Code:
`Source/Materials/*`, applied by `ArcVoice::updateControl`. Gate:
`Tests/MaterialTests.cpp`, `Tests/TuningTests.cpp`.

## What a material controls

| Field | Effect on the network |
|---|---|
| `nodeRatio[4]` | modal distribution: where A–D sit relative to the CORE |
| `nodeDetuneCents` | small fixed offsets (doublets) |
| `core/nodeDispersion` | per-loop stiffness → inharmonic stretch of each loop's own overtones |
| `t60Low / Mid / High` | frequency-dependent decay (seconds at 100 Hz / 1 kHz / 8 kHz, log-log) |
| `couplingScale` | how strongly this body transmits energy between resonators |
| `core/nodeSelectivity` | modal selectivity of excitation and radiation (see below) |
| `inject[4]`, `level[4]` | where exciter energy enters, how much each node radiates |
| `energyTuning` | pitch rise with node energy (tension modulation) |
| `hardnessBias`, `toneBias` | how the body responds to the exciter |
| `exciterGainDb[4]` | measured loudness normalisation per exciter |

## The four materials

| | node ratios (A B C D) | dispersion core/nodes | T60 100 Hz / 1 kHz / 8 kHz | coupling | selectivity core/nodes | energy tuning |
|---|---|---|---|---|---|---|
| **GLASS** — thin shell | 2.63 4.45 6.55 9.40 | 0.04 / 0.10 | 5.5 / 4.5 / 2.8 s | 0.85 | 0.75 / 0.85 | 0.001 |
| **METAL** — bell | 1.19 1.50 2.00 2.67 | 0.22 / 0.32 | 9.0 / 6.0 / 1.6 s | 1.2 | 0.45 / 0.55 | 0.002 |
| **WOOD** — free bar / body | 2.756 5.404 8.933 13.34 | 0.015 / 0.04 | 1.5 / 0.45 / 0.07 s | 0.75 | 0.60 / 0.80 | 0.004 |
| **MEMBRANE** — circular | 1.594 2.136 2.296 2.653 | 0.28 / 0.28 | 1.6 / 0.35 / 0.08 s | 0.9 | 0.80 / 0.85 | 0.04 |

Measured with identical settings (STRIKE, C4, neutral controls):

| | strongest partials / f0 | T60 at f0 | centroid |
|---|---|---|---|
| GLASS | 1.00 2.63 4.44 6.57 9.39 | 5.2 s | 1217 Hz |
| METAL | 1.00 1.19 1.50 1.98 2.09 2.68 (octave doublet beats) | 8.2 s | 1031 Hz |
| WOOD | 1.00 1.97 2.76 2.96 3.94 5.39 | 0.9 s | 605 Hz |
| MEMBRANE | 1.00 1.60 2.00 2.14 2.30 2.66 | 0.86 s | 417 Hz |

Minimum level-independent log-spectral distance between any two materials: > 3 dB for
STRIKE (measured 28 dB for strike's minimum pair), and > 2 dB for every exciter
(measured minimum 4.2 dB, BOW).

## Modal selectivity — why it lives at the ports

A loop is a full harmonic series. A glass partial, a bar mode or a drum mode is a single
mode. Three placements were built and measured:

1. **Dissipative in-loop bandpass** (banded waveguide) — clean modal spectra, but a
   selective node dissipates everything that is not its own mode. Coupling then acts as
   a damper: GLASS T60 at f0 fell from 3.4 s to 0.6 s at default coupling (predicted
   exactly by the coupling estimator: per-pass factor 0.933).
2. **One-pole steep enough to band** — its DC gain exceeds its gain at f0; the DC cap
   then shortened the fundamental and left sub-f0 modes ringing for 280 s.
3. **Selectivity at the ports (chosen)**: loops keep only the material's natural loss, so
   coupling is *reactive* (energy is exchanged, never destroyed), while each node's
   excitation and radiation pass `S(z) = (1−s) + s·BP_f0(z)` (constant-0 dB-peak bandpass
   at the node's fundamental: |S| ≤ 1, S(f0) = 1). This is the physics of excitation
   position and radiation efficiency. Coupling no longer shortens decay (3.37 s with or
   without coupling).

Exceptions, both deliberate: the CORE is more string-like when plucked/bowed/blown
(selectivity × 0.3 / 0.55 / 0.6), and while bowed/blown the CORE also gets in-loop
selectivity (a sustained drive replaces what coupling dissipates) so stick-slip locks
onto the fundamental of high-Q materials instead of squealing on upper modes.

## Ratios avoid the CORE's harmonics
Two coupled loops with near-identical modes form a doublet split by ≈ φ·f/π — tens of Hz
at default coupling, heard as roughness. Measured: GLASS 4.93 vs the CORE's 5th
harmonic → 4.84/4.96 (31 Hz beat at C4); METAL's hum node (0.5) has a loop overtone on
f0 → −50/+51 cent pair (pitch ambiguous). Hence GLASS ratios sit ~0.4 from integers,
WOOD uses free-bar modes, METAL drops the hum and keeps its nominal-octave doublet on
purpose ("complex beating").

## TENSION
`ratio_eff = ratio^(1 + 0.9·(T − 0.5))` (stretch/compress around the CORE), dispersion
× (0.4 + 1.2·T), HF decay × 2^((T−0.5)·1.5). The CORE is not moved: TENSION is not master
pitch. Measured: METAL node A 1.114 / 1.150 / 1.186 / 1.224 / 1.263 for T = 0.1…0.9
(model within 1 %); fundamental within 0.27 cents for T ∈ [0.2, 0.8] on GLASS/METAL/WOOD,
within 8.3 cents at the extremes (T = 0 / 1 push nodes to 0.6×–43× f0).

## Material Inspector
MASS (T60 × 2^(1.2·m'), coupling × 2^(−m'), energy tuning), BRIGHTNESS (HF decay,
selectivity × 2^(−2·b'), tone), LOSS (T60 × 2^(−4·l')), INHARMONICITY (dispersion ×
2^(3·i'), stretch). Measured on METAL: LOSS 0.2 → 0.8 decay 7.4 → 32.1 dB over 2.3 s;
BRIGHTNESS 0.1 → 0.9 centroid 454 → 1148 Hz; INHARMONICITY 0.1 → 0.9 4th-partial stretch
1.015 → 1.054; MASS 0.1 → 0.9 T60 6.3 → 12.4 s.

## Material changes
Every material quantity morphs along a smoothstep over 300 ms (log domain for ratios
and decays), restarting from the current state if changed mid-morph. The dispersion
stage count is locked for the life of a note and all loop coefficients ramp per
sample. Measured METAL → GLASS mid-note: worst impulsive HF event 3.9 dB above its
neighbourhood (the exponential glide it replaced jumped 56 dB in one 5 ms frame), and
the GLASS mode (2.635) present after the morph.

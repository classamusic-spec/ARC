# ARC — Resonator Design

The resonator primitive is the atom of the ARC network: every node (CORE, A, B, C, D)
is one recirculating waveguide loop. This document records the primitive's design and
the measurements that drove each decision. All numbers come from
`Tests/ResonatorTests.cpp` (`ARCTests resonator`); raw CSVs are in
`docs/measurements/phase2/`.

## 1. Primitives evaluated

| Primitive | File | Role in ARC |
|---|---|---|
| Fractional-delay waveguide loop | `Synthesis/WaveguideResonator.*` | **Network node** (energy carrier, coupling) |
| Damped comb | same loop with dispersion off | Harmonic node behaviour (strings, tubes) |
| Dispersive delay | `AllpassChain` inside the loop | Stiffness / inharmonicity (metal, glass, membrane) |
| Modal bank (complex one-pole modes) | `Synthesis/ModalResonator.h` | Exactly tuned modes; kept for body/material modes |
| Two-pole resonant modes | evaluated as the modal bank's recursion | Replaced by the complex-phasor form (see §5) |

Why a **loop** is the node primitive and not a modal bank: a delay loop carries *all*
partials up to Nyquist for a fixed cost (~5 ns/sample), it can be coupled to other
loops through an orthogonal scattering matrix with a *provable* stability bound (see
`NETWORK_COUPLING.md`), and it supports genuine feedback exciters (bow friction, air
jet). A modal node would need tens of modes per node to reach the same bandwidth and
its coupling stability can only be bounded conservatively.

## 2. Loop structure

```
 in ─►(+)─► delay line (k + allpass fraction) ─► loss G(z) ─► dispersion A(z)^K ─► y
```

* **Loss filter** `G(z) = b0 / (1 − a1 z⁻¹)`, designed so the per-pass gain gives the
  requested T60 at the fundamental *and* at an HF reference frequency
  (two-point match). This is what makes decay frequency dependent (materials specify
  low/high decay). Hard bound: `b0 ≤ (1 − a1)·0.999999`, so `|G| < 1` at every
  frequency (see §6, bug found by the extremes test).
* **Dispersion** — K first-order allpasses, each the bilinear transform of the analog
  allpass `(p − s)/(p + s)` with DC delay `amount·T0/K`. The analog pole
  `p = 2K·f0/amount` is independent of the sample rate → identical stretch pattern at
  every rate (measured: inharmonicity spread across 44.1–192 kHz = **0.7 %**).
  Stage count is limited by the loop budget (each stage costs ≥ 1 sample).
* **Tuning** — the line delay is `fs/f0 − τ_loss(ω0) − τ_disp(ω0)`, so damping and
  dispersion never detune the fundamental.

## 3. Fractional delay — measured comparison

Loop at 48 kHz, T60 3 s, no dispersion. Pitch measured by phase-slope estimation
(sub-0.001-cent resolution). HF gain measured at fractional delay 0.5 (worst case).
`hfDecayRatio` = T60 of the 12th harmonic for a loop of 100.5 samples divided by one of
100.0 samples (1.0 = the fractional part does not change the decay).

| metric | linear | hermite | lagrange3 | **thiran1** |
|---|---|---|---|---|
| max pitch error C1–C6 (cents) | 0.0107 | 0.0104 | 0.00006 | **0.00002** |
| max pitch error C7–C8 (cents) | 0.055 | 0.136 | 0.0074 | **0.00005** |
| gain @ fs/4, frac 0.5 (dB/pass) | −3.01 | −1.07 | −1.07 | **0.00** |
| gain @ 0.45 fs, frac 0.5 (dB/pass) | −16.1 | −12.7 | −12.7 | **0.00** |
| HF decay ratio (frac 0.5 vs 0) | 0.09 | 0.50 | 0.50 | **1.00** |
| sweep SNR, 3 kHz, 40-sample sweep (dB) | 37.1 | 62.1 | **68.1** | 49.0 |
| CPU (ns/sample) | 3.6 | 3.6 | 2.9 | **2.7** |

### Decision: first-order allpass (Thiran-type), exactly matched at f0

* **Pitch** — best everywhere. The plain DC-exact Thiran reached 7.2 cents error at
  C7/C8; replacing it with the closed form
  `a = sin(ω0(1−τ)/2) / sin(ω0(1+τ)/2)` (phase delay exactly τ at ω0) brought C7/C8 to
  0.00005 cents.
* **High-frequency behaviour** — the decisive factor. Lagrange/Hermite lose 1.07 dB
  *per pass* at fs/4 when the fraction is 0.5, so a note's HF decay time halves or
  doubles depending on its fractional delay: brightness and sustain would wobble
  chromatically across the keyboard, and FREEZE could never be lossless. The allpass
  has unit magnitude — HF loss is decided by the material's loss filter only.
* **Modulation** — the allpass's weakness (49 dB vs 68 dB SNR in an aggressive sweep).
  Mitigations implemented and measured:
  * coefficients recomputed every 16 samples and ramped per sample;
  * integer-tap hysteresis (switch only when the fraction leaves [0.35, 1.65));
  * result: a one-octave glide in 200 ms produces **no** measurable HF increase
    (−44.0 → −42.9 dB above 12 kHz) and *lower* waveform curvature than the steady
    state. In an extreme one-octave glide in 30 ms both interpolators show HF from
    Doppler resampling of the loop (Thiran −19.3 dB, Lagrange −24.5 dB), with no
    impulsive click (curvature 0.032 vs 0.024).
* **CPU** — cheapest (one multiply-add pair, one state).
* **Stability** — for τ ∈ [0.35, 1.65) the coefficient is bounded well inside the unit
  circle; near Nyquist, where no stable exact solution exists, it falls back to the
  DC-exact Thiran value (clamped to |a| ≤ 0.95).

## 4. Resonator quality gate — results

| Gate | Criterion | Measured | Result |
|---|---|---|---|
| PITCH | C1–C6 at 44.1/48/88.2/96/192 kHz, harmonic and dispersive, < 1 cent | worst 0.00007 (harmonic), 0.003 (dispersive) cents | PASS |
| PITCH | chromatic C3–C5, < 0.5 cent | worst 0.0001 cents | PASS |
| DECAY | T60 at fundamental within 10 % | worst 0.004 % | PASS |
| DECAY | T60 at HF reference within 25 % | worst 1.4 % | PASS |
| SAMPLE RATE | T60 spread / inharmonicity spread / centroid spread (< 20 kHz) | 0.002 % / 0.7 % / 5.4 % | PASS |
| AUTOMATION | octave in 200 ms: HF < +3 dB, curvature < 1.5× steady state | +1.1 dB, 0.49× | PASS |
| AUTOMATION | octave in 30 ms: no impulsive click, parity with Lagrange | curvature 2.1× steady, HF within 5.2 dB of Lagrange | PASS |
| EXTREMES | f 1 Hz…1 MHz, T60 0.1 ms…1000 s, dispersion 0…5, input 1000 | all finite, bounded | PASS |
| IMPULSE | inharmonicity monotonic in dispersion; harmonic when 0 | 1e-6 / 0.005 / 0.088 / 0.303 | PASS |
| HIGH FREQUENCY | 8/12/16/19 kHz loops at 48 kHz: stable, tuned | worst 0.00001 cents, peak ≤ 1 | PASS |
| CPU | loop incl. loss + 4-stage dispersion | 4.9 ns/sample (0.023 % of a core @ 48 kHz) | PASS |

## 5. Modal bank

Complex one-pole modes `z ← r·e^{jω}·z + x`, output `Im(z)`. Chosen over direct-form
two-pole biquads because the rotation form keeps full precision in float at long
decays and low frequencies. Measured: pitch error 0.0003 cents (32.7 Hz–20 kHz), T60
error 0.06 %, **0.41 ns per mode per sample**. It is used where exact, arbitrary mode
ratios are needed (material body modes) — see `MATERIAL_SYSTEM.md`.

## 6. Bugs the gate caught

1. **Unstable DC mode.** With the loss pole clamped (short loops, heavy damping), the
   two-point match produced `G(0) > 1` (e.g. 1.31 at 19.8 kHz) → the loop's DC mode
   grew to infinity. Fixed with the hard DC bound; the extremes test now covers it.
2. **Dispersion budget.** A shrink loop could exit with a tiny non-zero coefficient,
   leaving four ≥1-sample stages active while the line length assumed none → loops near
   Nyquist were up to 2 semitones flat. Replaced by stage-count budgeting + bisection.
3. **Thiran at high notes.** DC-exact Thiran + iterative refinement oscillated across the
   k/k+1 boundary (7 cents at C8). Replaced by the exact closed form.
4. **SR-dependent dispersion.** The original `D = 1 + amount·L/K` added a 1-sample floor
   per stage (an SR-dependent time), giving 18 % inharmonicity spread across rates.
   The bilinear-consistent `D = amount·L/K` reduced it to 0.7 %.

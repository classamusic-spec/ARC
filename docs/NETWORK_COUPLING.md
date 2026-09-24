# ARC — Network Coupling

ARC's oscillator is a network of five waveguide loops — **CORE, A, B, C, D** — that
exchange energy every sample through an orthogonal scattering matrix.
Code: `Source/Synthesis/CouplingMatrix.*`, `Source/Synthesis/ResonantNetwork.*`.
Tests: `Tests/NetworkTests.cpp` (`ARCTests network`), measurements in
`docs/measurements/phase3/`.

## 1. Structure

```
          y_i = loop_i.read()                 loop output (delay + loss + dispersion)
          u   = Q(n) · y                      scattering (5×5, orthogonal)
          loop_i.write(u_i + injection_i)     exciter energy enters here
          out = Σ g_i · pan_i · y_i            stereo pickup
```

Edges (every pair of the five nodes — 10 total) are grouped as they appear in the
Resonance Field:

| index | edges | role |
|---|---|---|
| 0–3 | CORE–A, CORE–B, CORE–C, CORE–D | spokes (central injection → nodes) |
| 4–7 | A–B, B–D, D–C, C–A | ring (through the small junction points) |
| 8–9 | A–D, B–C | cross |

Topologies select edges: **STAR** (spokes), **RING** (spokes + ring, default),
**WEB** (all), **CHAIN** (CORE–A–B–D–C: energy must travel node to node).

## 2. Coupling = rotation, not feedback gain

Coupling is expressed by a skew-symmetric generator `S` (`S_ab = −S_ba = θ_e`) and the
scattering matrix is its Cayley transform

```
Q = (I − S/2)⁻¹ (I + S/2)            QᵀQ = I  for every S
```

A single edge with generator θ rotates its two node signals by `φ = 2·atan(θ/2)`;
`sin²φ` of each node's energy crosses the edge per pass. There are **no feedback
coefficients to tune for stability**: whatever the coupling amount, topology, number of
edges or their order, `Q` only redistributes energy.

Measured: worst `‖QᵀQ − I‖_max` over 2000 random generators (θ up to 20) = 1.2e-7
(float round-off); single-edge rotations match `cos φ`/`sin φ` to 1e-5.

## 3. Stability argument

Each loop is `H_i(z) = z^{−k_i} T_i(z) G_i(z) A_i(z)^K` with
`T_i` (fractional allpass) and `A_i` (dispersion allpasses) of unit magnitude and the
loss filter `|G_i(e^{jω})| ≤ 0.999999` for **every** ω (hard DC bound, see
RESONATOR_DESIGN §6). The closed network is `y = H(z)(Q y + e)`.

* **Fixed parameters.** For |z| ≥ 1: `‖H(z)Q‖₂ ≤ max_i |H_i(z)| · ‖Q‖₂ < 1` (maximum
  modulus principle; H_i is analytic outside the unit circle). So `I − H(z)Q` is
  invertible on and outside the unit circle → the network is BIBO stable for **any**
  coupling, topology and tuning (small-gain theorem). With `|G_i| = 1` (lossless) the
  state energy is exactly conserved.
* **Automated parameters.** `Q(n)` is interpolated linearly between orthogonal matrices.
  By convexity `‖(1−t)Q₀ + tQ₁‖₂ ≤ (1−t)‖Q₀‖ + t‖Q₁‖ = 1`: an interpolated matrix can
  only lose energy. Loop coefficients ramp per sample; the only energy-injecting
  mechanism left is parametric pumping by time-varying delays, which (measured below)
  is bounded and negligible at ARC's modulation rates, and is capped by the voice energy
  governor in FREEZE.
* **No limiter hides anything.** The network has no saturation stage; the fuzz tests
  run on the raw network.

## 4. Quality gate — results

| Gate | Test | Measured | Result |
|---|---|---|---|
| ENERGY | impulse into CORE only reaches every node (ring, φ=0.2) | A −13.9, B −9.5, C −14.9, D −12.9 dB rel. core; peaks at 21–43 ms | PASS |
| ENERGY | uncoupled: nodes stay silent | exactly 0 | PASS |
| TOPOLOGY | CHAIN delivers energy in order A→B→D→C | peak times 52 → 56 → 71 → 81 ms | PASS |
| STABILITY | WEB, φ = 0.8 / 1.2 / π/2, T60 30 s, 10 s render | peak 0.58, 1-s RMS non-increasing | PASS |
| STABILITY | fuzz, 1500 random networks, abusive modulation (±2.5 % delay jumps + coupling steps every 16 samples), incl. near-lossless | 0 non-finite, stored/injected ≤ 1.68, peak ≤ 7.4 | PASS |
| STABILITY | fuzz, 1500 random lossy networks, smooth modulation (≤1 %, < 20 Hz) | stored/injected ≤ **0.973** (passive) | PASS |
| DECAY | broadband decay, energy monotonic | T60 1.59 s; RMS 5.5e-3 → 6.4e-6 → 9.7e-10 | PASS |
| TOPOLOGY | log-spectral distance between topologies (50 Hz–8 kHz) | 1.5 (star/ring) … 16.7 dB (web/chain) | PASS |
| AUTOMATION | coupling 0 → 1.5 rad → 0 in 400 ms while ringing | curvature 0.47× steady state, finite | PASS |
| LOSSLESS | T60 = 1e7 s, WEB φ=0.8, 10 s | energy ratio 0.996, max/min 1.028 | PASS |
| VISUAL DATA | per-node energy telemetry == manual mean square; edge flux 0 when uncoupled / absent | exact; star ring-flux = 0 | PASS |
| CPU | per voice network (5 loops, 4-stage dispersion, WEB), 48 kHz | 37 ns/sample static, 54 ns drifting (0.26 % of a core) | PASS |

Findings that changed the design:

* **Parametric pumping.** Near-lossless loops under abusive delay modulation stored up
  to 1.68× the injected energy (bounded). Lossy networks never did. Consequence: the
  FREEZE implementation must include an energy governor (Phase 10).
* **Edge telemetry.** The Cayley matrix of a STAR has indirect A↔B entries (one
  scattering event routes A→CORE→B). Telemetry therefore reports each edge's *own*
  rotation × endpoint energies (`sin²φ_e·(E_a+E_b)`), so a missing edge always reads 0
  and the UI can never show a connection that does not exist in the DSP.
* **Coupling detunes the fundamental** (mode repulsion): 0.45 / 1.8 / 5.4 cents at
  φ = 0.1 / 0.2 / 0.35 (RING, harmonic ratios). Addressed by the tuning system
  (TENSION/TUNING phase).

## 5. Control-rate cost

Loop redesign is cached: a full redesign only when T60/dispersion change or the
frequency drifts > 0.3 % from the cached design; otherwise a cheap retune (line length +
allpass only). Under continuous drift at 48 kHz: 8.9 full designs/s and 15 k retunes/s
per voice; the scattering matrix is recomputed only when a generator changes.

## 6. Coupling compensation (added in the tuning phase)

Coupling moves each loop's modes (mode repulsion). Voices pre-compensate every node
using the **exact** reduction of the network onto that node (Schur complement over the
other four loops, a 4×4 complex solve), evaluated at the frequency where the mode must
land:

```
R_i(ω) = Q_ii + Q_io (I − D_o(ω) Q_oo)⁻¹ D_o(ω) Q_oi        D_o = diag(H_k(ω)), k ≠ i
loop_i tuned to  f_i / (1 + arg R_i / 2π)       decay lengthened by 1/|R_i| (≤ 3×)
```

Validation: the bisection root of the exact characteristic equation det(I − D(ω)Q) = 0
matches the measured spectral peak to 0.01 cents, and the compensated voices land on
the note: worst 0.37 cents across GLASS/METAL/WOOD at coupling 0.2 / 0.35 / 0.5, where
the uncompensated shifts are up to 4.4 / 31 / 97 cents.

Two debugging lessons recorded in DEVELOPMENT_LOG: the coincidence taper must measure
distance to a loop's nearest **non-DC** mode (short loops sit near their DC mode at any
low frequency — this alone caused a 12 % under-correction), and the phase → frequency
conversion is exactly `arg R / 2π` (dividing by ω·τ_group introduced a 1/(1+shift) error).
Near a genuine coincidence (another loop resonant within ~0.07 rad of loop phase) the
modes split symmetrically and the correction tapers to zero.

### 6.1 Convergence near unison (Phase 9)
The corrections of coupled loops depend on each other, so the voice iterates them
(every 4 control blocks). With a node ~20 cents from unison with the CORE (found by the
factory preset audit: "Black Bell" held its level after release, centroid 6.4 kHz), the
iteration fell into a period-3 limit cycle — the CORE loop hopped 126 / 129 / 132 Hz
every few milliseconds, i.e. audio-rate loop FM that pumped energy parametrically. Two
changes:

* a **static coincidence taper** computed from the *intended* mode frequencies (which
  the corrections never move, so it cannot feed back), 4th-order in the phase distance
  with a width that scales with the pair's coupling: `taper = d⁴ / (d⁴ + (0.005 + |Q_ik|²)²)`.
  Two loops coupled by |Q_ik| split by ~2|Q_ik| rad; inside that region the per-loop
  correction is ill-posed, so the doublet is left centred on the note (measured: node
  at 0.985–1.015 × f0, WEB coupling 0.4 → doublet centroid within ±3.4 cents);
* under-relaxation 0.25 (was 0.5).

Authority raised from ±5.9 % to ±12 % (extreme TENSION: worst 1.9 cents, was 8.3).
Regression test `tuning/compensation converges near unison`: 42 configurations (ring /
web × coupling 0.35–0.7 × node ratio 0.97–1.03): loop wobble ≤ 0.19 cents (the old
estimator fails all three checks), released notes decay steadily.

### 6.2 The in-tune limit and the COUPLING curve (release candidate)

The release-candidate validation extended the compensation measurement from COUPLING
0.5 to the whole knob, and found the top of the range out of tune. With the Phase 9
curve (`φ = 1.45 c^1.6`), the compensated fundamental was:
* 12–40 cents off at 0.65;
* 100–164 cents off at 0.8;
* up to −186 cents off at 1.0.

At 0.8–1.0 the strongest partial near the note was a composite mode of the network, not
the note.

**Cause.** A trace of the CORE's correction gave, for glass:

| COUPLING | 0.55 | 0.60 | 0.62 | 0.64 and above |
|---|---|---|---|---|
| CORE correction | 7.0 % | 8.7 % | 10.9 % | 12 % (clamped) |

The ±12 % authority bound was reached at COUPLING 0.62–0.64 on every material. Pitch
was exact up to that point and broke immediately after.

Beyond about φ ≈ 0.75 rad a different, physical limit takes over. The CORE loses most
of its energy to the nodes every pass. The energy returns in phase only when a
material's nodes resonate near f0: METAL does, GLASS and WOOD do not. The CORE then
stops existing as a separate mode. With authority ±40 % on the old curve, GLASS and
MEMBRANE held pitch to the old maximum. METAL was still −100 cents at the old 0.8, and
WOOD's fundamental fell 37 dB under a non-octave mode.

**Fix.**
* **Authority ±30 %.** The CORE needs about 15 % at the new top of the range. The
  Phase 9 limit-cycle regression (`compensation converges near unison`, now also run at
  maximum COUPLING) is unchanged: loop wobble 0.18 cents, released notes decay.
* **COUPLING curve `φ = 0.7277 c^0.9432`.** The knob now ends at the rotation the old
  curve reached at 0.65, the largest at which every material stays in tune. The
  default 0.35 still gives 0.270 rad, so the default sound is unchanged. Factory
  presets were converted value by value to keep their rotation. Only Signal Swarm
  (formerly 0.66) moves, by −2.4 %, and the preset audit's loudness and distinctness
  figures are identical.
* RANDOM's coupling windows were converted to the new units (regeneration 0.08–0.92).
* The field's connection weight is renormalised so that full COUPLING reads as full
  weight, and the default reads as before.

**Result** (`tuning/coupling compensation keeps pitch`, all four materials, the whole
range):

| COUPLING | 0.2 – 0.65 | 0.8 | 1.0 |
|---|---|---|---|
| worst compensated error | 1.2 cents | 2.8 cents (metal) | 6.3 cents (metal); glass, wood, membrane ≤ 0.3 |
| note vs strongest nearby partial | 0 dB | 0 dB | −1.3 dB (metal), −0.8 dB (wood) |

Uncompensated, the same networks drift by up to 175 cents at the top. BOW and AIR now
speak across the whole range on every material, ring and web: −21 … −27 dB, within
0.7 cents; MEMBRANE AIR is +7 cents from its energy-dependent tuning. This ends the
Phase 9 "AIR above COUPLING 0.65" limitation, since that region is now beyond the knob.

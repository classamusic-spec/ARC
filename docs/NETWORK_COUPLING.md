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
modes split symmetrically and the correction tapers to zero. Authority ±100 cents.

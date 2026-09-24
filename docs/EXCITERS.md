# ARC — Exciters

How energy enters the network. Four genuinely different mechanisms (not presets of
one), selected per note. Code: `Source/Exciters/*`. Gate: `Tests/ExciterTests.cpp`,
`Tests/AirStabilityTests.cpp`. Measurements: `docs/measurements/phase4-6/`.

| Exciter | Mechanism | Open / closed loop | Injection |
|---|---|---|---|
| STRIKE | momentum-conserving contact pulse | open | CORE + nodes (spread 0.8) through modal selectivity |
| PLUCK | finger drag + release, burst, position comb | open | CORE + nodes (spread 0.45) through modal selectivity |
| BOW | stick-slip friction on the CORE's velocity | **closed** (reads the CORE loop) | CORE raw, nodes (0.3) filtered |
| AIR | turbulence + in-phase air-column drive | **closed** | CORE raw, nodes (0.55) filtered |

## STRIKE
`F(t) = A·sin^p(πt/Tc)` + contact noise, tone tilt, DC-blocked.
* Contact time `Tc` 0.25–9 ms (LENGTH), shortened by hardness × strength (felt stiffening).
* Exponent `p = 1 + 5·hardness` sharpens the pulse; noise grows with hardness².
* Amplitude normalised to **momentum** (pulse area), so a harder/shorter hit is brighter,
  not louder at low frequencies.
* Measured: centroid 862 Hz (hardness 0.1) → 2270 Hz (0.95); 1623 Hz (long contact) →
  2835 Hz (short); velocity 0.3 → 1.0: +12.9 dB and brighter. Energy front-loaded:
  first 100 ms ≥ 6 dB above 1.5 s on every material.

## PLUCK
A 0.6–6 ms displacement ramp (finger drags the string), sudden release, a one-period
Karplus-style noise burst (every note differs), then the pluck-position comb
`x[n] − x[n−P]`, `P = position × loop length` with a **fractional** delay.
* The comb removes the harmonics that have a node at the pluck point. Measured,
  12-pluck average, position ¼: h4 notch 37 dB, h8 notch 28 dB; position ½: even
  harmonics −37 to −40 dB.
* TONE = release sharpness (centroid 493 → 1887 Hz). DAMP = palm mute (loop T60 ×2^(−4·damp)).

## BOW
Friction force `F = env · dv · r(dv)`, `dv = vb − vs`, bow table
`r = clamp((|dv·slope| + knee)^−4, 0.01, 0.98)` (Smith/Cook). `vs` is the CORE loop output,
so the network's own period sets the pitch (Helmholtz-like stick-slip).
* PRESSURE → slope (stick region), SPEED → vb, FRICTION → knee + rosin noise.
* Contact force follows the envelope: the bow lands on attack and is **lifted** on
  release. (A bow that only slowed down stayed in contact and damped GLASS by 24 dB
  in 50 ms — found by the release test.)
* Sustained drives lock onto the most resonant CORE mode; on high-Q materials the
  CORE gets in-loop selectivity while bowed/blown so the lock is the fundamental.
* **Intonation tracker**: stick-slip pulls pitch (as on real bowed strings). A band-passed
  zero-crossing estimator on the CORE steers its tuning (±60 cents authority, 3–8
  periods per update). Measured bowed pitch error: ≤ 0.4 cents on all materials.
* Measured: level drift < 0.7 dB while bowing, HNR 13.5–48 dB (METAL's inharmonic
  nodes ring under the bow), spectral flatness ≤ 1.2e-5 (tonal, not noise).

## AIR
* **Turbulence**: gaussian noise, TONE low-pass, TURBULENCE amount × breath.
* **Air-column drive**: `inj = G·s·tanh(y/s)`, in phase with the CORE (no delay), with
  `G = breath·(loss·tonal + max(0, tonal−1)·6/f0)`. `loss` is the CORE's actual per-pass
  loss at f0 (from the voice), so `tonal = 1` is the self-oscillation threshold on
  *every* material; the second term gives a pitch-independent speaking time (~170 ms).
  FLOW moves from breath (below threshold) to tone (above).
* History: a flute-style half-period jet delay pulled pitch by up to 23 cents (the
  cubic's slope changes sign with breath pressure); a fixed drive only self-oscillated
  on high-Q materials (−42 dB on WOOD at C2). Both replaced as above.
* Measured: breathy vs tonal spectral flatness ratio > 1.5 on every material; pitch
  within 0.6 cents (MEMBRANE +5.9: its energy-dependent tuning); modulation depth
  ≤ −35 dB (no regulator hunting).

## Drive regulation (BOW, AIR)
An envelope follower on the CORE (10 ms attack / 120 ms release) eases the injected
drive when the CORE exceeds `ceiling × target`, so long-decay materials settle at a
musical level instead of accumulating energy (AIR on GLASS peaked at 20.5 before).

## Velocity / EXCITE / pressure
| | velocity | EXCITE | MPE pressure |
|---|---|---|---|
| STRIKE | energy^1.25 + hardness | amplitude + hardness | onset energy |
| PLUCK | energy^1.15 + sharpness | amplitude + sharpness | — |
| BOW | bow energy + pressure | bow speed + pressure | pressure + energy |
| AIR | breath | breath + drive | breath |

Measured (METAL, C4): velocity 0.2 → 1.0 = +17 dB for STRIKE/PLUCK, +3.7 / +3.4 dB for
BOW / AIR (regulated sustain; velocity shapes the attack and pressure). EXCITE 0.1 → 1.0:
+10.2 / +11.7 / +9.8 / +4.6 dB, and STRIKE/PLUCK centroid ×1.72 / ×1.76.

## Loudness normalisation
Measured per exciter × material at C2/C4/C6 (`docs/measurements/phase4-6`), then set per
material (`exciterGainDb`, morphs with the material) plus a per-exciter pitch slope
(STRIKE −0.55: momentum spreads over longer periods at low pitch). Result: all 16
combinations at −22 dB RMS at C4, within ±3 dB from C2 to C6 (AIR within ±5 dB).

# ARC — Product Specification (V1.0 release candidate)

**ARC — Resonant Network Synthesizer.** *Matter shapes sound.*

ARC has no oscillators. Each note injects energy into a network of five interconnected
virtual resonant bodies: a central **CORE** and four nodes, **A–D**. Energy travels
through the network, and every resonator stores, colours, transfers and dissipates it.
**The network itself is the oscillator.** The same network is on screen, as the
instrument's main surface, where it can be touched.

```
EXCITE ──► CONNECT ──► RESONATE ──► EVOLVE
exciter    coupling     materials    motion, gestures, CHAOS, FREEZE
```

## 1. What ARC is for

* **Sound range.** Recognisably physical sounds (glass, metal, wood, membranes, bells,
  strings, plucks, mallets, bowed and blown textures, metallic percussion) through to
  impossible ones: coupled bodies that trade energy, drones that evolve, alien
  acoustic objects, cinematic resonances.
* **Users.** Sound designers, film and game composers, ambient and electronic producers
  and experimental performers. Anyone who wants living acoustic behaviour without
  sampling it.
* **Not.** A subtractive, wavetable or sample-based synth, a cosmetic node visualiser,
  or an effects rack. ARC must sound complete with SPACE and DRIVE at zero.

## 2. Formats and platforms

| Item | V1.0 |
|---|---|
| Formats | VST3, Standalone; AU on macOS (enabled automatically there) |
| Platforms | macOS, Windows, Linux (portable C++20 / JUCE 8; built and validated on Linux) |
| Channels | stereo out, MIDI in (instrument) |
| Sample rates | any host rate; tested 22.05 – 192 kHz |
| Latency | zero |
| Plugin code / bundle | `Arc1` / `com.arcinstruments.arc`, version 1.0.0 |

## 3. The instrument surface

One screen and no piano keyboard; MIDI comes from the host or a controller. Layout,
following the locked reference:

| Area | Controls | Meaning |
|---|---|---|
| Header | logotype · preset `‹ name ♡ ›` with tags · settings · L/R meter · MASTER OUTPUT | |
| Left: **EXCITER** "how it begins" | STRIKE · PLUCK · BOW · AIR | how energy enters the network: an impact, a drag-and-release, stick-slip friction, a turbulent air column |
| Centre: **RESONANCE FIELD** | CORE + nodes A–D, drawn with their real connections | drag nodes to retune and move them; Alt-drag records gestures; click for inspectors |
| Right: **BODY / MATERIAL** "what it resonates as" | GLASS · METAL · WOOD · MEMBRANE | what the network is made of: modal ratios, decay, dispersion, coupling behaviour, energy-dependent tuning |
| Strip: macros | **EXCITE** · **COUPLING** · **TENSION** · **CHAOS** | energy put in · how strongly the bodies exchange energy (in tune over the whole range) · stiffness (partial stretch and node spacing; not a master pitch) · bounded, seeded irregularity |
| Strip: performance | **FREEZE** · **RANDOM** (Shift = regenerate) · **SYNC** | hold the network's energy · musical mutation · lock motion to the host tempo |
| Strip: motion | MOTION depth + rate or division | autonomous drift of the nodes (a small secondary control, see §8) |

Contextual inspectors (dark glass cards over the field) hold the depth: exciter and
material detail, per-node RATIO / DECAY / DAMP / LEVEL / PAN / LINK with gesture
record / loop / clear, CORE topology (STAR / RING / WEB / CHAIN), QUANTIZE, SPACE /
WIDTH / DRIVE, and settings (voice mode, voices, glide, bend range, release damping,
MPE, quality, window size). Hovering over anything shows its name and a one-line
explanation in the chamber's caption.

## 4. Sound engine (summary)

* Five waveguide resonators with exact tuning, frequency-dependent decay and
  dispersion (RESONATOR_DESIGN).
* Energy-preserving coupling: orthogonal Cayley scattering. It is unconditionally
  stable at any coupling, topology or automation, and coupling-induced detuning is
  compensated per node (NETWORK_COUPLING).
* Four physically distinct exciters. BOW and AIR are feedback exciters collocated at
  the CORE (EXCITERS).
* Four materials that reshape the network itself and morph continuously
  (MATERIAL_SYSTEM).
* Motion system: drift, recorded and looping gestures, SYNC to host tempo, seeded
  CHAOS, FREEZE with an energy governor, passive nonlinearities (MOTION_SYSTEM).
* Output stage: width, a small FDN ambience, ADAA drive, master gain, DC blocking and a
  safety soft-clip.

## 5. Playing

| Feature | V1.0 |
|---|---|
| Polyphony | 1–16 voices (default 8), 20 physical voices so stolen notes fade out in their own slots |
| Voice stealing | quietest released voice first, then quietest held; 5 ms fades, no clicks |
| Voice modes | POLY · MONO (last-note priority) · LEGATO (one network glides) |
| Pitch | MIDI note (tuned within 0.66 cents from C1 to C6), bend range 0–24 semitones, glide |
| Velocity | per exciter: strike energy and hardness, pluck energy and sharpness, bow energy and pressure, breath (STRIKE / PLUCK: +17 dB from velocity 0.2 to 1.0) |
| Sustain pedal | yes (bow and air keep driving until the pedal is released) |
| Expression | channel / poly aftertouch → exciter pressure; CC74 → timbre (HF decay and exciter tone) |
| MPE | lower zone (master channel 1, members 2–16), per-note bend ±48 semitones, pressure, timbre |
| Re-strike | a ringing note is re-excited in place, as on a real bell |
| Release | STRIKE / PLUCK ring naturally; RELEASE DAMPING adds a damper; BOW / AIR stop driving and the body decays |

## 6. Motion, CHAOS, FREEZE, RANDOM, SYNC

* **MOTION** makes the nodes drift around their positions (sines plus bounded random
  walks). With SYNC it is phase-locked to the host timeline at 1/4, 1/2, 1, 2, 4 or 8
  bars.
* **Gestures.** Alt-drag a node, or arm REC, to record a 128-point looping path of
  radius and angle. It is sample-rate independent, tempo-aware under SYNC and saved
  with presets and sessions.
* **CHAOS** is seeded and bounded: modal detuning, coupling-strength walks and extra
  routing. At 0 the network is bit-identical across seeds; presets recall their seed.
* **FREEZE** holds the energy of the voices that are sounding (energy governor at
  1.15× capture), transitions smoothly, and lets new notes decay normally.
* **RANDOM** is gentle mutation from weighted, material-aware distributions. Shift +
  RANDOM fully regenerates a patch ("Random Glass Bow" …) and reseeds CHAOS.
  Performance and setup parameters (master, polyphony, bend, MPE, quality) are never
  randomised.

## 7. Presets and state

396 factory presets in 12 categories: 46 signature sounds, including every example named
in the brief (marked *), and a 350-preset library (32 each of PADS, PLUCKED and STRUCK; 30
each of BOWED, AIR and PERCUSSION; 28 each of GLASS, METAL, WOOD and MEMBRANE; 26 each of
DRONES and EXPERIMENTAL). Every preset is measured before it ships: clean, at the same
loudness, in tune, true to its category and unlike every other preset. The full catalogue
with the rules and the measurements is in [PRESETS](PRESETS.md). The signature sounds:

| Category | Presets |
|---|---|
| PADS | Obsidian Bloom*, Soft Machine*, Aurora Lattice, Quiet Orbit |
| PLUCKED | Satellite String*, Crystal Thread*, Wire Garden*, Mercury String* |
| STRUCK | Black Bell*, Zero Gravity Bell*, Cold Plate*, Temple Lattice |
| BOWED | Frozen Wire*, Tension Bloom*, Rosin Glass, Bowed Timber |
| AIR | Glass Choir*, Resonant Fog*, Bottle Organ, Reed Array |
| GLASS | Glass Engine*, Prism Harp, Ice Lattice |
| METAL | Slow Alloy*, Copper Rain*, Iron Flower*, Gamelan Engine |
| WOOD | Magnetic Wood*, Hollow Marimba, Timber Code, Rosewood Keys |
| MEMBRANE | Membrane Sky*, Deep Frame*, Ghost Membrane*, Talking Skin |
| DRONES | Infinite Bow*, Black Monolith, Cathedral Air |
| PERCUSSION | Ceramic Pulse*, Tin Pulse, Log Network, Kinetic Kit |
| EXPERIMENTAL | Hollow Circuit*, Broken Halo*, Entropy Garden, Signal Swarm |

* **The browser** searches as you type (every word must match a name, tag, description or
  category), shows how many presets each category holds or matches, and steps through the
  results with the arrow keys.
* **User presets** are XML files (`~/Documents/ARC/Presets/*.arcpreset`). The library
  also has favourites, previous / next, a modified marker and a host program list.
* **Session state** covers every parameter, the four gestures, the CHAOS / motion seed,
  preset metadata and the editor size. It is versioned and restores bit-identically.
  Legacy or hostile state is handled safely.
* **Parameters** have stable, versioned IDs (`arc.<group>.<name>.v1`); there are 66
  (PARAMETERS), including PATCH LEVEL, each preset's stored loudness trim.

## 8. Design decisions that differ from the reference

The reference is the locked art direction. Four details were changed for function,
each keeping the composition:

1. **MOTION control in the strip's left corner.** The reference has the motto "MATTER
   SHAPES SOUND" there. Drift is part of ARC's identity ("EVOLVE"), and the brief
   allows MOTION as a contextual secondary control, so it is a small knob with a
   rate / division readout. The four macros stay large and central.
2. **Live status in the right corner.** The reference's "RESONANCE CONNECTS
   EVERYTHING" motto is replaced by voice mode, voices sounding and CPU (or host BPM
   under SYNC), with the same visual weight.
3. **The chamber readouts are real.** STABILITY, FLOW and HARMONICS are driven by CHAOS,
   measured edge flow and node-ratio harmonicity. The right-hand text shows the
   hovered node's ratio and level, or interaction hints.
4. **Settings gear in the header**, for the settings card (the reference has no
   settings access).

## 9. Quality

QUALITY sets the control period: ECO 1.33 ms, NORMAL 0.67 ms (default), HIGH
0.33 ms. Sixteen bowed voices at 48 kHz take 8.3 / 11.8 / 18.7 % of one core; NORMAL
and ECO stay within 1.2 / 1.7 dB of HIGH's spectrum at the same pitch. QUALITY can be
switched live without clicks.

## 10. Out of scope for V1 (planned)

Preset morphing (the state architecture already allows it), networks of 16+ nodes,
user-drawn topologies, audio-input excitation, advanced MPE, microtuning / Scala,
granular or spectral excitation, multiband coupling, full WDF physical systems and
complex collision models.

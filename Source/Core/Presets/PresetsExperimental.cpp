// Factory library: EXPERIMENTAL. The network pushed somewhere else: irrational and golden
// tunings, square-root lattices, Morse code, polyrhythms, reversed swells, circuit bending,
// fast orbits, black holes. Pitch is free here (the audit only checks that each one is clean,
// calibrated and unlike every other preset). See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addExperimental (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "EXPERIMENTAL";

    // Morse code for S-O-S on a sixteenth-note grid over two bars: 1 = the node on the octave
    // (in tune, ringing with the note), 0 = the node pushed 2.4 semitones away.
    auto morse = []
    {
        const char* code = "10101000111011101110001010100000"; // S (dit x3), O (dah x3), S, word gap
        std::vector<float> steps;
        for (const char* c = code; *c != 0; ++c)
            steps.push_back (*c == '1' ? 0.0f : 0.10f);
        return steps;
    }();

    auto ramp = [] (float sign)
    {
        std::vector<float> steps;
        for (int i = 0; i < 8; ++i)
            steps.push_back (sign * 0.02f * static_cast<float> (i));
        return steps;
    };

    add (Builder ("Quantum Foam", cat, "chaos, seething, noise, micro",
                  "Space at the smallest scale: tiny glass resonances flickering in and out of existence.")
             .air (0.30f, 0.98f, 0.86f)
             .material (glass, 0.10f, 0.86f, 0.30f, 0.90f)
             .macros (0.80f, 1.0f, 0.80f, 0.90f)
             .topology (web)
             .levels (0.90f, 0.90f, 0.90f, 0.90f)
             .motion (0.80f, 4.0f)
             .fx (1.5f, 0.40f));

    add (Builder ("Glitch Garden", cat, "glitch, stepping, synced, stuttering",
                  "A garden of resonances that jump between pitches on a sixteenth-note grid, never settling.")
             .pluck (0.20f, 0.20f, 0.70f)
             .material (metal, 0.30f, 0.66f, 0.30f, 0.60f)
             .macros (0.64f, 0.66f, 0.60f, 0.20f)
             .topology (web)
             .quantise()
             .synced (quarter, 0.0f)
             .gesture (0, stepGesture (0.25f, 0.5f,
                                       { 0.0f, 10 * kSemitone, -7 * kSemitone, 5 * kSemitone, 0.0f, -11 * kSemitone, 3 * kSemitone,
                                         8 * kSemitone },
                                       0.0f))
             .gesture (2, stepGesture (0.375f, 0.75f, { 0.0f, -5 * kSemitone, 7 * kSemitone, -2 * kSemitone, 9 * kSemitone, -9 * kSemitone }, 0.0f))
             .fx (1.4f, 0.26f, 0.20f));

    add (Builder ("Alien Radio", cat, "radio, sci-fi, warbling, transmission",
                  "A transmission from somewhere else: a tone warbling as if someone is tuning the dial.")
             .air (0.74f, 0.40f, 0.70f)
             .material (metal, 0.40f, 0.66f, 0.34f, 0.60f)
             .macros (0.64f, 0.66f, 0.64f, 0.30f)
             .topology (chain)
             .gesture (0, figureGesture (0.35f, 0.0f, 0.6f, 0.08f, 3, 5))
             .gesture (1, figureGesture (0.52f, 0.0f, 0.6f, 0.08f, 2, 7))
             .fx (1.3f, 0.30f, 0.40f));

    add (Builder ("Tectonic", cat, "grinding, sub, slow, chaos",
                  "Continental plates grinding: an immense, slow stick-slip rumble under everything.")
             .bow (0.90f, 0.10f, 0.90f)
             .material (membrane, 0.96f, 0.14f, 0.30f, 0.80f)
             .macros (0.66f, 0.80f, 0.06f, 0.50f)
             .topology (web)
             .motion (0.50f, 0.03f)
             .fx (1.5f, 0.70f, 0.30f));

    add (Builder ("Neural Lattice", cat, "lattice, irrational, coupled, firing",
                  "A lattice of square-root partials, every node wired to every other: one strike fires the whole network.")
             .strike (0.60f, 0.20f, 0.60f)
             .material (glass, 0.36f, 0.60f, 0.20f, 0.60f)
             .macros (0.64f, 1.0f, 0.56f, 0.30f)
             .topology (web)
             .links (1.0f, 1.0f, 1.0f, 1.0f)
             .chord (2.0f * std::sqrt (2.0f), 2.0f * std::sqrt (3.0f), 2.0f * std::sqrt (5.0f), 2.0f * std::sqrt (7.0f))
             .fx (1.5f, 0.44f));

    add (Builder ("Particle Accelerator", cat, "whirling, fast orbit, stereo, sci-fi",
                  "Partials whipped around the core at several revolutions a second: a whirling, Doppler-bent ring.")
             .air (0.70f, 0.30f, 0.76f)
             .material (glass, 0.30f, 0.74f, 0.24f, 0.56f)
             .macros (0.62f, 0.60f, 0.76f, 0.16f)
             .gesture (0, orbitGesture (0.25f, 0.0f, 1.0f, 0.04f))
             .gesture (1, orbitGesture (0.33f, 0.0f, -1.0f, 0.04f))
             .gesture (2, orbitGesture (0.50f, 0.0f, 1.0f, 0.04f))
             .gesture (3, orbitGesture (0.20f, 0.0f, -1.0f, 0.04f))
             .fx (1.5f, 0.36f));

    add (Builder ("Morse Resonance", cat, "morse, rhythmic, synced, coded",
                  "A held tone whose octave node spells S-O-S in Morse, stepping in and out of tune with the note.")
             .air (0.66f, 0.20f, 0.56f)
             .material (metal, 0.46f, 0.56f, 0.24f, 0.46f)
             .macros (0.60f, 0.70f, 0.70f, 0.04f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.90f, 0.30f, 0.30f, 0.30f)
             .synced (bar2, 0.0f)
             .gesture (0, stepGesture (4.0f, 8.0f, morse, 0.0f))
             .fx (1.2f, 0.30f));

    add (Builder ("Black Hole", cat, "dark, dense, sub, collapse",
                  "Everything falls in: the heaviest, darkest metal bowed almost to a standstill, collapsing into a dense rumble.")
             .bow (0.70f, 0.10f, 0.70f)
             .material (metal, 0.98f, 0.04f, 0.06f, 0.70f)
             .macros (0.64f, 0.90f, 0.04f, 0.40f)
             .topology (web)
             .motion (0.30f, 0.02f)
             .fx (1.5f, 1.0f, 0.20f));

    add (Builder ("Fractal Bells", cat, "golden ratio, self-similar, bells, glass",
                  "Glass bells whose partials are successive powers of the golden ratio: the same shape at every scale.")
             .strike (0.66f, 0.12f, 0.66f)
             .material (glass, 0.44f, 0.60f, 0.16f, 0.50f)
             .macros (0.62f, 0.50f, 0.50f, 0.06f)
             .chord (1.618f, 2.618f, 4.236f, 6.854f)
             .fx (1.4f, 0.50f));

    add (Builder ("Irrational Harp", cat, "irrational, pi, e, strange",
                  "A harp strung by a mathematician: sympathetic strings tuned to e, pi, two pi and e squared.")
             .pluck (0.24f, 0.12f, 0.62f)
             .material (wood, 0.44f, 0.56f, 0.28f, 0.50f)
             .macros (0.62f, 0.54f, 0.50f, 0.04f)
             .chord (2.71828f, 3.14159f, 6.28318f, 7.38906f)
             .fx (1.4f, 0.40f));

    add (Builder ("Tritone Engine", cat, "tritone, tension, resolution, synced",
                  "The devil's interval, bowed: two tritone nodes hold the tension for three beats and resolve to the fifth on the fourth.")
             .bow (0.46f, 0.56f, 0.54f)
             .material (metal, 0.40f, 0.70f, 0.26f, 0.50f)
             .macros (0.60f, 0.56f, 0.60f, 0.06f)
             .topology (chain)
             .chord (kTritone, 2.0f, 2.0f * kTritone, 4.0f)
             .synced (bar1, 0.0f)
             .gesture (0, stepGesture (2.0f, 4.0f, { 0.0f, 0.0f, 0.0f, kSemitone }, 0.3f))
             .gesture (2, stepGesture (2.0f, 4.0f, { 0.0f, 0.0f, 0.0f, kSemitone }, 0.3f))
             .fx (1.5f, 0.56f, 0.24f));

    add (Builder ("Bent Circuit", cat, "circuit bent, harsh, glitch, driven",
                  "A toy keyboard with its circuit shorted: harsh, overdriven and chattering.")
             .pluck (0.10f, 0.10f, 0.90f)
             .material (metal, 0.24f, 0.80f, 0.40f, 0.80f)
             .macros (0.70f, 0.80f, 0.80f, 0.64f)
             .topology (chain)
             .quantise()
             .synced (quarter, 0.0f)
             .gesture (1, stepGesture (0.125f, 0.25f, { 0.0f, 0.30f, -0.20f, 0.40f }, 0.0f))
             .gesture (3, stepGesture (0.1875f, 0.375f, { 0.10f, -0.30f, 0.25f }, 0.0f))
             .fx (1.2f, 0.20f, 0.90f));

    add (Builder ("Spectral Ghosts", cat, "ghostly, spectral, noise, inharmonic",
                  "The resonances of a room long after the music has stopped: faint, high, drifting partials made of air.")
             .air (0.28f, 0.94f, 0.66f)
             .material (glass, 0.36f, 0.64f, 0.08f, 0.76f)
             .macros (0.76f, 0.70f, 0.60f, 0.30f)
             .topology (ring)
             .chord (3.3f, 5.7f, 8.9f, 13.1f)
             .levels (0.90f, 0.90f, 0.90f, 0.90f)
             .gesture (0, wanderGesture (17.0f, 0.0f, 1.0f, 0.02f, seedFor ("Spectral Ghosts A")))
             .gesture (2, wanderGesture (23.0f, 0.0f, 1.0f, 0.02f, seedFor ("Spectral Ghosts C")))
             .fx (1.5f, 0.84f));

    add (Builder ("Metallic Speech", cat, "talking, robot, vowels, formant",
                  "A metal throat forming vowels: a buzzing tone that says something, over and over, in a language of its own.")
             .air (0.72f, 0.24f, 0.60f)
             .material (metal, 0.44f, 0.60f, 0.26f, 0.50f)
             .macros (0.62f, 0.60f, 0.70f, 0.06f)
             .topology (star)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.90f, 0.90f, 0.30f, 0.30f)
             .synced (bar1, 0.0f)
             .gesture (0, stepGesture (2.0f, 4.0f, { 0.0f, -0.20f, -0.40f, -0.10f }, 0.35f))
             .gesture (1, stepGesture (2.0f, 4.0f, { 0.0f, -0.18f, -0.30f, -0.10f }, 0.35f))
             .fx (1.1f, 0.24f, 0.30f));

    add (Builder ("Rewired", cat, "inside out, extremes, chain, strange",
                  "The network turned inside out: every node pushed to the far edge of its reach, alternately high and low, chained together.")
             .strike (0.60f, 0.20f, 0.56f)
             .material (wood, 0.46f, 0.56f, 0.30f, 0.50f)
             .macros (0.64f, 0.84f, 0.50f, 0.20f)
             .topology (chain)
             .nodes ("radius", 0.96f, 0.04f, 0.96f, 0.04f)
             .fx (1.4f, 0.34f));

    add (Builder ("Magnetic Storm", cat, "static, crackling, noise, chaos, aurora",
                  "A solar storm on the magnetic field: crackling, fizzing static with metallic resonances flaring through it.")
             .air (0.34f, 0.96f, 0.30f)
             .material (metal, 0.56f, 0.40f, 0.36f, 0.90f)
             .macros (0.80f, 0.90f, 0.40f, 0.90f)
             .topology (web)
             .levels (0.90f, 0.90f, 0.90f, 0.90f)
             .motion (0.90f, 4.0f)
             .fx (1.5f, 0.80f, 0.40f));

    add (Builder ("Frozen Time", cat, "still, stretched, slow, glass",
                  "One moment stretched out: glass bowed so gently and slowly the sound seems not to move at all.")
             .bow (0.24f, 0.18f, 0.30f)
             .material (glass, 0.52f, 0.56f, 0.06f, 0.50f)
             .macros (0.56f, 0.50f, 0.56f, 0.02f)
             .topology (ring)
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .gesture (0, orbitGesture (120.0f, 0.0f, 1.0f, 0.004f))
             .fx (1.5f, 0.90f));

    add (Builder ("Microtonal Maze", cat, "microtonal, quarter tones, beating, detuned",
                  "Harmonics bent by quarter tones: familiar intervals, slightly wrong, beating against each other.")
             .strike (0.60f, 0.16f, 0.60f)
             .material (glass, 0.40f, 0.60f, 0.24f, 0.50f)
             .macros (0.62f, 0.56f, 0.56f, 0.06f)
             .chord (2.0f * semis (0.5f), 3.0f * semis (-0.5f), 4.0f * semis (1.5f), 6.0f * semis (-1.5f))
             .fx (1.4f, 0.44f));

    add (Builder ("Gravity Well", cat, "falling, pitch glide, membrane, pluck",
                  "A pluck dropped into a well: its pitch sinks as the energy drains away.")
             .pluck (0.30f, 0.14f, 0.56f)
             .material (membrane, 0.22f, 0.50f, 0.20f, 0.50f)
             .macros (0.92f, 0.44f, 0.20f, 0.10f)
             .topology (star)
             .fx (1.3f, 0.50f));

    add (Builder ("Tension Snap", cat, "stiff, stretched, snap, inharmonic",
                  "Metal at breaking tension: partials stretched far apart by stiffness, a hard, springy snap.")
             .strike (0.80f, 0.05f, 0.76f)
             .material (metal, 0.20f, 0.80f, 0.30f, 1.0f)
             .macros (0.56f, 0.50f, 1.0f, 0.10f)
             .topology (star)
             .fx (1.2f, 0.30f));

    add (Builder ("Anti-Matter", cat, "reverse, swelling, sucking, synced",
                  "Tape played backwards: resonances sweep in every bar and snap away, as if the sound were being sucked in.")
             .air (0.60f, 0.40f, 0.56f)
             .material (glass, 0.40f, 0.60f, 0.30f, 0.50f)
             .macros (0.60f, 0.60f, 0.56f, 0.10f)
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .synced (bar1, 0.0f)
             .gesture (0, stepGesture (2.0f, 4.0f, ramp (1.0f), 1.0f))
             .gesture (1, stepGesture (2.0f, 4.0f, ramp (-1.0f), 1.0f))
             .fx (1.5f, 0.60f));

    add (Builder ("Hive Mind", cat, "swarm, buzzing, insects, restless",
                  "A hive thinking as one: buzzing skins with every node darting about on its own restless loop.")
             .air (0.66f, 0.50f, 0.60f)
             .material (membrane, 0.64f, 0.60f, 0.40f, 0.60f)
             .macros (0.64f, 0.70f, 0.56f, 0.40f)
             .topology (web)
             .gesture (0, wanderGesture (0.6f, 0.0f, 0.8f, 0.03f, seedFor ("Hive Mind A")))
             .gesture (1, wanderGesture (0.7f, 0.0f, 0.8f, 0.03f, seedFor ("Hive Mind B")))
             .gesture (2, wanderGesture (0.8f, 0.0f, 0.8f, 0.03f, seedFor ("Hive Mind C")))
             .gesture (3, wanderGesture (0.9f, 0.0f, 0.8f, 0.03f, seedFor ("Hive Mind D")))
             .fx (1.5f, 0.30f, 0.30f));

    add (Builder ("Clockwork Orbit", cat, "gears, clockwork, synced, orbit",
                  "Gears within gears: four nodes orbiting at one, two, four and eight beats per turn, like the wheels of a clock.")
             .strike (0.76f, 0.06f, 0.70f)
             .material (metal, 0.30f, 0.70f, 0.30f, 0.56f)
             .macros (0.64f, 0.60f, 0.66f, 0.10f)
             .topology (ring)
             .quantise()
             .synced (quarter, 0.0f)
             .gesture (0, orbitGesture (0.5f, 1.0f, 1.0f, 0.0f))
             .gesture (1, orbitGesture (1.0f, 2.0f, -1.0f, 0.0f))
             .gesture (2, orbitGesture (2.0f, 4.0f, 1.0f, 0.0f))
             .gesture (3, orbitGesture (4.0f, 8.0f, -1.0f, 0.0f))
             .fx (1.4f, 0.30f));

    add (Builder ("Polyrhythm Engine", cat, "polyrhythm, 3 against 4 against 5, synced, sequenced",
                  "Three nodes stepping three, four and five times to the bar over one bowed note: a polyrhythm made of resonance.")
             .bow (0.46f, 0.50f, 0.54f)
             .material (wood, 0.50f, 0.56f, 0.34f, 0.50f)
             .macros (0.60f, 0.56f, 0.54f, 0.06f)
             .quantise()
             .chord (2.0f, 3.0f, 8.0f, 12.0f)
             .synced (bar1, 0.0f)
             .gesture (0, stepGesture (2.0f, 4.0f, { 0.0f, 7 * kSemitone, 0.0f }, 0.1f))
             .gesture (1, stepGesture (2.0f, 4.0f, { 0.0f, 5 * kSemitone, 0.0f, 12 * kSemitone }, 0.1f))
             .gesture (2, stepGesture (2.0f, 4.0f, { 0.0f, 4 * kSemitone, 7 * kSemitone, 4 * kSemitone, 0.0f }, 0.1f))
             .fx (1.35f, 0.30f));

    add (Builder ("Data Rain", cat, "digital, cascading, pentatonic, synced",
                  "Green code falling down a screen: bright glass plucks whose resonances flicker through a pentatonic grid.")
             .pluck (0.16f, 0.16f, 0.80f)
             .material (glass, 0.24f, 0.74f, 0.30f, 0.50f)
             .macros (0.64f, 0.66f, 0.66f, 0.14f)
             .quantise()
             .synced (quarter, 0.0f)
             .gesture (0, stepGesture (1.0f, 2.0f,
                                       { 0.0f, 5 * kSemitone, 2 * kSemitone, 9 * kSemitone, 7 * kSemitone, 0.0f, -3 * kSemitone,
                                         4 * kSemitone },
                                       0.0f))
             .gesture (1, stepGesture (1.0f, 2.0f,
                                       { -5 * kSemitone, 0.0f, 7 * kSemitone, -3 * kSemitone, 0.0f, 9 * kSemitone, 2 * kSemitone,
                                         -7 * kSemitone },
                                       0.0f))
             .fx (1.5f, 0.36f));

    add (Builder ("Event Horizon", cat, "edge, bright, suspended, vast",
                  "At the edge of the black hole: every node pushed to its outer rim, bright and suspended, time slowing to a crawl.")
             .air (0.62f, 0.30f, 0.64f)
             .material (metal, 0.50f, 0.66f, 0.10f, 0.60f)
             .macros (0.60f, 0.76f, 0.66f, 0.16f)
             .topology (web)
             .nodes ("radius", 0.96f, 0.96f, 0.96f, 0.96f)
             .gesture (0, orbitGesture (60.0f, 0.0f, 1.0f, 0.0f))
             .gesture (2, orbitGesture (90.0f, 0.0f, -1.0f, 0.0f))
             .fx (1.5f, 0.96f));
}

} // namespace arc::presets::detail

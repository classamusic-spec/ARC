// Factory library: DRONES. Held, sustaining worlds: tanpura and shruti box, organum and
// overtone singing, horns, hums, ice, stone and machinery. Every drone must hold within 10 dB
// over two seconds (the library audit checks it). Slow gestures (tens of seconds, or eight
// synced bars) keep them moving without a pulse. See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addDrones (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "DRONES";

    // Overtone melody for Overtone Singer: harmonics 6, 8, 9, 10, 12, 10, 9, 8 of the note,
    // as radius offsets from harmonic 6 (radius = half an octave per 0.5).
    auto harmonicStep = [] (float h) { return 0.5f * std::log2 (h / 6.0f); };

    add (Builder ("Tanpura Haze", cat, "tanpura, sa-pa, shimmering, indian",
                  "A tanpura's endless Sa and Pa: steel strings buzzing on their bridges, harmonics shimmering in and out.")
             .bow (0.40f, 0.40f, 0.62f)
             .material (metal, 0.60f, 0.52f, 0.20f, 0.50f)
             .macros (0.60f, 0.52f, 0.50f, 0.20f)
             .topology (ring)
             .chord (1.5f, 2.0f, 3.0f, 4.0f)
             .gesture (1, wanderGesture (11.0f, 0.0f, 0.3f, 0.01f, seedFor ("Tanpura Haze B")))
             .gesture (3, wanderGesture (13.0f, 0.0f, 0.3f, 0.01f, seedFor ("Tanpura Haze D")))
             .motion (0.20f, 0.06f)
             .fx (1.4f, 0.56f));

    add (Builder ("Shruti Box", cat, "shruti box, reeds, bellows, drone",
                  "A shruti box pumped by hand: free reeds on Sa, Pa and the low Pa, the bellows swelling every two bars.")
             .air (0.66f, 0.18f, 0.44f)
             .material (metal, 0.60f, 0.44f, 0.36f, 0.46f)
             .macros (0.60f, 0.40f, 0.50f, 0.04f)
             .chord (0.75f, 1.5f, 2.0f, 3.0f)
             .synced (bar2, 0.0f)
             .gesture (0, breatheGesture (4.0f, 8.0f, 0.02f, 1))
             .gesture (2, breatheGesture (4.0f, 8.0f, 0.02f, 2))
             .fx (1.2f, 0.40f));

    add (Builder ("Overtone Singer", cat, "overtone, throat singing, melody, harmonics",
                  "Khoomei: one held drone, and a whistling overtone melody picked out of it harmonic by harmonic.")
             .air (0.66f, 0.22f, 0.40f)
             .material (wood, 0.66f, 0.40f, 0.30f, 0.50f)
             .macros (0.62f, 0.44f, 0.50f, 0.02f)
             .chord (2.0f, 4.0f, 6.0f, 8.0f)
             .levels (0.20f, 0.20f, 0.90f, 0.10f)
             .decays (0.50f, 0.50f, 0.90f, 0.50f)
             .gesture (2, stepGesture (12.0f, 0.0f,
                                       { 0.0f, harmonicStep (8.0f), harmonicStep (9.0f), harmonicStep (10.0f), harmonicStep (12.0f),
                                         harmonicStep (10.0f), harmonicStep (9.0f), harmonicStep (8.0f) },
                                       0.25f))
             .fx (1.2f, 0.56f));

    add (Builder ("Deep Space Hum", cat, "space, sub, dark, vast, slow",
                  "A low hum in deep space: heavy, dark metal barely bowed, with partials orbiting over most of a minute.")
             .bow (0.40f, 0.36f, 0.50f)
             .material (metal, 0.86f, 0.20f, 0.20f, 0.56f)
             .macros (0.60f, 0.52f, 0.36f, 0.14f)
             .topology (web)
             .gesture (0, orbitGesture (40.0f, 0.0f, 1.0f, 0.01f))
             .gesture (2, orbitGesture (56.0f, 0.0f, -1.0f, 0.01f))
             .fx (1.5f, 0.84f));

    add (Builder ("Organum", cat, "organum, fourths, fifths, chant, medieval",
                  "Medieval organum: voices held on the fourth, fifth and octave, hanging in the air of a stone nave.")
             .air (0.64f, 0.26f, 0.42f)
             .material (metal, 0.60f, 0.40f, 0.30f, 0.44f)
             .macros (0.60f, 0.40f, 0.50f, 0.02f)
             .chord (kFourth, kFifth, 2.0f, 3.0f)
             .fx (1.3f, 0.84f));

    add (Builder ("Dark Matter", cat, "dark, heavy, chaos, sub",
                  "A vast, slack skin bowed in the dark: a heavy, shifting mass you feel as much as hear.")
             .bow (0.56f, 0.30f, 0.60f)
             .material (membrane, 0.92f, 0.22f, 0.26f, 0.60f)
             .macros (0.62f, 0.66f, 0.30f, 0.30f)
             .topology (web)
             .motion (0.40f, 0.04f)
             .fx (1.4f, 0.66f, 0.16f));

    add (Builder ("Solar Drone", cat, "bright, radiant, glass, orbit",
                  "Sunlight as sound: a bright glass drone whose harmonics wheel slowly around the core like flares.")
             .air (0.70f, 0.20f, 0.72f)
             .material (glass, 0.44f, 0.70f, 0.24f, 0.46f)
             .macros (0.60f, 0.52f, 0.58f, 0.08f)
             .topology (star)
             .chord (2.0f, 3.0f, 4.0f, 10.0f)
             .gesture (0, orbitGesture (23.0f, 0.0f, 1.0f, 0.006f))
             .gesture (1, orbitGesture (29.0f, 0.0f, -1.0f, 0.006f))
             .gesture (2, orbitGesture (31.0f, 0.0f, 1.0f, 0.006f))
             .fx (1.5f, 0.66f));

    add (Builder ("Carnyx", cat, "carnyx, war horn, bronze, snarling",
                  "A Celtic carnyx held high over the battlefield: a bronze war horn snarling one long, driven note.")
             .air (0.90f, 0.30f, 0.60f)
             .material (metal, 0.66f, 0.54f, 0.40f, 0.56f)
             .macros (0.64f, 0.52f, 0.60f, 0.20f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .voice (VoiceMode::mono, 0.20f)
             .fx (1.3f, 0.60f, 0.48f));

    add (Builder ("Glacier", cat, "ice, slow, evolving, cold",
                  "Bowed ice creeping downhill: the partials shift so slowly you only notice after a minute.")
             .bow (0.30f, 0.30f, 0.40f)
             .material (glass, 0.66f, 0.42f, 0.18f, 0.50f)
             .macros (0.58f, 0.46f, 0.46f, 0.06f)
             .topology (ring)
             .gesture (0, wanderGesture (47.0f, 0.0f, 0.5f, 0.03f, seedFor ("Glacier A")))
             .gesture (1, wanderGesture (61.0f, 0.0f, 0.5f, 0.03f, seedFor ("Glacier B")))
             .gesture (3, wanderGesture (73.0f, 0.0f, 0.5f, 0.03f, seedFor ("Glacier D")))
             .fx (1.5f, 0.76f));

    add (Builder ("Earth Hum", cat, "earth, rumble, noise, sub, geological",
                  "The planet's own hum: filtered rumble resonating low in the ground, slow and immovable.")
             .air (0.30f, 0.90f, 0.20f)
             .material (wood, 0.90f, 0.20f, 0.14f, 0.50f)
             .macros (0.76f, 0.50f, 0.40f, 0.20f)
             .topology (chain)
             .levels (0.90f, 0.90f, 0.80f, 0.70f)
             .motion (0.30f, 0.03f)
             .fx (1.3f, 0.60f));

    add (Builder ("Pipe Drone", cat, "bagpipe, drones, reedy, roaring",
                  "The drones of a Highland pipe: bass and tenors roaring in octaves, one tenor a hair apart so it beats.")
             .air (0.84f, 0.24f, 0.58f)
             .material (wood, 0.56f, 0.56f, 0.36f, 0.50f)
             .macros (0.64f, 0.46f, 0.50f, 0.08f)
             .chord (2.0f, 4.0f * 1.003f, 6.0f, 8.0f)
             .fx (1.2f, 0.40f, 0.30f));

    add (Builder ("Mantra", cat, "chant, vowels, low, meditative",
                  "A low, wordless chant: the vowel turning slowly from 'o' to 'm' and back in a dim hall.")
             .air (0.60f, 0.40f, 0.36f)
             .material (membrane, 0.70f, 0.36f, 0.28f, 0.46f)
             .macros (0.60f, 0.52f, 0.46f, 0.06f)
             .topology (web)
             .chord (1.5f, 2.0f, 3.0f, 4.0f)
             .gesture (0, figureGesture (16.0f, 0.0f, 0.2f, 0.06f, 1, 1))
             .gesture (1, figureGesture (16.0f, 0.0f, 0.2f, 0.08f, 1, 2))
             .fx (1.5f, 0.70f));

    add (Builder ("Electric Hum", cat, "hum, mains, buzzing, harmonic, menacing",
                  "A transformer humming at the mains: buzzing, harmonic and faintly menacing.")
             .air (0.80f, 0.10f, 0.66f)
             .material (metal, 0.50f, 0.60f, 0.40f, 0.44f)
             .macros (0.62f, 0.60f, 0.70f, 0.10f)
             .topology (chain)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .fx (1.0f, 0.24f, 0.56f));

    add (Builder ("The Void", cat, "void, dark, noise, vast, empty",
                  "Nothing, amplified: faint air resonating in an unlit, endless space.")
             .air (0.22f, 0.96f, 0.30f)
             .material (glass, 0.80f, 0.20f, 0.12f, 0.56f)
             .macros (0.78f, 0.60f, 0.40f, 0.24f)
             .topology (web)
             .levels (0.90f, 0.90f, 0.90f, 0.90f)
             .fx (1.5f, 1.0f));

    add (Builder ("Singing Stones", cat, "stones, rubbed, grainy, inharmonic",
                  "River stones rubbed together until they sing: heavy, grainy and inharmonic.")
             .bow (0.66f, 0.24f, 0.70f)
             .material (glass, 0.80f, 0.30f, 0.30f, 0.70f)
             .macros (0.62f, 0.56f, 0.42f, 0.18f)
             .topology (web)
             .fx (1.3f, 0.50f));

    add (Builder ("Pythagoras", cat, "fifths, pure, stacked, ancient",
                  "Four pure fifths stacked on the note: bright, open and ancient, like a drone from a Pythagorean monochord.")
             .bow (0.40f, 0.44f, 0.50f)
             .material (metal, 0.56f, 0.50f, 0.22f, 0.46f)
             .macros (0.60f, 0.48f, 0.50f, 0.04f)
             .chord (1.5f, 2.25f, 3.375f, 5.0625f)
             .fx (1.4f, 0.50f));

    add (Builder ("Machine Room", cat, "industrial, machine, rhythmic, synced, hum",
                  "The engine room of a ship: a droning hum with valves and pistons ticking in time.")
             .air (0.72f, 0.36f, 0.52f)
             .material (metal, 0.66f, 0.46f, 0.36f, 0.62f)
             .macros (0.64f, 0.56f, 0.50f, 0.26f)
             .topology (ring)
             .synced (half, 0.0f)
             .gesture (0, stepGesture (1.0f, 2.0f, { 0.0f, 0.03f, 0.0f, 0.06f }, 0.1f))
             .gesture (1, stepGesture (1.0f, 2.0f, { 0.04f, 0.0f, 0.04f, 0.0f }, 0.1f))
             .fx (1.2f, 0.30f, 0.36f));

    add (Builder ("Polar Night", cat, "cold, dark, minor, still",
                  "Midwinter at the pole: a still, dark, bowed-glass minor chord under a sky that never lightens.")
             .bow (0.30f, 0.34f, 0.40f)
             .material (glass, 0.62f, 0.32f, 0.20f, 0.46f)
             .macros (0.58f, 0.46f, 0.44f, 0.06f)
             .chord (2.4f, 3.0f, 3.6f, 6.0f)
             .fx (1.5f, 0.80f));

    add (Builder ("Cello Drone", cat, "cello, open fifth, double stop, low",
                  "Two open cello strings bowed together for as long as the bow will last: a low, woody open fifth.")
             .bow (0.50f, 0.34f, 0.58f)
             .material (wood, 0.76f, 0.36f, 0.40f, 0.46f)
             .macros (0.60f, 0.40f, 0.44f, 0.04f)
             .chord (1.5f, 3.0f, 6.0f, 8.0f)
             .levels (0.80f, 0.50f, 0.30f, 0.20f)
             .fx (1.2f, 0.44f));

    add (Builder ("Wind Tunnel", cat, "wind, roar, noise, gusts",
                  "Air roaring through a wind tunnel: a gusting, resonant noise that never stops.")
             .air (0.34f, 0.96f, 0.56f)
             .material (metal, 0.50f, 0.50f, 0.30f, 0.60f)
             .macros (0.76f, 0.70f, 0.50f, 0.40f)
             .topology (web)
             .levels (0.90f, 0.90f, 0.90f, 0.90f)
             .motion (0.60f, 0.20f)
             .fx (1.5f, 0.40f));

    add (Builder ("Bell Drone", cat, "bell, endless, ringing, drone",
                  "A great bell that never stops ringing: one stroke, then a hum that hangs in the air while its partials circle.")
             .strike (0.40f, 0.40f, 0.40f)
             .material (metal, 0.70f, 0.40f, 0.04f, 0.50f)
             .macros (0.62f, 0.40f, 0.50f, 0.04f)
             .gesture (0, orbitGesture (19.0f, 0.0f, 1.0f, 0.004f))
             .gesture (3, orbitGesture (27.0f, 0.0f, -1.0f, 0.004f))
             .release (0.0f)
             .fx (1.4f, 0.60f));

    add (Builder ("Tidal Drone", cat, "tide, swelling, slow, synced",
                  "A drone that swells and ebbs like the tide, its resonances rising and falling over eight bars.")
             .bow (0.40f, 0.40f, 0.50f)
             .material (wood, 0.64f, 0.40f, 0.30f, 0.46f)
             .macros (0.60f, 0.50f, 0.46f, 0.06f)
             .chord (2.0f, 3.0f, 5.0f, 8.0f)
             .synced (bar8, 0.0f)
             .gesture (0, breatheGesture (16.0f, 32.0f, 0.04f, 1))
             .gesture (1, breatheGesture (16.0f, 32.0f, 0.04f, 2))
             .gesture (2, breatheGesture (16.0f, 32.0f, 0.04f, 3))
             .fx (1.5f, 0.70f));

    add (Builder ("Rust Belt", cat, "rust, industrial, roar, chaos, inharmonic",
                  "Wind forced through rusting pipes in an abandoned mill: a corroded, roaring drone.")
             .air (0.78f, 0.60f, 0.50f)
             .material (metal, 0.70f, 0.40f, 0.48f, 0.72f)
             .macros (0.64f, 0.60f, 0.44f, 0.34f)
             .topology (web)
             .fx (1.3f, 0.40f, 0.46f));

    add (Builder ("Crystal Drone", cat, "glass, bright, slow changes, synced",
                  "A bowed crystal drone whose upper voices shift a step every two bars: the harmony changes while the note holds.")
             .bow (0.30f, 0.46f, 0.36f)
             .material (glass, 0.40f, 0.66f, 0.20f, 0.46f)
             .macros (0.58f, 0.44f, 0.60f, 0.04f)
             .quantise()
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .synced (bar8, 0.0f)
             .gesture (1, stepGesture (16.0f, 32.0f, { 0.0f, 2 * kSemitone, 0.0f, -2 * kSemitone }, 0.6f))
             .gesture (3, stepGesture (16.0f, 32.0f, { 0.0f, 0.0f, 3 * kSemitone, 0.0f }, 0.6f))
             .fx (1.5f, 0.66f));

    add (Builder ("Low Orbit", cat, "orbit, circling, spatial, hum",
                  "A hum circling overhead: every harmonic on its own slow orbit around the listener.")
             .air (0.62f, 0.30f, 0.46f)
             .material (metal, 0.58f, 0.46f, 0.26f, 0.48f)
             .macros (0.60f, 0.44f, 0.60f, 0.06f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .gesture (0, orbitGesture (8.0f, 0.0f, 1.0f, 0.0f))
             .gesture (1, orbitGesture (12.0f, 0.0f, 1.0f, 0.0f))
             .gesture (2, orbitGesture (16.0f, 0.0f, -1.0f, 0.0f))
             .gesture (3, orbitGesture (20.0f, 0.0f, -1.0f, 0.0f))
             .fx (1.5f, 0.50f));

    add (Builder ("Harmonic Series", cat, "just intonation, harmonic seventh, 4:5:6:7, pure",
                  "Harmonics four to seven held over the note: a just-intoned seventh chord with no beating at all.")
             .bow (0.40f, 0.44f, 0.48f)
             .material (wood, 0.58f, 0.46f, 0.30f, 0.46f)
             .macros (0.60f, 0.52f, 0.50f, 0.02f)
             .chord (4.0f, 5.0f, 6.0f, 7.0f)
             .levels (0.60f, 0.56f, 0.52f, 0.48f)
             .fx (1.4f, 0.54f));
}

} // namespace arc::presets::detail

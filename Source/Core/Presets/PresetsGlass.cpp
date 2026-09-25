// Factory library: GLASS. The glass material under every exciter: bars, panes, tines,
// tumblers, fibres and ice. Glass partials start high (2.63 / 4.45 / 6.55 / 9.40 x CORE),
// so the tuned presets place nodes on harmonics inside that reach. Node angles are 0..1
// (0.5 = straight up, 0.25 = hard left, 0.75 = hard right). See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addGlass (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "GLASS";

    add (Builder ("Wine Glass Rim", cat, "wine glass, rubbed, pure, mono",
                  "A wet finger circling a wine glass: a pure, ringing tone that sways as the wine moves.")
             .bow (0.24f, 0.36f, 0.34f)
             .material (glass, 0.40f, 0.62f, 0.24f, 0.46f)
             .macros (0.56f, 0.20f, 0.56f, 0.04f)
             .voice (VoiceMode::legato, 0.08f)
             .gesture (0, breatheGesture (2.4f, 0.0f, 0.03f, 1))
             .gesture (1, swayGesture (3.1f, 0.0f, 0.3f, 0.02f))
             .fx (1.2f, 0.38f));

    add (Builder ("Glass Marimba", cat, "marimba, mallet, tuned, glass bars",
                  "Marimba bars cut from glass, tuned 1 : 4 : 10: the warmth of a marimba with a glassy ring.")
             .strike (0.40f, 0.40f, 0.46f)
             .material (glass, 0.54f, 0.48f, 0.40f, 0.44f)
             .macros (0.62f, 0.26f, 0.50f, 0.02f)
             .tune (0, 4.0f)
             .tune (3, 10.0f)
             .levels (0.60f, 0.20f, 0.20f, 0.40f)
             .fx (1.2f, 0.22f));

    add (Builder ("Shard Rain", cat, "shards, splintering, bright, chaos, inharmonic",
                  "A pane shattering in slow motion: hard, bright shards scattering through a tightly coupled web.")
             .strike (0.80f, 0.10f, 0.80f)
             .material (glass, 0.26f, 0.80f, 0.36f, 0.70f)
             .macros (0.60f, 0.80f, 0.70f, 0.40f)
             .topology (web)
             .motion (0.46f, 1.6f)
             .fx (1.5f, 0.40f));

    add (Builder ("Crystal Cavern", cat, "cavern, dark, drips, vast",
                  "Soft glass struck deep inside a crystal cave: dark tones, and a reverb that never seems to end.")
             .strike (0.30f, 0.56f, 0.30f)
             .material (glass, 0.70f, 0.28f, 0.22f, 0.48f)
             .macros (0.60f, 0.46f, 0.42f, 0.08f)
             .topology (ring)
             .chord (2.0f, 3.0f, 5.0f, 6.0f)
             .fx (1.5f, 0.92f));

    add (Builder ("Glass Harmonics", cat, "harmonics, flageolet, high, bell-like",
                  "Only the harmonics: a glass string touched at its nodes, so octaves and twelfths ring above the fundamental.")
             .pluck (0.08f, 0.10f, 0.66f)
             .material (glass, 0.36f, 0.62f, 0.30f, 0.44f)
             .macros (0.60f, 0.40f, 0.62f, 0.02f)
             .chord (4.0f, 6.0f, 8.0f, 12.0f)
             .levels (0.70f, 0.60f, 0.50f, 0.40f)
             .fx (1.3f, 0.34f));

    add (Builder ("Fiber Optic", cat, "fiber, pulses, stereo, synced, bright",
                  "Light racing down glass fibre: bright plucks whose partials spin around the stereo field in time.")
             .pluck (0.20f, 0.20f, 0.74f)
             .material (glass, 0.32f, 0.70f, 0.32f, 0.48f)
             .macros (0.62f, 0.44f, 0.66f, 0.06f)
             .topology (chain)
             .synced (quarter, 0.0f)
             .gesture (0, orbitGesture (0.5f, 1.0f, 1.0f, 0.004f))
             .gesture (1, orbitGesture (0.5f, 1.0f, -1.0f, 0.004f))
             .gesture (2, orbitGesture (1.0f, 2.0f, 1.0f, 0.004f))
             .fx (1.5f, 0.30f));

    add (Builder ("Refraction", cat, "plucked, stereo, bending, prism",
                  "Plucked glass seen through a prism: each partial is thrown to its own place in the stereo field and drifts there.")
             .pluck (0.22f, 0.08f, 0.72f)
             .material (glass, 0.40f, 0.70f, 0.16f, 0.50f)
             .macros (0.62f, 0.46f, 0.70f, 0.06f)
             .topology (star)
             .angles (0.30f, 0.70f, 0.20f, 0.80f)
             .gesture (0, figureGesture (7.0f, 0.0f, 0.8f, 0.01f, 1, 2))
             .gesture (1, figureGesture (9.0f, 0.0f, 0.8f, 0.01f, 2, 1))
             .fx (1.5f, 0.44f));

    add (Builder ("Ice Keys", cat, "keys, electric piano, glassy, bright",
                  "An electric piano with glass tines: a hard, glassy bite that settles into a clean bell tone.")
             .strike (0.66f, 0.18f, 0.66f)
             .material (glass, 0.40f, 0.62f, 0.44f, 0.46f)
             .macros (0.66f, 0.30f, 0.66f, 0.02f)
             .chord (2.0f, 3.0f, 8.0f, 12.0f)
             .release (0.55f)
             .fx (1.2f, 0.24f));

    add (Builder ("Quartz Oscillator", cat, "quartz, pure, long, sine",
                  "A quartz crystal set ringing: almost no loss, a pure tone with octaves stacked above it, fading very slowly.")
             .strike (0.50f, 0.30f, 0.50f)
             .material (glass, 0.36f, 0.50f, 0.08f, 0.40f)
             .macros (0.60f, 0.20f, 0.50f, 0.0f)
             .chord (2.0f, 4.0f, 8.0f, 16.0f)
             .levels (0.40f, 0.30f, 0.20f, 0.10f)
             .fx (1.1f, 0.30f));

    add (Builder ("Condensation", cat, "breath, window, noise, cold, airy",
                  "Breath fogging a cold window: filtered air that settles into faint glassy tones.")
             .air (0.30f, 0.86f, 0.70f)
             .material (glass, 0.36f, 0.62f, 0.20f, 0.52f)
             .macros (0.64f, 0.56f, 0.60f, 0.20f)
             .topology (web)
             .motion (0.30f, 0.12f)
             .fx (1.5f, 0.54f));

    add (Builder ("Window Pane", cat, "pane, rattle, thin, inharmonic",
                  "A knock on a thin window pane: a bright slap and a rattle in the frame.")
             .strike (0.70f, 0.14f, 0.60f)
             .material (glass, 0.22f, 0.58f, 0.54f, 0.66f)
             .macros (0.66f, 0.66f, 0.56f, 0.34f)
             .topology (web)
             .fx (1.1f, 0.18f, 0.28f));

    add (Builder ("Lens Flare", cat, "flare, bright, halo, wide",
                  "A bright pluck that flares into a wide halo of major-seventh light.")
             .pluck (0.16f, 0.06f, 0.84f)
             .material (glass, 0.34f, 0.76f, 0.30f, 0.46f)
             .macros (0.64f, 0.52f, 0.58f, 0.06f)
             .chord (2.5f, 3.0f, 7.5f, 12.0f)
             .fx (1.5f, 0.76f));

    add (Builder ("Frozen Lake", cat, "ice, low, boom, singing, inharmonic",
                  "Ice booming across a frozen lake: a low thud that sings on in long, bending, inharmonic tones.")
             .strike (0.56f, 0.34f, 0.36f)
             .material (glass, 0.86f, 0.36f, 0.18f, 0.78f)
             .macros (0.64f, 0.56f, 0.30f, 0.18f)
             .topology (web)
             .fx (1.4f, 0.70f));

    add (Builder ("Glass Organ", cat, "organ, drawbars, glass, steady",
                  "Organ drawbars made of glass tubes: steady, clear, and bright at the top.")
             .air (0.74f, 0.12f, 0.58f)
             .material (glass, 0.48f, 0.54f, 0.34f, 0.42f)
             .macros (0.60f, 0.40f, 0.50f, 0.0f)
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .levels (0.56f, 0.46f, 0.40f, 0.30f)
             .release (0.50f)
             .fx (1.2f, 0.40f));

    add (Builder ("Chandelier", cat, "chandelier, tinkling, prisms, inharmonic",
                  "A crystal chandelier brushed by a draught: a cloud of tiny prisms tinkling against each other.")
             .strike (0.74f, 0.12f, 0.76f)
             .material (glass, 0.28f, 0.74f, 0.26f, 0.60f)
             .macros (0.64f, 0.74f, 0.72f, 0.14f)
             .topology (web)
             .chord (4.5f, 6.8f, 9.6f, 13.0f)
             .motion (0.30f, 0.30f)
             .fx (1.5f, 0.50f));

    add (Builder ("Hourglass", cat, "falling, sequenced, slow, sand",
                  "Grains running through glass: a sympathetic node sinks a semitone every bar, so the ring slowly falls.")
             .pluck (0.24f, 0.18f, 0.60f)
             .material (glass, 0.42f, 0.56f, 0.30f, 0.46f)
             .macros (0.62f, 0.40f, 0.54f, 0.10f)
             .quantise()
             .chord (4.0f, 6.0f, 8.0f, 12.0f)
             .synced (bar8, 0.0f)
             .gesture (0, stepGesture (16.0f, 32.0f,
                                       { 0.0f, -1 * kSemitone, -2 * kSemitone, -3 * kSemitone, -4 * kSemitone, -5 * kSemitone,
                                         -6 * kSemitone, -7 * kSemitone },
                                       0.5f))
             .fx (1.3f, 0.40f));

    add (Builder ("Spun Sugar", cat, "delicate, sweet, short, sugar",
                  "Threads of spun sugar: tiny, sweet, brittle plucks on a sixth chord.")
             .pluck (0.30f, 0.34f, 0.80f)
             .material (glass, 0.20f, 0.72f, 0.46f, 0.44f)
             .macros (0.60f, 0.34f, 0.60f, 0.04f)
             .chord (3.333f, 4.0f, 5.0f, 10.0f)
             .fx (1.3f, 0.30f));

    add (Builder ("Glass Bass", cat, "bass, mono, thick, glass",
                  "A bass line cut from thick glass: a round, heavy pluck with a clear, bright edge.")
             .pluck (0.36f, 0.30f, 0.44f)
             .material (glass, 0.84f, 0.40f, 0.40f, 0.42f)
             .macros (0.66f, 0.30f, 0.44f, 0.04f)
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .voice (VoiceMode::mono, 0.04f)
             .fx (1.0f, 0.12f, 0.20f));

    add (Builder ("Vitreous", cat, "bowed, texture, chaos, evolving",
                  "Bowed glass left to its own devices: a slowly shifting, slightly unstable vitreous texture.")
             .bow (0.44f, 0.40f, 0.50f)
             .material (glass, 0.56f, 0.46f, 0.30f, 0.56f)
             .macros (0.60f, 0.62f, 0.48f, 0.30f)
             .topology (web)
             .motion (0.34f, 0.07f)
             .fx (1.45f, 0.62f));

    add (Builder ("Clear Water", cat, "water, glasses, pentatonic, wobble",
                  "Tumblers filled to a pentatonic scale and tapped with a spoon; the water keeps the pitches wobbling.")
             .strike (0.54f, 0.24f, 0.56f)
             .material (glass, 0.48f, 0.54f, 0.34f, 0.44f)
             .macros (0.62f, 0.34f, 0.50f, 0.04f)
             .chord (2.25f, 3.0f, 6.667f, 9.0f)
             .gesture (0, breatheGesture (1.7f, 0.0f, 0.02f, 1))
             .gesture (1, breatheGesture (2.3f, 0.0f, 0.02f, 1))
             .fx (1.35f, 0.34f));

    add (Builder ("Glass Tines", cat, "tines, kalimba, glass, soft",
                  "Kalimba tines made of glass: a soft thumb pluck with the high, bell-like tine partial above it.")
             .pluck (0.40f, 0.26f, 0.58f)
             .material (glass, 0.46f, 0.50f, 0.40f, 0.46f)
             .macros (0.62f, 0.28f, 0.52f, 0.02f)
             .tune (1, 5.9f)
             .levels (0.20f, 0.50f, 0.20f, 0.20f)
             .fx (1.2f, 0.26f));

    add (Builder ("Crystal Web", cat, "web, coupled, maj9, shimmering",
                  "Every node linked to every other: a ninth chord of glass that passes energy around like a spider's web.")
             .strike (0.56f, 0.26f, 0.60f)
             .material (glass, 0.40f, 0.58f, 0.30f, 0.48f)
             .macros (0.62f, 0.86f, 0.54f, 0.06f)
             .topology (web)
             .links (0.9f, 0.9f, 0.9f, 0.9f)
             .chord (2.25f, 3.0f, 3.75f, 9.0f)
             .fx (1.4f, 0.44f));

    add (Builder ("Snow Globe", cat, "twinkling, drifting, wide, winter",
                  "Shake it and watch: tiny glass flakes drift and twinkle around the stereo field.")
             .strike (0.44f, 0.20f, 0.70f)
             .material (glass, 0.30f, 0.66f, 0.28f, 0.50f)
             .macros (0.60f, 0.52f, 0.60f, 0.20f)
             .topology (ring)
             .gesture (0, wanderGesture (5.0f, 0.0f, 1.2f, 0.02f, seedFor ("Snow Globe A")))
             .gesture (1, wanderGesture (6.5f, 0.0f, 1.2f, 0.02f, seedFor ("Snow Globe B")))
             .gesture (2, wanderGesture (8.0f, 0.0f, 1.2f, 0.02f, seedFor ("Snow Globe C")))
             .gesture (3, wanderGesture (9.5f, 0.0f, 1.2f, 0.02f, seedFor ("Snow Globe D")))
             .fx (1.5f, 0.66f));

    add (Builder ("Glass Horn", cat, "horn, overblown, glass, mono",
                  "A horn blown from glass: bright, brassy and a little overblown, played one note at a time.")
             .air (0.90f, 0.14f, 0.70f)
             .material (glass, 0.52f, 0.60f, 0.40f, 0.44f)
             .macros (0.64f, 0.44f, 0.56f, 0.06f)
             .chord (2.0f, 3.0f, 4.0f, 10.0f)
             .voice (VoiceMode::mono, 0.08f)
             .fx (1.1f, 0.34f, 0.34f));

    add (Builder ("Mirror Maze", cat, "reflections, octaves, wide, spatial",
                  "The same note reflected in octaves from mirrors on every side, the far reflections circling you.")
             .pluck (0.18f, 0.14f, 0.66f)
             .material (glass, 0.36f, 0.62f, 0.26f, 0.46f)
             .macros (0.62f, 0.50f, 0.56f, 0.04f)
             .chord (2.0f, 4.0f, 8.0f, 16.0f)
             .angles (0.25f, 0.75f, 0.33f, 0.67f)
             .gesture (2, orbitGesture (6.0f, 0.0f, 1.0f, 0.0f))
             .gesture (3, orbitGesture (6.0f, 0.0f, -1.0f, 0.0f))
             .fx (1.5f, 0.80f));

    add (Builder ("Glass Lullaby", cat, "lullaby, soft, gentle, rocking",
                  "A soft glass lullaby: felt mallets on a major chord that rocks gently in half notes.")
             .strike (0.24f, 0.60f, 0.40f)
             .material (glass, 0.46f, 0.46f, 0.30f, 0.42f)
             .macros (0.58f, 0.34f, 0.50f, 0.02f)
             .chord (2.5f, 3.0f, 4.0f, 5.0f)
             .synced (bar1, 0.0f)
             .gesture (0, breatheGesture (1.0f, 2.0f, 0.02f, 1))
             .fx (1.3f, 0.46f));

    add (Builder ("Diamond Dust", cat, "glitter, tiny, bright, inharmonic, high",
                  "Ice crystals in sunlight: the hardest, smallest, brightest glass there is, glittering at the top of hearing.")
             .strike (0.92f, 0.06f, 0.92f)
             .material (glass, 0.14f, 0.90f, 0.40f, 0.72f)
             .macros (0.60f, 0.64f, 0.84f, 0.30f)
             .topology (web)
             .motion (0.40f, 2.4f)
             .fx (1.5f, 0.56f));

    add (Builder ("Borosilicate", cat, "lab glass, beakers, clinks, inharmonic",
                  "Laboratory glassware clinking on a bench: beakers and flasks at odd, unrelated pitches.")
             .strike (0.64f, 0.16f, 0.58f)
             .material (glass, 0.38f, 0.56f, 0.42f, 0.64f)
             .macros (0.64f, 0.40f, 0.60f, 0.10f)
             .topology (star)
             .chord (2.37f, 3.71f, 5.93f, 8.62f)
             .fx (1.2f, 0.20f));
}

} // namespace arc::presets::detail

// Factory library: AIR. Breath driving the CORE: flutes, reeds, pipes, vessels, voices and
// wind. FLOW crosses the speaking threshold near (0.6 + 3 * flow) * (1 - turbulence / 2) = 1,
// so the breath-only presets (Wind Harp, Air Chimes) sit deliberately below it and let the
// network filter the turbulence. No node sits within 5 % of unison. A membrane under a
// steady breath keeps its amplitude-driven tension (the skin's pitch glide never settles),
// so light skins read sharp (+12 cents at mass 0.46): membrane presets here keep mass >= 0.6.
// See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addAir (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "AIR";

    add (Builder ("Shakuhachi Dusk", cat, "shakuhachi, bamboo, breathy, mono",
                  "An end-blown bamboo flute at dusk: more breath than tone, with a husky, falling edge.")
             .air (0.52f, 0.66f, 0.40f)
             .material (wood, 0.62f, 0.38f, 0.44f, 0.46f)
             .macros (0.62f, 0.26f, 0.46f, 0.08f)
             .chord (2.0f, 3.0f, 5.0f, 8.0f)
             .levels (0.40f, 0.30f, 0.20f, 0.14f)
             .voice (VoiceMode::mono, 0.10f)
             .motion (0.10f, 0.40f)
             .fx (1.1f, 0.44f));

    add (Builder ("Pan Pipes", cat, "pan pipes, pentatonic, breathy, wood",
                  "A raft of stopped pipes: blow one and its pentatonic neighbours sound faintly along.")
             .air (0.58f, 0.52f, 0.52f)
             .material (wood, 0.52f, 0.48f, 0.40f, 0.44f)
             .macros (0.60f, 0.34f, 0.52f, 0.04f)
             .chord (2.25f, 5.0f, 6.0f, 13.333f)
             .levels (0.44f, 0.30f, 0.26f, 0.16f)
             .fx (1.3f, 0.46f));

    add (Builder ("Glass Flute", cat, "flute, glass, pure, bright",
                  "A flute cut from glass: a clean, bright tone with a halo of octave harmonics.")
             .air (0.66f, 0.24f, 0.66f)
             .material (glass, 0.42f, 0.60f, 0.30f, 0.44f)
             .macros (0.60f, 0.30f, 0.56f, 0.02f)
             .chord (2.0f, 4.0f, 6.0f, 8.0f)
             .levels (0.44f, 0.32f, 0.22f, 0.16f)
             .motion (0.08f, 0.22f)
             .fx (1.25f, 0.38f));

    add (Builder ("Principal Chorus", cat, "organ, pipes, flue, church",
                  "Organ flue pipes with the 4', 2 2/3', 1 3/5' and 1' ranks drawn, speaking into a stone church.")
             .air (0.70f, 0.14f, 0.46f)
             .material (wood, 0.58f, 0.46f, 0.36f, 0.42f)
             .macros (0.60f, 0.24f, 0.50f, 0.0f)
             .chord (2.0f, 3.0f, 5.0f, 8.0f)
             .levels (0.60f, 0.46f, 0.26f, 0.34f)
             .release (0.50f)
             .fx (1.2f, 0.62f));

    add (Builder ("Melodica Street", cat, "melodica, reed, nasal, street",
                  "A melodica busked on a street corner: nasal free reeds, bright and a little rough.")
             .air (0.76f, 0.22f, 0.64f)
             .material (metal, 0.46f, 0.60f, 0.40f, 0.42f)
             .macros (0.62f, 0.30f, 0.70f, 0.06f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.40f, 0.34f, 0.26f, 0.20f)
             .fx (0.9f, 0.18f, 0.16f));

    add (Builder ("Harmonium Celeste", cat, "harmonium, celeste, reeds, beating",
                  "A harmonium with the celeste stop drawn: two ranks of reeds a few cents apart, beating slowly.")
             .air (0.68f, 0.18f, 0.50f)
             .material (metal, 0.56f, 0.46f, 0.40f, 0.42f)
             .macros (0.60f, 0.34f, 0.70f, 0.02f)
             .chord (2.0f * 1.005f, 2.0f * 0.995f, 3.0f * 1.003f, 4.0f * 0.997f)
             .fx (1.2f, 0.36f));

    add (Builder ("Accordion Musette", cat, "accordion, musette, detuned, reeds",
                  "Three reeds per note, the outer two tuned wide: the wet, wavering musette of a Paris accordion.")
             .air (0.74f, 0.20f, 0.66f)
             .material (metal, 0.50f, 0.58f, 0.42f, 0.44f)
             .macros (0.62f, 0.40f, 0.70f, 0.04f)
             .chord (2.0f * 1.014f, 2.0f * 0.986f, 4.0f * 1.010f, 3.0f)
             .fx (1.0f, 0.22f, 0.12f));

    add (Builder ("Clay Ocarina", cat, "ocarina, vessel, round, pure",
                  "A clay vessel flute: a round, hollow, nearly pure tone with a soft breath around it.")
             .air (0.62f, 0.30f, 0.30f)
             .material (membrane, 0.66f, 0.26f, 0.36f, 0.40f)
             .macros (0.58f, 0.20f, 0.48f, 0.02f)
             .levels (0.20f, 0.16f, 0.12f, 0.10f)
             .fx (1.05f, 0.40f));

    add (Builder ("Whistle Wind", cat, "whistle, wind, high, airy",
                  "Wind whistling through a gap in the glass: high, airy and never quite still.")
             .air (0.60f, 0.54f, 0.82f)
             .material (glass, 0.30f, 0.70f, 0.30f, 0.50f)
             .macros (0.60f, 0.40f, 0.72f, 0.10f)
             .gesture (0, wanderGesture (6.0f, 0.0f, 0.4f, 0.02f, seedFor ("Whistle Wind A")))
             .gesture (3, wanderGesture (9.0f, 0.0f, 0.5f, 0.02f, seedFor ("Whistle Wind D")))
             .fx (1.4f, 0.54f));

    add (Builder ("Didgeridoo Drone", cat, "didgeridoo, mono, formant, growl",
                  "A long wooden tube droned with circular breathing: the mouth sweeps its formants as it plays.")
             .air (0.80f, 0.30f, 0.36f)
             .material (wood, 0.84f, 0.40f, 0.38f, 0.50f)
             .macros (0.64f, 0.46f, 0.40f, 0.10f)
             .chord (3.5f, 6.2f, 9.1f, 12.5f)
             .levels (0.50f, 0.54f, 0.40f, 0.26f)
             .voice (VoiceMode::mono, 0.0f)
             .gesture (1, breatheGesture (1.4f, 0.0f, 0.12f, 1))
             .gesture (2, breatheGesture (2.1f, 0.0f, 0.10f, 1))
             .fx (1.1f, 0.30f, 0.24f));

    add (Builder ("Sheng Cluster", cat, "sheng, mouth organ, cluster, reeds",
                  "A Chinese mouth organ: bright free reeds sounding fourths and fifths together, a shimmering cluster.")
             .air (0.72f, 0.18f, 0.60f)
             .material (metal, 0.44f, 0.58f, 0.36f, 0.44f)
             .macros (0.60f, 0.52f, 0.60f, 0.04f)
             .topology (web)
             .chord (1.5f, 2.0f, 2.25f, 3.0f)
             .fx (1.3f, 0.34f));

    add (Builder ("Breath of Glass", cat, "breath, glass, airy, fragile",
                  "Barely enough breath to make the glass speak: the tone flickers in and out of the air.")
             .air (0.38f, 0.80f, 0.56f)
             .material (glass, 0.46f, 0.50f, 0.26f, 0.46f)
             .macros (0.60f, 0.46f, 0.52f, 0.12f)
             .topology (ring)
             .motion (0.24f, 0.10f)
             .fx (1.45f, 0.60f));

    add (Builder ("Blown Bottles", cat, "bottles, blown, pentatonic, hollow",
                  "A row of bottles filled to a pentatonic scale: blow across one and its neighbours hum along.")
             .air (0.60f, 0.46f, 0.36f)
             .material (glass, 0.62f, 0.34f, 0.34f, 0.44f)
             .macros (0.60f, 0.40f, 0.48f, 0.06f)
             .topology (star)
             .chord (2.25f, 2.5f, 6.0f, 6.667f)
             .fx (1.35f, 0.40f));

    add (Builder ("Kettle Whistle", cat, "whistle, shrill, steam, glide",
                  "A kettle coming to the boil: a shrill, steamy whistle that slides from note to note.")
             .air (0.86f, 0.34f, 0.80f)
             .material (metal, 0.30f, 0.72f, 0.28f, 0.50f)
             .macros (0.62f, 0.30f, 0.78f, 0.16f)
             .voice (VoiceMode::legato, 0.40f)
             .fx (1.0f, 0.24f, 0.12f));

    add (Builder ("Flute Choir", cat, "flutes, choir, ensemble, wide",
                  "A flute choir spread across the room: soft, breathy, gently out of step with itself.")
             .air (0.60f, 0.34f, 0.54f)
             .material (wood, 0.50f, 0.52f, 0.36f, 0.44f)
             .macros (0.58f, 0.44f, 0.52f, 0.08f)
             .topology (web)
             .chord (2.0f, 3.0f, 8.0f, 12.0f)
             .motion (0.20f, 0.14f)
             .fx (1.5f, 0.52f));

    add (Builder ("Bansuri Glide", cat, "bansuri, bamboo, legato, glide",
                  "A bamboo bansuri: sweet and breathy, sliding slowly between notes like a meend.")
             .air (0.56f, 0.42f, 0.50f)
             .material (wood, 0.56f, 0.44f, 0.34f, 0.44f)
             .macros (0.60f, 0.28f, 0.48f, 0.04f)
             .chord (2.0f, 3.0f, 6.0f, 8.0f)
             .levels (0.36f, 0.26f, 0.16f, 0.12f)
             .voice (VoiceMode::legato, 0.30f)
             .motion (0.10f, 0.30f)
             .fx (1.2f, 0.52f));

    add (Builder ("Brass Breath", cat, "brass, horn, bright, driven",
                  "Lips buzzing into a brass bell: bright, harmonic and pushed into a little grit.")
             .air (0.88f, 0.10f, 0.74f)
             .material (metal, 0.56f, 0.62f, 0.40f, 0.42f)
             .macros (0.64f, 0.46f, 0.70f, 0.04f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.56f, 0.48f, 0.40f, 0.32f)
             .fx (1.0f, 0.26f, 0.36f));

    add (Builder ("Conch Call", cat, "conch, shell, horn, call, mono",
                  "A conch shell blown across a bay: one hollow, horn-like call and a long echo.")
             .air (0.80f, 0.28f, 0.40f)
             .material (membrane, 0.70f, 0.40f, 0.36f, 0.50f)
             .macros (0.62f, 0.40f, 0.50f, 0.06f)
             .chord (1.5f, 2.0f, 3.0f, 4.0f)
             .voice (VoiceMode::mono, 0.12f)
             .fx (1.2f, 0.72f, 0.14f));

    add (Builder ("Hollow Reed", cat, "clarinet, reed, odd harmonics, hollow",
                  "A cylindrical reed pipe: odd harmonics only, a hollow, woody clarinet voice.")
             .air (0.70f, 0.12f, 0.50f)
             .material (wood, 0.54f, 0.50f, 0.36f, 0.42f)
             .macros (0.60f, 0.40f, 0.50f, 0.02f)
             .chord (3.0f, 5.0f, 7.0f, 9.0f)
             .levels (0.56f, 0.42f, 0.30f, 0.20f)
             .fx (1.1f, 0.26f));

    add (Builder ("Wind Harp", cat, "aeolian, wind, strings, noise, harmonics",
                  "An Aeolian harp in the wind: no player, just moving air coaxing harmonics from the strings.")
             .air (0.26f, 0.90f, 0.62f)
             .material (wood, 0.44f, 0.56f, 0.14f, 0.40f)
             .macros (0.66f, 0.62f, 0.56f, 0.18f)
             .topology (web)
             .chord (3.0f, 4.0f, 6.0f, 8.0f)
             .levels (0.70f, 0.66f, 0.60f, 0.50f)
             .motion (0.30f, 0.06f)
             .fx (1.5f, 0.56f));

    add (Builder ("Vocal Tract", cat, "vocal, formant, vowel, talking",
                  "Four nodes set as the formants of a voice: the vowel slowly turns from 'ah' toward 'oh' and back.")
             .air (0.66f, 0.30f, 0.50f)
             .material (wood, 0.50f, 0.50f, 0.30f, 0.44f)
             .macros (0.60f, 0.50f, 0.50f, 0.04f)
             .topology (star)
             .chord (3.2f, 5.5f, 11.8f, 16.0f)
             .levels (0.70f, 0.60f, 0.40f, 0.20f)
             .gesture (0, figureGesture (3.0f, 0.0f, 0.0f, 0.10f, 1, 1))
             .gesture (1, figureGesture (3.0f, 0.0f, 0.0f, 0.14f, 1, 2))
             .fx (1.2f, 0.34f));

    add (Builder ("Recorder Consort", cat, "recorder, consort, dry, chamber",
                  "A consort of wooden recorders in a small dry room: plain, sweet, and in close harmony.")
             .air (0.60f, 0.20f, 0.62f)
             .material (wood, 0.46f, 0.56f, 0.32f, 0.42f)
             .macros (0.58f, 0.56f, 0.52f, 0.02f)
             .topology (web)
             .chord (2.5f, 3.0f, 5.0f, 12.0f)
             .fx (0.8f, 0.18f));

    add (Builder ("Steam Organ", cat, "calliope, steam, loud, wobbly, fairground",
                  "A steam calliope at the fairground: overblown whistles, loud, bright and wobbling.")
             .air (0.94f, 0.40f, 0.72f)
             .material (metal, 0.40f, 0.66f, 0.36f, 0.52f)
             .macros (0.66f, 0.36f, 0.66f, 0.18f)
             .motion (0.16f, 3.2f)
             .fx (1.1f, 0.30f, 0.42f));

    add (Builder ("Pipe Dream", cat, "dreamy, fifths, glass, orbit, ambient",
                  "Stacked fifths blown through glass while two nodes circle the core: a slow, weightless daydream.")
             .air (0.50f, 0.40f, 0.50f)
             .material (glass, 0.50f, 0.50f, 0.30f, 0.46f)
             .macros (0.58f, 0.52f, 0.50f, 0.08f)
             .chord (2.25f, 4.5f, 6.75f, 10.125f)
             .gesture (0, orbitGesture (11.0f, 0.0f, 1.0f, 0.004f))
             .gesture (3, orbitGesture (17.0f, 0.0f, -1.0f, 0.004f))
             .fx (1.5f, 0.80f));

    add (Builder ("Mirliton", cat, "kazoo, buzz, membrane, nasal",
                  "A membrane buzzing in the breath: the nasal, comic hum of a kazoo.")
             .air (0.72f, 0.46f, 0.46f)
             .material (membrane, 0.62f, 0.56f, 0.40f, 0.54f)
             .macros (0.62f, 0.50f, 0.54f, 0.10f)
             .topology (chain)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .fx (1.0f, 0.18f, 0.30f));

    add (Builder ("Low Whistle", cat, "low whistle, irish, soft, legato",
                  "An Irish low whistle: a round, hollow metal tube, soft and plaintive.")
             .air (0.62f, 0.34f, 0.40f)
             .material (metal, 0.70f, 0.34f, 0.34f, 0.42f)
             .macros (0.60f, 0.22f, 0.70f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.26f, 0.18f, 0.12f, 0.08f)
             .voice (VoiceMode::legato, 0.05f)
             .fx (1.1f, 0.42f));

    add (Builder ("Air Chimes", cat, "chimes, breeze, inharmonic, noise, metal",
                  "A breeze moving through hanging metal tubes: no strikes, just air setting inharmonic partials humming.")
             .air (0.28f, 0.84f, 0.70f)
             .material (metal, 0.36f, 0.64f, 0.18f, 0.72f)
             .macros (0.66f, 0.60f, 0.62f, 0.36f)
             .topology (web)
             .gesture (1, wanderGesture (13.0f, 0.0f, 0.6f, 0.03f, seedFor ("Air Chimes B")))
             .gesture (2, wanderGesture (19.0f, 0.0f, 0.6f, 0.03f, seedFor ("Air Chimes C")))
             .fx (1.5f, 0.60f));

    add (Builder ("Crystal Whistle", cat, "whistle, crystal, pure, high",
                  "A pure crystal whistle under high tension: clean, glassy and piercing.")
             .air (0.74f, 0.18f, 0.82f)
             .material (glass, 0.28f, 0.78f, 0.24f, 0.48f)
             .macros (0.60f, 0.26f, 0.84f, 0.02f)
             .voice (VoiceMode::legato, 0.12f)
             .fx (1.2f, 0.46f));

    add (Builder ("Breath Engine", cat, "sequenced, breathing, synced, rhythmic",
                  "A machine that breathes in time: two nodes step through interlocking sequences, so the breath plays a pattern.")
             .air (0.56f, 0.58f, 0.56f)
             .material (wood, 0.50f, 0.52f, 0.36f, 0.46f)
             .macros (0.60f, 0.54f, 0.52f, 0.08f)
             .topology (ring)
             .quantise()
             .chord (2.0f, 3.0f, 8.0f, 12.0f)
             .synced (bar1, 0.0f)
             .gesture (0, stepGesture (2.0f, 4.0f, { 0.0f, 7 * kSemitone, 3 * kSemitone, 10 * kSemitone }, 0.3f))
             .gesture (1, stepGesture (2.0f, 4.0f, { 5 * kSemitone, 0.0f, 12 * kSemitone, 7 * kSemitone, 0.0f, 3 * kSemitone }, 0.2f))
             .fx (1.3f, 0.34f));

    add (Builder ("Ghost Choir", cat, "choir, ghostly, breathy, dark, minor",
                  "Wordless, breathy voices on a minor seventh chord, drifting through a dark hall.")
             .air (0.48f, 0.66f, 0.46f)
             .material (membrane, 0.62f, 0.44f, 0.30f, 0.46f)
             .macros (0.60f, 0.50f, 0.48f, 0.16f)
             .topology (web)
             .chord (1.5f, 2.4f, 3.0f, 3.6f)
             .gesture (0, wanderGesture (14.0f, 0.0f, 0.5f, 0.012f, seedFor ("Ghost Choir A")))
             .gesture (2, wanderGesture (21.0f, 0.0f, 0.5f, 0.012f, seedFor ("Ghost Choir C")))
             .fx (1.5f, 0.86f));
}

} // namespace arc::presets::detail

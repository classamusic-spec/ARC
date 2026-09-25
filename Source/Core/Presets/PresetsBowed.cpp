// Factory library: BOWED. Stick-slip friction on the CORE: strings, saws, glasses, bowed
// metal and skins. No node sits within 5 % of unison (a bowed CORE would lock onto one side
// of the doublet). See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addBowed (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "BOWED";

    add (Builder ("Cello Nocturne", cat, "cello, wood, warm, expressive",
                  "A warm cello body under a slow bow: rich low partials and a woody resonance.")
             .bow (0.54f, 0.50f, 0.56f)
             .material (wood, 0.66f, 0.42f, 0.42f, 0.44f)
             .macros (0.60f, 0.32f, 0.48f, 0.04f)
             .chord (2.0f, 3.0f, 5.0f, 8.0f)
             .levels (0.56f, 0.44f, 0.34f, 0.24f)
             .fx (1.1f, 0.22f));

    add (Builder ("Sul Tasto Viola", cat, "viola, soft, over the fingerboard, dark",
                  "Bowed over the fingerboard: light pressure, a veiled and breathy viola.")
             .bow (0.26f, 0.40f, 0.40f)
             .material (wood, 0.56f, 0.32f, 0.40f, 0.42f)
             .macros (0.58f, 0.30f, 0.50f, 0.04f)
             .fx (1.15f, 0.26f));

    add (Builder ("Arco Bass", cat, "double bass, mono, low, wood",
                  "A double bass played arco: heavy, slack wood, one note at a time.")
             .bow (0.60f, 0.46f, 0.60f)
             .material (wood, 0.78f, 0.36f, 0.40f, 0.42f)
             .macros (0.62f, 0.30f, 0.40f, 0.04f)
             .voice (VoiceMode::mono, 0.05f)
             .fx (1.0f, 0.18f));

    add (Builder ("Bowed Cymbal", cat, "cymbal, bowed, inharmonic, bright",
                  "A bow drawn across the edge of a cymbal: a bright, singing, inharmonic scream.")
             .bow (0.62f, 0.56f, 0.64f)
             .material (metal, 0.42f, 0.66f, 0.28f, 0.76f)
             .macros (0.60f, 0.62f, 0.64f, 0.14f)
             .topology (web)
             .fx (1.3f, 0.34f));

    add (Builder ("Musical Saw", cat, "saw, singing, legato, glide",
                  "A bowed saw: a pure, singing, slightly eerie tone that glides between notes.")
             .bow (0.40f, 0.50f, 0.38f)
             .material (metal, 0.40f, 0.56f, 0.22f, 0.40f)
             .macros (0.58f, 0.22f, 0.66f, 0.02f)
             .topology (star)
             .voice (VoiceMode::legato, 0.22f)
             .motion (0.10f, 0.20f)
             .fx (1.2f, 0.36f));

    add (Builder ("Armonica", cat, "glass armonica, chord, wet, soft",
                  "Franklin's spinning bowls: a whole major chord of glasses answering each fingertip in a wet room.")
             .bow (0.28f, 0.42f, 0.30f)
             .material (glass, 0.50f, 0.44f, 0.30f, 0.42f)
             .macros (0.56f, 0.44f, 0.50f, 0.02f)
             .topology (ring)
             .chord (3.0f, 4.0f, 5.0f, 6.0f)
             .synced (bar2, 0.0f)
             .gesture (2, breatheGesture (4.0f, 8.0f, 0.02f, 1))
             .fx (1.4f, 0.62f));

    add (Builder ("Erhu Glide", cat, "erhu, membrane, nasal, legato",
                  "A two-string fiddle over a skin: nasal, expressive and gliding between notes.")
             .bow (0.46f, 0.54f, 0.56f)
             .material (membrane, 0.40f, 0.58f, 0.36f, 0.46f)
             .macros (0.60f, 0.30f, 0.58f, 0.04f)
             .voice (VoiceMode::legato, 0.12f)
             .fx (1.1f, 0.24f));

    add (Builder ("String Ensemble", cat, "ensemble, strings, wide, lush, section",
                  "A section of bowed wooden bodies, darker and further away, each slightly apart in tune and space.")
             .bow (0.44f, 0.46f, 0.50f)
             .material (wood, 0.66f, 0.30f, 0.38f, 0.46f)
             .macros (0.58f, 0.40f, 0.48f, 0.10f)
             .topology (web)
             .chord (2.0f * 1.004f, 3.0f * 0.996f, 4.0f * 1.005f, 6.0f * 0.995f)
             .motion (0.36f, 0.26f)
             .fx (1.5f, 0.48f));

    add (Builder ("Bowed Gong", cat, "gong, bowed, low, swell",
                  "A bow on the rim of a large gong: a slow, low swell of dark metal.")
             .bow (0.58f, 0.34f, 0.62f)
             .material (metal, 0.80f, 0.34f, 0.24f, 0.62f)
             .macros (0.60f, 0.66f, 0.40f, 0.10f)
             .topology (web)
             .fx (1.3f, 0.40f));

    add (Builder ("Tremolo Strings", cat, "tremolo, strings, synced, pulsing",
                  "Bowed wood with nodes breathing in sixteenth notes: a measured string tremolo.")
             .bow (0.50f, 0.50f, 0.54f)
             .material (wood, 0.58f, 0.46f, 0.38f, 0.44f)
             .macros (0.58f, 0.40f, 0.50f, 0.04f)
             .synced (quarter, 0.0f)
             .gesture (0, breatheGesture (0.125f, 0.25f, 0.06f, 1))
             .gesture (1, breatheGesture (0.125f, 0.25f, 0.06f, 1))
             .fx (1.3f, 0.30f));

    add (Builder ("Ice Bow", cat, "glass, cold, bright, high tension",
                  "Taut glass under a fast bow: cold, glittering and thin.")
             .bow (0.36f, 0.62f, 0.34f)
             .material (glass, 0.36f, 0.70f, 0.28f, 0.54f)
             .macros (0.58f, 0.34f, 0.74f, 0.04f)
             .fx (1.3f, 0.34f));

    add (Builder ("Bowed Vibes", cat, "vibraphone, bowed, pure, motor",
                  "A bowed vibraphone bar (1 : 4) with the motor turning slowly: pure and shimmering.")
             .bow (0.36f, 0.44f, 0.40f)
             .material (metal, 0.50f, 0.46f, 0.28f, 0.40f)
             .macros (0.58f, 0.26f, 0.50f, 0.02f)
             .tune (2, 3.0f).tune (3, 4.0f)
             .levels (0.30f, 0.30f, 0.40f, 0.70f)
             .synced (half, 0.0f)
             .gesture (3, breatheGesture (1.0f, 2.0f, 0.03f, 1))
             .fx (1.3f, 0.34f));

    add (Builder ("Nyckelharpa", cat, "keyed fiddle, sympathetic, folk, bright",
                  "A keyed fiddle with sympathetic strings on the fifth, octave and twelfth.")
             .bow (0.52f, 0.52f, 0.58f)
             .material (wood, 0.52f, 0.52f, 0.36f, 0.46f)
             .macros (0.60f, 0.52f, 0.54f, 0.04f)
             .chord (kFifth * 2.0f, 4.0f, 6.0f, 8.0f)
             .decays (0.70f, 0.72f, 0.72f, 0.70f)
             .fx (1.25f, 0.28f));

    add (Builder ("Hurdy Wheel", cat, "hurdy-gurdy, buzzing, drone, chain",
                  "A rosined wheel on gut: buzzing, rough and droning through a chain of sympathetic nodes.")
             .bow (0.66f, 0.56f, 0.76f)
             .material (wood, 0.54f, 0.54f, 0.40f, 0.52f)
             .macros (0.62f, 0.56f, 0.52f, 0.08f)
             .topology (chain)
             .chord (kFifth * 2.0f, 4.0f, 6.0f, 8.0f)
             .fx (1.2f, 0.24f, 0.20f));

    add (Builder ("Silk Bow", cat, "soft, membrane, silky, gentle",
                  "A very light bow on a soft membrane: silky and gentle.")
             .bow (0.22f, 0.38f, 0.30f)
             .material (membrane, 0.52f, 0.42f, 0.30f, 0.42f)
             .macros (0.56f, 0.30f, 0.50f, 0.04f)
             .fx (1.3f, 0.34f));

    add (Builder ("Rusty Hinge", cat, "creaking, scratchy, metal, high pressure",
                  "Heavy pressure and a slow bow on rusty metal: creaking, scratchy and alive.")
             .bow (0.88f, 0.26f, 0.80f)
             .material (metal, 0.54f, 0.50f, 0.40f, 0.66f)
             .macros (0.60f, 0.40f, 0.48f, 0.18f)
             .fx (1.1f, 0.20f));

    add (Builder ("Growl Skin", cat, "bass, membrane, growl, low",
                  "A bowed drum head, low and slack: a dark, growling bass.")
             .bow (0.62f, 0.40f, 0.66f)
             .material (membrane, 0.74f, 0.36f, 0.40f, 0.48f)
             .macros (0.62f, 0.34f, 0.36f, 0.06f)
             .voice (VoiceMode::mono, 0.06f)
             .fx (1.0f, 0.16f, 0.10f));

    add (Builder ("Flageolet", cat, "harmonics, violin, high, pure",
                  "Violin harmonics: a light bow on a taut string, the nodes on the upper partials.")
             .bow (0.30f, 0.56f, 0.40f)
             .material (wood, 0.40f, 0.60f, 0.34f, 0.42f)
             .macros (0.58f, 0.34f, 0.66f, 0.02f)
             .chord (4.0f, 6.0f, 8.0f, 12.0f)
             .fx (1.25f, 0.32f));

    add (Builder ("Choir of Glasses", cat, "glass, choir, major, web",
                  "A choir of bowed glasses tuned to a major chord, all touching in a web.")
             .bow (0.32f, 0.44f, 0.36f)
             .material (glass, 0.48f, 0.52f, 0.30f, 0.44f)
             .macros (0.58f, 0.56f, 0.52f, 0.04f)
             .topology (web)
             .chord (kMajor3 * 2.0f, kFifth * 2.0f, 4.0f, 5.0f * 2.0f)
             .fx (1.4f, 0.40f));

    add (Builder ("Quartet Web", cat, "quartet, strings, chamber, web",
                  "Four bowed bodies in one: a chamber quartet whose parts lean on each other.")
             .bow (0.48f, 0.50f, 0.54f)
             .material (wood, 0.58f, 0.48f, 0.38f, 0.44f)
             .macros (0.58f, 0.60f, 0.52f, 0.06f)
             .topology (web)
             .chord (kMajor3 * 2.0f, kFifth * 2.0f, 4.0f, 6.0f)
             .fx (1.35f, 0.30f));

    add (Builder ("Sul Ponticello", cat, "ponticello, glassy, bright, eerie",
                  "Bowed right at the bridge: glassy upper partials and an eerie, brittle edge.")
             .bow (0.46f, 0.60f, 0.70f)
             .material (wood, 0.44f, 0.76f, 0.34f, 0.54f)
             .macros (0.58f, 0.42f, 0.60f, 0.06f)
             .fx (1.2f, 0.28f));

    add (Builder ("Bowed Crotales", cat, "crotales, bowed, bell, high",
                  "Bowed antique cymbals: a high, pure, bell-like sustain.")
             .bow (0.36f, 0.50f, 0.36f)
             .material (metal, 0.30f, 0.66f, 0.20f, 0.56f)
             .macros (0.58f, 0.30f, 0.82f, 0.02f)
             .fx (1.35f, 0.40f));

    add (Builder ("Legato Lead", cat, "lead, legato, mono, bright, driven",
                  "A bright, driven bowed lead: mono, legato, hard on the string with a quick glide.")
             .bow (0.62f, 0.62f, 0.68f)
             .material (wood, 0.48f, 0.70f, 0.30f, 0.46f)
             .macros (0.62f, 0.44f, 0.58f, 0.04f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .voice (VoiceMode::legato, 0.07f)
             .motion (0.08f, 0.30f)
             .fx (1.0f, 0.18f, 0.30f));

    add (Builder ("Arco Shimmer", cat, "glass, orbit, shimmer, bowed",
                  "Bowed glass with two nodes orbiting at different speeds: a slowly turning shimmer.")
             .bow (0.34f, 0.46f, 0.40f)
             .material (glass, 0.44f, 0.58f, 0.30f, 0.48f)
             .macros (0.58f, 0.50f, 0.56f, 0.06f)
             .gesture (1, orbitGesture (5.0f, 0.0f, 1.0f, 0.006f))
             .gesture (2, orbitGesture (7.5f, 0.0f, -1.0f, 0.006f))
             .fx (1.45f, 0.36f));

    add (Builder ("Rubbed Skin", cat, "friction drum, membrane, rubbed, strange",
                  "A friction drum: a rubbed stick excites the skin into a strange, groaning tone.")
             .bow (0.72f, 0.30f, 0.72f)
             .material (membrane, 0.60f, 0.44f, 0.36f, 0.54f)
             .macros (0.60f, 0.44f, 0.46f, 0.12f)
             .topology (star)
             .fx (1.2f, 0.24f));

    add (Builder ("Steel Cello", cat, "cello, metal, bright, strange",
                  "A cello built from steel: the bow and phrasing of a cello, the ring of metal.")
             .bow (0.54f, 0.48f, 0.56f)
             .material (metal, 0.62f, 0.48f, 0.32f, 0.50f)
             .macros (0.60f, 0.36f, 0.48f, 0.04f)
             .chord (kFifth, 2.0f, 3.0f, 4.0f)
             .fx (1.2f, 0.28f));

    add (Builder ("Cantabile", cat, "singing, mono, vibrato, expressive",
                  "A singing bowed line with a gentle vibrato from a breathing node.")
             .bow (0.48f, 0.54f, 0.50f)
             .material (wood, 0.56f, 0.48f, 0.34f, 0.44f)
             .macros (0.60f, 0.34f, 0.52f, 0.02f)
             .voice (VoiceMode::mono, 0.05f)
             .gesture (0, breatheGesture (0.18f, 0.0f, 0.012f, 1))
             .fx (1.15f, 0.30f));

    add (Builder ("Bow and Chain", cat, "metal, chain, bloom, delayed",
                  "Bowed metal in a chain: the far nodes answer late, so every note blooms outward.")
             .bow (0.48f, 0.44f, 0.54f)
             .material (metal, 0.56f, 0.48f, 0.26f, 0.52f)
             .macros (0.58f, 0.70f, 0.50f, 0.06f)
             .topology (chain)
             .chord (kMajor3, kFifth * 1.5f, 3.0f, 4.5f)
             .fx (1.3f, 0.34f));

    add (Builder ("Spectral Bow", cat, "hollow, odd harmonics, glass, clarinet",
                  "Bowed glass tuned to the odd harmonics only: hollow, reedy, almost a clarinet.")
             .bow (0.40f, 0.48f, 0.44f)
             .material (glass, 0.48f, 0.50f, 0.32f, 0.42f)
             .macros (0.58f, 0.40f, 0.50f, 0.02f)
             .chord (3.0f, 5.0f, 7.0f, 9.0f)
             .fx (1.2f, 0.28f));

    add (Builder ("Bariolage", cat, "arpeggio, violin, sequenced, synced",
                  "The violinist's rocking across strings: a bowed body whose sympathetic node steps through the chord in eighths.")
             .bow (0.50f, 0.56f, 0.56f)
             .material (wood, 0.50f, 0.54f, 0.36f, 0.44f)
             .macros (0.60f, 0.46f, 0.54f, 0.02f)
             .quantise()
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .levels (0.70f, 0.36f, 0.30f, 0.24f)
             .synced (bar1, 0.0f)
             .gesture (0, stepGesture (2.0f, 4.0f,
                                       { 0.0f, 4 * kSemitone, 7 * kSemitone, 12 * kSemitone, 7 * kSemitone, 4 * kSemitone, 0.0f, -5 * kSemitone },
                                       0.15f))
             .fx (1.25f, 0.28f));
}

} // namespace arc::presets::detail

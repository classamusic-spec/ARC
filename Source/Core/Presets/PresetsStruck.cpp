// Factory library: STRUCK. Bells, keys, gongs, bowls and mallets: a momentum-conserving
// strike on the CORE, the body rings. See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addStruck (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "STRUCK";

    add (Builder ("Tine Piano", cat, "electric piano, tine, keys, tremolo",
                  "Soft hammers on metal tines with an auto-panning tremolo, the way a suitcase piano sings.")
             .strike (0.34f, 0.46f, 0.44f)
             .material (metal, 0.52f, 0.44f, 0.36f, 0.42f)
             .macros (0.62f, 0.28f, 0.50f, 0.02f)
             .chord (2.0f, 2.5f, 3.0f, 3.5f)
             .levels (0.42f, 0.30f, 0.24f, 0.16f)
             .synced (quarter, 0.0f)
             .gesture (0, swayGesture (0.5f, 1.0f, 1.2f, 0.0f))
             .gesture (1, swayGesture (0.5f, 1.0f, -1.2f, 0.0f))
             .release (0.35f)
             .fx (1.35f, 0.22f));

    add (Builder ("Carillon", cat, "bell, tower, bronze, partials",
                  "A tower bell: minor-third tierce, quint, nominal and superquint ringing over the hum.")
             .strike (0.66f, 0.26f, 0.56f)
             .material (metal, 0.62f, 0.50f, 0.28f, 0.50f)
             .macros (0.64f, 0.42f, 0.50f, 0.04f)
             .chord (kMinor3, kFifth, 2.0f, 3.0f)
             .fx (1.3f, 0.36f));

    add (Builder ("Motor Vibes", cat, "vibraphone, motor, tremolo, soft",
                  "Soft mallets on a vibraphone bar (the 1 : 4 overtone) with the motor turning in tempo.")
             .strike (0.36f, 0.48f, 0.46f)
             .material (metal, 0.50f, 0.46f, 0.34f, 0.40f)
             .macros (0.62f, 0.26f, 0.62f, 0.02f)
             .tune (1, 3.0f).tune (2, 4.0f)
             .levels (0.30f, 0.30f, 0.66f, 0.20f)
             .synced (quarter, 0.0f)
             .gesture (2, breatheGesture (0.5f, 1.0f, 0.02f, 1))
             .release (0.30f)
             .fx (1.3f, 0.26f));

    add (Builder ("Celesta Box", cat, "celesta, bright, small, keys",
                  "Small felt hammers on bright steel plates inside a wooden box: a celesta.")
             .strike (0.46f, 0.30f, 0.62f)
             .material (metal, 0.36f, 0.62f, 0.44f, 0.46f)
             .macros (0.62f, 0.30f, 0.60f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.50f, 0.34f, 0.26f, 0.18f)
             .release (0.30f)
             .fx (1.25f, 0.26f));

    add (Builder ("Glass Glockenspiel", cat, "glockenspiel, glass, bar, bright",
                  "Hard mallets on glass bars tuned with the free-bar overtones 2.76 and 5.40.")
             .strike (0.78f, 0.16f, 0.70f)
             .material (glass, 0.38f, 0.66f, 0.36f, 0.50f)
             .macros (0.62f, 0.26f, 0.56f, 0.02f)
             .tune (0, 2.756f).tune (1, 5.404f)
             .levels (0.62f, 0.44f, 0.20f, 0.16f)
             .fx (1.3f, 0.28f));

    add (Builder ("Tubular Hall", cat, "tubular bells, hall, orchestral, metal",
                  "Orchestral tubular bells in a large hall: the stretched 4th-7th tube partials.")
             .strike (0.70f, 0.24f, 0.56f)
             .material (metal, 0.58f, 0.50f, 0.26f, 0.56f)
             .macros (0.64f, 0.38f, 0.54f, 0.04f)
             .chord (2.0f * 1.01f, 2.0f * 1.43f, 2.0f * 1.83f, 4.47f)
             .fx (1.35f, 0.48f));

    add (Builder ("Singing Bowl", cat, "singing bowl, beating, meditation, metal",
                  "A struck bowl whose paired modes beat slowly against each other for a long time.")
             .strike (0.30f, 0.58f, 0.40f)
             .material (metal, 0.66f, 0.40f, 0.18f, 0.46f)
             .macros (0.60f, 0.36f, 0.48f, 0.04f)
             .chord (2.0f * 1.006f, 2.0f * 0.994f, 2.71f * 1.004f, 2.71f * 0.996f)
             .decays (0.80f, 0.80f, 0.76f, 0.76f)
             .fx (1.3f, 0.40f));

    add (Builder ("Handpan Circle", cat, "handpan, steel, tuned, soft",
                  "A steel handpan note: fundamental, octave and compound fifth tuned into the shell.")
             .strike (0.40f, 0.46f, 0.44f)
             .material (metal, 0.56f, 0.44f, 0.32f, 0.40f)
             .macros (0.62f, 0.34f, 0.50f, 0.02f)
             .topology (star)
             .tune (2, 2.0f).tune (3, 3.0f)
             .levels (0.30f, 0.30f, 0.66f, 0.56f)
             .fx (1.3f, 0.32f));

    add (Builder ("Steel Pan Sun", cat, "steel pan, tropical, bright, harmonic",
                  "A bright steel-pan note with its harmonic 2nd, 3rd and 4th partials.")
             .strike (0.52f, 0.32f, 0.60f)
             .material (metal, 0.46f, 0.58f, 0.38f, 0.42f)
             .macros (0.62f, 0.36f, 0.54f, 0.02f)
             .chord (2.0f, 2.5f, 3.0f, 4.0f)
             .levels (0.64f, 0.52f, 0.40f, 0.24f)
             .fx (1.2f, 0.24f));

    add (Builder ("Gong Bloom", cat, "gong, bloom, web, slow",
                  "A soft-mallet gong: energy spreads slowly through the web and the sound swells after the hit.")
             .strike (0.26f, 0.66f, 0.38f)
             .material (metal, 0.72f, 0.40f, 0.22f, 0.60f)
             .macros (0.64f, 0.74f, 0.42f, 0.08f)
             .topology (web)
             .fx (1.35f, 0.44f));

    add (Builder ("Tam-Tam Wash", cat, "tam-tam, wash, inharmonic, chaos",
                  "A large tam-tam hit hard: a bright, chaotic wash that keeps shifting.")
             .strike (0.74f, 0.30f, 0.62f)
             .material (metal, 0.60f, 0.62f, 0.26f, 0.78f)
             .macros (0.64f, 0.82f, 0.56f, 0.42f)
             .topology (web)
             .motion (0.24f, 0.30f)
             .fx (1.45f, 0.46f));

    add (Builder ("Glass Piano", cat, "piano, glass, keys, harmonic",
                  "Piano hammers on glass strings: a clear, crystalline keyboard.")
             .strike (0.50f, 0.34f, 0.54f)
             .material (glass, 0.50f, 0.52f, 0.40f, 0.44f)
             .macros (0.62f, 0.30f, 0.52f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.56f, 0.44f, 0.34f, 0.26f)
             .release (0.55f)
             .fx (1.3f, 0.26f));

    add (Builder ("Crotale Sparks", cat, "crotales, antique cymbal, bright, high",
                  "Antique cymbals: tiny, very taut bronze discs that ring bright and long.")
             .strike (0.84f, 0.12f, 0.72f)
             .material (metal, 0.30f, 0.70f, 0.24f, 0.58f)
             .macros (0.62f, 0.34f, 0.80f, 0.03f)
             .fx (1.35f, 0.34f));

    add (Builder ("Hammered Dulcimer", cat, "dulcimer, hammered, courses, bright",
                  "Light hammers on paired courses tuned a few cents apart: bright and shimmering.")
             .strike (0.66f, 0.18f, 0.66f)
             .material (metal, 0.42f, 0.60f, 0.40f, 0.48f)
             .macros (0.62f, 0.34f, 0.56f, 0.03f)
             .chord (2.0f * 1.004f, 2.0f * 0.996f, 3.0f, 4.0f)
             .fx (1.4f, 0.26f));

    add (Builder ("Music Box Dream", cat, "music box, tiny, sweet, lullaby",
                  "The steel comb of a music box, drifting slowly in a soft room.")
             .strike (0.60f, 0.20f, 0.68f)
             .material (metal, 0.30f, 0.64f, 0.40f, 0.48f)
             .macros (0.62f, 0.26f, 0.64f, 0.03f)
             .chord (1.5f, 2.0f, 3.0f, 4.0f)
             .levels (0.40f, 0.30f, 0.22f, 0.16f)
             .gesture (1, swayGesture (7.0f, 0.0f, 0.8f, 0.01f))
             .fx (1.3f, 0.44f));

    add (Builder ("Temple Gong", cat, "gong, temple, low, beating",
                  "A heavy temple gong struck softly: a low, slowly beating hum.")
             .strike (0.30f, 0.62f, 0.34f)
             .material (metal, 0.84f, 0.32f, 0.24f, 0.56f)
             .macros (0.66f, 0.46f, 0.36f, 0.06f)
             .chord (1.5f * 1.004f, 1.5f * 0.996f, 2.0f, 2.8f)
             .fx (1.3f, 0.40f));

    add (Builder ("Crystal Bowl", cat, "crystal bowl, glass, pure, long",
                  "A quartz singing bowl, struck with a suede mallet: pure and very long.")
             .strike (0.28f, 0.60f, 0.46f)
             .material (glass, 0.58f, 0.46f, 0.16f, 0.44f)
             .macros (0.62f, 0.30f, 0.50f, 0.02f)
             .decays (0.84f, 0.80f, 0.76f, 0.72f)
             .fx (1.35f, 0.40f));

    add (Builder ("Wind Bell", cat, "bell, swaying, orbit, outdoor",
                  "A small bell hanging in the wind: one node orbits so every note sways a little.")
             .strike (0.56f, 0.28f, 0.58f)
             .material (metal, 0.44f, 0.56f, 0.30f, 0.52f)
             .macros (0.62f, 0.40f, 0.58f, 0.06f)
             .gesture (3, orbitGesture (3.2f, 0.0f, 1.0f, 0.01f))
             .fx (1.45f, 0.36f));

    add (Builder ("Soft Mallet Keys", cat, "mallet, wood, soft, keys",
                  "Yarn mallets on wooden keys: soft, woody and warm.")
             .strike (0.26f, 0.56f, 0.40f)
             .material (wood, 0.56f, 0.40f, 0.40f, 0.42f)
             .macros (0.64f, 0.26f, 0.50f, 0.02f)
             .tune (0, 4.0f).tune (1, 9.8f)
             .levels (0.70f, 0.40f, 0.22f, 0.16f)
             .fx (1.2f, 0.24f));

    add (Builder ("Bell Tree", cat, "bell tree, cascade, chain, shimmer",
                  "Nested bells on a chain: the energy cascades from node to node after each strike.")
             .strike (0.62f, 0.22f, 0.66f)
             .material (metal, 0.38f, 0.64f, 0.30f, 0.54f)
             .macros (0.62f, 0.68f, 0.62f, 0.04f)
             .topology (chain)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .fx (1.45f, 0.38f));

    add (Builder ("Chime Garden", cat, "wind chimes, wander, web, outdoor",
                  "Metal tubes on a web, each node wandering on its own slow path like chimes in a breeze.")
             .strike (0.58f, 0.24f, 0.60f)
             .material (metal, 0.40f, 0.60f, 0.28f, 0.56f)
             .macros (0.62f, 0.56f, 0.58f, 0.10f)
             .topology (web)
             .gesture (0, wanderGesture (6.0f, 0.0f, 0.8f, 0.03f, 301u))
             .gesture (1, wanderGesture (7.0f, 0.0f, 0.8f, 0.03f, 302u))
             .gesture (2, wanderGesture (8.0f, 0.0f, 0.8f, 0.03f, 303u))
             .fx (1.5f, 0.40f));

    add (Builder ("Frozen Bell", cat, "bell, glass, endless, freeze",
                  "A glass bell with an almost endless decay. Play, then press FREEZE to hold it.")
             .strike (0.48f, 0.34f, 0.54f)
             .material (glass, 0.62f, 0.50f, 0.12f, 0.50f)
             .macros (0.62f, 0.40f, 0.54f, 0.04f)
             .decays (0.86f, 0.86f, 0.84f, 0.82f)
             .fx (1.4f, 0.46f));

    add (Builder ("Dark Carillon", cat, "bell, dark, heavy, bronze",
                  "A heavy carillon bell with the brightness filed off: dark bronze partials.")
             .strike (0.40f, 0.44f, 0.36f)
             .material (metal, 0.74f, 0.28f, 0.26f, 0.52f)
             .macros (0.64f, 0.40f, 0.44f, 0.04f)
             .chord (kMinor3, kFifth, 2.0f, 2.51f)
             .fx (1.3f, 0.38f));

    add (Builder ("Harmonic Gong", cat, "gong, tuned, harmonic, bloom",
                  "A gong forged onto the harmonic series: gong attack, organ-like bloom.")
             .strike (0.36f, 0.54f, 0.44f)
             .material (metal, 0.66f, 0.46f, 0.24f, 0.40f)
             .macros (0.64f, 0.60f, 0.46f, 0.04f)
             .topology (web)
             .chord (1.5f, 2.5f, 3.0f, 4.0f)
             .fx (1.35f, 0.40f));

    add (Builder ("Felt Piano", cat, "piano, felt, soft, intimate",
                  "Felt-dampened hammers: a soft, intimate piano with dampers that close on release.")
             .strike (0.22f, 0.62f, 0.36f)
             .material (metal, 0.56f, 0.38f, 0.36f, 0.42f)
             .macros (0.64f, 0.24f, 0.50f, 0.02f)
             .chord (2.0f, 1.5f, 3.0f, 4.0f)
             .levels (0.44f, 0.32f, 0.24f, 0.16f)
             .release (0.60f)
             .fx (1.2f, 0.24f));

    add (Builder ("Toy Piano", cat, "toy piano, tinny, detuned, bright",
                  "A toy piano: hard hammers on short metal rods, slightly out of tune with itself.")
             .strike (0.80f, 0.14f, 0.70f)
             .material (metal, 0.30f, 0.66f, 0.50f, 0.56f)
             .macros (0.64f, 0.30f, 0.62f, 0.04f)
             .chord (2.0f * 1.012f, 3.0f * 0.99f, 4.2f, 5.6f)
             .release (0.40f)
             .fx (1.1f, 0.18f));

    add (Builder ("Clockwork Bell", cat, "bell, ticking, steps, synced",
                  "A bell inside a clock: node A steps through its gears on every beat.")
             .strike (0.60f, 0.24f, 0.58f)
             .material (metal, 0.46f, 0.54f, 0.30f, 0.50f)
             .macros (0.62f, 0.50f, 0.56f, 0.04f)
             .quantise()
             .synced (bar1, 0.0f)
             .gesture (0, stepGesture (2.0f, 4.0f, { 0.0f, 5.0f * kSemitone, 0.0f, 7.0f * kSemitone }, 0.08f))
             .fx (1.3f, 0.30f));

    add (Builder ("Bronze Age", cat, "bronze, ancient, beating, dark",
                  "An ancient bronze bell, thick-walled and slightly out of round: slow beats.")
             .strike (0.52f, 0.36f, 0.42f)
             .material (metal, 0.70f, 0.36f, 0.28f, 0.62f)
             .macros (0.64f, 0.44f, 0.46f, 0.08f)
             .chord (kMinor3 * 1.004f, kFifth * 0.995f, 2.0f * 1.003f, 2.6f)
             .fx (1.3f, 0.34f));

    add (Builder ("Cloud Bell", cat, "bell, glass, airy, orbit",
                  "A soft glass bell drifting through a wide, airy space.")
             .strike (0.34f, 0.46f, 0.50f)
             .material (glass, 0.50f, 0.50f, 0.24f, 0.48f)
             .macros (0.62f, 0.44f, 0.52f, 0.06f)
             .gesture (1, orbitGesture (12.0f, 0.0f, -1.0f, 0.006f))
             .fx (1.5f, 0.60f));

    add (Builder ("Tuned Pipes", cat, "pipes, odd harmonics, hollow, metal",
                  "Struck closed pipes: only the odd harmonics, a hollow metallic tone.")
             .strike (0.58f, 0.26f, 0.54f)
             .material (metal, 0.48f, 0.52f, 0.34f, 0.40f)
             .macros (0.62f, 0.34f, 0.52f, 0.02f)
             .tune (2, 3.0f).tune (3, 5.0f)
             .levels (0.18f, 0.18f, 0.62f, 0.50f)
             .fx (1.25f, 0.28f));

    add (Builder ("Bonang Row", cat, "gamelan, pelog, kettle gong, bronze",
                  "A row of bronze kettle gongs tuned to the pelog scale: every note carries the scale.")
             .strike (0.62f, 0.26f, 0.52f)
             .material (metal, 0.52f, 0.50f, 0.32f, 0.54f)
             .macros (0.62f, 0.40f, 0.54f, 0.04f)
             .chord (semis (2.7f) * 2.0f, semis (5.4f), semis (6.7f), semis (9.5f) * 2.0f)
             .fx (1.3f, 0.30f));

    add (Builder ("Pulse Bell", cat, "bell, pulsing, synced, wah",
                  "A struck bell whose nodes breathe in eighth notes: the tail pulses in tempo.")
             .strike (0.56f, 0.28f, 0.56f)
             .material (metal, 0.50f, 0.54f, 0.26f, 0.50f)
             .macros (0.62f, 0.50f, 0.56f, 0.04f)
             .synced (quarter, 0.0f)
             .gesture (1, breatheGesture (0.25f, 0.5f, 0.05f, 1))
             .gesture (2, breatheGesture (0.25f, 0.5f, -0.05f, 1))
             .fx (1.35f, 0.30f));
}

} // namespace arc::presets::detail

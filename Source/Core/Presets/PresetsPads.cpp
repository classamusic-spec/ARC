// Factory library: PADS. Sustained, evolving beds: bowed or blown bodies whose nodes carry
// chords, slow drift and gestures. See docs/PRESETS.md for the catalogue.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addPads (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "PADS";

    add (Builder ("Silver Hour", cat, "bowed, glass, major 9, wide",
                  "Bowed glass whose four nodes hold a major ninth: the chord rings inside every note.")
             .bow (0.34f, 0.40f, 0.38f)
             .material (glass, 0.50f, 0.52f, 0.30f, 0.45f)
             .macros (0.56f, 0.42f, 0.52f, 0.06f)
             .chord (kMajor3 * 2.0f, kFifth * 2.0f, kMajor7 * 2.0f, 4.5f)
             .motion (0.18f, 0.05f)
             .release (0.10f)
             .fx (1.35f, 0.40f));

    add (Builder ("Velvet Engine", cat, "air, wood, warm, breathing",
                  "Soft breath through a wooden body; one node breathes in and out every two bars.")
             .air (0.46f, 0.40f, 0.30f)
             .material (wood, 0.62f, 0.34f, 0.38f, 0.40f)
             .macros (0.54f, 0.38f, 0.46f, 0.08f)
             .synced (bar2, 0.10f)
             .gesture (1, breatheGesture (4.0f, 8.0f, 0.05f, 1))
             .release (0.12f)
             .fx (1.2f, 0.32f));

    add (Builder ("Polar Choir", cat, "air, glass, choir, orbit",
                  "Blown glass in a web; two nodes circle the core like voices walking around a hall.")
             .air (0.52f, 0.34f, 0.60f)
             .material (glass, 0.44f, 0.60f, 0.32f, 0.48f)
             .macros (0.56f, 0.62f, 0.56f, 0.10f)
             .topology (web)
             .chord (3.0f, 4.0f, 6.0f, 8.0f)
             .gesture (0, orbitGesture (14.0f, 0.0f, 1.0f, 0.006f))
             .gesture (3, orbitGesture (19.0f, 0.0f, -1.0f, 0.006f))
             .release (0.10f)
             .fx (1.45f, 0.46f));

    add (Builder ("Nocturne Field", cat, "bowed, membrane, minor, dark",
                  "A bowed membrane under low tension; the nodes hold a minor seventh chord in the dark.")
             .bow (0.40f, 0.34f, 0.44f)
             .material (membrane, 0.66f, 0.30f, 0.34f, 0.40f)
             .macros (0.56f, 0.46f, 0.38f, 0.08f)
             .chord (kMinor3 * 2.0f, kFifth * 2.0f, kMinor7 * 2.0f, 4.0f)
             .motion (0.20f, 0.04f)
             .release (0.10f)
             .fx (1.25f, 0.42f));

    add (Builder ("Halo Drift", cat, "bowed, metal, chain, figure eight",
                  "Bowed metal in a chain: energy walks node to node while node B traces a slow figure eight.")
             .bow (0.36f, 0.42f, 0.48f)
             .material (metal, 0.55f, 0.46f, 0.26f, 0.50f)
             .macros (0.56f, 0.55f, 0.50f, 0.10f)
             .topology (chain)
             .chord (kMajor3, kFifth, kMajor7, 3.0f)
             .gesture (1, figureGesture (12.0f, 0.0f, 0.8f, 0.03f, 1, 2))
             .release (0.08f)
             .fx (1.3f, 0.38f));

    add (Builder ("Lumen", cat, "air, glass, bright, suspended",
                  "Bright breath on high-tension glass; the nodes sit on a suspended chord that never resolves.")
             .air (0.58f, 0.22f, 0.70f)
             .material (glass, 0.40f, 0.68f, 0.28f, 0.52f)
             .macros (0.56f, 0.40f, 0.66f, 0.05f)
             .chord (kFourth * 2.0f, kFifth * 2.0f, 4.0f, 6.0f)
             .motion (0.14f, 0.07f)
             .release (0.10f)
             .fx (1.4f, 0.36f));

    add (Builder ("Cloud Loom", cat, "bowed, wood, web, wander",
                  "Bowed wood woven through a web; every node wanders on its own slow path.")
             .bow (0.44f, 0.40f, 0.52f)
             .material (wood, 0.58f, 0.44f, 0.36f, 0.50f)
             .macros (0.56f, 0.58f, 0.50f, 0.22f)
             .topology (web)
             .gesture (0, wanderGesture (11.0f, 0.0f, 0.7f, 0.03f, 11u))
             .gesture (2, wanderGesture (13.0f, 0.0f, 0.7f, 0.03f, 23u))
             .release (0.10f)
             .fx (1.3f, 0.36f));

    add (Builder ("Slow Tide", cat, "air, membrane, synced, tide",
                  "Breath across a membrane that swells and ebbs over eight bars, locked to your tempo.")
             .air (0.48f, 0.56f, 0.36f)
             .material (membrane, 0.58f, 0.40f, 0.30f, 0.46f)
             .macros (0.56f, 0.44f, 0.46f, 0.10f)
             .synced (bar8, 0.26f)
             .gesture (0, breatheGesture (16.0f, 32.0f, 0.06f, 1))
             .gesture (2, breatheGesture (16.0f, 32.0f, -0.05f, 1))
             .release (0.12f)
             .fx (1.35f, 0.50f));

    add (Builder ("Amber Hall", cat, "bowed, wood, harmonic, warm",
                  "A warm bowed body with its nodes on the harmonic series, in a large amber room.")
             .bow (0.46f, 0.46f, 0.50f)
             .material (wood, 0.60f, 0.40f, 0.34f, 0.44f)
             .macros (0.56f, 0.34f, 0.50f, 0.04f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.62f, 0.58f, 0.52f, 0.46f)
             .motion (0.12f, 0.05f)
             .release (0.10f)
             .fx (1.2f, 0.58f));

    add (Builder ("Opaline", cat, "air, glass, pentatonic, soft",
                  "Gently blown glass tuned to a pentatonic cluster: soft, milky and consonant.")
             .air (0.44f, 0.30f, 0.52f)
             .material (glass, 0.48f, 0.48f, 0.34f, 0.44f)
             .macros (0.54f, 0.36f, 0.50f, 0.06f)
             .chord (kMajor2 * 2.0f, kMajor3 * 2.0f, kFifth * 2.0f, kMajor6 * 2.0f)
             .motion (0.16f, 0.06f)
             .release (0.12f)
             .fx (1.3f, 0.40f));

    add (Builder ("Northern Lights", cat, "bowed, glass, orbit, synced",
                  "Bowed glass curtains: nodes A and D orbit in opposite directions over two bars.")
             .bow (0.32f, 0.38f, 0.36f)
             .material (glass, 0.46f, 0.66f, 0.30f, 0.50f)
             .macros (0.56f, 0.50f, 0.62f, 0.08f)
             .chord (kMajor2 * 2.0f, kFifth * 2.0f, 4.5f, 6.0f)
             .synced (bar2, 0.0f)
             .gesture (0, orbitGesture (4.0f, 8.0f, 1.0f, 0.008f))
             .gesture (3, orbitGesture (4.0f, 8.0f, -1.0f, 0.008f))
             .release (0.10f)
             .fx (1.5f, 0.44f));

    add (Builder ("Paper Moon", cat, "air, membrane, fragile, wide",
                  "A fragile breath of turbulence behind a thin membrane: more air than tone.")
             .air (0.36f, 0.78f, 0.44f)
             .material (membrane, 0.40f, 0.50f, 0.30f, 0.52f)
             .macros (0.52f, 0.36f, 0.52f, 0.12f)
             .motion (0.26f, 0.08f)
             .release (0.14f)
             .fx (1.5f, 0.52f));

    add (Builder ("Iron Lung", cat, "air, metal, breathing, synced",
                  "Air pumped through iron pipes: the network inhales on every bar.")
             .air (0.56f, 0.36f, 0.44f)
             .material (metal, 0.60f, 0.42f, 0.30f, 0.52f)
             .macros (0.56f, 0.48f, 0.46f, 0.06f)
             .synced (bar1, 0.12f)
             .gesture (0, breatheGesture (2.0f, 4.0f, 0.08f, 1))
             .gesture (1, breatheGesture (2.0f, 4.0f, 0.06f, 1))
             .release (0.10f)
             .fx (1.2f, 0.34f, 0.06f));

    add (Builder ("Deep Current", cat, "bowed, metal, dark, heavy",
                  "A heavy, slack metal body bowed slowly; the lowest pad in the library.")
             .bow (0.48f, 0.34f, 0.56f)
             .material (metal, 0.80f, 0.26f, 0.28f, 0.50f)
             .macros (0.56f, 0.44f, 0.32f, 0.10f)
             .gesture (1, wanderGesture (17.0f, 0.0f, 0.5f, 0.02f, 7u))
             .release (0.08f)
             .fx (1.2f, 0.40f));

    add (Builder ("Glasshouse", cat, "bowed, glass, fifths, web",
                  "Stacked fifths in bowed glass, all coupled in a web: open and luminous.")
             .bow (0.36f, 0.44f, 0.40f)
             .material (glass, 0.46f, 0.58f, 0.32f, 0.46f)
             .macros (0.56f, 0.56f, 0.52f, 0.06f)
             .topology (web)
             .chord (kFifth * 2.0f, 4.5f, 6.75f, 10.125f)
             .motion (0.18f, 0.06f)
             .release (0.10f)
             .fx (1.4f, 0.42f));

    add (Builder ("Cinder Veil", cat, "air, wood, warm, grit",
                  "Warm breath through charred wood with a little grit from the drive stage.")
             .air (0.60f, 0.46f, 0.34f)
             .material (wood, 0.56f, 0.36f, 0.40f, 0.52f)
             .macros (0.56f, 0.40f, 0.48f, 0.10f)
             .motion (0.20f, 0.07f)
             .release (0.10f)
             .fx (1.2f, 0.30f, 0.22f));

    add (Builder ("Satin Web", cat, "bowed, membrane, web, soft",
                  "A soft bowed membrane where every node touches every other: smooth and enveloping.")
             .bow (0.30f, 0.36f, 0.34f)
             .material (membrane, 0.54f, 0.44f, 0.30f, 0.44f)
             .macros (0.54f, 0.60f, 0.50f, 0.06f)
             .topology (web)
             .motion (0.22f, 0.05f)
             .release (0.10f)
             .fx (1.35f, 0.40f));

    add (Builder ("Morning Frost", cat, "air, glass, cold, sparkle",
                  "Cold, bright air over frosted glass; quick drift makes the upper partials glint.")
             .air (0.50f, 0.28f, 0.74f)
             .material (glass, 0.36f, 0.72f, 0.30f, 0.56f)
             .macros (0.56f, 0.46f, 0.62f, 0.12f)
             .motion (0.26f, 0.42f)
             .release (0.10f)
             .fx (1.45f, 0.36f));

    add (Builder ("Solace", cat, "bowed, wood, major, pure",
                  "A pure bowed wooden body with a plain major triad in its nodes. Calm and still.")
             .bow (0.40f, 0.44f, 0.42f)
             .material (wood, 0.58f, 0.42f, 0.36f, 0.40f)
             .macros (0.56f, 0.30f, 0.50f, 0.02f)
             .chord (kMajor3 * 2.0f, kFifth * 2.0f, 4.0f, 5.0f)
             .release (0.12f)
             .fx (1.15f, 0.36f));

    add (Builder ("Monsoon", cat, "air, membrane, rain, turbulent",
                  "Heavy turbulence drumming on a membrane network: the hiss and hum of warm rain.")
             .air (0.62f, 0.86f, 0.40f)
             .material (membrane, 0.50f, 0.46f, 0.36f, 0.50f)
             .macros (0.58f, 0.52f, 0.50f, 0.28f)
             .topology (web)
             .synced (bar1, 0.20f)
             .gesture (1, wanderGesture (2.0f, 4.0f, 0.6f, 0.03f, 41u))
             .release (0.12f)
             .fx (1.5f, 0.46f));

    add (Builder ("Starlit Canopy", cat, "bowed, glass, septimal, high",
                  "High, taut bowed glass on the 3rd, 5th and 7th harmonics: a canopy of pure intervals.")
             .bow (0.30f, 0.42f, 0.34f)
             .material (glass, 0.38f, 0.62f, 0.28f, 0.48f)
             .macros (0.56f, 0.44f, 0.70f, 0.04f)
             .chord (3.0f, 5.0f, 7.0f, 9.0f)
             .gesture (2, orbitGesture (21.0f, 0.0f, 1.0f, 0.004f))
             .release (0.10f)
             .fx (1.45f, 0.44f));

    add (Builder ("Quartz Choir", cat, "air, glass, quartal, chain",
                  "Stacked fourths in blown glass, passed along a chain: modern, open, unresolved.")
             .air (0.54f, 0.30f, 0.58f)
             .material (glass, 0.44f, 0.56f, 0.34f, 0.46f)
             .macros (0.56f, 0.50f, 0.54f, 0.06f)
             .topology (chain)
             .chord (kFourth * 2.0f, 16.0f / 9.0f * 2.0f, 64.0f / 27.0f * 2.0f, 6.0f)
             .motion (0.16f, 0.05f)
             .release (0.10f)
             .fx (1.4f, 0.40f));

    add (Builder ("Ember Choir", cat, "air, wood, minor 7, warm",
                  "A warm choir of wooden pipes breathing a minor seventh chord.")
             .air (0.52f, 0.36f, 0.40f)
             .material (wood, 0.58f, 0.40f, 0.36f, 0.44f)
             .macros (0.56f, 0.40f, 0.48f, 0.06f)
             .chord (kMinor3 * 2.0f, kFifth * 2.0f, kMinor7 * 2.0f, 4.0f)
             .motion (0.16f, 0.06f)
             .release (0.12f)
             .fx (1.3f, 0.40f));

    add (Builder ("Levitation", cat, "bowed, metal, lydian, figure eight",
                  "Bowed metal in the lydian mode; two nodes float on figure-eight paths over four bars.")
             .bow (0.34f, 0.40f, 0.44f)
             .material (metal, 0.50f, 0.52f, 0.28f, 0.48f)
             .macros (0.56f, 0.48f, 0.56f, 0.06f)
             .chord (kMajor2 * 2.0f, kTritone * 2.0f, kMajor7, 3.0f)
             .synced (bar4, 0.0f)
             .gesture (1, figureGesture (8.0f, 16.0f, 0.7f, 0.03f, 1, 2))
             .gesture (2, figureGesture (8.0f, 16.0f, -0.7f, 0.03f, 1, 2))
             .release (0.10f)
             .fx (1.4f, 0.42f));

    add (Builder ("Hologram", cat, "bowed, glass, quantised, shimmer",
                  "Semitone-quantised bowed glass with chaos flickering between neighbouring notes.")
             .bow (0.32f, 0.44f, 0.38f)
             .material (glass, 0.42f, 0.60f, 0.30f, 0.50f)
             .macros (0.56f, 0.52f, 0.56f, 0.30f)
             .quantise()
             .nodes ("radius", 0.625f, 0.458f, 0.708f, 0.375f)
             .motion (0.30f, 0.24f)
             .release (0.10f)
             .fx (1.45f, 0.40f));

    add (Builder ("Snowfield", cat, "air, membrane, quiet, vast",
                  "Almost silent air over a soft membrane in a vast, wide space.")
             .air (0.40f, 0.62f, 0.32f)
             .material (membrane, 0.56f, 0.36f, 0.28f, 0.42f)
             .macros (0.52f, 0.34f, 0.48f, 0.08f)
             .motion (0.18f, 0.04f)
             .release (0.14f)
             .fx (1.5f, 0.70f));

    add (Builder ("Undertow", cat, "bowed, metal, low, pulling",
                  "A low bowed alloy with node A pulled slowly back and forth under the surface.")
             .bow (0.52f, 0.36f, 0.58f)
             .material (metal, 0.72f, 0.32f, 0.30f, 0.54f)
             .macros (0.56f, 0.52f, 0.40f, 0.10f)
             .topology (star)
             .gesture (0, swayGesture (9.0f, 0.0f, 1.1f, 0.04f))
             .release (0.08f)
             .fx (1.25f, 0.36f, 0.08f));

    add (Builder ("Solar Wind", cat, "air, metal, bright, orbit",
                  "Bright, fast air streaming through a metal network; one node orbits every second.")
             .air (0.64f, 0.48f, 0.66f)
             .material (metal, 0.44f, 0.62f, 0.34f, 0.52f)
             .macros (0.56f, 0.56f, 0.58f, 0.14f)
             .topology (web)
             .gesture (2, orbitGesture (1.1f, 0.0f, 1.0f, 0.01f))
             .release (0.10f)
             .fx (1.45f, 0.40f));

    add (Builder ("Moss Garden", cat, "bowed, wood, earthy, organic",
                  "Bowed wood grown over with a low, close cluster: earthy, muffled and always moving.")
             .bow (0.46f, 0.36f, 0.64f)
             .material (wood, 0.76f, 0.24f, 0.44f, 0.56f)
             .macros (0.56f, 0.50f, 0.42f, 0.34f)
             .topology (star)
             .chord (kMajor2 * 2.0f, kMinor3 * 2.0f, 2.8f, 7.0f)
             .levels (0.86f, 0.82f, 0.60f, 0.40f)
             .motion (0.24f, 0.09f)
             .release (0.12f)
             .fx (1.45f, 0.34f));

    add (Builder ("Violet Hour", cat, "bowed, membrane, lydian, dusk",
                  "Bowed membrane at dusk: a major seventh with a raised fourth hanging in the nodes.")
             .bow (0.34f, 0.40f, 0.40f)
             .material (membrane, 0.52f, 0.46f, 0.30f, 0.46f)
             .macros (0.56f, 0.44f, 0.60f, 0.06f)
             .chord (kMajor3 * 2.0f, kTritone * 2.0f, kMajor7 * 2.0f, 3.0f)
             .motion (0.16f, 0.05f)
             .release (0.12f)
             .fx (1.4f, 0.46f));

    add (Builder ("Sea Glass", cat, "air, glass, slow orbit, synced",
                  "Worn, soft glass tones rolled by a sixteen-bar orbit.")
             .air (0.46f, 0.40f, 0.46f)
             .material (glass, 0.52f, 0.44f, 0.36f, 0.46f)
             .macros (0.56f, 0.46f, 0.48f, 0.12f)
             .synced (bar8, 0.14f)
             .gesture (1, orbitGesture (32.0f, 32.0f, 1.0f, 0.01f))
             .release (0.12f)
             .fx (1.35f, 0.46f));

    add (Builder ("Distant Engine", cat, "bowed, metal, chain, machine",
                  "A far-off machine hum: bowed metal chained node to node, pulsing every half bar.")
             .bow (0.50f, 0.40f, 0.60f)
             .material (metal, 0.62f, 0.44f, 0.34f, 0.56f)
             .macros (0.56f, 0.50f, 0.44f, 0.12f)
             .topology (chain)
             .chord (kFifth, 2.0f, 8.0f / 3.0f, 4.0f)
             .synced (half, 0.10f)
             .gesture (3, breatheGesture (1.0f, 2.0f, 0.05f, 1))
             .release (0.10f)
             .fx (1.2f, 0.40f, 0.26f));
}

} // namespace arc::presets::detail

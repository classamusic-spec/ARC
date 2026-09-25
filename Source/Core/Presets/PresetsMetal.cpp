// Factory library: METAL. Steel, iron, copper, tin and titanium: springs, rails, sheets,
// forks, cones, plates and found objects. Metal partials start close to the CORE
// (1.19 / 1.5 / 2.0 / 2.67), so harmonic chords (2, 3, 4, 5) need TENSION near 0.7 to fit
// inside the nodes' reach. See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addMetal (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "METAL";

    add (Builder ("Spring Coil", cat, "spring, boing, dispersive, inharmonic",
                  "A long steel spring plucked at one end: a dispersive, boinging twang that rattles down the coil.")
             .pluck (0.10f, 0.12f, 0.70f)
             .material (metal, 0.36f, 0.62f, 0.16f, 0.86f)
             .macros (0.62f, 0.60f, 0.40f, 0.20f)
             .topology (chain)
             .motion (0.20f, 0.8f)
             .fx (1.3f, 0.28f));

    add (Builder ("Rusted Pipes", cat, "pipes, rusty, dull, found, inharmonic",
                  "Scaffold pipes gone to rust: a dull, clanky ring that dies quickly in the corrosion.")
             .strike (0.60f, 0.26f, 0.40f)
             .material (metal, 0.62f, 0.30f, 0.56f, 0.64f)
             .macros (0.62f, 0.44f, 0.46f, 0.16f)
             .topology (star)
             .fx (1.2f, 0.34f, 0.12f));

    add (Builder ("Chainmail", cat, "chain, rattle, chaos, inharmonic, short",
                  "A fist of chainmail dropped on stone: hundreds of rings rattling against each other.")
             .strike (0.82f, 0.08f, 0.74f)
             .material (metal, 0.24f, 0.70f, 0.62f, 0.74f)
             .macros (0.62f, 0.84f, 0.62f, 0.56f)
             .topology (web)
             .motion (0.50f, 3.0f)
             .fx (1.3f, 0.20f));

    add (Builder ("Titanium", cat, "titanium, pure, bright, light",
                  "A light, stiff titanium bar: a clean, bright ping with a long, pure ring.")
             .strike (0.70f, 0.12f, 0.70f)
             .material (metal, 0.28f, 0.72f, 0.14f, 0.40f)
             .macros (0.62f, 0.26f, 0.80f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .fx (1.3f, 0.36f));

    add (Builder ("Quicksilver", cat, "liquid, mercury, legato, moving",
                  "Liquid metal under the bow: the partials slide and pool around each held note.")
             .bow (0.36f, 0.50f, 0.42f)
             .material (metal, 0.44f, 0.58f, 0.26f, 0.50f)
             .macros (0.58f, 0.44f, 0.56f, 0.08f)
             .voice (VoiceMode::legato, 0.20f)
             .gesture (0, wanderGesture (4.0f, 0.0f, 0.8f, 0.04f, seedFor ("Quicksilver A")))
             .gesture (1, wanderGesture (5.5f, 0.0f, 0.8f, 0.04f, seedFor ("Quicksilver B")))
             .fx (1.4f, 0.40f));

    add (Builder ("Railroad", cat, "rail, hammer, clang, long, inharmonic",
                  "A hammer on a steel rail: a hard clang and a long singing ring running away down the line.")
             .strike (0.88f, 0.10f, 0.60f)
             .material (metal, 0.76f, 0.56f, 0.10f, 0.68f)
             .macros (0.64f, 0.50f, 0.56f, 0.10f)
             .topology (chain)
             .fx (1.2f, 0.50f));

    add (Builder ("Wire Choir", cat, "wires, bowed, choir, chord",
                  "Taut steel wires bowed together on a major chord: a thin, silvery choir.")
             .bow (0.40f, 0.46f, 0.46f)
             .material (metal, 0.48f, 0.52f, 0.26f, 0.44f)
             .macros (0.58f, 0.56f, 0.70f, 0.06f)
             .topology (web)
             .chord (1.5f, 2.0f, 2.5f, 3.0f)
             .fx (1.45f, 0.46f));

    add (Builder ("Tin Tines", cat, "tin, tines, buzzy, small",
                  "Tin tines riveted to a cigar box: small, bright, and buzzing against the lid.")
             .pluck (0.34f, 0.22f, 0.64f)
             .material (metal, 0.30f, 0.62f, 0.44f, 0.56f)
             .macros (0.62f, 0.34f, 0.56f, 0.06f)
             .fx (1.1f, 0.20f, 0.26f));

    add (Builder ("Thunder Sheet", cat, "thunder, sheet, rumble, chaos, inharmonic",
                  "A theatre thunder sheet shaken hard: a rumbling, roaring wobble of thin steel.")
             .strike (0.54f, 0.50f, 0.30f)
             .material (metal, 0.30f, 0.40f, 0.20f, 0.80f)
             .macros (0.66f, 0.72f, 0.28f, 0.50f)
             .topology (web)
             .motion (0.56f, 1.2f)
             .fx (1.5f, 0.56f, 0.18f));

    add (Builder ("Forge Glow", cat, "warm, dark, bowed, glowing, driven",
                  "Iron still glowing from the forge, bowed slowly: a dark, warm, saturated hum.")
             .bow (0.62f, 0.34f, 0.60f)
             .material (metal, 0.72f, 0.36f, 0.30f, 0.50f)
             .macros (0.60f, 0.48f, 0.44f, 0.10f)
             .topology (ring)
             .chord (1.5f, 2.0f, 3.0f, 4.0f)
             .fx (1.2f, 0.34f, 0.34f));

    add (Builder ("Foundry", cat, "heavy, iron, industrial, driven, inharmonic",
                  "Heavy castings struck in a foundry hall: dark, driven, clanging iron.")
             .strike (0.76f, 0.30f, 0.44f)
             .material (metal, 0.86f, 0.40f, 0.34f, 0.62f)
             .macros (0.66f, 0.66f, 0.36f, 0.24f)
             .topology (web)
             .fx (1.3f, 0.60f, 0.44f));

    add (Builder ("Hollow Steel", cat, "tube, hollow, nasal, plucked",
                  "A plucked steel tube: hollow and nasal, singing on the odd half-harmonics.")
             .pluck (0.26f, 0.30f, 0.50f)
             .material (metal, 0.56f, 0.48f, 0.34f, 0.46f)
             .macros (0.62f, 0.40f, 0.64f, 0.04f)
             .chord (1.5f, 2.5f, 3.5f, 4.5f)
             .fx (1.2f, 0.26f));

    add (Builder ("Bell Plate", cat, "bell plate, orchestral, dark, long",
                  "An orchestral bell plate: a slab of steel that tolls darker and longer than any tubular bell.")
             .strike (0.56f, 0.40f, 0.36f)
             .material (metal, 0.80f, 0.36f, 0.12f, 0.60f)
             .macros (0.64f, 0.46f, 0.40f, 0.06f)
             .topology (star)
             .fx (1.3f, 0.50f));

    add (Builder ("Copper Coil", cat, "copper, coil, warm, beating, detuned",
                  "A coil of copper wire plucked: warm, and every partial split into a slowly beating pair.")
             .pluck (0.18f, 0.20f, 0.52f)
             .material (metal, 0.50f, 0.44f, 0.22f, 0.70f)
             .macros (0.62f, 0.50f, 0.46f, 0.10f)
             .topology (chain)
             .chord (1.19f * 1.01f, 1.19f * 0.99f, 2.0f * 1.008f, 2.0f * 0.992f)
             .fx (1.35f, 0.30f));

    add (Builder ("Tuning Fork", cat, "tuning fork, sine, pure, reference",
                  "A struck tuning fork: an almost pure sine, with a faint high clang that dies at once.")
             .strike (0.30f, 0.30f, 0.40f)
             .material (metal, 0.40f, 0.44f, 0.10f, 0.40f)
             .macros (0.58f, 0.18f, 0.80f, 0.0f)
             .tune (3, 6.25f)
             .levels (0.06f, 0.06f, 0.06f, 0.30f)
             .decays (0.5f, 0.5f, 0.5f, 0.10f)
             .fx (1.0f, 0.24f));

    add (Builder ("Magnet Hum", cat, "ebow, sustain, pure, string",
                  "A magnetic driver hovering over a steel string: an endless, pure, bowless sustain.")
             .air (0.66f, 0.04f, 0.40f)
             .material (metal, 0.50f, 0.50f, 0.24f, 0.44f)
             .macros (0.58f, 0.30f, 0.60f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.30f, 0.24f, 0.18f, 0.12f)
             .fx (1.2f, 0.36f));

    add (Builder ("Tuned Rails", cat, "rails, whole tone, hard mallets, bright",
                  "Steel rails cut to a whole-tone scale and struck with hard mallets: bright and dreamlike.")
             .strike (0.74f, 0.18f, 0.62f)
             .material (metal, 0.58f, 0.56f, 0.24f, 0.44f)
             .macros (0.62f, 0.42f, 0.50f, 0.04f)
             .chord (2.25f, 2.52f, 2.83f, 3.17f)
             .fx (1.3f, 0.36f));

    add (Builder ("Chrome Keys", cat, "keys, fm, bright, digital",
                  "Hard, bright, chrome-plated keys: a bell-like bite over a short, glassy body, like classic digital keys.")
             .strike (0.84f, 0.10f, 0.76f)
             .material (metal, 0.40f, 0.66f, 0.40f, 0.44f)
             .macros (0.64f, 0.34f, 0.72f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .decays (0.50f, 0.40f, 0.30f, 0.20f)
             .release (0.50f)
             .fx (1.2f, 0.22f, 0.10f));

    add (Builder ("Razor Wire", cat, "wire, cutting, bright, driven",
                  "A taut, thin wire snapped hard: a cutting, buzzing twang.")
             .pluck (0.06f, 0.08f, 0.92f)
             .material (metal, 0.16f, 0.82f, 0.30f, 0.60f)
             .macros (0.66f, 0.46f, 0.86f, 0.20f)
             .fx (1.1f, 0.18f, 0.42f));

    add (Builder ("Underwater Bell", cat, "bell, submerged, dull, wobbling",
                  "A bell rung underwater: dull, slow, its partials wobbling as the water moves.")
             .strike (0.40f, 0.42f, 0.22f)
             .material (metal, 0.66f, 0.18f, 0.26f, 0.56f)
             .macros (0.60f, 0.46f, 0.46f, 0.06f)
             .gesture (0, figureGesture (3.2f, 0.0f, 0.2f, 0.03f, 1, 1))
             .gesture (1, figureGesture (4.1f, 0.0f, 0.2f, 0.03f, 1, 1))
             .gesture (2, figureGesture (5.3f, 0.0f, 0.2f, 0.03f, 1, 1))
             .fx (1.3f, 0.66f));

    add (Builder ("Steel Wool", cat, "noise, hiss, scrubbed, metallic",
                  "Noise scrubbed through a dense metal web: a fine, metallic hiss with a ghost of pitch.")
             .air (0.24f, 0.94f, 0.80f)
             .material (metal, 0.30f, 0.66f, 0.36f, 0.80f)
             .macros (0.66f, 0.80f, 0.64f, 0.40f)
             .topology (web)
             .fx (1.5f, 0.30f));

    add (Builder ("Oil Drum", cat, "drum, oil barrel, boomy, dark",
                  "An empty oil drum hit with a fist: a dark, boomy, hollow metal thud.")
             .strike (0.44f, 0.46f, 0.30f)
             .material (metal, 0.88f, 0.32f, 0.40f, 0.56f)
             .macros (0.66f, 0.40f, 0.32f, 0.08f)
             .topology (star)
             .fx (1.1f, 0.28f));

    add (Builder ("Brake Drum", cat, "brake drum, found, clangy, inharmonic",
                  "A car brake drum struck with a steel beater: a harsh, clangy bell.")
             .strike (0.90f, 0.06f, 0.70f)
             .material (metal, 0.60f, 0.62f, 0.46f, 0.72f)
             .macros (0.64f, 0.38f, 0.60f, 0.10f)
             .topology (star)
             .fx (1.1f, 0.20f));

    add (Builder ("Pedal Steel", cat, "pedal steel, glide, legato, singing",
                  "A pedal steel: a plucked steel string that slides slowly up to each new note while it rings.")
             .pluck (0.24f, 0.16f, 0.56f)
             .material (metal, 0.48f, 0.54f, 0.22f, 0.42f)
             .macros (0.62f, 0.36f, 0.68f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .voice (VoiceMode::legato, 0.45f)
             .motion (0.08f, 0.30f)
             .fx (1.25f, 0.40f));

    add (Builder ("Resonator Cone", cat, "resonator, dobro, nasal, loud",
                  "A string over a spun aluminium cone: the loud, nasal bark of a resonator guitar.")
             .pluck (0.16f, 0.14f, 0.70f)
             .material (metal, 0.42f, 0.64f, 0.34f, 0.50f)
             .macros (0.64f, 0.52f, 0.56f, 0.06f)
             .topology (star)
             .links (0.9f, 0.9f, 0.5f, 0.5f)
             .chord (1.5f, 2.0f, 3.0f, 4.0f)
             .fx (1.1f, 0.22f, 0.16f));

    add (Builder ("Waterphone", cat, "waterphone, eerie, bending, inharmonic, horror",
                  "Bowed bronze rods over a water-filled bowl: eerie, bending tones straight from a horror score.")
             .bow (0.50f, 0.30f, 0.60f)
             .material (metal, 0.46f, 0.54f, 0.20f, 0.78f)
             .macros (0.60f, 0.56f, 0.52f, 0.20f)
             .topology (web)
             .gesture (0, wanderGesture (3.5f, 0.0f, 0.6f, 0.06f, seedFor ("Waterphone A")))
             .gesture (1, wanderGesture (4.5f, 0.0f, 0.6f, 0.06f, seedFor ("Waterphone B")))
             .gesture (2, wanderGesture (6.0f, 0.0f, 0.6f, 0.06f, seedFor ("Waterphone C")))
             .fx (1.5f, 0.66f));

    add (Builder ("Scrapyard", cat, "junk, found, clatter, chaos, inharmonic",
                  "A scrapyard in one hit: mismatched panels, springs and pipes clattering together.")
             .strike (0.80f, 0.20f, 0.56f)
             .material (metal, 0.54f, 0.50f, 0.44f, 0.84f)
             .macros (0.66f, 0.76f, 0.44f, 0.46f)
             .topology (web)
             .motion (0.40f, 0.9f)
             .fx (1.35f, 0.34f, 0.30f));

    add (Builder ("Reverb Plate", cat, "plate, dense, splashy, bright, inharmonic",
                  "The steel plate of a vintage reverb unit, tapped directly: a dense, bright, splashy bloom.")
             .strike (0.36f, 0.20f, 0.66f)
             .material (metal, 0.34f, 0.70f, 0.10f, 0.66f)
             .macros (0.62f, 0.90f, 0.62f, 0.10f)
             .topology (web)
             .links (0.95f, 0.95f, 0.95f, 0.95f)
             .fx (1.5f, 0.30f));
}

} // namespace arc::presets::detail

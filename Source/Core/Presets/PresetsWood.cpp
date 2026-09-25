// Factory library: WOOD. Bars, boxes, logs, soundboards, bamboo, balsa and oak: struck,
// plucked, bowed and blown. Wood partials follow a free bar (2.76 / 5.40 / 8.93 / 13.34 x
// CORE); the tuned presets retune them the way a marimba maker does (1 : 4, 1 : 3, 1 : 4 : 10).
// See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addWood (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "WOOD";

    add (Builder ("Balafon", cat, "balafon, gourd, buzzing, african",
                  "Rosewood keys over gourds fitted with buzzing membranes: a woody knock wrapped in a papery buzz.")
             .strike (0.62f, 0.20f, 0.56f)
             .material (wood, 0.50f, 0.52f, 0.46f, 0.46f)
             .macros (0.64f, 0.44f, 0.50f, 0.22f)
             .tune (0, 4.0f)
             .fx (1.2f, 0.22f, 0.30f));

    add (Builder ("Mbira Calabash", cat, "mbira, calabash, buzzing, thumb piano",
                  "A Shona mbira inside a calabash, bottle caps buzzing on the rim: a warm, rattling thumb piano.")
             .pluck (0.36f, 0.24f, 0.56f)
             .material (wood, 0.52f, 0.50f, 0.40f, 0.50f)
             .macros (0.64f, 0.46f, 0.50f, 0.26f)
             .topology (star)
             .tune (1, 6.27f)
             .fx (1.2f, 0.34f, 0.28f));

    add (Builder ("Bamboo Chimes", cat, "bamboo, chimes, hollow, clunky, inharmonic",
                  "Bamboo wind chimes knocking together: hollow, clunky and gently out of step.")
             .strike (0.50f, 0.20f, 0.48f)
             .material (wood, 0.34f, 0.50f, 0.50f, 0.70f)
             .macros (0.62f, 0.60f, 0.56f, 0.30f)
             .topology (web)
             .gesture (0, wanderGesture (7.0f, 0.0f, 0.8f, 0.03f, seedFor ("Bamboo Chimes A")))
             .gesture (2, wanderGesture (9.0f, 0.0f, 0.8f, 0.03f, seedFor ("Bamboo Chimes C")))
             .fx (1.5f, 0.48f));

    add (Builder ("Xylophone", cat, "xylophone, hard mallets, bright, orchestral",
                  "Hard mallets on rosewood bars tuned to the twelfth: bright, dry and precise.")
             .strike (0.86f, 0.08f, 0.74f)
             .material (wood, 0.36f, 0.68f, 0.48f, 0.44f)
             .macros (0.64f, 0.28f, 0.56f, 0.02f)
             .tune (0, 3.0f)
             .levels (0.70f, 0.20f, 0.20f, 0.20f)
             .fx (1.15f, 0.22f));

    add (Builder ("Guitar Body Tap", cat, "guitar body, knock, percussive, woody",
                  "A knuckle on the top of an acoustic guitar: the body's air and plate resonances answering the knock.")
             .strike (0.54f, 0.34f, 0.38f)
             .material (wood, 0.62f, 0.40f, 0.56f, 0.56f)
             .macros (0.66f, 0.56f, 0.44f, 0.06f)
             .topology (star)
             .fx (1.1f, 0.14f));

    add (Builder ("Cigar Box Slide", cat, "slide, cigar box, blues, gritty",
                  "A three-string cigar-box guitar played with a bottleneck: raw, boxy and sliding into every note.")
             .pluck (0.20f, 0.18f, 0.62f)
             .material (wood, 0.44f, 0.56f, 0.40f, 0.50f)
             .macros (0.64f, 0.46f, 0.52f, 0.08f)
             .topology (star)
             .chord (2.0f, 3.0f, 8.0f, 12.0f)
             .voice (VoiceMode::legato, 0.18f)
             .fx (1.0f, 0.20f, 0.36f));

    add (Builder ("Clavichord", cat, "clavichord, early music, intimate, vibrato",
                  "A brass tangent pressing a string: thin, intimate and quiet, with a trembling Bebung under the key.")
             .pluck (0.12f, 0.30f, 0.70f)
             .material (wood, 0.40f, 0.62f, 0.44f, 0.44f)
             .macros (0.70f, 0.30f, 0.56f, 0.0f)
             .chord (2.0f, 3.0f, 8.0f, 12.0f)
             .gesture (0, breatheGesture (0.2f, 0.0f, 0.010f, 1))
             .release (0.60f)
             .fx (0.9f, 0.16f));

    add (Builder ("Treehouse", cat, "planks, hollow, structure, warm",
                  "A plank floor knocked in a treehouse: a hollow, woody thump and the whole structure ringing along.")
             .strike (0.42f, 0.40f, 0.36f)
             .material (wood, 0.68f, 0.40f, 0.42f, 0.56f)
             .macros (0.64f, 0.70f, 0.40f, 0.10f)
             .topology (web)
             .fx (1.3f, 0.34f));

    add (Builder ("Driftwood", cat, "weathered, soft, grey, spacious",
                  "Sea-worn driftwood tapped on the shore: soft and grey, with the open air around it.")
             .strike (0.36f, 0.50f, 0.30f)
             .material (wood, 0.56f, 0.30f, 0.62f, 0.60f)
             .macros (0.62f, 0.40f, 0.44f, 0.08f)
             .fx (1.4f, 0.70f));

    add (Builder ("Woodpecker", cat, "knock, sharp, hollow trunk, short",
                  "A hard little beak on a hollow trunk: short, sharp knocks and a hollow answer from the tree.")
             .strike (0.96f, 0.04f, 0.70f)
             .material (wood, 0.40f, 0.60f, 0.64f, 0.52f)
             .macros (0.66f, 0.50f, 0.50f, 0.06f)
             .topology (star)
             .fx (1.2f, 0.36f));

    add (Builder ("Pine Needles", cat, "tiny, bright, pizzicato, short",
                  "Tiny, bright plucks like pine needles snapping: a dry, glittering pizzicato.")
             .pluck (0.22f, 0.30f, 0.72f)
             .material (wood, 0.26f, 0.70f, 0.44f, 0.48f)
             .macros (0.62f, 0.34f, 0.66f, 0.06f)
             .fx (1.35f, 0.34f));

    add (Builder ("Mokugyo", cat, "temple, wooden fish, round, dark",
                  "A temple wooden fish struck with a padded stick: round, dark and hollow, for keeping time in a chant.")
             .strike (0.50f, 0.34f, 0.32f)
             .material (wood, 0.72f, 0.36f, 0.50f, 0.50f)
             .macros (0.64f, 0.34f, 0.42f, 0.02f)
             .topology (star)
             .tune (0, 2.0f)
             .fx (1.2f, 0.40f));

    add (Builder ("Marimba Bass", cat, "marimba, low, soft mallets, resonators",
                  "The lowest bars of a concert marimba over long resonators: deep, soft and blooming.")
             .strike (0.30f, 0.56f, 0.34f)
             .material (wood, 0.76f, 0.40f, 0.34f, 0.44f)
             .macros (0.64f, 0.30f, 0.48f, 0.02f)
             .tune (0, 4.0f)
             .tune (1, 10.0f)
             .levels (0.50f, 0.30f, 0.20f, 0.20f)
             .fx (1.2f, 0.26f));

    add (Builder ("Hollow Log", cat, "slit drum, log, two tongues, hollow",
                  "A hollowed log with two tongues cut a fifth apart: strike one and the other hums.")
             .strike (0.58f, 0.26f, 0.40f)
             .material (wood, 0.66f, 0.40f, 0.48f, 0.54f)
             .macros (0.64f, 0.52f, 0.46f, 0.06f)
             .topology (star)
             .tune (0, 1.5f)
             .levels (0.80f, 0.30f, 0.20f, 0.20f)
             .fx (1.2f, 0.30f));

    add (Builder ("Spruce Harp", cat, "harp, spruce, warm, resonant",
                  "A concert harp with a spruce soundboard: warm gut strings and a big, woody bloom.")
             .pluck (0.28f, 0.10f, 0.52f)
             .material (wood, 0.50f, 0.52f, 0.28f, 0.44f)
             .macros (0.62f, 0.44f, 0.52f, 0.02f)
             .chord (2.0f, 3.0f, 5.0f, 8.0f)
             .fx (1.35f, 0.44f));

    add (Builder ("Bamboo Pipes", cat, "bamboo, tuned, hollow, round",
                  "Tuned bamboo tubes struck with a rubber paddle: hollow, round, and ringing in a major chord.")
             .strike (0.34f, 0.40f, 0.44f)
             .material (wood, 0.46f, 0.48f, 0.38f, 0.44f)
             .macros (0.62f, 0.44f, 0.50f, 0.04f)
             .chord (2.0f, 5.0f, 6.0f, 8.0f)
             .fx (1.3f, 0.32f));

    add (Builder ("Wooden Toy", cat, "toy, cute, bright, detuned",
                  "A child's wooden xylophone, a little out of tune: bright, cute and wonky.")
             .strike (0.70f, 0.14f, 0.64f)
             .material (wood, 0.30f, 0.62f, 0.50f, 0.46f)
             .macros (0.62f, 0.30f, 0.56f, 0.04f)
             .chord (2.0f * 1.03f, 3.0f * 0.97f, 8.0f * 1.02f, 12.0f)
             .fx (1.1f, 0.20f));

    add (Builder ("Forest Floor", cat, "twigs, crackle, chaos, inharmonic, texture",
                  "Twigs and bark underfoot: a crackling, splintering knock of dry wood.")
             .strike (0.74f, 0.10f, 0.50f)
             .material (wood, 0.30f, 0.46f, 0.66f, 0.78f)
             .macros (0.64f, 0.72f, 0.44f, 0.60f)
             .topology (web)
             .motion (0.50f, 2.2f)
             .fx (1.4f, 0.40f));

    add (Builder ("Luthier's Bench", cat, "tap tones, plate, luthier, inharmonic",
                  "A luthier tapping a violin plate to hear its modes: bright, woody tap tones that ring briefly.")
             .strike (0.46f, 0.26f, 0.52f)
             .material (wood, 0.44f, 0.60f, 0.30f, 0.62f)
             .macros (0.62f, 0.40f, 0.56f, 0.04f)
             .topology (star)
             .chord (2.7f, 5.2f, 9.8f, 14.0f)
             .fx (1.1f, 0.18f));

    add (Builder ("Phase Shift", cat, "phasing, minimalist, sequenced, bowed",
                  "Two nodes play the same six-note pattern, one a hair slower than the other: the patterns drift apart and home again.")
             .bow (0.46f, 0.48f, 0.52f)
             .material (wood, 0.52f, 0.50f, 0.36f, 0.44f)
             .macros (0.60f, 0.50f, 0.52f, 0.02f)
             .quantise()
             .chord (2.0f, 3.0f, 8.0f, 12.0f)
             .gesture (0, stepGesture (3.0f, 0.0f, { 0.0f, 4 * kSemitone, 7 * kSemitone, 4 * kSemitone, 12 * kSemitone, 7 * kSemitone }, 0.1f))
             .gesture (1, stepGesture (3.1f, 0.0f, { 0.0f, 4 * kSemitone, 7 * kSemitone, 4 * kSemitone, 12 * kSemitone, 7 * kSemitone }, 0.1f))
             .fx (1.35f, 0.30f));

    add (Builder ("Porch Stomp", cat, "stomp, low, thump, boxy",
                  "A boot stomping a wooden porch: a low, boxy thump that keeps the time.")
             .strike (0.40f, 0.60f, 0.20f)
             .material (wood, 0.90f, 0.30f, 0.66f, 0.50f)
             .macros (0.70f, 0.40f, 0.36f, 0.04f)
             .topology (star)
             .fx (1.0f, 0.16f, 0.20f));

    add (Builder ("Kora Bridge", cat, "kora, harp-lute, bright, cascading",
                  "A West African kora: bright strings on a tall bridge over a calabash, ringing into each other.")
             .pluck (0.18f, 0.12f, 0.66f)
             .material (wood, 0.44f, 0.58f, 0.30f, 0.46f)
             .macros (0.62f, 0.52f, 0.52f, 0.04f)
             .topology (ring)
             .chord (2.0f, 3.0f, 5.0f, 12.0f)
             .fx (1.4f, 0.40f));

    add (Builder ("Sawmill", cat, "saw, screaming, harsh, chaos, inharmonic",
                  "A plank screaming through the saw: heavy pressure, fast bow and splintering, harsh overtones.")
             .bow (0.92f, 0.62f, 0.86f)
             .material (wood, 0.50f, 0.66f, 0.44f, 0.72f)
             .macros (0.64f, 0.56f, 0.52f, 0.34f)
             .topology (web)
             .fx (1.2f, 0.26f, 0.36f));

    add (Builder ("Harpsichord Quill", cat, "harpsichord, baroque, bright, jangly",
                  "A quill plucking brass strings with the 4' stop drawn: bright, jangly and baroque.")
             .pluck (0.06f, 0.20f, 0.84f)
             .material (wood, 0.40f, 0.70f, 0.34f, 0.44f)
             .macros (0.62f, 0.44f, 0.56f, 0.02f)
             .chord (2.0f, 3.0f, 8.0f, 12.0f)
             .levels (0.70f, 0.40f, 0.30f, 0.20f)
             .release (0.60f)
             .fx (1.1f, 0.30f));

    add (Builder ("Balsa Plucks", cat, "balsa, light, papery, short",
                  "Plucks on featherweight balsa: papery, soft and gone almost at once.")
             .pluck (0.40f, 0.26f, 0.56f)
             .material (wood, 0.26f, 0.52f, 0.46f, 0.48f)
             .macros (0.62f, 0.30f, 0.52f, 0.04f)
             .fx (1.2f, 0.26f));

    add (Builder ("Creaking Oak", cat, "creak, slow bow, stick-slip, groaning",
                  "An old oak bending in the wind: a very slow, heavy bow that sticks and slips into a creaking groan.")
             .bow (0.84f, 0.14f, 0.84f)
             .material (wood, 0.80f, 0.40f, 0.40f, 0.60f)
             .macros (0.60f, 0.40f, 0.40f, 0.20f)
             .fx (1.2f, 0.40f));

    add (Builder ("Tonewood", cat, "breath, soundboard, warm, singing",
                  "Breath across a spruce soundboard: the whole plate warms up and sings a major seventh along with the air.")
             .air (0.62f, 0.36f, 0.46f)
             .material (wood, 0.54f, 0.50f, 0.34f, 0.46f)
             .macros (0.60f, 0.54f, 0.52f, 0.06f)
             .topology (web)
             .chord (2.5f, 6.0f, 7.5f, 10.0f)
             .fx (1.3f, 0.40f));

    add (Builder ("Rainstick", cat, "rainstick, pebbles, noise, chaos, texture",
                  "A cactus rainstick turned over: pebbles trickling through wooden thorns, a soft rushing patter.")
             .air (0.40f, 0.96f, 0.64f)
             .material (wood, 0.32f, 0.54f, 0.30f, 0.80f)
             .macros (0.74f, 0.76f, 0.54f, 0.64f)
             .topology (web)
             .levels (0.90f, 0.90f, 0.90f, 0.90f)
             .motion (0.60f, 1.8f)
             .fx (1.5f, 0.44f));
}

} // namespace arc::presets::detail

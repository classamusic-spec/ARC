// Factory library: PERCUSSION. A kit and a toolbox: kick, snare, hats and cymbals, hand
// drums, bells, clicks and found objects. Every hit must fall at least 18 dB within 1.5 s
// while the key is still held (the library audit checks it), so LOSS runs high and the
// noisy ones lean on CHAOS and a web topology rather than on noise. Pitch follows the
// keyboard but is not held to a tolerance here. See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addPercussion (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "PERCUSSION";

    add (Builder ("Kick Skin", cat, "kick, bass drum, punch, low",
                  "A tight kick drum: a hard beater, a thump of skin and a quick drop in pitch.")
             .strike (0.80f, 0.20f, 0.30f)
             .material (membrane, 0.56f, 0.30f, 0.70f, 0.50f)
             .macros (0.90f, 0.20f, 0.20f, 0.02f)
             .topology (star)
             .levels (0.30f, 0.30f, 0.30f, 0.30f)
             .fx (1.0f, 0.10f, 0.20f));

    add (Builder ("Snare Crack", cat, "snare, crack, noisy, chaos",
                  "A snare drum's crack: a tight skin and a web of chaotic partials standing in for the wires.")
             .strike (0.92f, 0.08f, 0.76f)
             .material (membrane, 0.30f, 0.66f, 0.66f, 0.70f)
             .macros (0.72f, 0.80f, 0.60f, 0.70f)
             .topology (web)
             .fx (1.1f, 0.24f, 0.24f));

    add (Builder ("Closed Hat", cat, "hi-hat, closed, tick, bright",
                  "A closed hi-hat: two tight cymbals, a bright metallic tick and nothing more.")
             .strike (0.96f, 0.03f, 0.92f)
             .material (metal, 0.10f, 0.92f, 0.90f, 0.92f)
             .macros (0.66f, 0.84f, 0.84f, 0.70f)
             .topology (web)
             .decays (0.10f, 0.10f, 0.10f, 0.10f)
             .fx (1.2f, 0.12f));

    add (Builder ("Open Hat", cat, "hi-hat, open, sizzle, bright",
                  "An open hi-hat: a loose, sizzling wash that fades quickly.")
             .strike (0.90f, 0.06f, 0.88f)
             .material (metal, 0.14f, 0.88f, 0.70f, 0.92f)
             .macros (0.66f, 0.84f, 0.80f, 0.70f)
             .topology (web)
             .fx (1.3f, 0.18f));

    add (Builder ("Ride Ping", cat, "ride, ping, cymbal, bell",
                  "A stick tip on a ride cymbal: a clear ping over a soft, short wash.")
             .strike (0.80f, 0.06f, 0.74f)
             .material (metal, 0.40f, 0.72f, 0.70f, 0.70f)
             .macros (0.64f, 0.56f, 0.70f, 0.30f)
             .topology (web)
             .fx (1.3f, 0.22f));

    add (Builder ("Crash Wash", cat, "crash, cymbal, wash, bright",
                  "A crash cymbal: an explosive, bright splash of metal that spreads wide and falls away.")
             .strike (0.86f, 0.14f, 0.80f)
             .material (metal, 0.30f, 0.80f, 0.80f, 0.90f)
             .macros (0.70f, 0.90f, 0.66f, 0.70f)
             .topology (web)
             .decays (0.30f, 0.30f, 0.30f, 0.30f)
             .motion (0.40f, 2.0f)
             .fx (1.5f, 0.34f));

    add (Builder ("Rim Click", cat, "rim, click, cross-stick, dry",
                  "A cross-stick on the rim: a dry, woody click.")
             .strike (0.94f, 0.03f, 0.70f)
             .material (wood, 0.20f, 0.70f, 0.84f, 0.56f)
             .macros (0.66f, 0.30f, 0.66f, 0.06f)
             .topology (star)
             .fx (1.0f, 0.12f));

    add (Builder ("Cowbell", cat, "cowbell, two tones, latin, classic",
                  "A cowbell: two clanging tones a wide fifth apart, short and loud.")
             .strike (0.86f, 0.06f, 0.62f)
             .material (metal, 0.40f, 0.60f, 0.70f, 0.56f)
             .macros (0.66f, 0.36f, 0.56f, 0.04f)
             .topology (star)
             .tune (0, 1.48f)
             .levels (0.90f, 0.10f, 0.10f, 0.10f)
             .fx (1.0f, 0.14f));

    add (Builder ("Clave", cat, "clave, rosewood, click, pure",
                  "Two rosewood claves: a pure, piercing click.")
             .strike (0.90f, 0.04f, 0.66f)
             .material (wood, 0.30f, 0.66f, 0.62f, 0.44f)
             .macros (0.66f, 0.20f, 0.60f, 0.02f)
             .topology (star)
             .fx (1.0f, 0.20f));

    add (Builder ("Floor Tom", cat, "floor tom, kit, low, punchy",
                  "A floor tom hit hard: a low, punchy thud with a short, falling ring.")
             .strike (0.66f, 0.20f, 0.40f)
             .material (membrane, 0.74f, 0.40f, 0.62f, 0.50f)
             .macros (0.74f, 0.30f, 0.40f, 0.04f)
             .fx (1.1f, 0.20f));

    add (Builder ("Bartok Snap", cat, "snap pizzicato, string, slap, short",
                  "A Bartok pizzicato: the string pulled up and snapped back against the fingerboard.")
             .pluck (0.50f, 0.30f, 0.76f)
             .material (wood, 0.40f, 0.62f, 0.62f, 0.50f)
             .macros (0.80f, 0.30f, 0.60f, 0.10f)
             .fx (1.0f, 0.16f, 0.10f));

    add (Builder ("Glass Clink", cat, "glass, clink, toast, bright",
                  "Two glasses touched in a toast: a bright, clear clink.")
             .strike (0.60f, 0.06f, 0.80f)
             .material (glass, 0.22f, 0.80f, 0.60f, 0.50f)
             .macros (0.62f, 0.40f, 0.66f, 0.04f)
             .fx (1.2f, 0.24f));

    add (Builder ("Anvil", cat, "anvil, hammer, clang, hard",
                  "A hammer on an anvil: a hard, dense, ringing clang.")
             .strike (0.98f, 0.04f, 0.72f)
             .material (metal, 0.80f, 0.66f, 0.82f, 0.72f)
             .macros (0.72f, 0.40f, 0.64f, 0.10f)
             .decays (0.30f, 0.30f, 0.30f, 0.30f)
             .topology (star)
             .fx (1.1f, 0.26f, 0.10f));

    add (Builder ("Triangle", cat, "triangle, bright, high, orchestral",
                  "A steel triangle: a bright, high, shimmering ding.")
             .strike (0.74f, 0.05f, 0.84f)
             .material (metal, 0.18f, 0.84f, 0.62f, 0.62f)
             .macros (0.58f, 0.30f, 0.80f, 0.04f)
             .fx (1.3f, 0.30f));

    add (Builder ("Woodblock", cat, "woodblock, hollow, tock, dry",
                  "A hollow woodblock: a dry, hollow tock.")
             .strike (0.86f, 0.06f, 0.56f)
             .material (wood, 0.44f, 0.54f, 0.70f, 0.50f)
             .macros (0.66f, 0.40f, 0.56f, 0.04f)
             .topology (star)
             .tune (0, 2.6f)
             .fx (1.0f, 0.18f));

    add (Builder ("Sub Drop", cat, "sub, 808, boom, low",
                  "A deep sub boom that swoops down and falls away: the low end of an electronic kit.")
             .strike (0.54f, 0.50f, 0.14f)
             .material (membrane, 0.96f, 0.16f, 0.56f, 0.46f)
             .macros (0.92f, 0.20f, 0.16f, 0.02f)
             .topology (star)
             .levels (0.20f, 0.20f, 0.20f, 0.20f)
             .fx (1.0f, 0.10f, 0.30f));

    add (Builder ("Bottle Tap", cat, "bottle, tap, glass, hollow",
                  "A ring tapped on a beer bottle: a hollow, glassy tink.")
             .strike (0.70f, 0.06f, 0.60f)
             .material (glass, 0.52f, 0.52f, 0.60f, 0.46f)
             .macros (0.64f, 0.36f, 0.50f, 0.04f)
             .topology (star)
             .fx (1.1f, 0.18f));

    add (Builder ("Frying Pan", cat, "pan, kitchen, clonk, found",
                  "A cast-iron frying pan hit with a wooden spoon: a thick, clonking bong.")
             .strike (0.84f, 0.08f, 0.56f)
             .material (metal, 0.64f, 0.50f, 0.80f, 0.70f)
             .macros (0.66f, 0.46f, 0.50f, 0.10f)
             .decays (0.30f, 0.30f, 0.30f, 0.30f)
             .topology (star)
             .fx (1.0f, 0.16f));

    add (Builder ("Flower Pot", cat, "terracotta, pot, earthy, dry",
                  "A terracotta flower pot upturned and tapped: an earthy, dry, pitched tock.")
             .strike (0.70f, 0.08f, 0.50f)
             .material (glass, 0.50f, 0.42f, 0.66f, 0.52f)
             .macros (0.64f, 0.30f, 0.48f, 0.04f)
             .topology (star)
             .tune (0, 2.4f)
             .fx (1.1f, 0.20f));

    add (Builder ("Brush Skin", cat, "brushes, snare, swish, soft",
                  "Wire brushes swept across a snare head: a soft, papery swish.")
             .strike (0.24f, 0.90f, 0.70f)
             .material (membrane, 0.40f, 0.60f, 0.70f, 0.66f)
             .macros (0.70f, 0.70f, 0.56f, 0.64f)
             .topology (web)
             .fx (1.2f, 0.20f));

    add (Builder ("Timbale", cat, "timbale, steel shell, bright, latin",
                  "A timbale with a steel shell: a bright, cracking skin and a ringing rim.")
             .strike (0.86f, 0.08f, 0.70f)
             .material (membrane, 0.30f, 0.70f, 0.54f, 0.50f)
             .macros (0.68f, 0.40f, 0.66f, 0.06f)
             .tune (3, 4.2f)
             .levels (0.30f, 0.30f, 0.30f, 0.80f)
             .fx (1.1f, 0.20f));

    add (Builder ("Gong Choke", cat, "gong, choked, short, dark",
                  "A gong struck and immediately grabbed: a dark bloom of metal cut short.")
             .strike (0.70f, 0.18f, 0.50f)
             .material (metal, 0.70f, 0.50f, 0.84f, 0.70f)
             .macros (0.70f, 0.66f, 0.46f, 0.20f)
             .decays (0.30f, 0.30f, 0.30f, 0.30f)
             .topology (web)
             .fx (1.3f, 0.30f));

    add (Builder ("Step Kit", cat, "sequenced, synced, evolving, kit",
                  "A drum whose resonances step in sixteenths and triplets: play the same note twice and it answers differently.")
             .strike (0.74f, 0.10f, 0.56f)
             .material (membrane, 0.42f, 0.54f, 0.64f, 0.56f)
             .macros (0.70f, 0.60f, 0.50f, 0.20f)
             .topology (ring)
             .quantise()
             .synced (quarter, 0.0f)
             .gesture (0, stepGesture (0.5f, 1.0f, { 0.0f, 5 * kSemitone, -3 * kSemitone, 7 * kSemitone }, 0.05f))
             .gesture (1, stepGesture (0.5f, 1.0f, { 0.0f, -5 * kSemitone, 4 * kSemitone }, 0.05f))
             .fx (1.3f, 0.22f));

    add (Builder ("Cardboard Box", cat, "cardboard, box, dull, papery",
                  "A fist on a cardboard box: a dull, papery, dead thump.")
             .strike (0.50f, 0.30f, 0.30f)
             .material (wood, 0.50f, 0.20f, 0.90f, 0.60f)
             .macros (0.76f, 0.40f, 0.40f, 0.10f)
             .topology (star)
             .fx (1.0f, 0.14f));

    add (Builder ("Industrial Hit", cat, "industrial, steel door, driven, clang",
                  "A sledgehammer on a steel door: a driven, clanging, distorted impact.")
             .strike (0.96f, 0.10f, 0.60f)
             .material (metal, 0.70f, 0.56f, 0.86f, 0.84f)
             .macros (0.74f, 0.70f, 0.50f, 0.40f)
             .decays (0.30f, 0.30f, 0.30f, 0.30f)
             .topology (web)
             .fx (1.3f, 0.36f, 0.56f));

    add (Builder ("Watch Tick", cat, "tick, watch, tiny, click",
                  "The escapement of a pocket watch: a tiny, precise metallic tick.")
             .strike (0.90f, 0.02f, 0.90f)
             .material (metal, 0.06f, 0.90f, 0.92f, 0.60f)
             .macros (0.62f, 0.30f, 0.86f, 0.10f)
             .fx (0.9f, 0.10f));

    add (Builder ("Hand Clap", cat, "clap, hands, noisy, chaos",
                  "A hand clap: two palms, a burst of noisy, fleshy air.")
             .strike (0.80f, 0.12f, 0.64f)
             .material (membrane, 0.12f, 0.64f, 0.84f, 0.80f)
             .macros (0.76f, 0.86f, 0.60f, 0.80f)
             .topology (web)
             .fx (1.3f, 0.26f));

    add (Builder ("Agogo", cat, "agogo, bells, pair, bright",
                  "A pair of agogo bells a fourth apart: bright, clanking, and made for samba.")
             .strike (0.84f, 0.06f, 0.70f)
             .material (metal, 0.30f, 0.70f, 0.68f, 0.52f)
             .macros (0.64f, 0.34f, 0.64f, 0.04f)
             .topology (star)
             .tune (0, kFourth)
             .levels (0.80f, 0.10f, 0.10f, 0.10f)
             .fx (1.2f, 0.20f));

    add (Builder ("Syndrum Pew", cat, "syndrum, pew, disco, pitch drop",
                  "A 70s Syndrum: a light skin hit hard so its pitch dives, pew.")
             .strike (0.76f, 0.10f, 0.50f)
             .material (membrane, 0.20f, 0.56f, 0.56f, 0.40f)
             .macros (0.94f, 0.20f, 0.30f, 0.02f)
             .topology (star)
             .levels (0.20f, 0.20f, 0.20f, 0.20f)
             .fx (1.2f, 0.30f, 0.14f));

    add (Builder ("Door Knock", cat, "knock, door, heavy, wood",
                  "A knock on a heavy oak door: a low, solid rap with the door's hollow behind it.")
             .strike (0.70f, 0.14f, 0.36f)
             .material (wood, 0.72f, 0.36f, 0.72f, 0.56f)
             .macros (0.70f, 0.40f, 0.44f, 0.04f)
             .topology (star)
             .fx (1.0f, 0.22f));
}

} // namespace arc::presets::detail

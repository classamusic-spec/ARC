// Factory library: MEMBRANE. Skins of every size: kettles, tablas, frame drums, surdos, taiko,
// balloons and paper. An ideal membrane rings at 1.59 / 2.14 / 2.30 / 2.65 x CORE; a kettle or a
// loaded skin (timpani, tabla) pulls those modes toward harmonics, which the pitched drums copy.
// Skin tension rises with amplitude, so light skins struck hard glide down as they ring.
// See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addMembrane (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "MEMBRANE";

    add (Builder ("Timpani", cat, "timpani, kettle, orchestral, pitched",
                  "A calf-skin timpani over a copper kettle: the bowl pulls the skin's modes into a near-harmonic, pitched boom.")
             .strike (0.44f, 0.40f, 0.40f)
             .material (membrane, 0.78f, 0.40f, 0.22f, 0.40f)
             .macros (0.66f, 0.40f, 0.50f, 0.02f)
             .chord (1.5f, 2.0f, 2.5f, 3.0f)
             .levels (0.70f, 0.56f, 0.40f, 0.30f)
             .fx (1.3f, 0.50f));

    add (Builder ("Tabla Dayan", cat, "tabla, dayan, harmonic, ringing",
                  "The treble tabla: the black syahi loads the skin until its modes ring as true harmonics, a singing 'Na'.")
             .strike (0.70f, 0.12f, 0.64f)
             .material (membrane, 0.46f, 0.60f, 0.26f, 0.40f)
             .macros (0.66f, 0.46f, 0.62f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .levels (0.70f, 0.60f, 0.50f, 0.40f)
             .fx (1.1f, 0.24f));

    add (Builder ("Tabla Bayan", cat, "tabla, bayan, bass, pitch glide",
                  "The bass tabla: a loose, heavy skin struck hard so its pitch swoops down as it rings, the famous 'Ge'.")
             .strike (0.60f, 0.30f, 0.34f)
             .material (membrane, 0.34f, 0.40f, 0.30f, 0.50f)
             .macros (0.82f, 0.30f, 0.34f, 0.04f)
             .fx (1.1f, 0.26f));

    add (Builder ("Conga Slap", cat, "conga, slap, bright, latin",
                  "An open-palm slap on a conga: a bright, cracking attack over a short, pitched tone.")
             .strike (0.86f, 0.06f, 0.70f)
             .material (membrane, 0.40f, 0.62f, 0.56f, 0.56f)
             .macros (0.66f, 0.40f, 0.58f, 0.06f)
             .fx (1.1f, 0.20f));

    add (Builder ("Bodhran", cat, "bodhran, frame drum, irish, warm",
                  "An Irish bodhran hit with a tipper: a warm thump, the hand behind the skin shaping the tone.")
             .strike (0.50f, 0.34f, 0.36f)
             .material (membrane, 0.62f, 0.40f, 0.44f, 0.52f)
             .macros (0.64f, 0.34f, 0.44f, 0.04f)
             .topology (star)
             .fx (1.1f, 0.24f));

    add (Builder ("Djembe", cat, "djembe, goblet, bass, slap",
                  "A goatskin djembe: the goblet's air adds a deep bass under the skin's bright, cutting tone.")
             .strike (0.72f, 0.18f, 0.52f)
             .material (membrane, 0.50f, 0.54f, 0.40f, 0.54f)
             .macros (0.68f, 0.44f, 0.52f, 0.06f)
             .chord (0.88f, 2.2f, 2.9f, 3.8f)
             .fx (1.2f, 0.26f));

    add (Builder ("Membrane Bass", cat, "bass, mono, rubbery, deep",
                  "A string plucked over a tight skin: a round, rubbery, deep bass with a drum's thump in it.")
             .pluck (0.30f, 0.30f, 0.40f)
             .material (membrane, 0.76f, 0.34f, 0.40f, 0.44f)
             .macros (0.66f, 0.40f, 0.40f, 0.02f)
             .chord (1.5f, 2.0f, 3.0f, 4.0f)
             .voice (VoiceMode::mono, 0.05f)
             .fx (1.0f, 0.12f, 0.24f));

    add (Builder ("Ocean Drum", cat, "ocean drum, surf, noise, beads, unpitched",
                  "Beads rolling across the inside of a frame drum: the slow wash of surf on a beach.")
             .air (0.26f, 0.96f, 0.52f)
             .material (membrane, 0.50f, 0.40f, 0.30f, 0.60f)
             .macros (0.74f, 0.64f, 0.44f, 0.50f)
             .topology (web)
             .levels (0.90f, 0.90f, 0.90f, 0.90f)
             .motion (0.60f, 0.08f)
             .fx (1.5f, 0.60f));

    add (Builder ("Marching Quads", cat, "marching, quads, crisp, tight",
                  "Kevlar-headed marching tenors cranked tight: crisp, cutting, and every drum ringing a little in sympathy.")
             .strike (0.90f, 0.06f, 0.72f)
             .material (membrane, 0.36f, 0.66f, 0.40f, 0.46f)
             .macros (0.66f, 0.40f, 0.78f, 0.04f)
             .chord (1.26f, 1.5f, 1.68f, 2.0f)
             .levels (0.40f, 0.40f, 0.40f, 0.40f)
             .fx (1.3f, 0.30f));

    add (Builder ("Skin Harp", cat, "harp, skin, soft, warm",
                  "Harp strings running over a drum head: the skin warms and softens every pluck into a padded, humming tone.")
             .pluck (0.24f, 0.14f, 0.56f)
             .material (membrane, 0.56f, 0.50f, 0.30f, 0.44f)
             .macros (0.62f, 0.52f, 0.56f, 0.04f)
             .topology (ring)
             .chord (2.0f, 3.0f, 4.0f, 5.0f)
             .fx (1.35f, 0.40f));

    add (Builder ("Taiko Hall", cat, "taiko, huge, low, hall",
                  "An o-daiko struck with both arms in a wooden hall: a huge, low impact and a long, rolling decay.")
             .strike (0.80f, 0.40f, 0.30f)
             .material (membrane, 0.90f, 0.34f, 0.30f, 0.50f)
             .macros (0.72f, 0.40f, 0.36f, 0.06f)
             .topology (star)
             .fx (1.4f, 0.74f, 0.20f));

    add (Builder ("Bongo Pair", cat, "bongos, pair, high, bright",
                  "A pair of bongos a fourth apart: hit the macho and the hembra answers.")
             .strike (0.80f, 0.08f, 0.66f)
             .material (membrane, 0.30f, 0.62f, 0.46f, 0.50f)
             .macros (0.66f, 0.46f, 0.66f, 0.04f)
             .topology (star)
             .tune (0, kFourth)
             .levels (0.80f, 0.20f, 0.20f, 0.20f)
             .fx (1.2f, 0.20f));

    add (Builder ("Surdo", cat, "surdo, samba, deep, boom",
                  "A samba surdo: a deep, long, open boom that anchors the whole bateria.")
             .strike (0.56f, 0.46f, 0.26f)
             .material (membrane, 0.86f, 0.30f, 0.34f, 0.46f)
             .macros (0.70f, 0.30f, 0.40f, 0.04f)
             .fx (1.1f, 0.30f));

    add (Builder ("Dhol", cat, "dhol, bhangra, loud, driven",
                  "A double-headed dhol: a booming bass skin and a snapping treble skin, loud and driven.")
             .strike (0.84f, 0.16f, 0.50f)
             .material (membrane, 0.64f, 0.52f, 0.40f, 0.52f)
             .macros (0.72f, 0.54f, 0.50f, 0.10f)
             .topology (chain)
             .tune (0, 2.4f)
             .fx (1.2f, 0.28f, 0.34f));

    add (Builder ("Tar Frame", cat, "tar, frame drum, open, ringing",
                  "A wide, thin tar frame drum struck near the rim: bright, open and ringing.")
             .strike (0.62f, 0.14f, 0.56f)
             .material (membrane, 0.44f, 0.56f, 0.30f, 0.50f)
             .macros (0.64f, 0.44f, 0.56f, 0.04f)
             .topology (ring)
             .fx (1.35f, 0.44f));

    add (Builder ("Roto Toms", cat, "roto toms, tuned, melodic, punchy",
                  "Tunable roto-tom heads pitched to the note: melodic, punchy and dry.")
             .strike (0.66f, 0.16f, 0.54f)
             .material (membrane, 0.48f, 0.54f, 0.34f, 0.44f)
             .macros (0.64f, 0.34f, 0.56f, 0.02f)
             .quantise()
             .chord (1.5f, 2.25f, 3.0f, 4.5f)
             .fx (1.2f, 0.26f));

    add (Builder ("Water Drum", cat, "water drum, gourd, bloop, wobble",
                  "A half-gourd floating upside down in water, struck: a deep, round 'bloop' that wobbles with the water.")
             .strike (0.40f, 0.44f, 0.24f)
             .material (membrane, 0.74f, 0.30f, 0.26f, 0.60f)
             .macros (0.64f, 0.44f, 0.38f, 0.06f)
             .gesture (0, breatheGesture (0.9f, 0.0f, 0.04f, 1))
             .gesture (1, breatheGesture (1.3f, 0.0f, 0.04f, 1))
             .fx (1.3f, 0.40f));

    add (Builder ("Distant Thunder", cat, "thunder, distant, rumble, chaos, inharmonic",
                  "Thunder rolling miles away: a soft, long, low rumble that never quite settles.")
             .strike (0.30f, 0.80f, 0.16f)
             .material (membrane, 0.84f, 0.26f, 0.24f, 0.70f)
             .macros (0.70f, 0.66f, 0.30f, 0.44f)
             .topology (web)
             .motion (0.50f, 0.3f)
             .fx (1.5f, 0.86f));

    add (Builder ("Drum Circle", cat, "ensemble, many drums, sympathetic, wide",
                  "A circle of drums around you: strike one and the others hum along from every side.")
             .strike (0.64f, 0.24f, 0.48f)
             .material (membrane, 0.56f, 0.46f, 0.40f, 0.50f)
             .macros (0.66f, 0.76f, 0.50f, 0.10f)
             .topology (web)
             .chord (1.2f, 1.5f, 1.8f, 2.4f)
             .angles (0.25f, 0.75f, 0.10f, 0.90f)
             .fx (1.5f, 0.40f));

    add (Builder ("Breathing Drum", cat, "resonant, swelling, slow, moving",
                  "A low drum whose resonances swell and recede after every hit, as if the skin were breathing.")
             .strike (0.46f, 0.30f, 0.40f)
             .material (membrane, 0.60f, 0.44f, 0.20f, 0.48f)
             .macros (0.64f, 0.52f, 0.46f, 0.04f)
             .gesture (0, breatheGesture (2.6f, 0.0f, 0.10f, 1))
             .gesture (1, breatheGesture (3.4f, 0.0f, 0.10f, 1))
             .gesture (2, breatheGesture (4.2f, 0.0f, 0.10f, 1))
             .fx (1.4f, 0.56f));

    add (Builder ("Cuica Whine", cat, "cuica, friction, squeaky, samba, glide",
                  "A samba cuica: a stick rubbed inside the drum, whining and squeaking up and down.")
             .bow (0.60f, 0.64f, 0.66f)
             .material (membrane, 0.36f, 0.64f, 0.34f, 0.50f)
             .macros (0.62f, 0.34f, 0.72f, 0.06f)
             .voice (VoiceMode::legato, 0.12f)
             .fx (1.1f, 0.24f));

    add (Builder ("Heartbeat", cat, "heartbeat, low, muffled, soft",
                  "A soft, muffled, very low thump, felt more than heard.")
             .strike (0.24f, 0.70f, 0.14f)
             .material (membrane, 0.92f, 0.20f, 0.60f, 0.46f)
             .macros (0.74f, 0.30f, 0.36f, 0.02f)
             .topology (star)
             .fx (1.0f, 0.20f, 0.10f));

    add (Builder ("Soft Toms", cat, "toms, felt mallets, warm, round",
                  "Tom-toms played with felt mallets: warm, round and gently pitched.")
             .strike (0.26f, 0.54f, 0.36f)
             .material (membrane, 0.62f, 0.40f, 0.34f, 0.46f)
             .macros (0.64f, 0.34f, 0.44f, 0.02f)
             .fx (1.2f, 0.34f));

    add (Builder ("Paper Drum", cat, "paper, papery, light, buzzy, inharmonic",
                  "A drum skinned with paper: light, papery and buzzing, gone in a moment.")
             .strike (0.60f, 0.12f, 0.62f)
             .material (membrane, 0.16f, 0.62f, 0.62f, 0.64f)
             .macros (0.64f, 0.44f, 0.56f, 0.30f)
             .fx (1.1f, 0.18f));

    add (Builder ("Gran Cassa", cat, "bass drum, orchestral, huge, felt",
                  "An orchestral bass drum struck with a big felt beater: a huge, soft, blooming boom.")
             .strike (0.36f, 0.66f, 0.20f)
             .material (membrane, 0.94f, 0.28f, 0.22f, 0.52f)
             .macros (0.72f, 0.40f, 0.34f, 0.04f)
             .topology (star)
             .fx (1.3f, 0.66f));

    add (Builder ("Timpani Roll", cat, "timpani, roll, rumble, sustained",
                  "A timpani roll: soft sticks alternating so fast the kettle sustains as one rumbling, pitched tone.")
             .air (0.30f, 0.94f, 0.30f)
             .material (membrane, 0.78f, 0.40f, 0.22f, 0.40f)
             .macros (0.74f, 0.44f, 0.50f, 0.10f)
             .chord (1.5f, 2.0f, 2.5f, 3.0f)
             .levels (0.90f, 0.80f, 0.70f, 0.60f)
             .fx (1.3f, 0.50f));

    add (Builder ("Balloon Tap", cat, "balloon, rubbery, boingy, playful",
                  "A finger flicking a party balloon: a light, rubbery boing with a squeak in it.")
             .strike (0.40f, 0.20f, 0.60f)
             .material (membrane, 0.14f, 0.60f, 0.46f, 0.40f)
             .macros (0.64f, 0.30f, 0.70f, 0.08f)
             .fx (1.1f, 0.24f));

    add (Builder ("Trampoline", cat, "boing, pitch glide, playful, loose",
                  "A slack skin hit hard: the pitch leaps up with the impact and slides back down as it rings.")
             .strike (0.70f, 0.40f, 0.36f)
             .material (membrane, 0.26f, 0.42f, 0.24f, 0.46f)
             .macros (0.86f, 0.30f, 0.24f, 0.04f)
             .fx (1.2f, 0.34f));
}

} // namespace arc::presets::detail

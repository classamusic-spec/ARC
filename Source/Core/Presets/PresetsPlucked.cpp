// Factory library: PLUCKED. Harps, zithers, lutes, basses and impossible strings: a
// finger drags and releases the CORE, the network answers. See docs/PRESETS.md.

#include "Core/PresetBuilder.h"

namespace arc::presets::detail
{

void addPlucked (Library& lib)
{
    auto add = [&lib] (const Builder& b) { lib.push_back (b.build()); };
    const char* cat = "PLUCKED";

    add (Builder ("Steel Tine Harp", cat, "harp, metal, harmonic, bright",
                  "A bright steel harp whose four nodes ring harmonic partials (2, 5/2, 3, 4) in sympathy.")
             .pluck (0.16f, 0.02f, 0.70f)
             .material (metal, 0.46f, 0.60f, 0.40f, 0.46f)
             .macros (0.60f, 0.34f, 0.54f, 0.04f)
             .chord (2.0f, 2.5f, 3.0f, 4.0f)
             .fx (1.3f, 0.28f));

    add (Builder ("Nylon Dusk", cat, "guitar, wood, warm, soft",
                  "Warm nylon on a wooden body, plucked away from the bridge: round and intimate.")
             .pluck (0.24f, 0.10f, 0.42f)
             .material (wood, 0.56f, 0.40f, 0.44f, 0.44f)
             .macros (0.60f, 0.30f, 0.50f, 0.03f)
             .fx (1.15f, 0.20f));

    add (Builder ("Koto Mist", cat, "koto, pentatonic, metal, oriental",
                  "Koto-like silk strings over sympathetic nodes tuned to the hirajoshi scale.")
             .pluck (0.12f, 0.04f, 0.62f)
             .material (metal, 0.50f, 0.54f, 0.42f, 0.46f)
             .macros (0.60f, 0.36f, 0.52f, 0.04f)
             .chord (kMajor2 * 2.0f, kMinor3, kFifth, kMinor6)
             .fx (1.3f, 0.30f));

    add (Builder ("Sitar Sympathy", cat, "sitar, sympathetic, buzz, drone",
                  "A bright, buzzing string: sympathetic nodes on the drone intervals ring along a chain.")
             .pluck (0.08f, 0.00f, 0.80f)
             .material (metal, 0.44f, 0.64f, 0.34f, 0.58f)
             .macros (0.62f, 0.58f, 0.56f, 0.08f)
             .topology (chain)
             .chord (kFifth, 2.0f, 3.0f, 4.0f)
             .decays (0.72f, 0.72f, 0.70f, 0.68f)
             .fx (1.3f, 0.24f, 0.18f));

    add (Builder ("Banjo Skin", cat, "banjo, membrane, twangy, bright",
                  "A string over a drum head: bright, twangy and quick to die away.")
             .pluck (0.10f, 0.06f, 0.78f)
             .material (membrane, 0.34f, 0.62f, 0.52f, 0.46f)
             .macros (0.62f, 0.36f, 0.58f, 0.04f)
             .topology (star)
             .fx (1.1f, 0.14f));

    add (Builder ("Oud Night", cat, "oud, wood, maqam, dark",
                  "A dark, fretless lute whose nodes sit on neutral (maqam) intervals.")
             .pluck (0.20f, 0.08f, 0.40f)
             .material (wood, 0.62f, 0.34f, 0.40f, 0.46f)
             .macros (0.60f, 0.34f, 0.44f, 0.04f)
             .chord (11.0f / 9.0f * 2.0f, kFourth * 2.0f, 27.0f / 16.0f * 2.0f, 5.5f)
             .fx (1.2f, 0.24f));

    add (Builder ("Pizzicato Glass", cat, "pizzicato, glass, short, dry",
                  "Short, dry pizzicato on glass strings: every note a small, clean point.")
             .pluck (0.22f, 0.44f, 0.58f)
             .material (glass, 0.42f, 0.52f, 0.60f, 0.44f)
             .macros (0.62f, 0.26f, 0.52f, 0.02f)
             .release (0.50f)
             .fx (1.1f, 0.16f));

    add (Builder ("Dulcimer Rain", cat, "dulcimer, metal, web, cascading",
                  "Plucked dulcimer courses in a web; drift scatters the sympathetic ringing like rain.")
             .pluck (0.14f, 0.00f, 0.68f)
             .material (metal, 0.44f, 0.58f, 0.36f, 0.50f)
             .macros (0.60f, 0.50f, 0.54f, 0.14f)
             .topology (web)
             .chord (2.0f * 1.003f, 2.0f * 0.997f, 3.0f, 4.0f * 0.998f)
             .motion (0.32f, 0.45f)
             .fx (1.4f, 0.30f));

    add (Builder ("Muted Bass", cat, "bass, mono, muted, wood",
                  "A palm-muted wooden bass: short, round and tight. Mono, with a hint of glide.")
             .pluck (0.26f, 0.34f, 0.40f)
             .material (wood, 0.74f, 0.36f, 0.38f, 0.40f)
             .macros (0.68f, 0.26f, 0.44f, 0.02f)
             .chord (2.0f, 3.0f, 4.0f, 6.0f)
             .voice (VoiceMode::mono, 0.03f)
             .fx (1.0f, 0.08f));

    add (Builder ("Silver Autoharp", cat, "autoharp, chord, metal, shimmer",
                  "Every note strums a major chord through its sympathetic nodes: an instant autoharp.")
             .pluck (0.18f, 0.02f, 0.62f)
             .material (metal, 0.48f, 0.56f, 0.34f, 0.46f)
             .macros (0.60f, 0.46f, 0.52f, 0.04f)
             .chord (kMajor3, kFifth, 2.0f, 2.5f)
             .fx (1.35f, 0.30f));

    add (Builder ("Twelve Strings", cat, "12-string, chorus, metal, jangle",
                  "Octave courses a few cents apart: the jangling chorus of a twelve-string.")
             .pluck (0.14f, 0.04f, 0.72f)
             .material (metal, 0.46f, 0.60f, 0.38f, 0.48f)
             .macros (0.60f, 0.38f, 0.54f, 0.04f)
             .chord (2.0f * 1.004f, 2.0f * 0.997f, 3.0f * 1.003f, 4.0f)
             .fx (1.45f, 0.24f));

    add (Builder ("Guzheng Bend", cat, "guzheng, legato, glide, pentatonic",
                  "A long zither string bent between notes: play legato and the pitch slides.")
             .pluck (0.12f, 0.02f, 0.64f)
             .material (metal, 0.52f, 0.54f, 0.38f, 0.46f)
             .macros (0.62f, 0.34f, 0.52f, 0.04f)
             .chord (kMajor2 * 2.0f, kMajor3, kFifth, kMajor6)
             .voice (VoiceMode::legato, 0.14f)
             .fx (1.3f, 0.30f));

    add (Builder ("Lute Garden", cat, "lute, wood, renaissance, gentle",
                  "A gentle lute with gut strings and a small, sweet wooden body.")
             .pluck (0.30f, 0.12f, 0.46f)
             .material (wood, 0.52f, 0.44f, 0.46f, 0.42f)
             .macros (0.60f, 0.32f, 0.52f, 0.03f)
             .chord (2.0f, 3.0f, 5.0f, 8.0f)
             .levels (0.60f, 0.50f, 0.40f, 0.30f)
             .fx (1.2f, 0.26f));

    add (Builder ("Crystal Koto", cat, "koto, glass, bright, pentatonic",
                  "A koto strung with glass threads: brittle, bright and pentatonic.")
             .pluck (0.10f, 0.00f, 0.74f)
             .material (glass, 0.40f, 0.62f, 0.38f, 0.46f)
             .macros (0.60f, 0.32f, 0.54f, 0.03f)
             .chord (kMajor2 * 2.0f, kMinor3 * 2.0f, kFifth * 2.0f, kMinor6 * 4.0f)
             .fx (1.35f, 0.32f));

    add (Builder ("Shamisen Snap", cat, "shamisen, membrane, snappy, percussive",
                  "A hard plectrum on a skin-bodied lute: a snappy attack with a buzzing, nasal tail.")
             .pluck (0.06f, 0.18f, 0.86f)
             .material (membrane, 0.36f, 0.64f, 0.46f, 0.52f)
             .macros (0.64f, 0.38f, 0.60f, 0.06f)
             .fx (1.1f, 0.16f, 0.12f));

    add (Builder ("Harp of Wires", cat, "harp, metal, long, web",
                  "Long bronze wires in a web: plucked notes bloom into a slow sympathetic halo.")
             .pluck (0.20f, 0.00f, 0.56f)
             .material (metal, 0.58f, 0.48f, 0.26f, 0.48f)
             .macros (0.60f, 0.56f, 0.50f, 0.06f)
             .topology (web)
             .decays (0.80f, 0.80f, 0.78f, 0.76f)
             .fx (1.4f, 0.40f));

    add (Builder ("Pendulum Harp", cat, "harp, glass, synced, sway",
                  "Plucked glass whose brightest node swings like a pendulum, once per bar.")
             .pluck (0.18f, 0.02f, 0.66f)
             .material (glass, 0.44f, 0.58f, 0.40f, 0.46f)
             .macros (0.60f, 0.40f, 0.52f, 0.04f)
             .synced (bar1, 0.0f)
             .gesture (1, swayGesture (2.0f, 4.0f, 1.3f, 0.02f))
             .fx (1.45f, 0.30f));

    add (Builder ("Resonant Arp", cat, "arpeggio, steps, synced, metal",
                  "Held notes arpeggiate by themselves: node A steps through a minor chord in sixteenths.")
             .pluck (0.16f, 0.04f, 0.66f)
             .material (metal, 0.48f, 0.56f, 0.34f, 0.46f)
             .macros (0.62f, 0.52f, 0.52f, 0.04f)
             .quantise()
             .synced (quarter, 0.0f)
             .gesture (0, stepGesture (0.5f, 1.0f, { 0.0f, 3.0f * kSemitone, 7.0f * kSemitone, 12.0f * kSemitone }, 0.12f))
             .fx (1.3f, 0.26f));

    add (Builder ("Clav Circuit", cat, "clavinet, funky, muted, bright",
                  "A muted, bright wooden string: tight, funky and percussive, like a clavinet.")
             .pluck (0.06f, 0.30f, 0.84f)
             .material (wood, 0.44f, 0.66f, 0.42f, 0.48f)
             .macros (0.70f, 0.28f, 0.56f, 0.02f)
             .chord (2.0f, 3.0f, 5.0f, 7.0f)
             .fx (1.0f, 0.10f, 0.20f));

    add (Builder ("Hollow Pluck", cat, "hollow, even harmonics, metal, woody",
                  "Plucked at the exact middle: the even harmonics vanish and the tone turns hollow.")
             .pluck (0.50f, 0.04f, 0.56f)
             .material (metal, 0.50f, 0.50f, 0.40f, 0.46f)
             .macros (0.62f, 0.30f, 0.50f, 0.03f)
             .chord (1.5f, 2.5f, 3.0f * 1.003f, 5.0f * 0.997f)
             .fx (1.2f, 0.22f));

    add (Builder ("Gossamer", cat, "glass, soft, delicate, ambient",
                  "Soft plucks on fine glass threads, left to ring into a wide space.")
             .pluck (0.26f, 0.00f, 0.40f)
             .material (glass, 0.48f, 0.48f, 0.30f, 0.44f)
             .macros (0.58f, 0.40f, 0.50f, 0.06f)
             .motion (0.16f, 0.10f)
             .fx (1.5f, 0.52f));

    add (Builder ("Bass Harp", cat, "bass, harp, metal, deep",
                  "The low strings of a concert harp: heavy, slack and resonant.")
             .pluck (0.22f, 0.04f, 0.46f)
             .material (metal, 0.78f, 0.40f, 0.34f, 0.44f)
             .macros (0.62f, 0.34f, 0.38f, 0.03f)
             .chord (kFifth, 2.0f, 3.0f, 4.0f)
             .fx (1.2f, 0.26f));

    add (Builder ("Chime Strings", cat, "chime, metal, high tension, bell",
                  "High-tension steel that rings like a chime: plucked bells.")
             .pluck (0.14f, 0.00f, 0.72f)
             .material (metal, 0.40f, 0.62f, 0.32f, 0.56f)
             .macros (0.60f, 0.40f, 0.76f, 0.04f)
             .fx (1.35f, 0.34f));

    add (Builder ("Desert Lute", cat, "lute, maqam, wood, dry",
                  "A dry desert lute: short, woody, with sympathetic nodes on the rast maqam.")
             .pluck (0.16f, 0.22f, 0.60f)
             .material (wood, 0.50f, 0.52f, 0.52f, 0.46f)
             .macros (0.62f, 0.30f, 0.54f, 0.04f)
             .chord (kMajor2 * 2.0f, 27.0f / 22.0f * 2.0f, kFourth * 2.0f, 6.0f)
             .fx (1.1f, 0.14f));

    add (Builder ("Neon Pluck", cat, "synthetic, bright, driven, pop",
                  "A bright metal pluck pushed through the drive stage: synthetic and poppy.")
             .pluck (0.10f, 0.20f, 0.82f)
             .material (metal, 0.40f, 0.66f, 0.46f, 0.44f)
             .macros (0.66f, 0.32f, 0.56f, 0.03f)
             .chord (2.0f, 1.5f, 3.0f, 5.0f)
             .fx (1.3f, 0.20f, 0.34f));

    add (Builder ("Kinetic Harp", cat, "harp, glass, wander, synced",
                  "A glass harp whose nodes wander in tempo: repeated notes never sound the same twice.")
             .pluck (0.20f, 0.02f, 0.62f)
             .material (glass, 0.42f, 0.56f, 0.40f, 0.48f)
             .macros (0.60f, 0.46f, 0.54f, 0.10f)
             .synced (bar1, 0.18f)
             .gesture (0, wanderGesture (2.0f, 4.0f, 0.9f, 0.03f, 101u))
             .gesture (2, wanderGesture (2.0f, 4.0f, 0.9f, 0.03f, 202u))
             .fx (1.4f, 0.30f));

    add (Builder ("Fretless Bass", cat, "bass, fretless, legato, wood",
                  "A fretless bass: legato notes slide, the tone is woody and singing.")
             .pluck (0.28f, 0.16f, 0.42f)
             .material (wood, 0.70f, 0.40f, 0.40f, 0.42f)
             .macros (0.66f, 0.28f, 0.42f, 0.02f)
             .voice (VoiceMode::legato, 0.09f)
             .fx (1.0f, 0.10f));

    add (Builder ("Ghost Strings", cat, "membrane, airy, soft, haunting",
                  "Soft plucks on skin-backed strings, washed into a haunting space.")
             .pluck (0.34f, 0.04f, 0.36f)
             .material (membrane, 0.54f, 0.40f, 0.30f, 0.46f)
             .macros (0.58f, 0.44f, 0.48f, 0.08f)
             .motion (0.20f, 0.07f)
             .fx (1.45f, 0.56f));

    add (Builder ("Mandolin Bright", cat, "mandolin, doubled, metal, bright",
                  "Paired steel courses, bright and short, with the shimmer of doubled strings.")
             .pluck (0.10f, 0.14f, 0.76f)
             .material (metal, 0.38f, 0.64f, 0.48f, 0.50f)
             .macros (0.62f, 0.34f, 0.60f, 0.03f)
             .chord (2.0f * 1.005f, 2.0f * 0.995f, 3.0f, 5.0f)
             .fx (1.3f, 0.20f));

    add (Builder ("Prepared String", cat, "prepared, inharmonic, metal, bolts",
                  "Bolts between the strings: nodes on golden-ratio intervals make every pluck clang.")
             .pluck (0.20f, 0.10f, 0.60f)
             .material (metal, 0.52f, 0.52f, 0.42f, 0.66f)
             .macros (0.62f, 0.52f, 0.52f, 0.16f)
             .chord (1.618f, 2.618f, 1.618f * 1.618f * 1.618f, 4.236f)
             .fx (1.2f, 0.22f));

    add (Builder ("Rubber Band", cat, "rubbery, membrane, boing, playful",
                  "A slack band over a membrane: the pitch sags as each note dies. Boing.")
             .pluck (0.30f, 0.10f, 0.48f)
             .material (membrane, 0.30f, 0.52f, 0.44f, 0.40f)
             .macros (0.72f, 0.24f, 0.30f, 0.02f)
             .topology (star)
             .fx (1.1f, 0.12f));

    add (Builder ("Pluck Constellation", cat, "glass, sparkling, stars, wide",
                  "Plucked glass with nodes scattered like stars: 5/2, 10/3, 9/2, 27/4.")
             .pluck (0.16f, 0.00f, 0.70f)
             .material (glass, 0.40f, 0.60f, 0.36f, 0.50f)
             .macros (0.60f, 0.44f, 0.56f, 0.06f)
             .topology (web)
             .chord (2.5f, 10.0f / 3.0f, 4.5f, 6.75f)
             .fx (1.5f, 0.36f));
}

} // namespace arc::presets::detail

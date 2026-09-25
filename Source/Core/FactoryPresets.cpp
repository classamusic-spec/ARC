#include "Core/FactoryPresets.h"

#include <algorithm>

#include "Core/PresetBuilder.h"

namespace arc::presets
{

namespace
{
using namespace detail;

// Measured loudness trims (Tests/LibraryTests.cpp, "calibrate loudness"): every preset's test
// chord lands on the same loudness, so browsing never jumps in level.
struct Trim
{
    const char* name;
    float dB;
};
constexpr Trim kCalibration[] = {
#include "Presets/Calibration.inc"
    { nullptr, 0.0f }
};

void applyCalibration (std::vector<PresetDefinition>& presets)
{
    for (auto& p : presets)
        for (const auto& t : kCalibration)
            if (t.name != nullptr && p.name == t.name)
            {
                bool found = false;
                for (auto& kv : p.values)
                    if (kv.first == params::patchLevel)
                        kv.second = t.dB, found = true;
                if (! found)
                    p.values.emplace_back (params::patchLevel, t.dB);
                break;
            }
}

std::vector<PresetDefinition> build()
{
    std::vector<PresetDefinition> v;
    auto add = [&v] (const Builder& b) { v.push_back (b.build()); };

    // ---------------------------------------------------------------- PADS
    add (Builder ("Obsidian Bloom", "PADS", "dark, evolving, bowed, wide",
                  "A dark bowed metal lattice that slowly blooms as energy spreads through a web of resonators.", 0x0B5D1A01u)
             .bow (0.38f, 0.42f, 0.46f)
             .material (metal, 0.62f, 0.34f, 0.32f, 0.55f)
             .macros (0.58f, 0.685f, 0.44f, 0.16f)
             .topology (Topology::web)
             // Node A at 1.11 x (not 1.04): a bowed CORE locks onto one side of a
             // near-unison doublet, which played this preset 70 cents sharp.
             .nodes ("radius", 0.45f, 0.57f, 0.33f, 0.66f)
             .nodes ("decay", 0.62f, 0.62f, 0.58f, 0.58f)
             .nodes ("damp", 0.58f, 0.58f, 0.62f, 0.62f)
             .nodes ("level", 0.72f, 0.70f, 0.66f, 0.66f)
             .nodes ("link", 0.82f, 0.80f, 0.78f, 0.78f)
             .motion (0.22f, 0.07f)
             .release (0.12f)
             .fx (1.2f, 0.32f));

    add (Builder ("Soft Machine", "PADS", "warm, breathy, soft, wood",
                  "Breath through a wooden body: soft, warm and quietly moving.", 0x50F7AC01u)
             .air (0.50f, 0.48f, 0.36f)
             .material (wood, 0.55f, 0.40f, 0.42f, 0.45f)
             .macros (0.55f, 0.477f, 0.46f, 0.12f)
             .motion (0.28f, 0.09f)
             .release (0.15f)
             .voice (VoiceMode::poly, 0.08f)
             .fx (1.15f, 0.30f));

    add (Builder ("Aurora Lattice", "PADS", "shimmer, glass, airy, wide",
                  "Blown glass nodes in a web; slow drift makes the partials shimmer like light.", 0xA0B0EA01u)
             .air (0.50f, 0.38f, 0.58f)
             .material (glass, 0.45f, 0.58f, 0.35f)
             .macros (0.55f, 0.641f, 0.52f, 0.20f)
             .topology (Topology::web)
             .nodes ("radius", 0.56f, 0.44f, 0.60f, 0.40f)
             .motion (0.38f, 0.06f)
             .release (0.12f)
             .fx (1.4f, 0.42f));

    add (Builder ("Quiet Orbit", "PADS", "soft, bowed, orbit, synced",
                  "A gently bowed membrane with two nodes orbiting the core in tempo.", 0x0B17A101u)
             .bow (0.30f, 0.36f, 0.40f)
             .material (membrane, 0.60f, 0.40f, 0.38f)
             .macros (0.52f, 0.536f, 0.48f, 0.08f)
             .synced (bar4, 0.0f)
             .gesture (1, orbitGesture (8.0f, 16.0f, 1.0f, 0.0f))
             .gesture (2, orbitGesture (8.0f, 16.0f, -1.0f, 0.0f))
             .release (0.15f)
             .fx (1.3f, 0.38f));

    // ---------------------------------------------------------------- DRONES
    add (Builder ("Infinite Bow", "DRONES", "endless, bowed, metal, drone",
                  "An endless bowed metal drone. Hold a chord and let the network breathe.", 0x1DF1B001u)
             .bow (0.46f, 0.46f, 0.50f)
             .material (metal, 0.60f, 0.44f, 0.22f)
             .macros (0.60f, 0.641f, 0.50f, 0.20f)
             .motion (0.30f, 0.05f)
             .release (0.06f)
             .fx (1.25f, 0.35f));

    add (Builder ("Black Monolith", "DRONES", "dark, heavy, cluster, bowed",
                  "A heavy, low-tension metal mass bowed into a slowly shifting cluster.", 0xB1AC0101u)
             .bow (0.60f, 0.40f, 0.56f)
             .material (metal, 0.80f, 0.30f, 0.30f, 0.62f)
             .macros (0.62f, 0.873f, 0.34f, 0.26f)
             .topology (Topology::web)
             .nodes ("radius", 0.26f, 0.31f, 0.70f, 0.76f)
             .nodes ("level", 0.66f, 0.66f, 0.62f, 0.62f)
             .motion (0.24f, 0.04f)
             .release (0.08f)
             .fx (1.2f, 0.40f, 0.10f));

    add (Builder ("Cathedral Air", "DRONES", "air, vast, metal, evolving",
                  "Wind through a vast metal structure; turbulence excites a slowly drifting network.", 0xCA7ED201u)
             .air (0.56f, 0.52f, 0.40f)
             .material (metal, 0.55f, 0.40f, 0.22f)
             .macros (0.56f, 0.753f, 0.48f, 0.18f)
             .topology (Topology::web)
             .motion (0.30f, 0.05f)
             .release (0.08f)
             .fx (1.4f, 0.55f));

    // ---------------------------------------------------------------- PLUCKED
    add (Builder ("Satellite String", "PLUCKED", "plucked, metal, orbit, stereo",
                  "Plucked metal strings with a resonator orbiting the core, sweeping the stereo field.", 0x5A7E1101u)
             .pluck (0.18f, 0.08f, 0.64f)
             .material (metal, 0.50f, 0.58f, 0.45f)
             .macros (0.62f, 0.439f, 0.52f, 0.08f)
             .gesture (0, orbitGesture (6.0f, 0.0f, 1.0f, 0.004f))
             .fx (1.35f, 0.24f));

    add (Builder ("Crystal Thread", "PLUCKED", "glass, delicate, harp, bright",
                  "Thin glass threads, plucked; the nodes are tuned to the harmonic series.", 0xC7157401u)
             .pluck (0.30f, 0.04f, 0.74f)
             .material (glass, 0.40f, 0.60f, 0.40f)
             .macros (0.60f, 0.269f, 0.58f, 0.04f)
             .tune (0, 3.0f).tune (1, 4.0f).tune (2, 6.0f).tune (3, 8.0f)
             .fx (1.3f, 0.30f));

    add (Builder ("Wire Garden", "PLUCKED", "plucked, sympathetic, metal, web",
                  "Plucked wires with sympathetic strings; chaos routes energy through the web.", 0x61AE6A01u)
             .pluck (0.24f, 0.12f, 0.56f)
             .material (metal, 0.48f, 0.52f, 0.34f)
             .macros (0.60f, 0.753f, 0.50f, 0.30f)
             .topology (Topology::web)
             .tune (0, 1.5f).tune (1, 2.0f).tune (2, 3.0f).tune (3, 4.0f)
             .motion (0.18f, 0.15f)
             .fx (1.3f, 0.26f));

    add (Builder ("Mercury String", "PLUCKED", "liquid, legato, glide, metal",
                  "A heavy, liquid metal string. Legato playing glides the whole network.", 0x3E7C0101u)
             .pluck (0.20f, 0.05f, 0.50f)
             .material (metal, 0.66f, 0.46f, 0.40f)
             .macros (0.74f, 0.367f, 0.48f, 0.10f)
             .voice (VoiceMode::legato, 0.16f)
             .motion (0.14f, 0.12f)
             .fx (1.1f, 0.20f));

    // ---------------------------------------------------------------- STRUCK
    add (Builder ("Black Bell", "STRUCK", "bell, dark, heavy, metal",
                  "A deep, dark bell struck with a soft mallet: tierce, quint and nominal partials over slow beating.", 0xB1ACBE01u)
             .strike (0.40f, 0.50f, 0.36f)
             .material (metal, 0.75f, 0.36f, 0.30f, 0.62f)
             .macros (0.64f, 0.439f, 0.45f, 0.10f)
             .topology (Topology::web)
             .nodes ("decay", 0.62f, 0.62f, 0.60f, 0.60f)
             .fx (1.2f, 0.30f));

    add (Builder ("Zero Gravity Bell", "STRUCK", "glass, bell, floating, orbit",
                  "A weightless glass bell; two nodes drift in slow orbits around the core.", 0x2E70B001u)
             .strike (0.55f, 0.34f, 0.60f)
             .material (glass, 0.30f, 0.52f, 0.30f)
             .macros (0.60f, 0.367f, 0.52f, 0.08f)
             .gesture (0, orbitGesture (10.0f, 0.0f, 1.0f, 0.004f))
             .gesture (3, orbitGesture (14.0f, 0.0f, -1.0f, 0.004f))
             .fx (1.4f, 0.40f));

    add (Builder ("Cold Plate", "STRUCK", "plate, bright, metallic, hard",
                  "A thin, cold steel plate hit hard: dense, bright partials.", 0xC01D9A01u)
             .strike (0.80f, 0.15f, 0.76f)
             .material (metal, 0.38f, 0.70f, 0.40f, 0.72f)
             .macros (0.62f, 0.556f, 0.60f, 0.06f)
             .topology (Topology::web)
             .nodes ("radius", 0.60f, 0.70f, 0.76f, 0.84f)
             .fx (1.2f, 0.20f));

    add (Builder ("Temple Lattice", "STRUCK", "bell, tuned, ritual, ring",
                  "Tuned metal bowls: the nodes sit on a fifth, an octave, a twelfth and two octaves.", 0x7E3A1E01u)
             .strike (0.50f, 0.34f, 0.50f)
             .material (metal, 0.58f, 0.46f, 0.34f, 0.45f)
             .macros (0.60f, 0.439f, 0.50f, 0.06f)
             .tune (0, 1.5f).tune (1, 2.0f).tune (2, 3.0f).tune (3, 4.0f)
             .fx (1.2f, 0.30f));

    // ---------------------------------------------------------------- BOWED
    add (Builder ("Frozen Wire", "BOWED", "bowed, icy, bright, tension",
                  "A high-tension bowed wire, bright and glassy. Try FREEZE while holding a chord.", 0xF207E101u)
             .bow (0.54f, 0.60f, 0.60f)
             .material (metal, 0.42f, 0.70f, 0.25f)
             .macros (0.58f, 0.269f, 0.72f, 0.08f)
             .topology (Topology::chain)
             .nodes ("radius", 0.56f, 0.62f, 0.68f, 0.74f)
             .fx (1.2f, 0.26f));

    add (Builder ("Tension Bloom", "BOWED", "bowed, wood, expressive, bloom",
                  "A bowed wooden body under rising tension; energy blooms across the coupled nodes.", 0x7B100101u)
             .bow (0.50f, 0.50f, 0.56f)
             .material (wood, 0.55f, 0.50f, 0.40f)
             .macros (0.58f, 0.685f, 0.64f, 0.12f)
             .motion (0.15f, 0.10f)
             .fx (1.15f, 0.24f));

    add (Builder ("Rosin Glass", "BOWED", "glass harmonica, bowed, pure, soft",
                  "Bowed glass bowls: a pure, singing tone with a soft rosin edge.", 0x70516A01u)
             .bow (0.40f, 0.44f, 0.40f)
             .material (glass, 0.48f, 0.46f, 0.34f)
             .macros (0.56f, 0.240f, 0.50f, 0.06f)
             .fx (1.2f, 0.30f));

    add (Builder ("Bowed Timber", "BOWED", "cello, wood, warm, bowed",
                  "A warm, cello-like bowed wooden body.", 0xB0717B01u)
             .bow (0.56f, 0.55f, 0.60f)
             .material (wood, 0.62f, 0.46f, 0.44f)
             .macros (0.60f, 0.333f, 0.50f, 0.06f)
             .fx (1.0f, 0.18f));

    // ---------------------------------------------------------------- AIR
    add (Builder ("Glass Choir", "AIR", "blown, glass, vocal, soft",
                  "Breath across glass: soft, vocal and gently moving.", 0x61A5C401u)
             .air (0.56f, 0.34f, 0.56f)
             .material (glass, 0.46f, 0.52f, 0.36f)
             .macros (0.56f, 0.367f, 0.50f, 0.14f)
             .motion (0.20f, 0.12f)
             .fx (1.25f, 0.40f));

    add (Builder ("Resonant Fog", "AIR", "foggy, breathy, membrane, ambient",
                  "Turbulent air diffused through membranes: a soft, foggy resonance.", 0xF0600101u)
             .air (0.54f, 0.72f, 0.30f)
             .material (membrane, 0.55f, 0.40f, 0.30f)
             .macros (0.62f, 0.556f, 0.46f, 0.24f)
             .topology (Topology::web)
             .motion (0.34f, 0.08f)
             .fx (1.3f, 0.50f));

    add (Builder ("Bottle Organ", "AIR", "blown, pipes, tuned, wood",
                  "A row of blown wooden pipes; the nodes are tuned to harmonic intervals.", 0xB0770E01u)
             .air (0.64f, 0.26f, 0.55f)
             .material (wood, 0.50f, 0.50f, 0.40f)
             .macros (0.58f, 0.211f, 0.50f, 0.04f)
             .tune (0, 2.0f).tune (1, 4.0f).tune (2, 6.0f).tune (3, 8.0f)
             .fx (1.1f, 0.22f));

    add (Builder ("Reed Array", "AIR", "reedy, metal, buzzy, lead",
                  "Air driven hard into a metal network: a buzzy, reed-like lead.", 0x4EED0A01u)
             .air (0.72f, 0.20f, 0.62f)
             .material (metal, 0.50f, 0.56f, 0.44f)
             .macros (0.62f, 0.269f, 0.50f, 0.06f)
             .voice (VoiceMode::poly, 0.08f)
             .fx (1.0f, 0.16f, 0.14f));

    // ---------------------------------------------------------------- GLASS
    add (Builder ("Glass Engine", "GLASS", "glass, rhythmic, synced, motion",
                  "Struck glass inside a machine: tempo-synced motion makes the network pulse.", 0x61A5E601u)
             .strike (0.70f, 0.20f, 0.68f)
             .material (glass, 0.44f, 0.56f, 0.42f)
             .macros (0.62f, 0.556f, 0.55f, 0.06f)
             .synced (half, 0.30f)
             .gesture (1, swayGesture (2.0f, 4.0f, 0.9f, 0.02f))
             .fx (1.25f, 0.22f));

    add (Builder ("Prism Harp", "GLASS", "harp, glass, tuned, sparkling",
                  "Plucked glass tuned to the harmonic series: a sparkling, prismatic harp.", 0x9215A401u)
             .pluck (0.14f, 0.0f, 0.70f)
             .material (glass, 0.42f, 0.56f, 0.38f)
             .macros (0.60f, 0.333f, 0.50f, 0.05f)
             .tune (0, 2.0f).tune (1, 3.0f).tune (2, 4.0f).tune (3, 6.0f)
             .fx (1.35f, 0.30f));

    add (Builder ("Ice Lattice", "GLASS", "glass, shimmer, web, cold",
                  "A web of struck glass resonators; strong coupling makes the partials shimmer.", 0x1CE1A701u)
             .strike (0.62f, 0.24f, 0.62f)
             .material (glass, 0.40f, 0.60f, 0.34f, 0.55f)
             .macros (0.60f, 0.923f, 0.64f, 0.14f)
             .topology (Topology::web)
             .motion (0.20f, 0.15f)
             .fx (1.3f, 0.34f));

    // ---------------------------------------------------------------- METAL
    add (Builder ("Slow Alloy", "METAL", "bowed, metal, slow, beating",
                  "Slowly bowed alloy with gentle beating between detuned nodes.", 0x51A0A101u)
             .bow (0.40f, 0.36f, 0.54f)
             .material (metal, 0.62f, 0.44f, 0.30f, 0.60f)
             .macros (0.56f, 0.556f, 0.50f, 0.22f)
             .motion (0.20f, 0.06f)
             .fx (1.2f, 0.26f));

    add (Builder ("Copper Rain", "METAL", "plucked, bright, cascading, chaos",
                  "Bright copper plucks scattered by chaos and drift, like rain on metal.", 0xC0DDE201u)
             .pluck (0.12f, 0.0f, 0.80f)
             .material (metal, 0.40f, 0.64f, 0.46f)
             .macros (0.62f, 0.367f, 0.54f, 0.36f)
             .topology (Topology::web)
             .motion (0.40f, 0.60f)
             .fx (1.45f, 0.30f));

    add (Builder ("Iron Flower", "METAL", "struck, blooming, synced, iron",
                  "Struck iron whose energy blooms outward while the nodes open and close in tempo.", 0x1F10E201u)
             .strike (0.60f, 0.30f, 0.54f)
             .material (metal, 0.56f, 0.48f, 0.34f)
             .macros (0.62f, 0.974f, 0.50f, 0.10f)
             .topology (Topology::web)
             .synced (bar2, 0.30f)
             .fx (1.2f, 0.22f));

    add (Builder ("Gamelan Engine", "METAL", "gamelan, beating, tuned, synced",
                  "Metal keys paired against slightly detuned partners for a shimmering gamelan beat.", 0x6A3E1A01u)
             .strike (0.62f, 0.26f, 0.52f)
             .material (metal, 0.52f, 0.50f, 0.38f, 0.52f)
             .macros (0.62f, 0.516f, 0.52f, 0.12f)
             .tune (0, 1.5f * 1.0046f).tune (1, 2.0f * 0.9960f).tune (2, 3.0f * 1.0035f).tune (3, 4.0f * 0.9971f)
             .synced (bar1, 0.18f)
             .fx (1.3f, 0.24f));

    // ---------------------------------------------------------------- WOOD
    add (Builder ("Magnetic Wood", "WOOD", "wood, coupled, warm, mallet",
                  "Wooden bars pulled together by strong coupling: warm, round and alive.", 0x3A6E7101u)
             .strike (0.50f, 0.30f, 0.50f)
             .material (wood, 0.50f, 0.50f, 0.40f)
             .macros (0.62f, 0.824f, 0.50f, 0.10f)
             .motion (0.12f, 0.20f)
             .fx (1.15f, 0.18f));

    add (Builder ("Hollow Marimba", "WOOD", "marimba, tuned, mallet, round",
                  "A marimba-like bar: the nodes carry the tuned 1 : 4 : 10 bar overtones.", 0x3A21BA01u)
             .strike (0.46f, 0.36f, 0.48f)
             .material (wood, 0.50f, 0.46f, 0.40f)
             .macros (0.62f, 0.269f, 0.50f, 0.03f)
             .tune (0, 4.0f).tune (1, 10.0f)
             .nodes ("level", 0.80f, 0.70f, 0.30f, 0.25f)
             .fx (1.1f, 0.16f));

    add (Builder ("Timber Code", "WOOD", "kalimba, plucked, synced, pattern",
                  "Plucked wooden tines; synced node motion turns held notes into patterns.", 0x7133C0D1u)
             .pluck (0.30f, 0.22f, 0.50f)
             .material (wood, 0.46f, 0.52f, 0.44f)
             .macros (0.62f, 0.402f, 0.50f, 0.08f)
             .synced (half, 0.28f)
             .gesture (0, swayGesture (2.0f, 4.0f, 1.2f, 0.0f))
             .fx (1.2f, 0.18f));

    add (Builder ("Rosewood Keys", "WOOD", "keys, soft, wood, tuned",
                  "Soft, tuned wooden keys with a gentle, rounded attack.", 0x405E0D01u)
             .strike (0.36f, 0.48f, 0.42f)
             .material (wood, 0.58f, 0.42f, 0.36f)
             .macros (0.60f, 0.269f, 0.50f, 0.04f)
             .tune (0, 2.0f).tune (1, 3.0f).tune (2, 5.0f).tune (3, 8.0f)
             .fx (1.1f, 0.20f));

    // ---------------------------------------------------------------- MEMBRANE
    add (Builder ("Membrane Sky", "MEMBRANE", "membrane, soft, open, ambient",
                  "Soft mallets on wide membranes, ringing into open space.", 0x3E3B5C01u)
             .strike (0.30f, 0.60f, 0.40f)
             .material (membrane, 0.60f, 0.46f, 0.26f)
             .macros (0.62f, 0.556f, 0.46f, 0.10f)
             .motion (0.16f, 0.10f)
             .fx (1.3f, 0.45f));

    add (Builder ("Deep Frame", "MEMBRANE", "frame drum, low, warm, hand",
                  "A deep frame drum: low, warm and bending slightly with the strike.", 0xDEEFF201u)
             .strike (0.36f, 0.46f, 0.36f)
             .material (membrane, 0.70f, 0.40f, 0.52f)
             .macros (0.66f, 0.269f, 0.38f, 0.04f)
             .topology (Topology::star)
             .fx (1.0f, 0.12f));

    add (Builder ("Ghost Membrane", "MEMBRANE", "breath, skin, haunting, air",
                  "Breath through a stretched skin: a haunting, hollow tone.", 0x60573E01u)
             .air (0.50f, 0.60f, 0.36f)
             .material (membrane, 0.52f, 0.44f, 0.34f)
             .macros (0.56f, 0.439f, 0.46f, 0.20f)
             .motion (0.26f, 0.09f)
             .fx (1.25f, 0.44f));

    add (Builder ("Talking Skin", "MEMBRANE", "talking drum, expressive, velocity",
                  "A tight membrane whose pitch rises with strike energy: play harder to make it talk.", 0x7A1C5C01u)
             .strike (0.56f, 0.30f, 0.50f)
             .material (membrane, 0.28f, 0.50f, 0.46f)
             .macros (0.74f, 0.269f, 0.54f, 0.04f)
             .topology (Topology::star)
             .fx (1.0f, 0.12f));

    // ---------------------------------------------------------------- PERCUSSION
    add (Builder ("Ceramic Pulse", "PERCUSSION", "ceramic, short, dry, percussive",
                  "Short, dry ceramic hits with a glassy click.", 0xCE3A1C01u)
             .strike (0.80f, 0.15f, 0.56f)
             .material (glass, 0.34f, 0.46f, 0.76f)
             .macros (0.64f, 0.185f, 0.52f, 0.04f)
             .topology (Topology::star)
             .release (0.40f)
             .fx (1.0f, 0.08f));

    add (Builder ("Tin Pulse", "PERCUSSION", "tin, short, metallic, clank",
                  "Short tin-can clanks: bright and trashy.", 0x71B9A101u)
             .strike (0.76f, 0.14f, 0.62f)
             .material (metal, 0.28f, 0.56f, 0.80f, 0.70f)
             .macros (0.64f, 0.301f, 0.56f, 0.10f)
             .topology (Topology::star)
             .release (0.40f)
             .fx (1.1f, 0.08f));

    add (Builder ("Log Network", "PERCUSSION", "log drum, wood, tuned, round",
                  "Tuned log drums: round wooden tones with nodes on fifths and octaves.", 0x106E7201u)
             .strike (0.50f, 0.30f, 0.46f)
             .material (wood, 0.60f, 0.44f, 0.58f)
             .macros (0.64f, 0.301f, 0.50f, 0.04f)
             .tune (0, 1.5f).tune (1, 3.0f).tune (2, 6.0f).tune (3, 8.0f)
             .fx (1.1f, 0.10f));

    add (Builder ("Kinetic Kit", "PERCUSSION", "kit, chaos, synced, evolving",
                  "A membrane kit in constant motion: every hit lands on a slightly different network.", 0x4E7C1701u)
             .strike (0.60f, 0.20f, 0.50f)
             .material (membrane, 0.40f, 0.50f, 0.60f)
             .macros (0.66f, 0.556f, 0.50f, 0.46f)
             .topology (Topology::web)
             .synced (quarter, 0.50f)
             .fx (1.2f, 0.12f, 0.18f));

    // ---------------------------------------------------------------- EXPERIMENTAL
    add (Builder ("Hollow Circuit", "EXPERIMENTAL", "synthetic, hollow, air, quantised",
                  "Air through a quantised membrane circuit: hollow, synthetic tones that wander.", 0x40C12C01u)
             .air (0.68f, 0.30f, 0.42f)
             .material (membrane, 0.50f, 0.52f, 0.40f)
             .macros (0.58f, 0.777f, 0.60f, 0.44f)
             .topology (Topology::chain)
             .quantise()
             .nodes ("radius", 0.25f, 0.75f, 0.33f, 0.67f)
             .motion (0.30f, 0.50f)
             .fx (1.3f, 0.26f, 0.14f));

    add (Builder ("Broken Halo", "EXPERIMENTAL", "glass, chaos, shattered, bells",
                  "Shattered glass bells: high chaos breaks the halo of partials apart.", 0xB70CE401u)
             .strike (0.74f, 0.20f, 0.64f)
             .material (glass, 0.42f, 0.56f, 0.36f, 0.76f)
             .macros (0.62f, 0.873f, 0.56f, 0.72f)
             .topology (Topology::web)
             .motion (0.34f, 0.30f)
             .fx (1.4f, 0.36f, 0.08f));

    add (Builder ("Entropy Garden", "EXPERIMENTAL", "chaos, plucked, glass, unstable",
                  "Plucked glass at the edge of order: every note grows differently.", 0xE7A06A01u)
             .pluck (0.36f, 0.10f, 0.60f)
             .material (glass, 0.44f, 0.54f, 0.40f)
             .macros (0.60f, 0.685f, 0.52f, 0.86f)
             .topology (Topology::web)
             .motion (0.60f, 1.20f)
             .fx (1.35f, 0.30f));

    add (Builder ("Signal Swarm", "EXPERIMENTAL", "swarm, air, metal, noisy",
                  "Air forced through a chained metal network with heavy chaos: a buzzing swarm.", 0x5163A101u)
             .air (0.74f, 0.60f, 0.70f)
             .material (metal, 0.46f, 0.58f, 0.46f)
             .macros (0.60f, 1.000f, 0.54f, 0.58f)
             .topology (Topology::chain)
             .motion (0.50f, 0.80f)
             .fx (1.2f, 0.24f, 0.24f));

    // The library: every category's full collection (the signature presets above open
    // each category).
    addPads (v);
    addPlucked (v);
    addStruck (v);
    addBowed (v);
    addAir (v);
    addGlass (v);
    addMetal (v);
    addWood (v);
    addMembrane (v);
    addDrones (v);
    addPercussion (v);
    addExperimental (v);

    applyCalibration (v);

    // Display order = category order (stable: signature presets first).
    const auto& order = categories();
    std::stable_sort (v.begin(), v.end(), [&order] (const PresetDefinition& a, const PresetDefinition& b)
                      {
                          auto rank = [&order] (const std::string& c)
                          { return std::distance (order.begin(), std::find (order.begin(), order.end(), c)); };
                          return rank (a.category) < rank (b.category);
                      });
    return v;
}
} // namespace

const std::vector<std::string>& categories()
{
    static const std::vector<std::string> c { "PADS", "PLUCKED", "STRUCK", "BOWED", "AIR", "GLASS", "METAL",
                                              "WOOD", "MEMBRANE", "DRONES", "PERCUSSION", "EXPERIMENTAL" };
    return c;
}

const std::vector<PresetDefinition>& factoryPresets()
{
    static const std::vector<PresetDefinition> presets = build();
    return presets;
}

} // namespace arc::presets

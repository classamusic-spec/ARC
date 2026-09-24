// Phase 4 — exciter quality gate: STRIKE, PLUCK, BOW, AIR behave as genuinely
// different physical mechanisms; velocity and EXCITE act per exciter type.

#include "ArcTest.h"
#include "VoiceRig.h"

using namespace arctest;

namespace
{
constexpr double kSr = 48000.0;
constexpr double kC4 = 261.6256;

RigSettings rig (arc::ExciterType e, arc::MaterialType m = arc::MaterialType::wood)
{
    auto s = defaultRig();
    s.exciter = e;
    s.material = m;
    return s;
}

double rmsDb (const Signal& x, double t0, double len)
{
    return 20.0 * std::log10 (rms (x, static_cast<int> (t0 * kSr), static_cast<int> (len * kSr)) + 1e-12);
}
} // namespace

TEST_CASE ("exciters", "strike is a transient impact")
{
    for (auto m : { arc::MaterialType::glass, arc::MaterialType::metal, arc::MaterialType::wood, arc::MaterialType::membrane })
    {
        auto s = rig (arc::ExciterType::strike, m);
        s.releaseDamping = 0.0f;
        const auto r = renderVoice (s, 60, 0.8f, 3.0, 3.0, kSr);
        // Energy is front-loaded and then decays freely: no sustained drive.
        const double early = rmsDb (r.mono, 0.0, 0.1), late = rmsDb (r.mono, 1.5, 0.3);
        MEASURE (std::string (materialName (m)) + ".earlyMinusLateDb", early - late);
        CHECK (early - late > 6.0);
        CHECK (allFinite (r.mono));
    }
    // Hardness and contact length shape the spectrum; velocity hardens and raises level.
    auto centroid = [] (float hardness, float length, float velocity)
    {
        auto s = rig (arc::ExciterType::strike, arc::MaterialType::metal);
        s.ex.strikeHardness = hardness;
        s.ex.strikeLength = length;
        const auto r = renderVoice (s, 60, velocity, 0.5, 0.5, kSr);
        return std::make_pair (spectralCentroid (r.mono, kSr, 0, static_cast<int> (0.1 * kSr)), rms (r.mono, 0, 4800));
    };
    const auto soft = centroid (0.1f, 0.35f, 0.8f), hard = centroid (0.95f, 0.35f, 0.8f);
    const auto longC = centroid (0.55f, 0.9f, 0.8f), shortC = centroid (0.55f, 0.05f, 0.8f);
    const auto quiet = centroid (0.55f, 0.35f, 0.3f), loud = centroid (0.55f, 0.35f, 1.0f);
    MEASURE ("centroid.soft", soft.first);
    MEASURE ("centroid.hard", hard.first);
    MEASURE ("centroid.longContact", longC.first);
    MEASURE ("centroid.shortContact", shortC.first);
    MEASURE ("rms.velocity0.3", quiet.second);
    MEASURE ("rms.velocity1.0", loud.second);
    MEASURE ("centroid.velocity0.3", quiet.first);
    MEASURE ("centroid.velocity1.0", loud.first);
    CHECK (hard.first > soft.first * 1.15);
    CHECK (shortC.first > longC.first * 1.15);
    CHECK (loud.second > quiet.second * 2.0);
    CHECK (loud.first > quiet.first);
}

TEST_CASE ("exciters", "pluck position comb")
{
    // Harmonic CORE (wood, no coupling, nodes silent). The Karplus-style burst gives
    // every note random harmonic weights, so spectra are averaged over 12 plucks.
    // Plucking at 1/4 of the length puts a node on harmonics 4 and 8.
    const double f0 = arc::dsp::midiToHz (48);
    auto averagedHarmonicsDb = [&] (float position)
    {
        std::array<double, 10> acc {};
        for (uint32_t seed = 1; seed <= 12; ++seed)
        {
            auto s = rig (arc::ExciterType::pluck, arc::MaterialType::wood);
            s.coupling = 0.0f;
            for (auto& n : s.nodes)
                n.level = 0.0f;
            s.ex.pluckPosition = position;
            s.ex.pluckTone = 0.9f;
            const auto r = renderVoice (s, 48, 0.8f, 1.0, 1.0, kSr, seed * 7919u);
            const auto spec = computeSpectrum (r.mono, kSr, 1 << 17, 0, static_cast<int> (0.5 * kSr));
            for (int k = 1; k < 10; ++k)
            {
                double m = 0;
                peakFrequency (spec, k * f0 * 0.98, k * f0 * 1.02, &m);
                acc[static_cast<size_t> (k)] += m * m;
            }
        }
        std::array<double, 10> db {};
        for (int k = 1; k < 10; ++k)
            db[static_cast<size_t> (k)] = 10.0 * std::log10 (acc[static_cast<size_t> (k)] / 12.0 + 1e-24);
        return db;
    };
    const auto quarter = averagedHarmonicsDb (0.25f);
    for (int k : { 3, 4, 5, 7, 8, 9 })
        MEASURE ("pos0.25.h" + std::to_string (k) + "dB", quarter[static_cast<size_t> (k)]);
    CHECK (quarter[4] < 0.5 * (quarter[3] + quarter[5]) - 12.0);
    CHECK (quarter[8] < 0.5 * (quarter[7] + quarter[9]) - 12.0);

    const auto half = averagedHarmonicsDb (0.5f);
    for (int k : { 1, 2, 3, 4 })
        MEASURE ("pos0.5.h" + std::to_string (k) + "dB", half[static_cast<size_t> (k)]);
    CHECK (half[2] < 0.5 * (half[1] + half[3]) - 12.0);
    CHECK (half[4] < 0.5 * (half[3] + half[5]) - 12.0);

    // TONE brightens the pluck.
    auto cent = [&] (float tone)
    {
        auto t = rig (arc::ExciterType::pluck, arc::MaterialType::wood);
        t.ex.pluckTone = tone;
        const auto rr = renderVoice (t, 48, 0.8f, 0.5, 0.5, kSr);
        return spectralCentroid (rr.mono, kSr, 0, static_cast<int> (0.2 * kSr));
    };
    const double dark = cent (0.1f), bright = cent (0.95f);
    MEASURE ("centroid.tone0.1", dark);
    MEASURE ("centroid.tone0.95", bright);
    CHECK (bright > dark * 1.3);
}

TEST_CASE ("exciters", "bow is sustained friction not noise")
{
    for (auto m : { arc::MaterialType::glass, arc::MaterialType::metal, arc::MaterialType::wood, arc::MaterialType::membrane })
    {
        auto s = rig (arc::ExciterType::bow, m);
        const auto r = renderVoice (s, 55, 0.8f, 2.5, 4.0, kSr);
        const double f0 = arc::dsp::midiToHz (55);
        const std::string tag = materialName (m);
        // Sustained: level steady while bowing.
        const double a = rmsDb (r.mono, 0.8, 0.4), b = rmsDb (r.mono, 1.8, 0.4);
        // Periodic at f0 (self-oscillation of the network), harmonic, not noise-like.
        const auto spec = computeSpectrum (r.mono, kSr, 1 << 17, static_cast<int> (1.0 * kSr), static_cast<int> (1.2 * kSr));
        const double hnr = harmonicToNoiseDb (spec, f0, 12);
        const double flat = spectralFlatness (spec, 100.0, 6000.0);
        Signal seg (r.mono.begin() + static_cast<long> (1.0 * kSr), r.mono.begin() + static_cast<long> (2.4 * kSr));
        const double cents = centsBetween (refineFrequencyByPhase (seg, kSr, f0), f0);
        // Release: bow lifts, body decays.
        const double after = rmsDb (r.mono, 3.6, 0.3);
        MEASURE (tag + ".levelDriftDb", b - a);
        MEASURE (tag + ".hnrDb", hnr);
        MEASURE (tag + ".flatness", flat);
        MEASURE (tag + ".pitchCents", cents);
        MEASURE (tag + ".releaseDropDb", b - after);
        CHECK (std::abs (b - a) < 2.0);
        CHECK (hnr > 10.0);  // periodic tone (METAL's inharmonic nodes ring under the bow: ~13 dB)
        CHECK (flat < 0.05); // not noise
        CHECK (std::abs (cents) < 3.0);
        // After release the body rings on with its own decay: short-decay materials fall
        // quickly, long-ringing ones (glass, metal) keep sounding, but always fall.
        const bool longRinging = m == arc::MaterialType::glass || m == arc::MaterialType::metal;
        CHECK (b - after > (longRinging ? 3.0 : 20.0));
    }
    // PRESSURE changes the tone colour of the bowed wood.
    auto cent = [&] (float pressure)
    {
        auto s = rig (arc::ExciterType::bow, arc::MaterialType::wood);
        s.ex.bowPressure = pressure;
        const auto r = renderVoice (s, 55, 0.8f, 2.0, 2.0, kSr);
        return spectralCentroid (r.mono, kSr, static_cast<int> (1.0 * kSr), static_cast<int> (0.8 * kSr));
    };
    const double lo = cent (0.1f), hi = cent (0.9f);
    MEASURE ("wood.centroid.pressure0.1", lo);
    MEASURE ("wood.centroid.pressure0.9", hi);
    CHECK (std::abs (hi - lo) / lo > 0.05);
}

TEST_CASE ("exciters", "air is continuous turbulent excitation")
{
    for (auto m : { arc::MaterialType::glass, arc::MaterialType::metal, arc::MaterialType::wood, arc::MaterialType::membrane })
    {
        const std::string tag = materialName (m);
        auto breathy = rig (arc::ExciterType::air, m);
        breathy.ex.airFlow = 0.15f;
        breathy.ex.airTurbulence = 0.9f;
        auto tonal = rig (arc::ExciterType::air, m);
        tonal.ex.airFlow = 0.85f;
        tonal.ex.airTurbulence = 0.2f;
        const auto rb = renderVoice (breathy, 60, 0.8f, 2.0, 3.0, kSr);
        const auto rt = renderVoice (tonal, 60, 0.8f, 2.0, 3.0, kSr);
        const auto sb = computeSpectrum (rb.mono, kSr, 1 << 16, static_cast<int> (1.0 * kSr), static_cast<int> (0.8 * kSr));
        const auto st = computeSpectrum (rt.mono, kSr, 1 << 16, static_cast<int> (1.0 * kSr), static_cast<int> (0.8 * kSr));
        const double flatB = spectralFlatness (sb, 100.0, 8000.0), flatT = spectralFlatness (st, 100.0, 8000.0);
        const double sustain = rmsDb (rt.mono, 1.0, 0.8), after = rmsDb (rt.mono, 2.7, 0.3);
        MEASURE (tag + ".flatness.breathy", flatB);
        MEASURE (tag + ".flatness.tonal", flatT);
        MEASURE (tag + ".releaseDropDb", sustain - after);
        // Turbulence makes it breathier (flatter spectrum); flow makes it tonal.
        CHECK (flatB > flatT * 1.5);
        CHECK (sustain > -40.0);
        const bool longRinging = m == arc::MaterialType::glass || m == arc::MaterialType::metal;
        CHECK (sustain - after > (longRinging ? 3.0 : 20.0));
        CHECK (allFinite (rt.mono) && allFinite (rb.mono));
    }
}

TEST_CASE ("exciters", "velocity and EXCITE act per exciter")
{
    for (int e = 0; e < 4; ++e)
    {
        const auto type = static_cast<arc::ExciterType> (e);
        const bool sustained = e >= 2;
        const std::string tag = exciterName (type);
        double prev = -200.0;
        bool monotonic = true;
        for (float v : { 0.2f, 0.5f, 0.8f, 1.0f })
        {
            const auto r = renderVoice (rig (type, arc::MaterialType::metal), 60, v, 1.2, 1.2, kSr);
            const double lvl = sustained ? rmsDb (r.mono, 0.6, 0.5) : rmsDb (r.mono, 0.0, 0.3);
            MEASURE (tag + ".velocity" + std::to_string (v).substr (0, 3) + ".rmsDb", lvl);
            monotonic = monotonic && lvl > prev;
            prev = lvl;
        }
        CHECK_MSG (monotonic, tag);

        auto s0 = rig (type, arc::MaterialType::metal), s1 = rig (type, arc::MaterialType::metal);
        s0.excite = 0.1f;
        s1.excite = 1.0f;
        const auto r0 = renderVoice (s0, 60, 0.8f, 1.2, 1.2, kSr), r1 = renderVoice (s1, 60, 0.8f, 1.2, 1.2, kSr);
        const double l0 = sustained ? rmsDb (r0.mono, 0.6, 0.5) : rmsDb (r0.mono, 0.0, 0.3);
        const double l1 = sustained ? rmsDb (r1.mono, 0.6, 0.5) : rmsDb (r1.mono, 0.0, 0.3);
        const double c0 = spectralCentroid (r0.mono, kSr, 0, static_cast<int> (0.3 * kSr));
        const double c1 = spectralCentroid (r1.mono, kSr, 0, static_cast<int> (0.3 * kSr));
        MEASURE (tag + ".excite.levelGainDb", l1 - l0);
        MEASURE (tag + ".excite.centroidRatio", c1 / c0);
        CHECK (l1 > l0 + 3.0);
        if (type == arc::ExciterType::strike || type == arc::ExciterType::pluck)
            CHECK (c1 > c0); // transient exciters also get brighter/harder
    }
}

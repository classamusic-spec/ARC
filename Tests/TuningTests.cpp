// Phase 5 — TENSION / tuning quality gate.

#include "ArcTest.h"
#include "VoiceRig.h"

using namespace arctest;

namespace
{
constexpr double kSr = 48000.0;

/** Settled fundamental (cents vs the MIDI note) measured in [t0, t0 + len). */
double settledCents (const RigResult& r, int note, double t0, double len)
{
    const double f0 = arc::dsp::midiToHz (note);
    const auto a = r.mono.begin() + static_cast<long> (t0 * kSr);
    Signal seg (a, a + static_cast<long> (len * kSr));
    const double coarse = peakFrequency (seg, kSr, f0 * 0.96, f0 * 1.04);
    return centsBetween (refineFrequencyByPhase (seg, kSr, coarse > 0 ? coarse : f0), f0);
}

/** Frequency of the strongest partial within [lo, hi] x f0. */
double partialRatio (const RigResult& r, int note, double lo, double hi)
{
    const double f0 = arc::dsp::midiToHz (note);
    const auto spec = computeSpectrum (r.mono, kSr, 1 << 18, 0, static_cast<int> (1.5 * kSr));
    return peakFrequency (spec, f0 * lo, f0 * hi) / f0;
}
} // namespace

TEST_CASE ("tuning", "tension is not master pitch")
{
    // Musical range T in [0.2, 0.8]: < 2 cents. The extremes (0, 1) stretch nodes to
    // 0.6x..43x f0; loops a few samples long then pull the CORE's series (documented
    // extreme behaviour): < 10 cents.
    double worst = 0, worstExtreme = 0;
    for (auto m : { arc::MaterialType::glass, arc::MaterialType::metal, arc::MaterialType::wood })
        for (float t : { 0.0f, 0.2f, 0.35f, 0.5f, 0.65f, 0.8f, 1.0f })
        {
            auto s = defaultRig();
            s.material = m;
            s.tension = t;
            s.releaseDamping = 0.0f;
            const auto r = renderVoice (s, 60, 0.8f, 2.5, 2.5, kSr);
            const double c = settledCents (r, 60, 0.4, 1.8);
            const bool extreme = t == 0.0f || t == 1.0f;
            (extreme ? worstExtreme : worst) = std::max (extreme ? worstExtreme : worst, std::abs (c));
            CHECK_MSG (std::abs (c) < (extreme ? 10.0 : 2.0), materialName (m) << " T=" << t << " cents=" << c);
        }
    MEASURE ("worstFundamentalCents_T0.2_0.8", worst);
    MEASURE ("worstFundamentalCents_T0_or_1", worstExtreme);
}

TEST_CASE ("tuning", "tension stretches the modal spacing")
{
    // METAL node A (tierce, 1.19 at neutral tension) follows ratio^(1 + 0.9 (T - 0.5)).
    double prev = 0;
    for (float t : { 0.1f, 0.3f, 0.5f, 0.7f, 0.9f })
    {
        auto s = defaultRig();
        s.material = arc::MaterialType::metal;
        s.tension = t;
        s.coupling = 0.0f;
        s.releaseDamping = 0.0f;
        const auto r = renderVoice (s, 57, 0.8f, 2.0, 2.0, kSr);
        const double expected = std::pow (1.19, 1.0 + 0.9 * (t - 0.5));
        const double measured = partialRatio (r, 57, expected * 0.97, expected * 1.03);
        MEASURE ("tension" + std::to_string (t).substr (0, 3) + ".nodeA_ratio", measured);
        CHECK_MSG (std::abs (measured / expected - 1.0) < 0.01, "T=" << t << " expected " << expected << " got " << measured);
        CHECK (measured > prev);
        prev = measured;
    }
}

TEST_CASE ("tuning", "pitch tracks MIDI C1-C6")
{
    double worst = 0;
    for (auto combo : { std::make_pair (arc::ExciterType::strike, arc::MaterialType::glass),
                        std::make_pair (arc::ExciterType::strike, arc::MaterialType::metal),
                        std::make_pair (arc::ExciterType::strike, arc::MaterialType::wood),
                        std::make_pair (arc::ExciterType::pluck, arc::MaterialType::wood),
                        std::make_pair (arc::ExciterType::bow, arc::MaterialType::wood),
                        std::make_pair (arc::ExciterType::air, arc::MaterialType::glass) })
    {
        for (int note : { 24, 36, 48, 60, 72, 84 })
        {
            auto s = defaultRig();
            s.exciter = combo.first;
            s.material = combo.second;
            s.releaseDamping = 0.0f;
            const double len = note <= 36 ? 3.0 : 2.0;
            const auto r = renderVoice (s, note, 0.8f, len, len, kSr);
            const double c = settledCents (r, note, 0.5, len - 0.6);
            worst = std::max (worst, std::abs (c));
            CHECK_MSG (std::abs (c) < 3.0, exciterName (combo.first) << "." << materialName (combo.second) << " note "
                                                                   << note << " cents " << c);
        }
    }
    MEASURE ("worstCents_C1_C6", worst);
}

TEST_CASE ("tuning", "membrane pitch glide settles")
{
    // MEMBRANE rises with energy (tension modulation) and settles on the note.
    auto s = defaultRig();
    s.material = arc::MaterialType::membrane;
    s.releaseDamping = 0.0f;
    const auto r = renderVoice (s, 48, 1.0f, 3.0, 3.0, kSr);
    const double early = settledCents (r, 48, 0.0, 0.08);
    const double late = settledCents (r, 48, 0.6, 1.0);
    MEASURE ("membrane.early_cents", early);
    MEASURE ("membrane.settled_cents", late);
    CHECK (early > late + 5.0);
    CHECK (std::abs (late) < 4.0);
}

TEST_CASE ("tuning", "chromatic intervals")
{
    double worst = 0;
    for (int note = 48; note <= 72; ++note)
    {
        auto s = defaultRig();
        s.exciter = arc::ExciterType::pluck;
        s.material = arc::MaterialType::wood;
        s.releaseDamping = 0.0f;
        const auto r = renderVoice (s, note, 0.8f, 1.6, 1.6, kSr);
        worst = std::max (worst, std::abs (settledCents (r, note, 0.3, 1.2)));
    }
    MEASURE ("worstCents_C3_C5_pluckWood", worst);
    CHECK (worst < 2.0);
}

TEST_CASE ("tuning", "node radius and quantise")
{
    // Radius maps to +-1 octave around the material ratio; QUANTISE snaps to semitones.
    struct Case
    {
        float radius;
        bool quantise;
        double factor;
    };
    const Case cases[] = { { 1.0f, false, 2.0 }, { 0.0f, false, 0.5 }, { 0.75f, false, std::exp2 (0.5) },
                           { 0.54f, true, std::exp2 (1.0 / 12.0) }, { 0.46f, true, std::exp2 (-1.0 / 12.0) } };
    for (const auto& c : cases)
    {
        auto s = defaultRig();
        s.material = arc::MaterialType::metal;
        s.coupling = 0.0f;
        s.quantise = c.quantise;
        s.nodes[0].radius = c.radius;
        s.releaseDamping = 0.0f;
        const auto r = renderVoice (s, 55, 0.8f, 2.0, 2.0, kSr);
        const double expected = 1.19 * c.factor;
        const double measured = partialRatio (r, 55, expected * 0.985, expected * 1.015);
        MEASURE ("radius" + std::to_string (c.radius).substr (0, 4) + (c.quantise ? "q" : "") + ".ratio", measured);
        CHECK_MSG (std::abs (measured / expected - 1.0) < 0.004, "radius " << c.radius << " expected " << expected << " got " << measured);
    }
}

TEST_CASE ("tuning", "coupling compensation keeps pitch")
{
    for (auto m : { arc::MaterialType::glass, arc::MaterialType::metal, arc::MaterialType::wood })
        for (float coupling : { 0.2f, 0.35f, 0.5f })
        {
            double cents[2];
            for (int comp = 0; comp < 2; ++comp)
            {
                auto s = defaultRig();
                s.material = m;
                s.coupling = coupling;
                s.compensation = comp == 1;
                s.releaseDamping = 0.0f;
                const auto r = renderVoice (s, 60, 0.8f, 2.5, 2.5, kSr);
                cents[comp] = settledCents (r, 60, 0.4, 1.8);
            }
            const std::string tag = std::string (materialName (m)) + ".coupling" + std::to_string (coupling).substr (0, 4);
            MEASURE (tag + ".uncompensatedCents", cents[0]);
            MEASURE (tag + ".compensatedCents", cents[1]);
            CHECK_MSG (std::abs (cents[1]) < 1.5, tag << " " << cents[1]);
        }
}

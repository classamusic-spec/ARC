// Phases 8-10 — motion, gestures, chaos, nonlinear behaviour, FREEZE.

#include "Analysis.h"
#include "ArcTest.h"

#include "Engine/ArcEngine.h"

using namespace arctest;

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 256;

struct Rendered
{
    Signal mono;
    std::vector<float> radiusA; // telemetry per block
};

/** Renders `seconds`; `script(engine, t)` runs before each block. */
template <typename Script>
Rendered run (arc::ArcEngine& e, double seconds, Script&& script)
{
    Rendered r;
    const int blocks = static_cast<int> (seconds * kSr / kBlock);
    std::vector<float> l (kBlock), rr (kBlock);
    for (int b = 0; b < blocks; ++b)
    {
        script (e, b * kBlock / kSr);
        e.render (l.data(), rr.data(), kBlock);
        for (int i = 0; i < kBlock; ++i)
            r.mono.push_back (0.5f * (l[static_cast<size_t> (i)] + rr[static_cast<size_t> (i)]));
        r.radiusA.push_back (arc::Telemetry::load (e.getTelemetry().nodeRadius[0]));
    }
    return r;
}

arc::EngineParams base()
{
    arc::EngineParams p;
    p.space = 0.0f;
    p.masterGainDb = 0.0f;
    p.chaos = 0.0f;
    return p;
}

double levelDb (const Signal& x, double t, double len = 0.2)
{
    return 20.0 * std::log10 (rms (x, static_cast<int> (t * kSr), static_cast<int> (len * kSr)) + 1e-12);
}
} // namespace

TEST_CASE ("freeze", "freeze holds energy without runaway")
{
    struct Case
    {
        const char* name;
        float chaos, motion;
        arc::MaterialType material;
    };
    for (const auto& c : { Case { "metal.static", 0.0f, 0.0f, arc::MaterialType::metal },
                           Case { "glass.chaos1.motion1", 1.0f, 1.0f, arc::MaterialType::glass },
                           Case { "wood.chaos0.5", 0.5f, 0.3f, arc::MaterialType::wood } })
    {
        arc::ArcEngine e;
        auto p = base();
        p.material = c.material;
        p.chaos = c.chaos;
        p.motionDepth = c.motion;
        p.motionRateHz = 1.5f;
        e.setParameters (p);
        e.prepare (kSr, kBlock);
        const auto r = run (e, 20.0, [&] (arc::ArcEngine& en, double t)
                           {
                               if (t == 0.0)
                                   for (int n : { 48, 55, 60 })
                                       en.noteOn (1, n, 0.9f);
                               if (t >= 0.25 && ! p.freeze)
                               {
                                   p.freeze = true;
                                   en.setParameters (p);
                               }
                           });
        const double l2 = levelDb (r.mono, 2.0, 1.0), l10 = levelDb (r.mono, 10.0, 1.0), l19 = levelDb (r.mono, 18.9, 1.0);
        MEASURE (std::string (c.name) + ".levelDb@2s", l2);
        MEASURE (std::string (c.name) + ".levelDb@10s", l10);
        MEASURE (std::string (c.name) + ".levelDb@19s", l19);
        CHECK (allFinite (r.mono));
        CHECK (l19 < l2 + 1.5);   // no slow runaway (governor)
        CHECK (l19 > l2 - 6.0);   // energy is actually held (vs >60 dB of normal decay)
        CHECK (peakAbs (r.mono) <= 1.0);
    }
}

TEST_CASE ("freeze", "freeze engage is smooth and new notes play normally")
{
    arc::ArcEngine e;
    auto p = base();
    p.material = arc::MaterialType::glass;
    e.setParameters (p);
    e.prepare (kSr, kBlock);
    const double engageAt = 0.5;
    const auto r = run (e, 6.0, [&] (arc::ArcEngine& en, double t)
                       {
                           if (t == 0.0)
                               en.noteOn (1, 57, 0.9f);
                           if (t >= engageAt && ! p.freeze)
                           {
                               p.freeze = true;
                               en.setParameters (p);
                           }
                           if (std::abs (t - 2.0) < 0.5 * kBlock / kSr)
                               en.noteOn (1, 76, 0.9f); // new note while frozen
                       });
    // Smooth engage: no impulsive HF event around the switch.
    const int frame = static_cast<int> (0.005 * kSr);
    std::vector<double> hf;
    for (int i = static_cast<int> ((engageAt - 0.05) * kSr); i + frame < static_cast<int> ((engageAt + 0.1) * kSr); i += frame)
        hf.push_back (highBandRatioDb (r.mono, kSr, 12000.0, i, frame) + 20.0 * std::log10 (rms (r.mono, i, frame) + 1e-12));
    double worst = -1e9;
    for (size_t k = 3; k + 3 < hf.size(); ++k)
        worst = std::max (worst, hf[k] - std::max ((hf[k - 1] + hf[k - 2] + hf[k - 3]) / 3.0, (hf[k + 1] + hf[k + 2] + hf[k + 3]) / 3.0));
    MEASURE ("engage.worstHfSpikeDb", worst);
    CHECK (worst < 12.0);
    // The note played during FREEZE is not captured: it decays, the frozen one holds.
    const double f76 = arc::dsp::midiToHz (76), f57 = arc::dsp::midiToHz (57);
    const double t76a = measureT60 (Signal (r.mono.begin() + static_cast<long> (2.0 * kSr), r.mono.end()), kSr, f76, 5.0, 25.0, 4.0);
    MEASURE ("noteDuringFreeze.t60", t76a);
    CHECK (t76a > 0.0 && t76a < 20.0);
    const auto spec1 = computeSpectrum (r.mono, kSr, 1 << 16, static_cast<int> (1.0 * kSr), static_cast<int> (0.5 * kSr));
    const auto spec2 = computeSpectrum (r.mono, kSr, 1 << 16, static_cast<int> (5.3 * kSr), static_cast<int> (0.5 * kSr));
    double m1 = 0, m2 = 0;
    peakFrequency (spec1, f57 * 0.98, f57 * 1.02, &m1);
    peakFrequency (spec2, f57 * 0.98, f57 * 1.02, &m2);
    MEASURE ("frozenNote.levelChangeDb_1s_to_5.3s", 20.0 * std::log10 (m2 / m1));
    CHECK (std::abs (20.0 * std::log10 (m2 / m1)) < 3.0);
}

TEST_CASE ("chaos", "zero chaos is repeatable, seeds are deterministic, extreme is bounded")
{
    auto render = [] (float chaosAmount, uint32_t seed)
    {
        arc::ArcEngine e;
        auto p = base();
        p.chaos = chaosAmount;
        p.material = arc::MaterialType::metal;
        e.setParameters (p);
        e.prepare (kSr, kBlock);
        e.postSeed (seed);
        return run (e, 2.0, [] (arc::ArcEngine& en, double t)
                    {
                        if (t == 0.0)
                            en.noteOn (1, 60, 0.8f);
                    })
            .mono;
    };
    auto maxDiff = [] (const Signal& a, const Signal& b)
    {
        double d = 0;
        for (size_t i = 0; i < a.size(); ++i)
            d = std::max (d, static_cast<double> (std::abs (a[i] - b[i])));
        return d;
    };
    const auto z1 = render (0.0f, 1), z2 = render (0.0f, 99);
    MEASURE ("chaos0.maxDiff_differentSeeds", maxDiff (z1, z2));
    CHECK (maxDiff (z1, z2) == 0.0); // no randomness at all at CHAOS 0
    const auto a1 = render (0.8f, 7), a2 = render (0.8f, 7), b = render (0.8f, 8);
    MEASURE ("chaos0.8.maxDiff_sameSeed", maxDiff (a1, a2));
    MEASURE ("chaos0.8.maxDiff_otherSeed", maxDiff (a1, b));
    CHECK (maxDiff (a1, a2) == 0.0);
    CHECK (maxDiff (a1, b) > 1.0e-4);
    const auto x = render (1.0f, 3);
    MEASURE ("chaos1.peak", peakAbs (x));
    CHECK (allFinite (x));
    CHECK (peakAbs (x) <= 1.0);
}

TEST_CASE ("motion", "motion moves the network and follows tempo when synced")
{
    arc::ArcEngine e;
    auto p = base();
    p.motionDepth = 1.0f;
    p.motionRateHz = 0.5f;
    e.setParameters (p);
    e.prepare (kSr, kBlock);
    const auto r = run (e, 8.0, [] (arc::ArcEngine& en, double t)
                       {
                           if (t == 0.0)
                               en.noteOn (1, 60, 0.8f);
                       });
    const auto [mn, mx] = std::minmax_element (r.radiusA.begin(), r.radiusA.end());
    MEASURE ("depth1.radiusA_range", *mx - *mn);
    CHECK (*mx - *mn > 0.08);

    arc::ArcEngine s;
    auto q = base();
    q.motionDepth = 0.0f;
    s.setParameters (q);
    s.prepare (kSr, kBlock);
    const auto rs = run (s, 2.0, [] (arc::ArcEngine&, double) {});
    const auto [smn, smx] = std::minmax_element (rs.radiusA.begin(), rs.radiusA.end());
    MEASURE ("depth0.radiusA_range", *smx - *smn);
    CHECK (*smx - *smn == 0.0f);
}

TEST_CASE ("motion", "synced drift is phase-locked to the host timeline")
{
    // SYNC + a playing host: the drift is a function of the song position, so the same
    // bar always moves the network the same way (and a relocated playhead lands on the
    // same shape). 1-bar cycle at 120 bpm = 2 s; the pattern repeats after 4 cycles.
    auto render = [] (double startPpq)
    {
        arc::ArcEngine e;
        auto p = base();
        p.motionDepth = 1.0f;
        p.sync = true;
        p.motionDivision = arc::SyncDivision::bar1;
        e.setParameters (p);
        e.prepare (kSr, kBlock);
        std::vector<float> radius;
        std::vector<float> l (kBlock), r (kBlock);
        for (int b = 0; b < static_cast<int> (4.0 * kSr / kBlock); ++b)
        {
            arc::TransportInfo t;
            t.valid = t.playing = true;
            t.bpm = 120.0;
            t.ppqPosition = startPpq + b * kBlock / kSr * 2.0;
            e.setTransport (t);
            e.render (l.data(), r.data(), kBlock);
            radius.push_back (arc::Telemetry::load (e.getTelemetry().nodeRadius[1]));
        }
        return radius;
    };
    const auto a = render (0.0);
    const auto b = render (16.0); // 4 bars later: same position in the 4-cycle pattern
    const auto c = render (2.0);  // half a bar later: different shape
    double sameDiff = 0, otherDiff = 0, range = 0;
    const auto [mn, mx] = std::minmax_element (a.begin(), a.end());
    range = *mx - *mn;
    // Skip the first 0.5 s (offset smoothing settles from rest).
    for (size_t i = static_cast<size_t> (0.5 * kSr / kBlock); i < a.size(); ++i)
    {
        sameDiff = std::max (sameDiff, static_cast<double> (std::abs (a[i] - b[i])));
        otherDiff = std::max (otherDiff, static_cast<double> (std::abs (a[i] - c[i])));
    }
    MEASURE ("radiusRange", range);
    MEASURE ("maxDiff_sameBarPosition", sameDiff);
    MEASURE ("maxDiff_halfBarLater", otherDiff);
    CHECK (range > 0.05);
    CHECK (sameDiff < 1.0e-3);
    CHECK (otherDiff > 0.02);
}

TEST_CASE ("gesture", "gesture build, serialise and playback")
{
    // A 1.3 s circular drag of node A.
    std::vector<arc::GestureSample> raw;
    for (int k = 0; k <= 130; ++k)
    {
        const double t = k * 0.01;
        raw.push_back ({ t, static_cast<float> (0.5 + 0.2 * std::sin (6.2831853 * t / 1.3)),
                         static_cast<float> (-0.785 + 0.6 * std::sin (6.2831853 * t / 1.3 + 1.0)) });
    }
    const auto g = arc::buildGesture (raw, 0.0, 4.0);
    CHECK (g.valid);
    CHECK (std::abs (g.durationSeconds - 1.3f) < 0.02f);
    const auto back = arc::Gesture::deserialise (g.serialise());
    CHECK (back.valid);
    double worst = 0;
    for (int i = 0; i < arc::Gesture::kPoints; ++i)
        worst = std::max ({ worst, static_cast<double> (std::abs (back.dRadius[static_cast<size_t> (i)] - g.dRadius[static_cast<size_t> (i)])),
                            static_cast<double> (std::abs (back.dAngle[static_cast<size_t> (i)] - g.dAngle[static_cast<size_t> (i)])) });
    MEASURE ("serialiseRoundTripMaxError", worst);
    CHECK (worst < 1.0e-5);
    // Loop closure: end meets start.
    CHECK (std::abs (g.dRadius.back() - g.dRadius.front()) < 0.02f);

    // Tempo snap: 1.3 s at 120 bpm (2 beats/s) = 2.6 beats -> nearest division 2 beats (1/2).
    const auto gs = arc::buildGesture (raw, 2.0, 4.0);
    MEASURE ("syncedBeats", gs.durationBeats);
    CHECK (gs.durationBeats == 2.0f);

    // Playback: node A's effective radius follows the path with the recorded period.
    arc::ArcEngine e;
    auto p = base();
    e.setParameters (p);
    e.prepare (kSr, kBlock);
    CHECK (e.postGesture (0, g));
    const auto r = run (e, 4.0, [] (arc::ArcEngine&, double) {});
    const auto [mn, mx] = std::minmax_element (r.radiusA.begin() + 10, r.radiusA.end());
    MEASURE ("playback.radiusRange", *mx - *mn);
    CHECK (std::abs ((*mx - *mn) - 0.4f) < 0.06f);
    // Period from the telemetry trace (autocorrelation peak near 1.3 s).
    const double blockDt = kBlock / kSr;
    double bestLag = 0, bestCorr = -1e9;
    for (int lag = static_cast<int> (0.8 / blockDt); lag < static_cast<int> (2.0 / blockDt); ++lag)
    {
        double c = 0;
        for (size_t i = 10; i + static_cast<size_t> (lag) < r.radiusA.size(); ++i)
            c += (r.radiusA[i] - 0.5) * (r.radiusA[i + static_cast<size_t> (lag)] - 0.5);
        if (c > bestCorr)
        {
            bestCorr = c;
            bestLag = lag * blockDt;
        }
    }
    MEASURE ("playback.periodSeconds", bestLag);
    CHECK (std::abs (bestLag - 1.3) < 0.03);
}

TEST_CASE ("gesture", "a drag around the core becomes a seamless orbit")
{
    // One clockwise lap in 2 s, starting and ending at the top (angle wraps at +-pi).
    std::vector<arc::GestureSample> raw;
    for (int k = 0; k <= 200; ++k)
    {
        const double t = k * 0.01;
        double a = 6.283185307 * t / 2.0; // 0 .. 2 pi
        a -= 6.283185307 * std::floor ((a + 3.14159265) / 6.283185307);
        raw.push_back ({ t, 0.6f, static_cast<float> (a) });
    }
    const auto g = arc::buildGesture (raw, 0.0, 4.0);
    REQUIRE (g.valid);
    // Sample finely across the whole cycle, including the wrap from the end to the
    // start: every step must be small (no spin back), and the path covers a full turn.
    double maxStep = 0, travelled = 0;
    float prevR = 0, prevA = 0;
    g.sample (0.0, prevR, prevA);
    for (int i = 1; i <= 4000; ++i)
    {
        float r, a;
        g.sample (i / 2000.0, r, a); // two cycles
        const double step = std::abs (arc::wrapAngle (a - prevA));
        maxStep = std::max (maxStep, step);
        travelled += arc::wrapAngle (a - prevA);
        prevA = a;
        prevR = r;
    }
    MEASURE ("maxAngleStepPerSample_rad", maxStep);
    MEASURE ("turnsOverTwoCycles", travelled / 6.283185307);
    CHECK (maxStep < 0.02);
    CHECK (std::abs (travelled / 6.283185307 - 2.0) < 0.05);
}

TEST_CASE ("gesture", "synced gesture follows host tempo")
{
    std::vector<arc::GestureSample> raw;
    for (int k = 0; k <= 100; ++k)
        raw.push_back ({ k * 0.02, static_cast<float> (0.5 + 0.2 * std::sin (6.2831853 * k / 100.0)), -0.785f });
    auto g = arc::buildGesture (raw, 2.0, 4.0); // 2 s at 120 bpm = 4 beats = 1 bar
    CHECK (g.durationBeats == 4.0f);
    for (double bpm : { 120.0, 90.0 })
    {
        arc::ArcEngine e;
        auto p = base();
        p.sync = true;
        e.setParameters (p);
        e.prepare (kSr, kBlock);
        e.postGesture (0, g);
        double ppq = 0.0;
        std::vector<float> trace;
        std::vector<float> l (kBlock), r (kBlock);
        for (int b = 0; b < static_cast<int> (8.0 * kSr / kBlock); ++b)
        {
            arc::TransportInfo t;
            t.valid = true;
            t.playing = true;
            t.bpm = bpm;
            t.ppqPosition = ppq;
            e.setTransport (t);
            e.render (l.data(), r.data(), kBlock);
            ppq += kBlock / kSr * bpm / 60.0;
            trace.push_back (arc::Telemetry::load (e.getTelemetry().nodeRadius[0]));
        }
        // One cycle per bar: 60 / bpm * 4 seconds.
        const double blockDt = kBlock / kSr, expected = 60.0 / bpm * 4.0;
        double bestLag = 0, bestCorr = -1e9;
        for (int lag = static_cast<int> (0.5 * expected / blockDt); lag < static_cast<int> (1.5 * expected / blockDt); ++lag)
        {
            double c = 0;
            for (size_t i = 0; i + static_cast<size_t> (lag) < trace.size(); ++i)
                c += (trace[i] - 0.5) * (trace[i + static_cast<size_t> (lag)] - 0.5);
            if (c > bestCorr)
            {
                bestCorr = c;
                bestLag = lag * blockDt;
            }
        }
        MEASURE ("bpm" + std::to_string (static_cast<int> (bpm)) + ".periodSeconds", bestLag);
        CHECK (std::abs (bestLag - expected) < 0.03);
    }
}

TEST_CASE ("nonlinear", "extreme energy saturates coupling and stays bounded")
{
    arc::ArcEngine e;
    auto p = base();
    p.excite = 1.0f;
    p.coupling = 1.0f;
    p.material = arc::MaterialType::metal;
    p.exciterParams.strikeHardness = 1.0f;
    p.polyphony = 16;
    p.releaseDamping = 0.0f;
    e.setParameters (p);
    e.prepare (kSr, kBlock);
    // Hammer 16 voices repeatedly at full velocity.
    const auto r = run (e, 6.0, [] (arc::ArcEngine& en, double t)
                       {
                           if (std::fmod (t, 0.05) < kBlock / kSr)
                               for (int n = 0; n < 16; ++n)
                                   en.noteOn (1, 30 + n * 2, 1.0f);
                       });
    MEASURE ("peak", peakAbs (r.mono));
    MEASURE ("nonFiniteEvents", static_cast<double> (arc::Telemetry::load (e.getTelemetry().nonFiniteEvents)));
    CHECK (allFinite (r.mono));
    CHECK (peakAbs (r.mono) <= 1.0);
    CHECK (arc::Telemetry::load (e.getTelemetry().nonFiniteEvents) == 0u);
}

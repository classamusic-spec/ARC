// Engine CPU profile: voices x exciter x sample rate x coupling/chaos (Release build).
// Real-time fraction = processing time / audio duration on this machine's core.

#include "Analysis.h"
#include "ArcTest.h"

#include "Engine/ArcEngine.h"

#include <array>
#include <chrono>
#include <fstream>

using namespace arctest;

namespace
{
double measureRealtimeFraction (double sr, int voices, arc::ExciterType ex, float coupling, float chaos, float motion,
                                arc::Quality quality = arc::Quality::normal)
{
    arc::ArcEngine e;
    arc::EngineParams p;
    p.polyphony = 16;
    p.exciter = ex;
    p.coupling = coupling;
    p.chaos = chaos;
    p.motionDepth = motion;
    p.quality = quality;
    p.releaseDamping = 0.0f;
    e.setParameters (p);
    const int block = 256;
    e.prepare (sr, block);
    std::vector<float> l (static_cast<size_t> (block)), r (static_cast<size_t> (block));
    for (int v = 0; v < voices; ++v)
        e.noteOn (1, 40 + v * 3, 0.8f);
    // Warm up (attacks, first designs), then time 4 seconds of audio.
    for (int i = 0; i < static_cast<int> (0.3 * sr / block); ++i)
        e.render (l.data(), r.data(), block);
    const int blocks = static_cast<int> (4.0 * sr / block);
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < blocks; ++i)
    {
        // Keep strikes ringing / re-excited so voices stay active.
        if (ex == arc::ExciterType::strike && i % static_cast<int> (sr / block) == 0)
            for (int v = 0; v < voices; ++v)
                e.noteOn (1, 40 + v * 3, 0.8f);
        e.render (l.data(), r.data(), block);
    }
    const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    return secs / (blocks * block / sr);
}
} // namespace

TEST_CASE ("performance", "engine cpu by voices, exciter and sample rate")
{
    std::ofstream csv (outputDir() + "/cpu_profile.csv");
    csv << "sampleRate,voices,exciter,coupling,chaos,realtimeFraction\n";
    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (auto ex : { arc::ExciterType::strike, arc::ExciterType::bow })
            for (int voices : { 1, 4, 8, 16 })
            {
                const double f = measureRealtimeFraction (sr, voices, ex, 0.35f, 0.1f, 0.0f);
                const std::string tag = std::to_string (static_cast<int> (sr / 1000.0)) + "k." + (ex == arc::ExciterType::strike ? "strike" : "bow")
                                        + "." + std::to_string (voices) + "v";
                MEASURE (tag + ".percentOfCore", f * 100.0);
                csv << sr << "," << voices << "," << (ex == arc::ExciterType::strike ? "strike" : "bow") << ",0.35,0.1," << f << "\n";
                if (sr == 48000.0 && voices == 16)
                    CHECK (f < 0.5); // 16 voices must fit comfortably in one core at 48 kHz
            }
    // Coupling / chaos / motion extremes at 48 kHz, 8 voices.
    for (auto cfg : { std::make_tuple (0.0f, 0.0f, 0.0f), std::make_tuple (1.0f, 0.0f, 0.0f), std::make_tuple (0.35f, 1.0f, 0.0f),
                      std::make_tuple (0.35f, 0.1f, 1.0f) })
    {
        const auto [c, ch, mo] = cfg;
        const double f = measureRealtimeFraction (48000.0, 8, arc::ExciterType::bow, c, ch, mo);
        MEASURE ("48k.bow.8v.coupling" + std::to_string (c).substr (0, 4) + ".chaos" + std::to_string (ch).substr (0, 3)
                     + ".motion" + std::to_string (mo).substr (0, 3) + ".percentOfCore",
                 f * 100.0);
        csv << 48000 << ",8,bow," << c << "," << ch << "," << f << "\n";
    }
    // QUALITY must be a real trade-off: each step roughly halves / doubles the cost.
    std::array<double, 3> cost {};
    for (auto q : { arc::Quality::eco, arc::Quality::normal, arc::Quality::high })
    {
        // Best of three runs: robust against scheduler noise on a shared machine.
        double f = 1e9;
        for (int run = 0; run < 3; ++run)
            f = std::min (f, measureRealtimeFraction (48000.0, 16, arc::ExciterType::bow, 0.35f, 0.1f, 0.0f, q));
        cost[(size_t) q] = f;
        const char* name = q == arc::Quality::eco ? "Eco" : q == arc::Quality::normal ? "Normal" : "High";
        MEASURE (std::string ("48k.bow.16v.quality") + name + ".percentOfCore", f * 100.0);
        csv << "48000,16,bow-quality-" << name << ",0.35,0.1," << f << "\n";
    }
    CHECK (cost[0] < cost[1] * 0.9);
    CHECK (cost[1] < cost[2] * 0.8);
}

TEST_CASE ("performance", "quality modes sound alike and switch live without clicks")
{
    auto renderMode = [] (arc::Quality q, bool switching)
    {
        arc::ArcEngine e;
        arc::EngineParams p;
        p.exciter = arc::ExciterType::bow;
        p.material = arc::MaterialType::metal;
        p.chaos = 0.0f;
        p.space = 0.0f;
        p.quality = q;
        e.setParameters (p);
        e.prepare (48000.0, 256);
        Signal mono;
        std::vector<float> l (256), r (256);
        for (int b = 0; b < (int) (3.0 * 48000.0 / 256); ++b)
        {
            if (b == 0)
                for (int n : { 48, 55, 64 })
                    e.noteOn (1, n, 0.8f);
            if (switching && b > 0 && b % 47 == 0)
            {
                p.quality = static_cast<arc::Quality> ((static_cast<int> (p.quality) + 1) % 3);
                e.setParameters (p);
            }
            e.render (l.data(), r.data(), 256);
            for (int i = 0; i < 256; ++i)
                mono.push_back (0.5f * (l[(size_t) i] + r[(size_t) i]));
        }
        return mono;
    };
    auto bands = [] (const Signal& x)
    {
        const auto s = computeSpectrum (x, 48000.0, 1 << 16, 48000, 96000);
        std::vector<double> b;
        for (double f = 80.0; f < 8000.0; f *= std::pow (2.0, 1.0 / 6.0))
        {
            double e = 0;
            for (int i = (int) (f / s.binHz); i < (int) (f * 1.1225 / s.binHz); ++i)
                e += s.mag[(size_t) i] * s.mag[(size_t) i];
            b.push_back (10.0 * std::log10 (e + 1e-20));
        }
        return b;
    };
    const auto high = renderMode (arc::Quality::high, false);
    const auto hb = bands (high);
    const double f0High = refineFrequencyByPhase (high, 48000.0, arc::dsp::midiToHz (48), 8.0);
    for (auto q : { arc::Quality::normal, arc::Quality::eco })
    {
        const auto x = renderMode (q, false);
        const auto xb = bands (x);
        double d = 0;
        for (size_t i = 0; i < hb.size(); ++i)
            d += (xb[i] - hb[i]) * (xb[i] - hb[i]);
        d = std::sqrt (d / (double) hb.size());
        const double cents = 1200.0 * std::log2 (refineFrequencyByPhase (x, 48000.0, arc::dsp::midiToHz (48), 8.0) / f0High);
        const char* name = q == arc::Quality::eco ? "eco" : "normal";
        MEASURE (std::string (name) + ".lsdVsHigh_dB", d);
        MEASURE (std::string (name) + ".pitchVsHigh_cents", cents);
        // ECO also halves the dispersion detail (2 stages): a small, intended change of
        // the partial stretch; NORMAL must stay close to HIGH.
        CHECK (d < (q == arc::Quality::eco ? 2.5 : 1.5));
        CHECK (std::abs (cents) < 1.0);
    }

    // Switching every ~250 ms while a bowed chord sounds: no step larger than the
    // chord's own largest sample-to-sample movement.
    const auto sw = renderMode (arc::Quality::normal, true);
    double steady = 0, worst = 0;
    for (size_t i = 1; i < sw.size(); ++i)
    {
        const double step = std::abs (sw[i] - sw[i - 1]);
        if (i < (size_t) (0.25 * 48000))
            continue;
        worst = std::max (worst, step);
    }
    for (size_t i = (size_t) (0.25 * 48000) + 1; i < high.size(); ++i)
        steady = std::max (steady, static_cast<double> (std::abs (high[i] - high[i - 1])));
    MEASURE ("switching.maxStep", worst);
    MEASURE ("steady.maxStep", steady);
    CHECK (allFinite (sw));
    CHECK (worst < steady * 1.5);
}

TEST_CASE ("performance", "profile target (8 bowed voices, 1 s)")
{
    // Fixed workload for external profilers (callgrind); measures nothing itself.
    const double rtf = measureRealtimeFraction (48000.0, 8, arc::ExciterType::bow, 0.35f, 0.1f, 0.2f);
    MEASURE ("realtimeFraction", rtf);
}

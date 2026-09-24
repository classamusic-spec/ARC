// Engine CPU profile: voices x exciter x sample rate x coupling/chaos (Release build).
// Real-time fraction = processing time / audio duration on this machine's core.

#include "Analysis.h"
#include "ArcTest.h"

#include "Engine/ArcEngine.h"

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
    for (auto q : { arc::Quality::eco, arc::Quality::normal, arc::Quality::high })
    {
        const double f = measureRealtimeFraction (48000.0, 16, arc::ExciterType::strike, 0.35f, 0.1f, 0.0f, q);
        MEASURE (std::string ("48k.strike.16v.quality") + (q == arc::Quality::eco ? "Eco" : q == arc::Quality::normal ? "Normal" : "High")
                     + ".percentOfCore",
                 f * 100.0);
    }
}

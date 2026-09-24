// AIR drive regulation must hold a steady tone (no hunting / tremolo).
#include "ArcTest.h"
#include "VoiceRig.h"

using namespace arctest;

namespace
{
/** Depth (dB) of the strongest amplitude modulation between 3 and 40 Hz. Envelope
    frames span exactly one period of f0 (sub-period frames alias 2 f0 against the frame
    rate and read as a fake tremolo). */
double tremoloDepthDb (const Signal& x, double sr, double f0, double t0, double t1, double* rateHz)
{
    const int frame = static_cast<int> (std::lround (sr / f0));
    Signal env;
    for (int i = static_cast<int> (t0 * sr); i + frame < static_cast<int> (t1 * sr); i += frame)
        env.push_back (static_cast<float> (rms (x, i, frame)));
    double mean = 0;
    for (float v : env)
        mean += v;
    mean /= static_cast<double> (env.size());
    for (auto& v : env)
        v = static_cast<float> (v - mean);
    const double frameRate = sr / frame;
    const auto s = computeSpectrum (env, frameRate, 1 << 14);
    double best = 0;
    const double f = peakFrequency (s, 3.0, 40.0, &best);
    if (rateHz != nullptr)
        *rateHz = f;
    // Modulation index relative to the mean level (Hann-windowed amplitude spectrum).
    const double depth = 4.0 * best / static_cast<double> (env.size()) / mean;
    return 20.0 * std::log10 (depth + 1e-9);
}
} // namespace

TEST_CASE ("exciters", "air and bow hold a steady level")
{
    for (auto e : { arc::ExciterType::air, arc::ExciterType::bow })
        for (auto m : { arc::MaterialType::glass, arc::MaterialType::metal, arc::MaterialType::wood, arc::MaterialType::membrane })
        {
            auto s = defaultRig();
            s.exciter = e;
            s.material = m;
            const auto r = renderVoice (s, 60, 0.8f, 3.0, 3.0, 48000.0);
            double rate = 0;
            const double depth = tremoloDepthDb (r.mono, 48000.0, arc::dsp::midiToHz (60), 1.0, 2.9, &rate);
            const std::string tag = std::string (exciterName (e)) + "." + materialName (m);
            MEASURE (tag + ".tremoloDepthDb", depth);
            MEASURE (tag + ".tremoloRateHz", rate);
            CHECK_MSG (depth < -30.0, tag << " modulation " << depth << " dB at " << rate << " Hz");
        }
}

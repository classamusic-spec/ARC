// Phase 2 — resonator primitives quality gate.
//
// Covers: fractional-delay interpolator comparison (pitch, HF loss, decay
// consistency, modulation artefacts, CPU), waveguide pitch C1..C6 at five
// sample rates, frequency-dependent decay accuracy, automation smoothness,
// extremes, near-Nyquist behaviour, impulse responses and modal-bank checks.

#include "Analysis.h"
#include "ArcTest.h"

#include "Synthesis/ModalResonator.h"
#include "Synthesis/WaveguideResonator.h"

#include <fstream>

using namespace arc::dsp;
namespace dsp = arc::dsp;
using namespace arctest;

namespace
{
const char* interpName (Interpolation i)
{
    switch (i)
    {
        case Interpolation::linear:    return "linear";
        case Interpolation::hermite:   return "hermite";
        case Interpolation::lagrange3: return "lagrange3";
        case Interpolation::thiran1:   return "thiran1";
    }
    return "?";
}

Signal renderImpulse (WaveguideResonator& r, double seconds)
{
    const int n = static_cast<int> (seconds * r.getSampleRate());
    Signal out (static_cast<size_t> (n));
    for (int i = 0; i < n; ++i)
        out[static_cast<size_t> (i)] = r.process (i == 0 ? 1.0f : 0.0f);
    return out;
}

double measurePitchCents (WaveguideResonator& r, double f0, double seconds)
{
    r.reset();
    const auto ir = renderImpulse (r, seconds);
    const double coarse = peakFrequency (ir, r.getSampleRate(), f0 * 0.97, f0 * 1.03);
    const double fine = refineFrequencyByPhase (ir, r.getSampleRate(), coarse > 0 ? coarse : f0);
    return centsBetween (fine, f0);
}

const Interpolation kAllInterps[] = { Interpolation::linear, Interpolation::hermite, Interpolation::lagrange3,
                                      Interpolation::thiran1 };
} // namespace

// ---------------------------------------------------------------------------
// Fractional delay evaluation (documents the interpolator decision)
// ---------------------------------------------------------------------------

TEST_CASE ("resonator", "fractional delay comparison")
{
    const double sr = 48000.0;
    std::ofstream csv (outputDir() + "/fractional_delay_comparison.csv");
    csv << "interp,metric,value\n";

    for (auto mode : kAllInterps)
    {
        const std::string name = interpName (mode);

        // (a) pitch accuracy of a lightly damped loop, C1..C8 + chromatic octave
        WaveguideResonator r;
        r.prepare (sr, 15.0);
        double maxErr = 0, maxErrHigh = 0;
        std::vector<int> notes = { 24, 36, 48, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 84 };
        for (int note : notes)
        {
            ResonatorSettings s;
            s.frequency = midiToHz (note);
            s.t60Fundamental = 3.0;
            s.t60High = 1.5;
            s.interpolation = mode;
            s.dispersion = 0.0;
            r.configure (s);
            const double cents = measurePitchCents (r, s.frequency, note < 40 ? 4.0 : 2.0);
            maxErr = std::max (maxErr, std::abs (cents));
        }
        for (int note : { 96, 108 })
        {
            ResonatorSettings s;
            s.frequency = midiToHz (note);
            s.t60Fundamental = 1.0;
            s.t60High = 0.8;
            s.interpolation = mode;
            r.configure (s);
            maxErrHigh = std::max (maxErrHigh, std::abs (measurePitchCents (r, s.frequency, 1.0)));
        }
        MEASURE (name + ".maxPitchErrorCents_C1_C6", maxErr);
        MEASURE (name + ".maxPitchErrorCents_C7_C8", maxErrHigh);
        csv << name << ",maxPitchErrorCents_C1_C6," << maxErr << "\n";
        csv << name << ",maxPitchErrorCents_C7_C8," << maxErrHigh << "\n";

        // (b) interpolator magnitude at fraction 0.5 (worst case) near fs/4 and 0.45 fs
        {
            DelayLine line;
            line.allocate (64);
            auto setting = FractionalDelaySetting::make (10.5, mode);
            float st = 0.0f;
            Signal ir (4096, 0.0f);
            for (size_t i = 0; i < ir.size(); ++i)
            {
                ir[i] = readInterpolated (line, setting, mode, st);
                line.write (i == 0 ? 1.0f : 0.0f);
            }
            // Unwindowed DFT at two frequencies.
            auto magAt = [&] (double f)
            {
                double re = 0, im = 0;
                for (size_t i = 0; i < ir.size(); ++i)
                {
                    re += ir[i] * std::cos (kTwoPi * f * static_cast<double> (i) / sr);
                    im -= ir[i] * std::sin (kTwoPi * f * static_cast<double> (i) / sr);
                }
                return 20.0 * std::log10 (std::sqrt (re * re + im * im));
            };
            const double m1 = magAt (0.25 * sr), m2 = magAt (0.45 * sr);
            MEASURE (name + ".gainDb_frac0.5_at_0.25fs", m1);
            MEASURE (name + ".gainDb_frac0.5_at_0.45fs", m2);
            csv << name << ",gainDb_frac0.5_at_0.25fs," << m1 << "\n";
            csv << name << ",gainDb_frac0.5_at_0.45fs," << m2 << "\n";
        }

        // (c) decay consistency: T60 of the 12th harmonic for loops whose
        //     fractional part is 0.0 vs 0.5 (same nominal decay settings).
        {
            double t60s[2];
            const double lengths[2] = { 100.0, 100.5 };
            for (int j = 0; j < 2; ++j)
            {
                ResonatorSettings s;
                s.frequency = sr / lengths[j];
                s.t60Fundamental = 2.0;
                s.t60High = 2.0; // flat requested decay
                s.interpolation = mode;
                r.configure (s);
                r.reset();
                const auto ir = renderImpulse (r, 2.5);
                t60s[j] = measureT60 (ir, sr, 12.0 * s.frequency, 5.0, 35.0, 2.0, s.frequency);
            }
            const double ratio = t60s[1] / t60s[0];
            MEASURE (name + ".hfDecayRatio_frac0.5_vs_0", ratio);
            csv << name << ",hfDecayRatio_frac0.5_vs_0," << ratio << "\n";
        }

        // (d) modulation artefacts: 3 kHz sine through a delay swept 100 -> 140 samples
        //     over 0.5 s; error vs the analytically delayed sine.
        {
            DelayLine line;
            line.allocate (512);
            float st = 0.0f;
            const int n = static_cast<int> (0.5 * sr);
            const double f = 3000.0;
            double errE = 0, sigE = 0, maxJump = 0;
            float prevErr = 0.0f;
            for (int i = 0; i < n + 400; ++i)
            {
                const double t = std::max (0, i - 400) / static_cast<double> (n);
                const double delay = 100.0 + 40.0 * t;
                const auto set = FractionalDelaySetting::make (delay, mode);
                const float y = readInterpolated (line, set, mode, st);
                line.write (static_cast<float> (std::sin (kTwoPi * f * i / sr)));
                if (i >= 400)
                {
                    const double ideal = std::sin (kTwoPi * f * (i - delay) / sr);
                    const double e = y - ideal;
                    errE += e * e;
                    sigE += ideal * ideal;
                    maxJump = std::max (maxJump, std::abs (e - prevErr));
                    prevErr = static_cast<float> (e);
                }
            }
            const double snr = 10.0 * std::log10 (sigE / std::max (errE, 1e-30));
            MEASURE (name + ".sweepSNRdB_3kHz", snr);
            MEASURE (name + ".sweepMaxErrorJump", maxJump);
            csv << name << ",sweepSNRdB_3kHz," << snr << "\n";
            csv << name << ",sweepMaxErrorJump," << maxJump << "\n";
        }

        // (e) CPU per read (ns)
        {
            DelayLine line;
            line.allocate (4096);
            auto set = FractionalDelaySetting::make (1000.37, mode);
            float st = 0.0f;
            volatile float sink = 0.0f;
            dsp::Random rng;
            const double ns = nanosPerCall ([&]
                                            {
                                                const float y = readInterpolated (line, set, mode, st);
                                                line.write (rng.bipolar() + 0.5f * y);
                                                sink = y;
                                            },
                                            5'000'000);
            (void) sink;
            MEASURE (name + ".nsPerSample", ns);
            csv << name << ",nsPerSample," << ns << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Resonator quality gate (chosen configuration: Thiran1 + phase refinement)
// ---------------------------------------------------------------------------

TEST_CASE ("resonator", "pitch C1-C6 across sample rates")
{
    const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    const int notes[] = { 24, 36, 48, 60, 72, 84 };
    double worst = 0, worstDispersive = 0;

    for (double sr : rates)
    {
        WaveguideResonator r;
        r.prepare (sr, 15.0);
        for (int note : notes)
        {
            for (double dispersion : { 0.0, 0.25 })
            {
                ResonatorSettings s;
                s.frequency = midiToHz (note);
                s.t60Fundamental = 3.0;
                s.t60High = 0.8;
                s.dispersion = dispersion;
                r.configure (s);
                const double cents = measurePitchCents (r, s.frequency, note < 40 ? 4.0 : 2.0);
                if (dispersion == 0.0)
                    worst = std::max (worst, std::abs (cents));
                else
                    worstDispersive = std::max (worstDispersive, std::abs (cents));
                CHECK_MSG (std::abs (cents) < 1.0, "sr=" << sr << " note=" << note << " disp=" << dispersion
                                                         << " cents=" << cents);
            }
        }
    }
    MEASURE ("worstPitchErrorCents_harmonic", worst);
    MEASURE ("worstPitchErrorCents_dispersive", worstDispersive);
}

TEST_CASE ("resonator", "chromatic intervals")
{
    const double sr = 48000.0;
    WaveguideResonator r;
    r.prepare (sr, 15.0);
    double worst = 0;
    for (int note = 48; note <= 72; ++note)
    {
        ResonatorSettings s;
        s.frequency = midiToHz (note);
        s.t60Fundamental = 2.0;
        s.t60High = 0.6;
        r.configure (s);
        const double cents = measurePitchCents (r, s.frequency, 2.0);
        worst = std::max (worst, std::abs (cents));
    }
    MEASURE ("worstChromaticErrorCents_C3_C5", worst);
    CHECK (worst < 0.5);
}

TEST_CASE ("resonator", "frequency dependent decay accuracy")
{
    const double sr = 48000.0;
    WaveguideResonator r;
    r.prepare (sr, 15.0);

    struct Case
    {
        double f, t60f, t60h, href;
    };
    const Case cases[] = { { 110.0, 0.5, 0.1, 4000.0 }, { 110.0, 2.0, 0.4, 4000.0 }, { 220.0, 6.0, 1.5, 4000.0 },
                           { 440.0, 3.0, 3.0, 4000.0 }, { 55.0, 4.0, 0.3, 2000.0 } };
    double worstFund = 0, worstHigh = 0;
    for (auto& c : cases)
    {
        ResonatorSettings s;
        s.frequency = c.f;
        s.t60Fundamental = c.t60f;
        s.t60High = c.t60h;
        s.hfReference = c.href;
        r.configure (s);
        r.reset();
        const auto ir = renderImpulse (r, std::min (4.0, c.t60f * 1.2 + 0.3));
        const double measuredF = measureT60 (ir, sr, c.f, 5.0, 35.0, 2.0, c.f);
        // Nearest harmonic to the HF reference; window spans 2 fundamental periods
        // so neighbouring harmonics fall into the smoothing nulls.
        const double hk = std::round (c.href / c.f) * c.f;
        const double measuredH = measureT60 (ir, sr, hk, 5.0, 35.0, 2.0, c.f);
        const double errF = measuredF / c.t60f - 1.0;
        const double errH = measuredH / c.t60h - 1.0;
        worstFund = std::max (worstFund, std::abs (errF));
        worstHigh = std::max (worstHigh, std::abs (errH));
        CHECK_MSG (std::abs (errF) < 0.10, "f=" << c.f << " requested " << c.t60f << " measured " << measuredF);
        CHECK_MSG (std::abs (errH) < 0.25, "HF @" << hk << " requested " << c.t60h << " measured " << measuredH);
    }
    MEASURE ("worstT60RelErr_fundamental", worstFund);
    MEASURE ("worstT60RelErr_hfReference", worstHigh);
}

TEST_CASE ("resonator", "sample rate consistency")
{
    const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    std::vector<double> t60s, centroids, inharm;
    for (double sr : rates)
    {
        WaveguideResonator r;
        r.prepare (sr, 15.0);
        ResonatorSettings s;
        s.frequency = 196.0;
        s.t60Fundamental = 2.0;
        s.t60High = 0.5;
        s.hfReference = 3000.0;
        s.dispersion = 0.2;
        r.configure (s);
        const auto ir = renderImpulse (r, 2.5);
        t60s.push_back (measureT60 (ir, sr, 196.0, 5.0, 35.0, 2.0, 196.0));
        centroids.push_back (spectralCentroid (ir, sr, 0, static_cast<int> (0.5 * sr)));
        const auto spec = computeSpectrum (ir, sr, 1 << 18);
        inharm.push_back (inharmonicity (spec, 196.0, 6));
    }
    auto spread = [] (const std::vector<double>& v)
    {
        const auto [mn, mx] = std::minmax_element (v.begin(), v.end());
        return (*mx - *mn) / *mn;
    };
    MEASURE ("t60Spread", spread (t60s));
    MEASURE ("inharmonicitySpread", spread (inharm));
    MEASURE ("inharmonicity@48k", inharm[1]);
    MEASURE ("centroid@44k1", centroids[0]);
    MEASURE ("centroid@192k", centroids[4]);
    MEASURE ("centroidSpread_below20k", spread (centroids));
    CHECK (spread (t60s) < 0.05);
    CHECK (spread (inharm) < 0.15);
    CHECK (spread (centroids) < 0.15);
}

namespace
{
struct GlideResult
{
    double hfBeforeDb, hfDuringDb, curvatureBefore, curvatureDuring;
    bool finite;
    Signal out;
};

/** Rings a 220 Hz loop, then glides one octave over glideMs with engine-style
    control (16-sample updates, per-sample coefficient ramps). */
GlideResult runGlide (Interpolation mode, double glideMs)
{
    const double sr = 48000.0;
    WaveguideResonator r;
    r.prepare (sr, 15.0);
    ResonatorSettings s;
    s.frequency = 220.0;
    s.t60Fundamental = 4.0;
    s.t60High = 1.0;
    s.dispersion = 0.1;
    s.interpolation = mode;
    r.configure (s);
    r.reset();

    const int total = static_cast<int> (0.6 * sr);
    const int glideStart = static_cast<int> (0.2 * sr), glideLen = static_cast<int> (glideMs * 0.001 * sr);
    constexpr int control = 16;
    GlideResult g {};
    g.out.resize (static_cast<size_t> (total));
    for (int i = 0; i < total; ++i)
    {
        if (i % control == 0 && i >= glideStart && i <= glideStart + glideLen + control)
        {
            const double t = std::min (1.0, (i + control - glideStart) / static_cast<double> (glideLen));
            s.frequency = 220.0 * std::exp2 (t);
            r.configure (s, control);
        }
        g.out[static_cast<size_t> (i)] = r.process (i == 0 ? 1.0f : 0.0f);
    }
    // Curvature (|2nd difference|) peaks: an impulsive click shows up here directly.
    for (int i = glideStart - 4096; i < glideStart + glideLen; ++i)
    {
        const auto u = static_cast<size_t> (i);
        const double d2 = std::abs (g.out[u] - 2.0f * g.out[u - 1] + g.out[u - 2]);
        (i < glideStart ? g.curvatureBefore : g.curvatureDuring) =
            std::max (i < glideStart ? g.curvatureBefore : g.curvatureDuring, d2);
    }
    const int win = 2048;
    g.hfBeforeDb = highBandRatioDb (g.out, sr, 12000.0, glideStart - win, win);
    g.hfDuringDb = highBandRatioDb (g.out, sr, 12000.0, glideStart, win);
    g.finite = allFinite (g.out);
    return g;
}
} // namespace

TEST_CASE ("resonator", "frequency automation is click free")
{
    // Realistic fast automation: one octave in 200 ms.
    const auto real = runGlide (Interpolation::thiran1, 200.0);
    MEASURE ("octave200ms.hfRatioDb_before", real.hfBeforeDb);
    MEASURE ("octave200ms.hfRatioDb_during", real.hfDuringDb);
    MEASURE ("octave200ms.curvature_before", real.curvatureBefore);
    MEASURE ("octave200ms.curvature_during", real.curvatureDuring);
    CHECK (real.finite);
    CHECK (real.hfDuringDb < real.hfBeforeDb + 3.0);
    CHECK (real.curvatureDuring < 1.5 * real.curvatureBefore);
    writeWav (outputDir() + "/resonator_glide_200ms.wav", real.out, 48000.0);

    // Extreme automation: one octave in 30 ms. Doppler resampling of the loop
    // content legitimately adds HF for any interpolator; assert no impulsive click
    // and parity with the Lagrange reference.
    const auto extreme = runGlide (Interpolation::thiran1, 30.0);
    const auto reference = runGlide (Interpolation::lagrange3, 30.0);
    MEASURE ("octave30ms.thiran.hfRatioDb_during", extreme.hfDuringDb);
    MEASURE ("octave30ms.lagrange.hfRatioDb_during", reference.hfDuringDb);
    MEASURE ("octave30ms.thiran.curvature_during", extreme.curvatureDuring);
    MEASURE ("octave30ms.lagrange.curvature_during", reference.curvatureDuring);
    CHECK (extreme.finite);
    CHECK (extreme.curvatureDuring < 4.0 * extreme.curvatureBefore);
    CHECK (extreme.hfDuringDb < reference.hfDuringDb + 6.0);
    writeWav (outputDir() + "/resonator_glide_30ms.wav", extreme.out, 48000.0);
}

TEST_CASE ("resonator", "extremes stay finite and bounded")
{
    for (double sr : { 44100.0, 192000.0 })
    {
        WaveguideResonator r;
        r.prepare (sr, 15.0);
        const double freqs[] = { 1.0, 15.0, 20000.0, 0.45 * sr, 0.49 * sr, 1.0e6 };
        const double decays[] = { 1.0e-4, 0.01, 1000.0 };
        const double disps[] = { 0.0, 0.6, 5.0 };
        for (double f : freqs)
            for (double d : decays)
                for (double disp : disps)
                {
                    ResonatorSettings s;
                    s.frequency = f;
                    s.t60Fundamental = d;
                    s.t60High = d * 0.5;
                    s.dispersion = disp;
                    r.configure (s);
                    r.reset();
                    double peak = 0.0;
                    bool finite = true;
                    for (int i = 0; i < 20000; ++i)
                    {
                        const float y = r.process (i < 10 ? 1000.0f : 0.0f);
                        finite = finite && std::isfinite (y);
                        peak = std::max (peak, static_cast<double> (std::abs (y)));
                    }
                    CHECK_MSG (finite, "f=" << f << " t60=" << d << " disp=" << disp);
                    // Loop gain < 1: output bounded by input energy (10 x 1000) and number of passes.
                    CHECK_MSG (peak < 1.0e5, "peak=" << peak << " f=" << f << " t60=" << d);
                }
    }
}

TEST_CASE ("resonator", "near nyquist stability and tuning")
{
    const double sr = 48000.0;
    WaveguideResonator r;
    r.prepare (sr, 15.0);
    for (double f : { 8000.0, 12000.0, 16000.0, 19000.0 })
    {
        ResonatorSettings s;
        s.frequency = f;
        s.t60Fundamental = 1.0;
        s.t60High = 1.0;
        s.hfReference = 20000.0;
        s.dispersion = 0.3;
        r.configure (s);
        r.reset();
        const auto ir = renderImpulse (r, 1.0);
        CHECK (allFinite (ir));
        CHECK (peakAbs (ir) <= 1.0001);
        const double measured = refineFrequencyByPhase (ir, sr, peakFrequency (ir, sr, f * 0.95, std::min (f * 1.05, 23900.0)), 32.0);
        const double cents = centsBetween (measured, f);
        MEASURE ("pitchErrorCents@" + std::to_string (static_cast<int> (f)), cents);
        CHECK (std::abs (cents) < 5.0);
    }
}

TEST_CASE ("resonator", "impulse responses and dispersion")
{
    const double sr = 48000.0;
    WaveguideResonator r;
    r.prepare (sr, 15.0);
    const double f0 = 130.81; // C3
    double prevInharm = -1.0;
    for (double disp : { 0.0, 0.1, 0.3, 0.5 })
    {
        ResonatorSettings s;
        s.frequency = f0;
        s.t60Fundamental = 3.0;
        s.t60High = 1.2;
        s.dispersion = disp;
        r.configure (s);
        r.reset();
        const auto ir = renderImpulse (r, 3.0);
        const auto spec = computeSpectrum (ir, sr, 1 << 18);
        const double inh = inharmonicity (spec, f0, 8);
        MEASURE ("inharmonicity_disp" + std::to_string (disp).substr (0, 3), inh);
        CHECK (inh > prevInharm); // monotonic in the dispersion amount
        prevInharm = inh;
        writeWav (outputDir() + "/resonator_ir_C3_disp" + std::to_string (disp).substr (0, 3) + ".wav", ir, sr);
    }
    CHECK (prevInharm > 0.02);
}

TEST_CASE ("resonator", "cpu per resonator")
{
    const double sr = 48000.0;
    WaveguideResonator r;
    r.prepare (sr, 15.0);
    ResonatorSettings s;
    s.frequency = 110.0;
    s.dispersion = 0.3;
    s.dispersionStages = 4;
    r.configure (s);
    float x = 0.0f;
    volatile float sink = 0.0f;
    long i = 0;
    const double ns = nanosPerCall ([&]
                                    {
                                        const float y = r.process ((i++ & 4095) == 0 ? 0.5f : 0.0f);
                                        x = y;
                                        sink = x;
                                    },
                                    5'000'000);
    (void) sink;
    MEASURE ("nsPerSample_thiran_loss_4stageDispersion", ns);
    MEASURE ("percentOfRealtimeCore_per_resonator@48k", ns * sr * 1.0e-9 * 100.0);
    CHECK (ns < 100.0);
}

// ---------------------------------------------------------------------------
// Modal bank
// ---------------------------------------------------------------------------

TEST_CASE ("resonator", "modal bank tuning decay and cpu")
{
    const double sr = 48000.0;
    ModalBank bank;
    bank.setSampleRate (sr);
    bank.setNumModes (1);
    double worstCents = 0, worstT60 = 0;
    for (double f : { 32.7, 261.63, 1046.5, 8000.0, 20000.0 })
    {
        for (double t60 : { 0.3, 3.0 })
        {
            bank.reset();
            bank.setMode (0, f, t60, 1.0f);
            Signal ir (static_cast<size_t> (sr * std::min (4.0, t60 * 1.3 + 0.3)));
            for (size_t i = 0; i < ir.size(); ++i)
                ir[i] = bank.process (i == 0 ? 1.0f : 0.0f);
            CHECK (allFinite (ir));
            const double measured = refineFrequencyByPhase (ir, sr, f);
            worstCents = std::max (worstCents, std::abs (centsBetween (measured, f)));
            const double m = measureT60 (ir, sr, f, 5.0, 35.0, std::min (8.0, std::max (1.0, f * t60 / 40.0)));
            worstT60 = std::max (worstT60, std::abs (m / t60 - 1.0));
        }
    }
    MEASURE ("modal.worstPitchErrorCents", worstCents);
    MEASURE ("modal.worstT60RelErr", worstT60);
    CHECK (worstCents < 0.1);
    CHECK (worstT60 < 0.05);

    bank.setNumModes (16);
    for (int m = 0; m < 16; ++m)
        bank.setMode (m, 200.0 * (m + 1) * 1.07, 2.0, 0.1f);
    volatile float sink = 0.0f;
    long i = 0;
    const double ns = nanosPerCall ([&] { sink = bank.process ((i++ & 4095) == 0 ? 1.0f : 0.0f); }, 2'000'000);
    (void) sink;
    MEASURE ("modal.nsPerSample_16modes", ns);
    MEASURE ("modal.nsPerModeSample", ns / 16.0);
}

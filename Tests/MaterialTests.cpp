// Phase 6 — material quality gate: GLASS, METAL, WOOD, MEMBRANE are genuinely
// different network behaviours (modal distribution, decay, inharmonicity), for
// every exciter, and material changes are continuous.

#include "ArcTest.h"
#include "VoiceRig.h"

using namespace arctest;

namespace
{
constexpr double kSr = 48000.0;
constexpr int kNote = 60;

std::vector<double> bandProfileDb (const Signal& x, double t0, double len)
{
    const auto s = computeSpectrum (x, kSr, 1 << 16, static_cast<int> (t0 * kSr), static_cast<int> (len * kSr));
    std::vector<double> b;
    for (double f = 60.0; f < 12000.0; f *= std::pow (2.0, 1.0 / 6.0))
    {
        const int lo = static_cast<int> (f / s.binHz), hi = static_cast<int> (f * std::pow (2.0, 1.0 / 6.0) / s.binHz);
        double e = 0;
        for (int i = lo; i < hi; ++i)
            e += s.mag[static_cast<size_t> (i)] * s.mag[static_cast<size_t> (i)];
        b.push_back (10.0 * std::log10 (e + 1e-20));
    }
    return b;
}

double lsd (const std::vector<double>& a, const std::vector<double>& b)
{
    // Level-independent: remove the mean difference first.
    double mean = 0;
    for (size_t i = 0; i < a.size(); ++i)
        mean += a[i] - b[i];
    mean /= static_cast<double> (a.size());
    double d = 0;
    for (size_t i = 0; i < a.size(); ++i)
        d += (a[i] - b[i] - mean) * (a[i] - b[i] - mean);
    return std::sqrt (d / static_cast<double> (a.size()));
}

const arc::MaterialType kMaterials[] = { arc::MaterialType::glass, arc::MaterialType::metal, arc::MaterialType::wood,
                                         arc::MaterialType::membrane };
} // namespace

TEST_CASE ("materials", "materials are measurably different")
{
    const double f0 = arc::dsp::midiToHz (kNote);
    std::array<double, 4> t60 {}, centroid {}, inharm {};
    std::array<std::vector<double>, 4> profile;
    for (int m = 0; m < 4; ++m)
    {
        auto s = defaultRig();
        s.material = kMaterials[m];
        s.releaseDamping = 0.0f;
        const auto r = renderVoice (s, kNote, 0.8f, 4.0, 4.0, kSr);
        const auto spec = computeSpectrum (r.mono, kSr, 1 << 18, 0, static_cast<int> (1.5 * kSr));
        const std::string tag = materialName (kMaterials[m]);
        t60[static_cast<size_t> (m)] = measureT60 (r.mono, kSr, f0, 5.0, 30.0, 4.0);
        centroid[static_cast<size_t> (m)] = spectralCentroid (r.mono, kSr, 0, static_cast<int> (0.5 * kSr));
        profile[static_cast<size_t> (m)] = bandProfileDb (r.mono, 0.0, 1.0);
        // Modal distribution: the five strongest partials relative to f0.
        const auto parts = strongestPartials (spec, 6, f0 * 0.8, f0 * 13.0, f0 * 0.05);
        std::string plist;
        double dev = 0;
        int nDev = 0;
        for (auto& p : parts)
        {
            plist += std::to_string (p.freq / f0).substr (0, 5) + " ";
            // Inharmonicity: distance of each strong partial from the nearest harmonic.
            const double ratio = p.freq / f0;
            dev += std::abs (ratio - std::round (ratio)) / ratio;
            ++nDev;
        }
        inharm[static_cast<size_t> (m)] = nDev > 0 ? dev / nDev : 0.0;
        std::printf ("    %-9s strongest partials / f0: %s\n", tag.c_str(), plist.c_str());
        MEASURE (tag + ".t60_fundamental", t60[static_cast<size_t> (m)]);
        MEASURE (tag + ".centroidHz", centroid[static_cast<size_t> (m)]);
        MEASURE (tag + ".inharmonicity", inharm[static_cast<size_t> (m)]);

        // Characteristic modes of each material are present (strike, neutral tension).
        auto has = [&] (double ratio)
        {
            double mag = 0;
            const double f = peakFrequency (spec, f0 * ratio * 0.98, f0 * ratio * 1.02, &mag);
            double fundMag = 0;
            peakFrequency (spec, f0 * 0.98, f0 * 1.02, &fundMag);
            return f > 0 && std::abs (f / (f0 * ratio) - 1.0) < 0.012 && 20.0 * std::log10 (mag / fundMag) > -40.0;
        };
        switch (kMaterials[m])
        {
            case arc::MaterialType::glass:    CHECK (has (2.63)); CHECK (has (4.45)); CHECK (has (6.55)); break;
            case arc::MaterialType::metal:    CHECK (has (1.19)); CHECK (has (1.5)); CHECK (has (2.67)); break;
            case arc::MaterialType::wood:     CHECK (has (2.756)); CHECK (has (5.404)); break;
            case arc::MaterialType::membrane: CHECK (has (1.594)); CHECK (has (2.136)); CHECK (has (2.296)); break;
            case arc::MaterialType::count:    break;
        }
    }
    // Decay: metal rings longest, wood / membrane short.
    CHECK (t60[1] > t60[0]);
    CHECK (t60[0] > 2.0 * t60[2]);
    CHECK (t60[0] > 2.0 * t60[3]);
    // Wood is warm (lowest centroid of the struck materials with glass/metal bright).
    CHECK (centroid[2] < centroid[0]);
    CHECK (centroid[2] < centroid[1]);
    // Inharmonic modal sets: metal / glass / membrane far from the harmonic series; wood nearer.
    CHECK (inharm[3] > inharm[2]);
    CHECK (inharm[1] > inharm[2]);
    double minLsd = 1e9;
    for (int a = 0; a < 4; ++a)
        for (int b = a + 1; b < 4; ++b)
        {
            const double d = lsd (profile[static_cast<size_t> (a)], profile[static_cast<size_t> (b)]);
            MEASURE (std::string ("lsd.") + materialName (kMaterials[a]) + "_vs_" + materialName (kMaterials[b]), d);
            minLsd = std::min (minLsd, d);
        }
    CHECK (minLsd > 3.0);
}

TEST_CASE ("materials", "materials differ for every exciter")
{
    for (int e = 0; e < 4; ++e)
    {
        const auto type = static_cast<arc::ExciterType> (e);
        const bool sustained = e >= 2;
        std::array<std::vector<double>, 4> profile;
        for (int m = 0; m < 4; ++m)
        {
            auto s = defaultRig();
            s.exciter = type;
            s.material = kMaterials[m];
            const auto r = renderVoice (s, kNote, 0.8f, 1.6, 1.6, kSr);
            profile[static_cast<size_t> (m)] = bandProfileDb (r.mono, sustained ? 0.6 : 0.0, 0.9);
        }
        double minLsd = 1e9;
        for (int a = 0; a < 4; ++a)
            for (int b = a + 1; b < 4; ++b)
                minLsd = std::min (minLsd, lsd (profile[static_cast<size_t> (a)], profile[static_cast<size_t> (b)]));
        MEASURE (std::string (exciterName (type)) + ".minPairwiseLsdDb", minLsd);
        CHECK_MSG (minLsd > 2.0, exciterName (type));
    }
}

TEST_CASE ("materials", "material change is continuous")
{
    auto s = defaultRig();
    s.material = arc::MaterialType::metal;
    s.releaseDamping = 0.0f;
    const double switchAt = 0.8;
    const auto r = renderVoice (s, kNote, 0.9f, 2.5, 2.5, kSr, 1,
                                [switchAt] (double t, RigSettings& st)
                                {
                                    if (t >= switchAt)
                                        st.material = arc::MaterialType::glass;
                                });
    CHECK (allFinite (r.mono));
    // Click detector: HF level (> 12 kHz) in 5 ms frames. A discontinuity is a single-
    // frame spike above both neighbourhoods; the sweep of moving partials is not.
    const int frame = static_cast<int> (0.005 * kSr);
    std::vector<double> hf;
    for (int i = static_cast<int> ((switchAt - 0.1) * kSr); i + frame < static_cast<int> ((switchAt + 0.4) * kSr); i += frame)
        hf.push_back (highBandRatioDb (r.mono, kSr, 12000.0, i, frame) + 20.0 * std::log10 (rms (r.mono, i, frame) + 1e-12));
    double worstSpike = -1e9;
    for (size_t k = 3; k + 3 < hf.size(); ++k)
    {
        const double left = (hf[k - 1] + hf[k - 2] + hf[k - 3]) / 3.0, right = (hf[k + 1] + hf[k + 2] + hf[k + 3]) / 3.0;
        worstSpike = std::max (worstSpike, hf[k] - std::max (left, right));
    }
    double total = 0;
    for (int i = static_cast<int> ((switchAt) * kSr); i < static_cast<int> ((switchAt + 0.3) * kSr); i += frame)
        total = std::max (total, 0.0);
    MEASURE ("worstHfSpikeAboveNeighboursDb", worstSpike);
    CHECK (worstSpike < 12.0);
    // After the morph the glass mode (2.63) is present in the network.
    const double f0 = arc::dsp::midiToHz (kNote);
    const auto spec = computeSpectrum (r.mono, kSr, 1 << 17, static_cast<int> ((switchAt + 0.5) * kSr), static_cast<int> (1.0 * kSr));
    double mag = 0;
    const double f = peakFrequency (spec, f0 * 2.58, f0 * 2.68, &mag);
    MEASURE ("glassModeAfterMorph_ratio", f / f0);
    CHECK (std::abs (f / f0 - 2.63) < 0.03);
    writeWav (outputDir() + "/material_morph_metal_to_glass.wav", r.left, r.right, kSr);
}

TEST_CASE ("materials", "inspector modifiers")
{
    const double f0 = arc::dsp::midiToHz (kNote);
    auto render = [&] (arc::MaterialModifiers mods)
    {
        auto s = defaultRig();
        s.material = arc::MaterialType::metal;
        s.mods = mods;
        s.releaseDamping = 0.0f;
        return renderVoice (s, kNote, 0.8f, 3.0, 3.0, kSr);
    };
    arc::MaterialModifiers lo, hi;
    // LOSS
    lo.loss = 0.2f;
    hi.loss = 0.8f;
    // Decay slope between 0.2 s and 2.5 s (a LOSS of 0.2 rings ~19 s on METAL).
    auto dropDb = [&] (const RigResult& r)
    {
        return 20.0 * std::log10 (rms (r.mono, static_cast<int> (0.2 * kSr), 4800) / rms (r.mono, static_cast<int> (2.5 * kSr), 4800));
    };
    const double dropLow = dropDb (render (lo)), dropHigh = dropDb (render (hi));
    MEASURE ("loss0.2.dropDb_0.2to2.5s", dropLow);
    MEASURE ("loss0.8.dropDb_0.2to2.5s", dropHigh);
    CHECK (dropHigh > 2.5 * dropLow);
    // BRIGHTNESS
    lo = hi = {};
    lo.brightness = 0.1f;
    hi.brightness = 0.9f;
    const double cLo = spectralCentroid (render (lo).mono, kSr, static_cast<int> (0.3 * kSr), static_cast<int> (1.0 * kSr));
    const double cHi = spectralCentroid (render (hi).mono, kSr, static_cast<int> (0.3 * kSr), static_cast<int> (1.0 * kSr));
    MEASURE ("brightness0.1.centroid", cLo);
    MEASURE ("brightness0.9.centroid", cHi);
    CHECK (cHi > cLo * 1.2);
    // INHARMONICITY (dispersion): the stretch of the CORE's upper partials grows.
    lo = hi = {};
    lo.inharmonicity = 0.1f;
    hi.inharmonicity = 0.9f;
    auto stretch = [&] (const RigResult& r)
    {
        const auto spec = computeSpectrum (r.mono, kSr, 1 << 18, 0, static_cast<int> (1.5 * kSr));
        return peakFrequency (spec, f0 * 3.7, f0 * 4.6) / (4.0 * f0);
    };
    const double sLo = stretch (render (lo)), sHi = stretch (render (hi));
    MEASURE ("inharmonicity0.1.h4stretch", sLo);
    MEASURE ("inharmonicity0.9.h4stretch", sHi);
    CHECK (sHi > sLo);
    // MASS: heavier bodies ring longer.
    lo = hi = {};
    lo.mass = 0.1f;
    hi.mass = 0.9f;
    const double tLight = measureT60 (render (lo).mono, kSr, f0, 5.0, 30.0, 4.0);
    const double tHeavy = measureT60 (render (hi).mono, kSr, f0, 5.0, 30.0, 4.0);
    MEASURE ("mass0.1.t60", tLight);
    MEASURE ("mass0.9.t60", tHeavy);
    CHECK (tHeavy > tLight * 1.3);
}

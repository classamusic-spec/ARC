// Phase 3 — resonant network quality gate.

#include "Analysis.h"
#include "ArcTest.h"

#include "Synthesis/ResonantNetwork.h"

#include <fstream>

using namespace arc::dsp;
using namespace arctest;

namespace
{
constexpr double kSr = 48000.0;
constexpr int kControl = 16;

NetworkSettings makeSettings (double f0, const std::array<double, kNumNodes>& ratios, double t60, double t60High,
                              double dispersion, Topology topo, float phi)
{
    NetworkSettings s;
    const float pans[kNumNodes] = { 0.0f, -0.7f, 0.7f, -0.7f, 0.7f };
    for (int i = 0; i < kNumNodes; ++i)
    {
        auto& n = s.nodes[static_cast<size_t> (i)];
        n.frequency = f0 * ratios[static_cast<size_t> (i)];
        n.t60Fundamental = t60;
        n.t60High = t60High;
        n.hfReference = 5000.0;
        n.dispersion = dispersion;
        n.dispersionStages = 4;
        n.outputGain = i == 0 ? 1.0f : 0.8f;
        n.pan = pans[i];
    }
    const auto mask = topologyMask (topo);
    for (int e = 0; e < kNumEdges; ++e)
        s.edgeTheta[static_cast<size_t> (e)] = mask[static_cast<size_t> (e)] * generatorForRotation (phi);
    return s;
}

const std::array<double, kNumNodes> kHarmonicRatios { 1.0, 1.5, 2.0, 2.5, 3.0 };

struct Render
{
    Signal left, right, mono;
    std::vector<std::array<float, kNumNodes>> nodeEnergy; // per control block
};

/** Impulse into the given node weights, then ring. Settings re-applied every control block. */
Render renderNetwork (ResonantNetwork& net, const NetworkSettings& s, const std::array<float, kNumNodes>& inject,
                      double seconds)
{
    net.configure (s, true);
    const int n = static_cast<int> (seconds * kSr);
    Render r;
    r.left.resize (static_cast<size_t> (n));
    r.right.resize (static_cast<size_t> (n));
    r.mono.resize (static_cast<size_t> (n));
    float y[kNumNodes], inj[kNumNodes];
    for (int i = 0; i < n; ++i)
    {
        if (i % kControl == 0 && i > 0)
        {
            net.configure (s, false);
            r.nodeEnergy.push_back (net.takeNodeEnergy());
        }
        net.readOutputs (y);
        for (int k = 0; k < kNumNodes; ++k)
            inj[k] = i == 0 ? inject[static_cast<size_t> (k)] : 0.0f;
        net.writeInputs (y, inj);
        float l, rr;
        net.pickup (y, l, rr);
        r.left[static_cast<size_t> (i)] = l;
        r.right[static_cast<size_t> (i)] = rr;
        r.mono[static_cast<size_t> (i)] = 0.5f * (l + rr);
    }
    return r;
}

double dbPow (double e) { return 10.0 * std::log10 (e + 1e-30); }
} // namespace

TEST_CASE ("network", "cayley scattering is orthogonal")
{
    Random rng;
    rng.seed (1234);
    float worst = 0.0f;
    for (int t = 0; t < 2000; ++t)
    {
        std::array<float, kNumEdges> theta {};
        for (auto& v : theta)
            v = rng.uniform() * (t % 2 == 0 ? 2.0f : 20.0f);
        worst = std::max (worst, orthogonalityError (scatteringFromEdges (theta)));
    }
    MEASURE ("worstOrthogonalityError", worst);
    CHECK (worst < 2.0e-6f);

    // Single edge: rotation by phi between CORE and A.
    for (float phi : { 0.1f, 0.5f, 1.2f, 1.5707f })
    {
        std::array<float, kNumEdges> theta {};
        theta[0] = generatorForRotation (phi);
        const auto q = scatteringFromEdges (theta);
        CHECK (std::abs (q[0][0] - std::cos (phi)) < 1.0e-5f);
        CHECK (std::abs (std::abs (q[0][1]) - std::sin (phi)) < 1.0e-5f);
        CHECK (std::abs (q[2][2] - 1.0f) < 1.0e-6f);
    }
}

TEST_CASE ("network", "impulse into core propagates to every node")
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    const std::array<float, kNumNodes> coreOnly { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Uncoupled: energy must stay in the core.
    {
        auto s = makeSettings (110.0, kHarmonicRatios, 3.0, 1.0, 0.0, Topology::ring, 0.0f);
        net.reset();
        const auto r = renderNetwork (net, s, coreOnly, 0.5);
        double nodes = 0;
        for (auto& e : r.nodeEnergy)
            nodes += e[1] + e[2] + e[3] + e[4];
        MEASURE ("uncoupled.energyInNodesAtoD", nodes);
        CHECK (nodes == 0.0);
    }

    // Coupled (ring, phi = 0.2): energy arrives in every node.
    std::ofstream csv (outputDir() + "/network_impulse_node_energy.csv");
    csv << "time_s,core,A,B,C,D\n";
    for (Topology topo : { Topology::star, Topology::ring, Topology::chain })
    {
        auto s = makeSettings (110.0, kHarmonicRatios, 3.0, 1.0, 0.0, topo, 0.2f);
        net.reset();
        const auto r = renderNetwork (net, s, coreOnly, 3.0);
        std::array<double, kNumNodes> peak {}, peakTime {};
        for (size_t b = 0; b < r.nodeEnergy.size(); ++b)
        {
            const double t = (b + 1) * kControl / kSr;
            for (int i = 0; i < kNumNodes; ++i)
                if (r.nodeEnergy[b][static_cast<size_t> (i)] > peak[static_cast<size_t> (i)])
                {
                    peak[static_cast<size_t> (i)] = r.nodeEnergy[b][static_cast<size_t> (i)];
                    peakTime[static_cast<size_t> (i)] = t;
                }
            if (topo == Topology::ring && b % 30 == 0)
                csv << t << "," << r.nodeEnergy[b][0] << "," << r.nodeEnergy[b][1] << "," << r.nodeEnergy[b][2] << ","
                    << r.nodeEnergy[b][3] << "," << r.nodeEnergy[b][4] << "\n";
        }
        const std::string tn = topo == Topology::star ? "star" : topo == Topology::ring ? "ring" : "chain";
        for (int i = 1; i < kNumNodes; ++i)
        {
            const double relDb = dbPow (peak[static_cast<size_t> (i)]) - dbPow (peak[0]);
            MEASURE (tn + ".node" + std::string (1, "CABCD"[i]) + ".peakRelToCoreDb", relDb);
            MEASURE (tn + ".node" + std::string (1, "CABCD"[i]) + ".peakTime_s", peakTime[static_cast<size_t> (i)]);
            CHECK_MSG (relDb > -40.0, tn << " node " << i << " rel " << relDb);
        }
        CHECK (allFinite (r.mono));
        CHECK (peakAbs (r.mono) < 2.0);
        if (topo == Topology::chain)
        {
            // Along the chain CORE-A-B-D-C energy peaks later the further it travels.
            CHECK (peakTime[1] <= peakTime[2]);
            CHECK (peakTime[2] <= peakTime[4]);
            CHECK (peakTime[4] <= peakTime[3]);
        }
        if (topo == Topology::ring)
            writeWav (outputDir() + "/network_impulse_ring.wav", r.left, r.right, kSr);
    }
}

TEST_CASE ("network", "energy decays and output is bounded")
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    auto s = makeSettings (98.0, kHarmonicRatios, 2.0, 0.6, 0.15, Topology::ring, 0.35f);
    const std::array<float, kNumNodes> all { 1.0f, 0.5f, 0.5f, 0.5f, 0.5f };
    const auto r = renderNetwork (net, s, all, 5.0);
    const double t60 = measureBroadbandT60 (r.mono, kSr);
    MEASURE ("broadbandT60_s", t60);
    const double e1 = rms (r.mono, static_cast<int> (0.2 * kSr), 4800);
    const double e2 = rms (r.mono, static_cast<int> (2.0 * kSr), 4800);
    const double e3 = rms (r.mono, static_cast<int> (4.5 * kSr), 4800);
    MEASURE ("rms@0.2s", e1);
    MEASURE ("rms@2.0s", e2);
    MEASURE ("rms@4.5s", e3);
    CHECK (e2 < e1);
    CHECK (e3 < e2 * 0.1);
    CHECK (t60 > 0.8 && t60 < 3.0);
    CHECK (peakAbs (r.mono) < 3.0);
}

TEST_CASE ("network", "maximum coupling stays stable")
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    const std::array<float, kNumNodes> all { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    for (float phi : { 0.8f, 1.2f, 1.5707f })
    {
        auto s = makeSettings (65.0, { 1.0, 1.13, 2.76, 5.4, 0.51 }, 30.0, 10.0, 0.5, Topology::web, phi);
        net.reset();
        const auto r = renderNetwork (net, s, all, 10.0);
        CHECK (allFinite (r.mono));
        // Energy never grows: compare 1-second RMS windows.
        double prev = 1e9;
        bool monotone = true;
        for (int sec = 1; sec < 10; ++sec)
        {
            const double e = rms (r.mono, static_cast<int> (sec * kSr), static_cast<int> (kSr));
            monotone = monotone && e <= prev * 1.05;
            prev = e;
        }
        MEASURE ("phi" + std::to_string (phi).substr (0, 4) + ".peak", peakAbs (r.mono));
        CHECK (monotone);
        CHECK (peakAbs (r.mono) < 4.0);
    }
}

namespace
{
struct FuzzResult
{
    int nonFinite = 0, configs = 0, overInjected = 0;
    double worstRatio = 0.0, worstPeak = 0.0, worstTransient = 0.0;
};

/** Random networks, burst excitation, then ringing while every parameter is
    modulated at control rate. `abusive`: +-2.5 % delay jumps and large coupling
    steps every 16 samples. Otherwise: smooth drift (<= 1 %, < 20 Hz), as produced
    by ARC's motion/chaos smoothing. */
FuzzResult runFuzz (bool abusive, bool lossyOnly, int configs, uint32_t seed)
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    Random rng;
    rng.seed (seed);
    FuzzResult res;
    res.configs = configs;
    for (int c = 0; c < configs; ++c)
    {
        const double f0 = 20.0 * std::pow (200.0, rng.uniform());
        std::array<double, kNumNodes> ratios {};
        for (auto& v : ratios)
            v = 0.25 * std::pow (32.0, rng.uniform());
        const bool nearLossless = ! lossyOnly && rng.uniform() < 0.2;
        const double t60 = nearLossless ? 1.0e5 : 0.01 * std::pow (lossyOnly ? 3.0e3 : 1.0e4, rng.uniform());
        auto s = makeSettings (f0, ratios, t60, t60 * (0.05 + rng.uniform()), rng.uniform() * 0.8,
                               static_cast<Topology> (static_cast<int> (rng.uniform() * 4.0f) % 4),
                               rng.uniform() * 1.6f);
        for (auto& n : s.nodes)
            n.dispersionStages = static_cast<int> (rng.uniform() * 7.0f);
        std::array<double, kNumNodes> baseFreq {};
        for (int k = 0; k < kNumNodes; ++k)
            baseFreq[static_cast<size_t> (k)] = s.nodes[static_cast<size_t> (k)].frequency;
        const auto baseTheta = s.edgeTheta;
        const double driftRate = 0.5 + 19.0 * rng.uniform();
        net.reset();
        net.configure (s, true);

        double injected = 0.0, peak = 0.0, worstTransient = 0.0;
        float y[kNumNodes], inj[kNumNodes];
        // Smooth runs are 10x longer: passivity is judged on where the energy goes,
        // not on the transient redistribution between filter states and delay lines
        // (the stored-energy measure only counts line contents).
        const int n = abusive ? 4800 : 48000;
        for (int i = 0; i < n; ++i)
        {
            if (i % kControl == 0 && i > 0)
            {
                if (abusive)
                {
                    for (auto& node : s.nodes)
                        node.frequency *= std::exp2 ((rng.uniform() - 0.5f) * 0.05f);
                    for (auto& th : s.edgeTheta)
                        th = std::max (0.0f, th + (rng.uniform() - 0.5f) * 0.4f);
                }
                else
                {
                    const double ph = kTwoPi * driftRate * i / kSr;
                    for (int k = 0; k < kNumNodes; ++k)
                        s.nodes[static_cast<size_t> (k)].frequency =
                            baseFreq[static_cast<size_t> (k)] * (1.0 + 0.01 * std::sin (ph + k));
                    for (int e = 0; e < kNumEdges; ++e)
                        s.edgeTheta[static_cast<size_t> (e)] =
                            baseTheta[static_cast<size_t> (e)] * static_cast<float> (1.0 + 0.3 * std::sin (0.7 * ph + e));
                }
                net.configure (s, false);
                if (! abusive && i % 1024 == 0 && injected > 0.0)
                    worstTransient = std::max (worstTransient, net.storedEnergy() / injected);
            }
            net.readOutputs (y);
            for (int k = 0; k < kNumNodes; ++k)
            {
                inj[k] = i < 64 ? rng.bipolar() : 0.0f;
                injected += static_cast<double> (inj[k]) * inj[k];
                if (! std::isfinite (y[k]))
                    ++res.nonFinite;
                peak = std::max (peak, static_cast<double> (std::abs (y[k])));
            }
            net.writeInputs (y, inj);
        }
        const double ratio = net.storedEnergy() / injected;
        res.worstRatio = std::max (res.worstRatio, ratio);
        res.worstTransient = std::max (res.worstTransient, worstTransient);
        res.worstPeak = std::max (res.worstPeak, peak);
        if (ratio > (abusive ? 3.0 : 1.05) || worstTransient > 1.5)
            ++res.overInjected;
    }
    return res;
}
} // namespace

TEST_CASE ("network", "stability fuzz")
{
    // Abusive modulation on any network, including near-lossless ones: must stay
    // finite and bounded (parametric pumping of a lossless loop is bounded here and
    // limited long-term by the voice energy governor used by FREEZE).
    const auto abusive = runFuzz (true, false, 1500, 0xA11CE);
    MEASURE ("abusive.configs", abusive.configs);
    MEASURE ("abusive.nonFiniteSamples", abusive.nonFinite);
    MEASURE ("abusive.worstStoredOverInjected", abusive.worstRatio);
    MEASURE ("abusive.worstPeak", abusive.worstPeak);
    CHECK (abusive.nonFinite == 0);
    CHECK (abusive.overInjected == 0);
    CHECK (abusive.worstPeak < 50.0);

    // Realistic smooth modulation on lossy networks: passive — after 1 s the stored
    // energy is below what was injected; redistribution transients stay < 1.5x.
    const auto smooth = runFuzz (false, true, 1500, 0xBEEF);
    MEASURE ("smooth.configs", smooth.configs);
    MEASURE ("smooth.nonFiniteSamples", smooth.nonFinite);
    MEASURE ("smooth.worstStoredOverInjected_at1s", smooth.worstRatio);
    MEASURE ("smooth.worstTransientRatio", smooth.worstTransient);
    MEASURE ("smooth.worstPeak", smooth.worstPeak);
    CHECK (smooth.nonFinite == 0);
    CHECK (smooth.overInjected == 0);
}

TEST_CASE ("network", "topologies sound different")
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    const std::array<float, kNumNodes> coreOnly { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::vector<Spectrum> spectra;
    const char* names[] = { "star", "ring", "web", "chain" };
    for (int t = 0; t < 4; ++t)
    {
        auto s = makeSettings (110.0, { 1.0, 1.19, 1.5, 2.0, 2.52 }, 3.0, 1.0, 0.1, static_cast<Topology> (t), 0.45f);
        net.reset();
        const auto r = renderNetwork (net, s, coreOnly, 2.0);
        spectra.push_back (computeSpectrum (r.mono, kSr, 1 << 17));
        writeWav (outputDir() + std::string ("/network_topology_") + names[t] + ".wav", r.left, r.right, kSr);
    }
    // Log-spectral distance between 50 Hz and 8 kHz (1/3-octave-ish smoothing via band energy).
    auto bands = [] (const Spectrum& s)
    {
        std::vector<double> b;
        for (double f = 50.0; f < 8000.0; f *= std::pow (2.0, 1.0 / 6.0))
        {
            const int lo = static_cast<int> (f / s.binHz), hi = static_cast<int> (f * std::pow (2.0, 1.0 / 6.0) / s.binHz);
            double e = 0;
            for (int i = lo; i < hi; ++i)
                e += s.mag[static_cast<size_t> (i)] * s.mag[static_cast<size_t> (i)];
            b.push_back (10.0 * std::log10 (e + 1e-20));
        }
        return b;
    };
    double minDist = 1e9;
    for (int a = 0; a < 4; ++a)
        for (int b = a + 1; b < 4; ++b)
        {
            const auto ba = bands (spectra[static_cast<size_t> (a)]), bb = bands (spectra[static_cast<size_t> (b)]);
            double d = 0;
            for (size_t i = 0; i < ba.size(); ++i)
                d += (ba[i] - bb[i]) * (ba[i] - bb[i]);
            d = std::sqrt (d / ba.size());
            MEASURE (std::string ("lsd_") + names[a] + "_vs_" + names[b] + "_dB", d);
            minDist = std::min (minDist, d);
        }
    CHECK (minDist > 1.0);
}

TEST_CASE ("network", "coupling automation is safe")
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    auto s = makeSettings (130.8, kHarmonicRatios, 4.0, 1.2, 0.1, Topology::ring, 0.0f);
    net.configure (s, true);
    const int n = static_cast<int> (1.2 * kSr);
    Signal out (static_cast<size_t> (n));
    float y[kNumNodes], inj[kNumNodes];
    const std::array<float, kNumNodes> all { 1.0f, 0.5f, 0.5f, 0.5f, 0.5f };
    const int sweepStart = static_cast<int> (0.3 * kSr), sweepLen = static_cast<int> (0.2 * kSr);
    for (int i = 0; i < n; ++i)
    {
        if (i % kControl == 0 && i > 0)
        {
            float phi = 0.0f;
            if (i >= sweepStart && i < sweepStart + 2 * sweepLen)
            {
                const float t = static_cast<float> (i - sweepStart) / sweepLen;
                phi = 1.5f * (t < 1.0f ? t : 2.0f - t); // 0 -> 1.5 rad -> 0 in 400 ms
            }
            const auto mask = topologyMask (Topology::ring);
            for (int e = 0; e < kNumEdges; ++e)
                s.edgeTheta[static_cast<size_t> (e)] = mask[static_cast<size_t> (e)] * generatorForRotation (phi);
            net.configure (s, false);
        }
        net.readOutputs (y);
        for (int k = 0; k < kNumNodes; ++k)
            inj[k] = i == 0 ? all[static_cast<size_t> (k)] : 0.0f;
        net.writeInputs (y, inj);
        float l, r;
        net.pickup (y, l, r);
        out[static_cast<size_t> (i)] = 0.5f * (l + r);
    }
    double curvBefore = 0, curvDuring = 0;
    for (int i = sweepStart - 4800; i < sweepStart + 2 * sweepLen; ++i)
    {
        const auto u = static_cast<size_t> (i);
        const double d2 = std::abs (out[u] - 2.0f * out[u - 1] + out[u - 2]);
        (i < sweepStart ? curvBefore : curvDuring) = std::max (i < sweepStart ? curvBefore : curvDuring, d2);
    }
    MEASURE ("curvature_before", curvBefore);
    MEASURE ("curvature_during_sweep", curvDuring);
    MEASURE ("hfRatioDb_before", highBandRatioDb (out, kSr, 12000.0, sweepStart - 4096, 4096));
    MEASURE ("hfRatioDb_during", highBandRatioDb (out, kSr, 12000.0, sweepStart + sweepLen - 2048, 4096));
    CHECK (allFinite (out));
    CHECK (curvDuring < 2.0 * curvBefore);
    writeWav (outputDir() + "/network_coupling_sweep.wav", out, kSr);
}

TEST_CASE ("network", "lossless network conserves energy")
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    auto s = makeSettings (110.0, { 1.0, 1.19, 1.5, 2.0, 2.52 }, 1.0e7, 1.0e7, 0.2, Topology::web, 0.8f);
    net.configure (s, true);
    float y[kNumNodes], inj[kNumNodes];
    Random rng;
    const int n = static_cast<int> (10.0 * kSr);
    double e0 = 0, eEnd = 0, eMax = 0, eMin = 1e30;
    for (int i = 0; i < n; ++i)
    {
        net.readOutputs (y);
        for (int k = 0; k < kNumNodes; ++k)
            inj[k] = i < 32 ? rng.bipolar() * 0.3f : 0.0f;
        net.writeInputs (y, inj);
        if (i == static_cast<int> (0.1 * kSr))
            e0 = net.storedEnergy();
        if (i > static_cast<int> (0.1 * kSr) && i % 4800 == 0)
        {
            const double e = net.storedEnergy();
            eMax = std::max (eMax, e);
            eMin = std::min (eMin, e);
        }
    }
    eEnd = net.storedEnergy();
    MEASURE ("energy@0.1s", e0);
    MEASURE ("energy@10s", eEnd);
    MEASURE ("ratio_10s_over_0.1s", eEnd / e0);
    MEASURE ("maxOverMinDuringRun", eMax / eMin);
    // The line-content measure excludes filter state, so allow a few % fluctuation.
    CHECK (eEnd / e0 > 0.97 && eEnd / e0 < 1.03);
    CHECK (eMax / eMin < 1.1);
}

TEST_CASE ("network", "telemetry reflects dsp")
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    const std::array<float, kNumNodes> coreOnly { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    for (float phi : { 0.0f, 0.3f })
    {
        auto s = makeSettings (110.0, kHarmonicRatios, 3.0, 1.0, 0.0, Topology::star, phi);
        net.reset();
        net.configure (s, true);
        float y[kNumNodes], inj[kNumNodes] {};
        double manual[kNumNodes] {};
        const int n = 4800;
        for (int i = 0; i < n; ++i)
        {
            net.readOutputs (y);
            for (int k = 0; k < kNumNodes; ++k)
            {
                manual[k] += static_cast<double> (y[k]) * y[k];
                inj[k] = i == 0 ? coreOnly[static_cast<size_t> (k)] : 0.0f;
            }
            net.writeInputs (y, inj);
        }
        const auto e = net.takeNodeEnergy();
        for (int k = 0; k < kNumNodes; ++k)
            CHECK (std::abs (e[static_cast<size_t> (k)] - manual[k] / n) <= 1.0e-4 * (manual[k] / n) + 1e-12);
        const auto flux = net.edgeFlux (e);
        const double spokes = flux[0] + flux[1] + flux[2] + flux[3];
        const double ring = flux[4] + flux[5] + flux[6] + flux[7];
        MEASURE ("phi" + std::to_string (phi).substr (0, 3) + ".spokeFlux", spokes);
        if (phi == 0.0f)
            CHECK (spokes == 0.0);
        else
            CHECK (spokes > 0.0);
        CHECK (ring == 0.0); // star topology has no ring edges
    }
}

TEST_CASE ("network", "fundamental stays in tune when coupled")
{
    ResonantNetwork net;
    net.prepare (kSr, 15.0, kControl);
    const std::array<float, kNumNodes> all { 1.0f, 0.4f, 0.4f, 0.4f, 0.4f };
    for (float phi : { 0.0f, 0.1f, 0.2f, 0.35f })
    {
        auto s = makeSettings (130.81, kHarmonicRatios, 3.0, 1.0, 0.0, Topology::ring, phi);
        net.reset();
        const auto r = renderNetwork (net, s, all, 3.0);
        const double f = refineFrequencyByPhase (r.mono, kSr, peakFrequency (r.mono, kSr, 125.0, 136.0));
        MEASURE ("phi" + std::to_string (phi).substr (0, 4) + ".fundamentalCents", centsBetween (f, 130.81));
    }
}

TEST_CASE ("network", "cpu")
{
    for (double sr : { 48000.0, 96000.0 })
    {
        ResonantNetwork net;
        net.prepare (sr, 15.0, kControl);
        auto s = makeSettings (110.0, kHarmonicRatios, 3.0, 1.0, 0.3, Topology::web, 0.4f);
        net.configure (s, true);
        float y[kNumNodes], inj[kNumNodes] {};
        volatile float sink = 0.0f;
        long i = 0;
        // Static tuning.
        const double nsStatic = nanosPerCall ([&]
                                              {
                                                  if ((i & 15) == 0)
                                                      net.configure (s, false);
                                                  net.readOutputs (y);
                                                  inj[0] = (i & 8191) == 0 ? 0.5f : 0.0f;
                                                  net.writeInputs (y, inj);
                                                  sink = y[0];
                                                  ++i;
                                              },
                                              2'000'000);
        // Continuously drifting tuning (motion / chaos active).
        long designsBefore = net.fullDesignCount, retunesBefore = net.retuneCount;
        const double nsDrift = nanosPerCall ([&]
                                             {
                                                 if ((i & 15) == 0)
                                                 {
                                                     const double d = std::sin (i * 1.0e-5) * 0.02;
                                                     for (int k = 0; k < kNumNodes; ++k)
                                                         s.nodes[static_cast<size_t> (k)].frequency =
                                                             110.0 * kHarmonicRatios[static_cast<size_t> (k)] * (1.0 + d);
                                                     net.configure (s, false);
                                                 }
                                                 net.readOutputs (y);
                                                 inj[0] = (i & 8191) == 0 ? 0.5f : 0.0f;
                                                 net.writeInputs (y, inj);
                                                 sink = y[0];
                                                 ++i;
                                             },
                                             2'000'000);
        (void) sink;
        const std::string tag = sr == 48000.0 ? "@48k" : "@96k";
        MEASURE ("nsPerSample_static" + tag, nsStatic);
        MEASURE ("nsPerSample_drifting" + tag, nsDrift);
        MEASURE ("fullDesignsPerSecond_drifting" + tag,
                 (net.fullDesignCount - designsBefore) / (2.0e6 / sr));
        MEASURE ("retunesPerSecond_drifting" + tag, (net.retuneCount - retunesBefore) / (2.0e6 / sr));
        MEASURE ("coreFractionPerVoice_drifting" + tag, nsDrift * sr * 1e-9);
        CHECK (nsDrift < 400.0);
    }
}

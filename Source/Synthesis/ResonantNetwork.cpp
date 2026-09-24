#include "Synthesis/ResonantNetwork.h"

namespace arc::dsp
{

namespace
{
inline double relDiff (double a, double b) noexcept
{
    const double m = std::max (std::abs (a), std::abs (b));
    return m > 0.0 ? std::abs (a - b) / m : 0.0;
}
} // namespace

void ResonantNetwork::prepare (double newSampleRate, double minFrequency, int newControlInterval)
{
    sampleRate = newSampleRate;
    controlInterval = std::max (1, newControlInterval);
    for (auto& l : loops)
        l.prepare (sampleRate, minFrequency);
    for (auto& c : cache)
        c.valid = false;
    hasMatrix = false;
    reset();
}

void ResonantNetwork::reset() noexcept
{
    for (auto& l : loops)
        l.reset();
    energyAcc.fill (0.0f);
    energyCount = 0;
}

void ResonantNetwork::clearState() noexcept
{
    for (auto& l : loops)
        l.clearState();
    energyAcc.fill (0.0f);
    energyCount = 0;
}

void ResonantNetwork::updateLoop (int i, const NodeSettings& ns, bool immediate) noexcept
{
    auto& loop = loops[static_cast<size_t> (i)];
    auto& c = cache[static_cast<size_t> (i)];

    ResonatorSettings rs;
    rs.frequency = ns.frequency;
    rs.t60Fundamental = ns.t60Fundamental;
    rs.t60High = ns.t60High;
    rs.hfReference = ns.hfReference;
    rs.dispersion = ns.dispersion;
    rs.dispersionStages = ns.dispersionStages;
    rs.interpolation = Interpolation::thiran1;

    const int preferredK = loop.hasCoefficients() ? loop.currentIntegerDelay() : -1;
    const int ramp = immediate ? 0 : controlInterval;

    const bool needFull = ! c.valid || relDiff (rs.t60Fundamental, c.settings.t60Fundamental) > 1.0e-3
                          || relDiff (rs.t60High, c.settings.t60High) > 1.0e-3
                          || relDiff (rs.hfReference, c.settings.hfReference) > 1.0e-3
                          || std::abs (rs.dispersion - c.settings.dispersion) > 1.0e-4
                          || rs.dispersionStages != c.settings.dispersionStages
                          || relDiff (rs.frequency, c.settings.frequency) > 0.003;
    if (needFull)
    {
        c.coeffs = designLoop (rs, sampleRate, loop.maxLineDelay(), preferredK);
        c.settings = rs;
        c.valid = true;
        loop.setCoefficients (c.coeffs, ramp);
        ++fullDesignCount;
        return;
    }

    // Small frequency move: retune the line only (skip if unchanged).
    if (relDiff (rs.frequency, loop.getCoefficients().totalDelay > 0.0
                                   ? sampleRate / loop.getCoefficients().totalDelay
                                   : 0.0)
        < 1.0e-9)
        return;

    loop.setCoefficients (retuneLoop (c.coeffs, rs.frequency, sampleRate, loop.maxLineDelay(),
                                      Interpolation::thiran1, preferredK),
                          ramp);
    ++retuneCount;
}

void ResonantNetwork::configure (const NetworkSettings& settings, bool immediate) noexcept
{
    for (int i = 0; i < kNumNodes; ++i)
        updateLoop (i, settings.nodes[static_cast<size_t> (i)], immediate);

    // Scattering matrix: recompute only when the generator changed.
    bool thetaChanged = ! hasMatrix;
    for (int e = 0; e < kNumEdges && ! thetaChanged; ++e)
        thetaChanged = std::abs (settings.edgeTheta[static_cast<size_t> (e)] - lastTheta[static_cast<size_t> (e)]) > 1.0e-7f;
    if (thetaChanged)
    {
        std::array<float, kNumEdges> theta {};
        for (int e = 0; e < kNumEdges; ++e)
            theta[static_cast<size_t> (e)] = std::max (0.0f, settings.edgeTheta[static_cast<size_t> (e)]);
        qTarget = scatteringFromEdges (theta);
        lastTheta = settings.edgeTheta;
    }

    std::array<float, kNumNodes> targetL {}, targetR {};
    for (int i = 0; i < kNumNodes; ++i)
    {
        const auto& ns = settings.nodes[static_cast<size_t> (i)];
        const float angle = (clamp (ns.pan, -1.0f, 1.0f) + 1.0f) * 0.25f * kPiF;
        targetL[static_cast<size_t> (i)] = ns.outputGain * std::cos (angle);
        targetR[static_cast<size_t> (i)] = ns.outputGain * std::sin (angle);
    }

    if (immediate || ! hasMatrix)
    {
        q = qTarget;
        gainL = targetL;
        gainR = targetR;
        rampRemaining = 0;
        hasMatrix = true;
        return;
    }

    const float inv = 1.0f / static_cast<float> (controlInterval);
    for (int i = 0; i < kNumNodes; ++i)
    {
        for (int j = 0; j < kNumNodes; ++j)
            dq[static_cast<size_t> (i)][static_cast<size_t> (j)] =
                (qTarget[static_cast<size_t> (i)][static_cast<size_t> (j)] - q[static_cast<size_t> (i)][static_cast<size_t> (j)]) * inv;
        dGainL[static_cast<size_t> (i)] = (targetL[static_cast<size_t> (i)] - gainL[static_cast<size_t> (i)]) * inv;
        dGainR[static_cast<size_t> (i)] = (targetR[static_cast<size_t> (i)] - gainR[static_cast<size_t> (i)]) * inv;
    }
    rampRemaining = controlInterval;
}

void ResonantNetwork::process (const float* excitation, const float* injectWeights, float* outL, float* outR,
                               int n) noexcept
{
    float y[kNumNodes], inj[kNumNodes];
    for (int s = 0; s < n; ++s)
    {
        readOutputs (y);
        const float e = excitation != nullptr ? excitation[s] : 0.0f;
        for (int i = 0; i < kNumNodes; ++i)
            inj[i] = e * injectWeights[i];
        writeInputs (y, inj);
        float l, r;
        pickup (y, l, r);
        outL[s] = l;
        outR[s] = r;
    }
}

std::array<float, kNumNodes> ResonantNetwork::takeNodeEnergy() noexcept
{
    std::array<float, kNumNodes> e {};
    if (energyCount > 0)
        for (int i = 0; i < kNumNodes; ++i)
            e[static_cast<size_t> (i)] = energyAcc[static_cast<size_t> (i)] / static_cast<float> (energyCount);
    energyAcc.fill (0.0f);
    energyCount = 0;
    return e;
}

std::array<float, kNumEdges> ResonantNetwork::edgeFlux (const std::array<float, kNumNodes>& e) const noexcept
{
    // Each edge's own rotation phi_e = 2 atan(theta_e / 2) moves sin^2(phi_e) of each
    // endpoint's per-sample energy across the edge. (Entries of the full Q also contain
    // indirect paths through shared nodes; those are attributed to the edges they use.)
    std::array<float, kNumEdges> flux {};
    for (int k = 0; k < kNumEdges; ++k)
    {
        const auto& edge = kEdges[static_cast<size_t> (k)];
        const float phi = 2.0f * std::atan (0.5f * lastTheta[static_cast<size_t> (k)]);
        const float s = std::sin (phi);
        flux[static_cast<size_t> (k)] = s * s * (e[static_cast<size_t> (edge.a)] + e[static_cast<size_t> (edge.b)]);
    }
    return flux;
}

double ResonantNetwork::storedEnergy() const noexcept
{
    double e = 0.0;
    for (auto& l : loops)
        e += l.lineEnergy();
    return e;
}

} // namespace arc::dsp

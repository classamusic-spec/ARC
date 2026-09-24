#include "Synthesis/ResonantNetwork.h"

#include <complex>

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
    for (auto& f : injectFilter)
        f.reset();
    for (auto& f : pickupFilter)
        f.reset();
    energyAcc.fill (0.0f);
    energyCount = 0;
}

void ResonantNetwork::clearState() noexcept
{
    for (auto& l : loops)
        l.clearState();
    for (auto& f : injectFilter)
        f.reset();
    for (auto& f : pickupFilter)
        f.reset();
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
    rs.loopSelectivity = ns.loopSelectivity;
    rs.interpolation = Interpolation::thiran1;

    const int preferredK = loop.hasCoefficients() ? loop.currentIntegerDelay() : -1;
    const int ramp = immediate ? 0 : controlInterval;

    const bool needFull = ! c.valid || relDiff (rs.t60Fundamental, c.settings.t60Fundamental) > 1.0e-3
                          || relDiff (rs.t60High, c.settings.t60High) > 1.0e-3
                          || relDiff (rs.hfReference, c.settings.hfReference) > 1.0e-3
                          || std::abs (rs.dispersion - c.settings.dispersion) > 1.0e-4
                          || rs.dispersionStages != c.settings.dispersionStages
                          || std::abs (rs.loopSelectivity - c.settings.loopSelectivity) > 1.0e-3
                          || relDiff (rs.frequency, c.settings.frequency) > 0.003;
    if (needFull)
    {
        // Stage count is chosen at note start (immediate) and locked afterwards.
        const int locked = (immediate || ! c.valid) ? -1 : c.coeffs.dispStages;
        c.coeffs = designLoop (rs, sampleRate, loop.maxLineDelay(), preferredK, locked);
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
    {
        const auto& ns = settings.nodes[static_cast<size_t> (i)];
        updateLoop (i, ns, immediate);
        const auto sel = SelectivityStage::design (kTwoPi * clamp (ns.frequency, 1.0, 0.45 * sampleRate) / sampleRate,
                                                   kSelectivityQ, ns.selectivity);
        injectFilter[static_cast<size_t> (i)].set (sel);
        pickupFilter[static_cast<size_t> (i)].set (sel);
    }

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

std::array<ResonantNetwork::ModeCorrection, kNumNodes>
    ResonantNetwork::estimateModeCorrections (const std::array<double, kNumNodes>& modeFrequency) const noexcept
{
    using cd = std::complex<double>;
    std::array<ModeCorrection, kNumNodes> out {};
    if (! hasMatrix)
        return out;

    for (int i = 0; i < kNumNodes; ++i)
    {
        const auto& ci = loops[static_cast<size_t> (i)].getCoefficients();
        if (ci.totalDelay <= 0.0)
            continue;
        // Evaluate where the coupled mode must land: the mode condition
        // H_i(w) R_i(w) = 1 is imposed there, not at the loop's own tuning.
        const double fm = modeFrequency[static_cast<size_t> (i)];
        const double wi = fm > 0.0 ? kTwoPi * std::min (fm, 0.45 * sampleRate) / sampleRate : kTwoPi / ci.totalDelay;

        // Exact reduction onto loop i (Schur complement over the other four loops):
        //   R_i = Q_ii + Q_io (I - D_o Q_oo)^-1 D_o Q_oi,   D_o = diag(H_k(w_i))
        int idx[kNumNodes - 1];
        for (int k = 0, m = 0; k < kNumNodes; ++k)
            if (k != i)
                idx[m++] = k;

        cd h[kNumNodes - 1];
        double minResDistance = 1.0e9;
        for (int m = 0; m < kNumNodes - 1; ++m)
        {
            double mag, d;
            loopResponse (loops[static_cast<size_t> (idx[m])].getCoefficients(), wi, mag, d);
            h[m] = std::polar (mag, -wi * d);
            // Phase distance to loop k's nearest resonance *excluding its DC mode*
            // (short loops sit close to their DC mode at any low frequency; that is
            // not a coincidence and must not taper the correction).
            const double phase = wi * d;
            const double harmonic = std::max (1.0, std::round (phase / kTwoPi));
            const double dphi = phase - kTwoPi * harmonic;
            minResDistance = std::min (minResDistance, dphi * dphi);
        }

        // Coincidence tapers. Near another loop's resonance the two modes split
        // symmetrically (avoided crossing): that "shift" is not a tuning error, and the
        // per-loop correction is ill-posed. Two measures, both ~0.07 rad wide:
        //  - dynamic: distance of w_i from the other loops' *current* resonances;
        //  - static: distance between the *intended* mode frequencies, which the
        //    corrections never move, so it cannot feed back through them (measured:
        //    a node 20 cents from unison with the CORE drove the dynamic taper and the
        //    iteration into a limit cycle, i.e. audio-rate loop FM that pumped energy).
        //    Two loops coupled by |Q_ik| split by ~2|Q_ik| rad of loop phase; the
        //    ill-posed core of that region (|dphi| < |Q_ik|) is tapered and the 4th-order edge keeps
        //    well-separated nodes fully compensated.
        double staticTaper = 1.0;
        for (int k = 0; k < kNumNodes; ++k)
        {
            const double fk = modeFrequency[static_cast<size_t> (k)];
            if (k == i || fk <= 0.0 || fm <= 0.0)
                continue;
            const double ratio = fm / fk;
            const double dphi = kTwoPi * (ratio - std::max (1.0, std::round (ratio)));
            const double qik = static_cast<double> (qTarget[static_cast<size_t> (i)][static_cast<size_t> (k)]);
            const double w2 = 0.005 + qik * qik;
            const double d4 = dphi * dphi * dphi * dphi;
            staticTaper = std::min (staticTaper, d4 / (d4 + w2 * w2));
        }
        const double taper = minResDistance / (minResDistance + 0.005) * staticTaper;

        cd a[kNumNodes - 1][kNumNodes - 1], x[kNumNodes - 1];
        for (int r = 0; r < kNumNodes - 1; ++r)
        {
            for (int c = 0; c < kNumNodes - 1; ++c)
                a[r][c] = (r == c ? 1.0 : 0.0)
                          - h[r] * static_cast<double> (qTarget[static_cast<size_t> (idx[r])][static_cast<size_t> (idx[c])]);
            x[r] = h[r] * static_cast<double> (qTarget[static_cast<size_t> (idx[r])][static_cast<size_t> (i)]);
        }
        // Gaussian elimination with partial pivoting (4x4 complex).
        bool singular = false;
        for (int k = 0; k < kNumNodes - 1 && ! singular; ++k)
        {
            int piv = k;
            for (int r = k + 1; r < kNumNodes - 1; ++r)
                if (std::abs (a[r][k]) > std::abs (a[piv][k]))
                    piv = r;
            if (std::abs (a[piv][k]) < 1.0e-12)
            {
                singular = true;
                break;
            }
            if (piv != k)
            {
                for (int c = 0; c < kNumNodes - 1; ++c)
                    std::swap (a[k][c], a[piv][c]);
                std::swap (x[k], x[piv]);
            }
            for (int r = k + 1; r < kNumNodes - 1; ++r)
            {
                const cd f = a[r][k] / a[k][k];
                for (int c = k; c < kNumNodes - 1; ++c)
                    a[r][c] -= f * a[k][c];
                x[r] -= f * x[k];
            }
        }
        if (singular)
            continue;
        for (int r = kNumNodes - 2; r >= 0; --r)
        {
            cd v = x[r];
            for (int c = r + 1; c < kNumNodes - 1; ++c)
                v -= a[r][c] * x[c];
            x[r] = v / a[r][r];
        }
        cd rr (qTarget[static_cast<size_t> (i)][static_cast<size_t> (i)], 0.0);
        for (int m = 0; m < kNumNodes - 1; ++m)
            rr += static_cast<double> (qTarget[static_cast<size_t> (i)][static_cast<size_t> (idx[m])]) * x[m];

        // Mode condition at the intended frequency w: the loop must provide phase
        // 2 pi + arg(R) there, i.e. a loop tuned to f / (1 + arg(R) / 2 pi) (exact for a
        // loop whose phase delay is flat between its tuning and w, as at the fundamental).
        out[static_cast<size_t> (i)].frequencyShift = clamp (taper * std::arg (rr) / kTwoPi, -0.12, 0.12);
        out[static_cast<size_t> (i)].gainFactor = clamp (1.0 + taper * (std::abs (rr) - 1.0), 0.2, 1.0);
    }
    return out;
}

double ResonantNetwork::estimateCoreDetune (double coreFrequency) const noexcept
{
    (void) coreFrequency;
    return estimateModeCorrections()[0].frequencyShift;
}

double ResonantNetwork::storedEnergy() const noexcept
{
    double e = 0.0;
    for (auto& l : loops)
        e += l.lineEnergy();
    return e;
}

} // namespace arc::dsp

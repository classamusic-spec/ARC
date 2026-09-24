#pragma once

// The resonant network: five waveguide loops (CORE, A, B, C, D) exchanging energy
// through an orthogonal scattering matrix. This *is* ARC's oscillator.
//
//   per sample:   y_i = loop_i.read()                    (loop outputs)
//                 u   = Q(n) y                           (energy-preserving scattering)
//                 loop_i.write(u_i + injection_i)        (exciter energy enters here)
//                 out = sum_i  g_i * pan_i * y_i
//
// Q(n) is interpolated linearly between orthogonal matrices at control rate; any
// convex combination of orthogonal matrices has spectral norm <= 1, so automation
// can never make the scattering gain energy.

#include <array>

#include "Synthesis/CouplingMatrix.h"
#include "Synthesis/WaveguideResonator.h"

namespace arc::dsp
{

struct NodeSettings
{
    double frequency = 220.0;
    double t60Fundamental = 3.0;
    double t60High = 1.0;
    double hfReference = 5000.0;
    double dispersion = 0.0;
    int dispersionStages = 4;
    double selectivity = 0.0;     // modal selectivity of this node's excitation and pickup
    double loopSelectivity = 0.0; // in-loop selectivity (CORE under sustained exciters)
    float outputGain = 1.0f;
    float pan = 0.0f; // -1 (left) .. +1 (right)
};

struct NetworkSettings
{
    std::array<NodeSettings, kNumNodes> nodes {};
    std::array<float, kNumEdges> edgeTheta {}; // Cayley generator per edge (>= 0)
};

class ResonantNetwork
{
public:
    /** Allocates delay lines (not realtime safe). */
    void prepare (double sampleRate, double minFrequency, int controlInterval);

    /** Zeroes all state (not realtime critical; touches the whole buffers). */
    void reset() noexcept;

    /** Cheap state clear of the portions currently in use (voice re-use). */
    void clearState() noexcept;

    /** Control-rate update. `immediate` skips ramps (use at note start). */
    void configure (const NetworkSettings& settings, bool immediate) noexcept;

    /** Reads all loop outputs for this sample. */
    inline void readOutputs (float* y) noexcept
    {
        for (int i = 0; i < kNumNodes; ++i)
        {
            const float v = loops[static_cast<size_t> (i)].readOutput();
            y[i] = v;
            energyAcc[static_cast<size_t> (i)] += v * v;
        }
        ++energyCount;
    }

    /** Scatters y through Q and writes loop inputs; a mono excitation enters each node
        through its modal selectivity filter (the CORE unfiltered when `rawCore`, for
        feedback exciters whose dynamics must see the broadband loop). */
    inline void writeInputs (const float* y, float excitation, const float* weights, bool rawCore) noexcept
    {
        writeInputs (y, excitation, excitation, weights, rawCore);
    }

    /** As above with separate CORE and outer-node excitation. Feedback exciters (BOW
        friction, the AIR column) compute their force from the CORE's own motion: applied
        at the CORE it is a collocated (passive or intended negative) resistance, but the
        same state-dependent force injected into other nodes is non-collocated feedback,
        which is active (measured: a bow at rest made METAL node B self-oscillate). Such
        exciters pass only their feed-forward part (turbulence) as `nodeExcitation`. */
    inline void writeInputs (const float* y, float coreExcitation, float nodeExcitation, const float* weights,
                             bool rawCore) noexcept
    {
        float inj[kNumNodes];
        for (int i = 0; i < kNumNodes; ++i)
        {
            const float e = (i == kCore ? coreExcitation : nodeExcitation) * weights[i];
            inj[i] = (i == kCore && rawCore) ? e : injectFilter[static_cast<size_t> (i)].process (e);
        }
        writeInputs (y, inj);
    }

    /** Scatters y through Q and writes loop inputs (+ raw per-node injection). */
    inline void writeInputs (const float* y, const float* injection) noexcept
    {
        if (rampRemaining > 0)
        {
            for (int i = 0; i < kNumNodes; ++i)
                for (int j = 0; j < kNumNodes; ++j)
                    q[static_cast<size_t> (i)][static_cast<size_t> (j)] += dq[static_cast<size_t> (i)][static_cast<size_t> (j)];
            for (int i = 0; i < kNumNodes; ++i)
            {
                gainL[static_cast<size_t> (i)] += dGainL[static_cast<size_t> (i)];
                gainR[static_cast<size_t> (i)] += dGainR[static_cast<size_t> (i)];
            }
            --rampRemaining;
        }
        for (int i = 0; i < kNumNodes; ++i)
        {
            const auto& row = q[static_cast<size_t> (i)];
            float u = row[0] * y[0] + row[1] * y[1] + row[2] * y[2] + row[3] * y[3] + row[4] * y[4];
            loops[static_cast<size_t> (i)].writeInput (u + injection[i]);
        }
    }

    /** Stereo pickup of the loop outputs through each node's modal selectivity filter. */
    inline void pickup (const float* y, float& left, float& right) noexcept
    {
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < kNumNodes; ++i)
        {
            const float v = pickupFilter[static_cast<size_t> (i)].process (y[i]);
            l += gainL[static_cast<size_t> (i)] * v;
            r += gainR[static_cast<size_t> (i)] * v;
        }
        left = l;
        right = r;
    }

    /** Convenience block process: mono excitation distributed with per-node weights. */
    void process (const float* excitation, const float* injectWeights, float* outL, float* outR, int n) noexcept;

    // --- telemetry --------------------------------------------------------------------
    /** Mean-square loop output per node since the previous call; resets accumulators. */
    std::array<float, kNumNodes> takeNodeEnergy() noexcept;
    /** Average energy per sample moved across each edge given node energies. */
    std::array<float, kNumEdges> edgeFlux (const std::array<float, kNumNodes>& nodeEnergy) const noexcept;
    /** Approximate stored energy (sum of squares of the active delay-line contents). */
    double storedEnergy() const noexcept;

    /** Coupling side-effects on each loop's fundamental mode, from a first-order
        perturbation of det(I - H(z) Q) around that mode:
            R_i = Q_ii + sum_k Q_ik Q_ki H_k(w_i) / (1 - Q_kk H_k(w_i))
        arg(R_i) / 2pi is the relative frequency shift, |R_i| the extra per-pass gain.
        Voices pre-compensate both so COUPLING moves energy without detuning the
        network or shortening the material's decay (see NETWORK_COUPLING.md). */
    struct ModeCorrection
    {
        double frequencyShift = 0.0; // relative (f_actual = f_loop * (1 + shift))
        double gainFactor = 1.0;     // per-pass gain multiplier caused by coupling
    };
    /** modeFrequency: where each mode should end up (Hz); <= 0 uses the loop tuning. */
    std::array<ModeCorrection, kNumNodes> estimateModeCorrections (const std::array<double, kNumNodes>& modeFrequency) const noexcept;
    std::array<ModeCorrection, kNumNodes> estimateModeCorrections() const noexcept
    {
        return estimateModeCorrections ({});
    }

    /** CORE-only convenience (frequency shift). */
    double estimateCoreDetune (double coreFrequency) const noexcept;

    const Matrix5& currentMatrix() const noexcept { return q; }
    const Matrix5& targetMatrix() const noexcept { return qTarget; }
    const WaveguideResonator& loop (int i) const noexcept { return loops[static_cast<size_t> (i)]; }
    int getControlInterval() const noexcept { return controlInterval; }
    /** Ramp length for the next configure() (QUALITY changes the control rate live). */
    void setControlInterval (int n) noexcept { controlInterval = n > 0 ? n : 1; }
    double getSampleRate() const noexcept { return sampleRate; }

    /** Number of full loop redesigns performed (profiling / tests). */
    long fullDesignCount = 0;
    long retuneCount = 0;

private:
    struct DesignCache
    {
        ResonatorSettings settings;
        LoopCoefficients coeffs;
        bool valid = false;
    };

    void updateLoop (int i, const NodeSettings& ns, bool immediate) noexcept;

    std::array<WaveguideResonator, kNumNodes> loops;
    std::array<DesignCache, kNumNodes> cache;
    std::array<SelectivityStage, kNumNodes> injectFilter, pickupFilter;
    Matrix5 q {}, qTarget {}, dq {};
    std::array<float, kNumNodes> gainL {}, gainR {}, dGainL {}, dGainR {};
    std::array<float, kNumEdges> lastTheta {};
    std::array<float, kNumNodes> energyAcc {};
    int energyCount = 0;
    int rampRemaining = 0;
    int controlInterval = 16;
    bool hasMatrix = false;
    double sampleRate = 48000.0;
};

} // namespace arc::dsp

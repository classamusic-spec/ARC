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

    /** Scatters y through Q and writes loop inputs (+ per-node injection). */
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

    /** Stereo pickup of the loop outputs. */
    inline void pickup (const float* y, float& left, float& right) const noexcept
    {
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < kNumNodes; ++i)
        {
            l += gainL[static_cast<size_t> (i)] * y[i];
            r += gainR[static_cast<size_t> (i)] * y[i];
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

    const Matrix5& currentMatrix() const noexcept { return q; }
    const Matrix5& targetMatrix() const noexcept { return qTarget; }
    const WaveguideResonator& loop (int i) const noexcept { return loops[static_cast<size_t> (i)]; }
    int getControlInterval() const noexcept { return controlInterval; }
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

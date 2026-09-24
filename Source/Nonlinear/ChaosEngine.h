#pragma once

// CHAOS: controlled, bounded irregularity. Every modulation is a smoothed random walk
// with a hard bound and a deterministic seed (preset recall reproduces it). CHAOS never
// touches a feedback gain directly: detune moves loop lengths, coupling variation moves
// rotation angles (the scattering matrix stays orthogonal for any angle), and extra
// routing adds edges — none of which can make the network gain energy.
//
//   amount a = chaos^2 (gentle at low settings)
//   node detune      CORE +-12 a cents, nodes +-40 a cents, rate 0.15 + 2.5 chaos Hz
//   edge strength    x (1 + (0.15 chaos + 0.5 a) walk)       bounded to [0.35, 1.65]
//   extra routing    edges outside the topology get weight 0.5 a (0.5 + 0.5 walk)

#include <array>

#include "Motion/RandomWalk.h"
#include "Synthesis/CouplingMatrix.h"

namespace arc
{

class ChaosEngine
{
public:
    void reset (uint32_t seed) noexcept
    {
        for (size_t i = 0; i < detune.size(); ++i)
            detune[i].seed (seed ^ (0x9E3779B9u * static_cast<uint32_t> (i + 1)));
        for (size_t i = 0; i < edges.size(); ++i)
            edges[i].seed ((seed + 0x7F4A7C15u) ^ (0x85EBCA6Bu * static_cast<uint32_t> (i + 1)));
        cents.fill (0.0f);
        scale.fill (1.0f);
        routing.fill (0.0f);
    }

    void update (float chaos, double dt) noexcept
    {
        const float c = dsp::clamp (chaos, 0.0f, 1.0f);
        const float a = c * c;
        const double rate = 0.15 + 2.5 * static_cast<double> (c);
        for (size_t i = 0; i < detune.size(); ++i)
        {
            const float w = detune[i].next (dt, rate * (1.0 + 0.23 * static_cast<double> (i)));
            cents[i] = (i == 0 ? 12.0f : 40.0f) * a * w;
        }
        for (size_t e = 0; e < edges.size(); ++e)
        {
            const float w = edges[e].next (dt, rate * 0.8 * (1.0 + 0.17 * static_cast<double> (e)));
            scale[e] = dsp::clamp (1.0f + (0.15f * c + 0.5f * a) * w, 0.35f, 1.65f);
            routing[e] = 0.5f * a * (0.5f + 0.5f * w);
        }
    }

    float detuneCents (int node) const noexcept { return cents[static_cast<size_t> (node)]; }
    float edgeScale (int edge) const noexcept { return scale[static_cast<size_t> (edge)]; }
    float extraRouting (int edge) const noexcept { return routing[static_cast<size_t> (edge)]; }

private:
    std::array<RandomWalk, dsp::kNumNodes> detune {};
    std::array<RandomWalk, dsp::kNumEdges> edges {};
    std::array<float, dsp::kNumNodes> cents {};
    std::array<float, dsp::kNumEdges> scale {}, routing {};
};

} // namespace arc

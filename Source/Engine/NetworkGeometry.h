#pragma once

// Maps the Resonance Field (node positions, LINK, topology, COUPLING macro) to the
// network's per-edge coupling generators. Shared by the engine and the UI so the
// drawn connections are computed from the same numbers the DSP uses.

#include <array>
#include <cmath>

#include "Engine/EngineTypes.h"
#include "Materials/MaterialEngine.h"
#include "Synthesis/CouplingMatrix.h"

namespace arc
{

/** Default angles (radians, 0 = up, clockwise): A top-left, B top-right, C bottom-left, D bottom-right. */
inline constexpr std::array<float, 4> kDefaultNodeAngles { -0.785398f, 0.785398f, -2.356194f, 2.356194f };

/** COUPLING macro (0..1) -> maximum per-edge rotation (radians). */
inline float couplingToRotation (float coupling) noexcept
{
    const float c = std::fmin (std::fmax (coupling, 0.0f), 1.0f);
    return 1.45f * std::pow (c, 1.6f);
}

/** Node radius parameter (0..1) -> field radius (0.3..0.9 of the chamber radius). */
inline float fieldRadius (float radius) noexcept { return 0.3f + 0.6f * std::fmin (std::fmax (radius, 0.0f), 1.0f); }

struct FieldPoint
{
    float x, y;
};

inline FieldPoint nodePosition (float radius, float angle) noexcept
{
    const float r = fieldRadius (radius);
    return { r * std::sin (angle), -r * std::cos (angle) };
}

/** Radius -> tuning offset in octaves (+-1 octave around the material's ratio).
    With quantise, snapped to semitones. */
inline float radiusToOctaves (float radius, bool quantise) noexcept
{
    float o = (std::fmin (std::fmax (radius, 0.0f), 1.0f) - 0.5f) * 2.0f;
    if (quantise)
        o = std::round (o * 12.0f) / 12.0f;
    return o;
}

/** Inverse tuning map: the radius that puts node n (0..3) exactly on `ratio` x CORE for a
    material at the given TENSION / INHARMONICITY (unquantised). May fall outside 0..1
    when the ratio is out of the node's +-1 octave reach; callers clamp. */
inline float radiusForRatio (MaterialType material, int node, float ratio, float tension, float inharmonicity) noexcept
{
    const auto& prof = materialProfile (material);
    const auto u = static_cast<size_t> (node);
    const float stretch = MaterialEngine::tensionExponent (tension, inharmonicity);
    const double octaves = std::log2 (static_cast<double> (ratio))
                           - static_cast<double> (stretch) * std::log2 (static_cast<double> (prof.nodeRatio[u]))
                           - static_cast<double> (prof.nodeDetuneCents[u]) / 1200.0;
    return static_cast<float> (0.5 + 0.5 * octaves);
}

/** Angle -> stereo position (-1..1). */
inline float angleToPan (float angle) noexcept { return std::sin (angle); }

/** Per-edge weight from topology, LINK and proximity (closer nodes couple more). */
inline std::array<float, dsp::kNumEdges> edgeWeights (dsp::Topology topology, const std::array<NodeParams, 4>& nodes,
                                                      const std::array<FieldPoint, 4>& pos) noexcept
{
    const auto mask = dsp::topologyMask (topology);
    std::array<float, dsp::kNumEdges> w {};
    for (int e = 0; e < dsp::kNumEdges; ++e)
    {
        const auto& edge = dsp::kEdges[static_cast<size_t> (e)];
        float weight = 0.0f;
        if (edge.a == dsp::kCore)
        {
            weight = nodes[static_cast<size_t> (edge.b - 1)].link;
        }
        else
        {
            const auto& pa = pos[static_cast<size_t> (edge.a - 1)];
            const auto& pb = pos[static_cast<size_t> (edge.b - 1)];
            const float d = std::sqrt ((pa.x - pb.x) * (pa.x - pb.x) + (pa.y - pb.y) * (pa.y - pb.y));
            const float proximity = std::fmin (std::fmax (0.68f / std::fmax (d, 0.05f), 0.2f), 1.5f);
            weight = 0.9f * std::sqrt (nodes[static_cast<size_t> (edge.a - 1)].link * nodes[static_cast<size_t> (edge.b - 1)].link)
                     * proximity;
        }
        w[static_cast<size_t> (e)] = mask[static_cast<size_t> (e)] * weight;
    }
    return w;
}

/** Final generator per edge given COUPLING and a chaos multiplier per edge. */
inline std::array<float, dsp::kNumEdges> edgeGenerators (float coupling, const std::array<float, dsp::kNumEdges>& weights,
                                                         const std::array<float, dsp::kNumEdges>& chaosScale) noexcept
{
    const float phiMax = couplingToRotation (coupling);
    std::array<float, dsp::kNumEdges> theta {};
    for (size_t e = 0; e < theta.size(); ++e)
    {
        const float phi = std::fmin (phiMax * weights[e] * chaosScale[e], 1.55f);
        theta[e] = dsp::generatorForRotation (phi);
    }
    return theta;
}

} // namespace arc

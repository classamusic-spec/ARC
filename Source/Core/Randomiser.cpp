#include "Core/Randomiser.h"

#include <cmath>

#include "Core/Parameters.h"
#include "Engine/NetworkGeometry.h"

namespace arc::randomiser
{

namespace
{
float gauss (juce::Random& rng)
{
    // Box-Muller
    const double u1 = std::max (1.0e-12, rng.nextDouble());
    const double u2 = rng.nextDouble();
    return static_cast<float> (std::sqrt (-2.0 * std::log (u1)) * std::cos (6.283185307179586 * u2));
}

float uniform (juce::Random& rng, float lo, float hi) { return lo + (hi - lo) * rng.nextFloat(); }

float get (const ValueMap& v, const juce::String& id, float fallback)
{
    const auto it = v.find (id);
    return it != v.end() ? it->second : fallback;
}

/** Weighted choice; weights need not be normalised. */
template <size_t N>
int choose (juce::Random& rng, const float (&weights)[N])
{
    float total = 0.0f;
    for (auto w : weights)
        total += w;
    float x = rng.nextFloat() * total;
    for (size_t i = 0; i < N; ++i)
    {
        if (x < weights[i])
            return static_cast<int> (i);
        x -= weights[i];
    }
    return static_cast<int> (N - 1);
}

const juce::String& nid (int n, const char* field)
{
    // Stable storage for the 24 node ids.
    static const auto table = []
    {
        std::array<std::array<juce::String, 6>, 4> t;
        for (int i = 0; i < 4; ++i)
            for (int f = 0; f < 6; ++f)
                t[static_cast<size_t> (i)][static_cast<size_t> (f)] = params::nodeId (i, params::nodeFields[f]);
        return t;
    }();
    for (int f = 0; f < 6; ++f)
        if (std::strcmp (params::nodeFields[f], field) == 0)
            return table[static_cast<size_t> (n)][static_cast<size_t> (f)];
    jassertfalse;
    return table[0][0];
}

constexpr float kDefaultAngles[4] = { 0.375f, 0.625f, 0.125f, 0.875f };
} // namespace

void mutate (ValueMap& v, juce::Random& rng)
{
    // Moves a value by a Gaussian step, inside a musical window that is widened (never
    // forced) when the current value already sits outside it.
    auto nudge = [&] (const juce::String& id, float sd, float lo, float hi, float fallback)
    {
        const float cur = get (v, id, fallback);
        const float a = std::min (lo, cur), b = std::max (hi, cur);
        v[id] = juce::jlimit (a, b, cur + sd * gauss (rng));
    };

    nudge (params::excite, 0.04f, 0.35f, 0.9f, 0.6f);
    nudge (params::coupling, 0.05f, 0.1f, 0.75f, 0.35f);
    nudge (params::tension, 0.035f, 0.3f, 0.72f, 0.5f);
    nudge (params::chaos, 0.04f, 0.0f, 0.6f, 0.1f);

    // Only the active exciter's controls (the others are inaudible).
    switch (juce::roundToInt (get (v, params::exciterType, 0.0f)))
    {
        case 0:
            nudge (params::strikeHardness, 0.06f, 0.1f, 0.9f, 0.55f);
            nudge (params::strikeLength, 0.06f, 0.1f, 0.9f, 0.35f);
            nudge (params::strikeTone, 0.06f, 0.1f, 0.9f, 0.6f);
            break;
        case 1:
            nudge (params::pluckPosition, 0.05f, 0.08f, 0.5f, 0.22f);
            nudge (params::pluckDamp, 0.04f, 0.0f, 0.4f, 0.0f);
            nudge (params::pluckTone, 0.06f, 0.1f, 0.9f, 0.6f);
            break;
        case 2:
            nudge (params::bowPressure, 0.05f, 0.2f, 0.75f, 0.5f);
            nudge (params::bowSpeed, 0.05f, 0.2f, 0.8f, 0.55f);
            nudge (params::bowFriction, 0.05f, 0.25f, 0.75f, 0.5f);
            break;
        default:
            nudge (params::airFlow, 0.05f, 0.35f, 0.8f, 0.6f);
            nudge (params::airTurbulence, 0.06f, 0.1f, 0.85f, 0.45f);
            nudge (params::airTone, 0.06f, 0.15f, 0.85f, 0.5f);
            break;
    }

    nudge (params::mass, 0.04f, 0.25f, 0.8f, 0.5f);
    nudge (params::brightness, 0.04f, 0.25f, 0.75f, 0.5f);
    nudge (params::loss, 0.04f, 0.15f, 0.8f, 0.5f);
    nudge (params::inharmonicity, 0.04f, 0.25f, 0.8f, 0.5f);

    const bool quantised = get (v, params::quantise, 0.0f) > 0.5f;
    for (int n = 0; n < 4; ++n)
    {
        const auto& rid = nid (n, "radius");
        if (quantised)
        {
            // Stay on the semitone grid: occasionally step one semitone.
            if (rng.nextFloat() < 0.3f)
            {
                const float cur = get (v, rid, 0.5f);
                const float step = (rng.nextBool() ? 1.0f : -1.0f) / 24.0f;
                v[rid] = juce::jlimit (std::min (0.2f, cur), std::max (0.8f, cur), cur + step);
            }
        }
        else
            nudge (rid, 0.03f, 0.15f, 0.85f, 0.5f);

        const auto& aid = nid (n, "angle");
        float a = get (v, aid, kDefaultAngles[n]) + 0.02f * gauss (rng);
        v[aid] = a - std::floor (a);
        nudge (nid (n, "decay"), 0.04f, 0.25f, 0.8f, 0.5f);
        nudge (nid (n, "damp"), 0.04f, 0.25f, 0.75f, 0.5f);
        nudge (nid (n, "level"), 0.04f, 0.4f, 0.95f, 0.75f);
        nudge (nid (n, "link"), 0.05f, 0.3f, 1.0f, 0.75f);
    }

    if (get (v, params::motionDepth, 0.0f) > 0.0f)
        nudge (params::motionDepth, 0.04f, 0.0f, 0.6f, 0.0f);
}

void regenerate (ValueMap& v, juce::Random& rng)
{
    const float exciterWeights[4] = { 0.35f, 0.25f, 0.2f, 0.2f };
    const int exciter = choose (rng, exciterWeights);
    const int material = rng.nextInt (4);
    const float topologyWeights[4] = { 0.15f, 0.35f, 0.3f, 0.2f };
    const int topology = choose (rng, topologyWeights);
    const bool sustained = exciter >= 2;

    v[params::exciterType] = static_cast<float> (exciter);
    v[params::materialType] = static_cast<float> (material);
    v[params::topology] = static_cast<float> (topology);
    const bool quantised = rng.nextFloat() < 0.3f;
    v[params::quantise] = quantised ? 1.0f : 0.0f;

    // Macros: moderate, mostly-ordered networks. CHAOS is skewed low (u^2).
    v[params::excite] = uniform (rng, 0.5f, 0.72f);
    {
        const float a = rng.nextFloat(), b = rng.nextFloat();
        v[params::coupling] = 0.15f + 0.55f * 0.5f * (a + b); // triangular around 0.425
    }
    const float tension = juce::jlimit (0.34f, 0.66f, 0.5f + 0.07f * gauss (rng));
    v[params::tension] = tension;
    const float u = rng.nextFloat();
    v[params::chaos] = 0.45f * u * u;

    v[params::strikeHardness] = uniform (rng, 0.25f, 0.8f);
    v[params::strikeLength] = uniform (rng, 0.15f, 0.6f);
    v[params::strikeTone] = uniform (rng, 0.3f, 0.8f);
    v[params::pluckPosition] = uniform (rng, 0.1f, 0.4f);
    v[params::pluckDamp] = uniform (rng, 0.0f, 0.3f);
    v[params::pluckTone] = uniform (rng, 0.3f, 0.8f);
    v[params::bowPressure] = uniform (rng, 0.3f, 0.65f);
    v[params::bowSpeed] = uniform (rng, 0.3f, 0.65f);
    v[params::bowFriction] = uniform (rng, 0.35f, 0.65f);
    v[params::airFlow] = uniform (rng, 0.45f, 0.72f);
    v[params::airTurbulence] = uniform (rng, 0.2f, 0.7f);
    v[params::airTone] = uniform (rng, 0.3f, 0.7f);

    // Material modifiers, with inharmonicity windows per material.
    static constexpr float inharmLo[4] = { 0.35f, 0.35f, 0.3f, 0.3f };
    static constexpr float inharmHi[4] = { 0.7f, 0.75f, 0.6f, 0.65f };
    const float inharm = uniform (rng, inharmLo[material], inharmHi[material]);
    v[params::mass] = uniform (rng, 0.3f, 0.72f);
    v[params::brightness] = uniform (rng, 0.32f, 0.68f);
    v[params::loss] = sustained ? uniform (rng, 0.2f, 0.55f) : uniform (rng, 0.28f, 0.7f);
    v[params::inharmonicity] = inharm;

    // Node tuning: interval grid (quantised), the harmonic series, or the material's own
    // modal ratios with a small spread.
    const float tuningStyle = rng.nextFloat();
    static constexpr int intervals[] = { -12, -7, -5, 0, 0, 5, 7, 12 };
    static constexpr float harmonics[] = { 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f, 10.0f, 12.0f };
    const auto mat = static_cast<MaterialType> (material);
    for (int n = 0; n < 4; ++n)
    {
        float radius = 0.5f;
        if (quantised)
            radius = 0.5f + static_cast<float> (intervals[rng.nextInt (8)]) / 24.0f;
        else if (tuningStyle < 0.45f)
        {
            const float natural = std::pow (materialProfile (mat).nodeRatio[static_cast<size_t> (n)],
                                            MaterialEngine::tensionExponent (tension, inharm))
                                  * std::exp2 (0.25f * gauss (rng));
            float best = harmonics[0];
            for (float h : harmonics)
                if (std::abs (std::log2 (h / natural)) < std::abs (std::log2 (best / natural)))
                    best = h;
            radius = juce::jlimit (0.05f, 0.95f, radiusForRatio (mat, n, best, tension, inharm));
        }
        else
            radius = juce::jlimit (0.2f, 0.8f, 0.5f + 0.1f * gauss (rng));
        v[nid (n, "radius")] = radius;

        const float a = kDefaultAngles[n] + 0.05f * gauss (rng);
        v[nid (n, "angle")] = a - std::floor (a);
        v[nid (n, "decay")] = uniform (rng, 0.35f, 0.72f);
        v[nid (n, "damp")] = uniform (rng, 0.3f, 0.7f);
        v[nid (n, "level")] = uniform (rng, 0.55f, 0.9f);
        v[nid (n, "link")] = uniform (rng, 0.45f, 0.95f);
    }

    v[params::motionDepth] = rng.nextFloat() < 0.5f ? 0.0f : uniform (rng, 0.08f, 0.4f);
    v[params::motionRate] = std::exp (uniform (rng, std::log (0.04f), std::log (0.5f)));
    v[params::releaseDamping] = sustained ? uniform (rng, 0.08f, 0.25f) : uniform (rng, 0.15f, 0.4f);
    v[params::width] = uniform (rng, 0.9f, 1.35f);
    v[params::space] = uniform (rng, 0.08f, 0.38f);
    v[params::drive] = rng.nextFloat() < 0.75f ? 0.0f : uniform (rng, 0.0f, 0.2f);
}

} // namespace arc::randomiser

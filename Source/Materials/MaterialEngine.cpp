#include "Materials/MaterialEngine.h"

#include <algorithm>
#include <cmath>

namespace arc
{

namespace
{
inline float lerpLog (float a, float b, float t) noexcept
{
    return std::exp (std::log (std::max (a, 1.0e-6f)) * (1.0f - t) + std::log (std::max (b, 1.0e-6f)) * t);
}
inline float lerpLin (float a, float b, float t) noexcept { return a + (b - a) * t; }

MaterialProfile interpolate (const MaterialProfile& a, const MaterialProfile& b, float t) noexcept
{
    MaterialProfile m = b;
    for (size_t i = 0; i < 4; ++i)
    {
        m.nodeRatio[i] = lerpLog (a.nodeRatio[i], b.nodeRatio[i], t);
        m.nodeDetuneCents[i] = lerpLin (a.nodeDetuneCents[i], b.nodeDetuneCents[i], t);
        m.inject[i] = lerpLin (a.inject[i], b.inject[i], t);
        m.level[i] = lerpLin (a.level[i], b.level[i], t);
        m.exciterGainDb[i] = lerpLin (a.exciterGainDb[i], b.exciterGainDb[i], t);
    }
    m.coreDispersion = lerpLin (a.coreDispersion, b.coreDispersion, t);
    m.nodeDispersion = lerpLin (a.nodeDispersion, b.nodeDispersion, t);
    m.t60Low = lerpLog (a.t60Low, b.t60Low, t);
    m.t60Mid = lerpLog (a.t60Mid, b.t60Mid, t);
    m.t60High = lerpLog (a.t60High, b.t60High, t);
    m.couplingScale = lerpLog (a.couplingScale, b.couplingScale, t);
    m.coreSelectivity = lerpLin (a.coreSelectivity, b.coreSelectivity, t);
    m.nodeSelectivity = lerpLin (a.nodeSelectivity, b.nodeSelectivity, t);
    m.coreLevel = lerpLin (a.coreLevel, b.coreLevel, t);
    m.energyTuning = lerpLin (a.energyTuning, b.energyTuning, t);
    m.hardnessBias = lerpLin (a.hardnessBias, b.hardnessBias, t);
    m.toneBias = lerpLin (a.toneBias, b.toneBias, t);
    m.outputGain = lerpLog (a.outputGain, b.outputGain, t);
    return m;
}
} // namespace

void MaterialEngine::prepare (double updatesPerSecond, double morphSeconds)
{
    morphTime = morphSeconds;
    step = static_cast<float> (1.0 / std::max (1.0, morphSeconds * updatesPerSecond));
    current = from = materialProfile (target);
    progress = 1.0f;
    update ({}, 0.5f);
}

void MaterialEngine::setMaterial (MaterialType t, bool immediate) noexcept
{
    if (immediate)
    {
        target = t;
        current = from = materialProfile (t);
        progress = 1.0f;
        return;
    }
    if (t == target && progress >= 1.0f)
        return;
    // Restart from wherever the network is now (also mid-morph).
    from = current;
    target = t;
    progress = 0.0f;
}

void MaterialEngine::update (const MaterialModifiers& mods, float tension) noexcept
{
    if (progress < 1.0f)
    {
        progress = std::min (1.0f, progress + step);
        const float sm = progress * progress * (3.0f - 2.0f * progress);
        current = interpolate (from, materialProfile (target), sm);
    }
    settled = progress;

    // --- modifiers + TENSION ----------------------------------------------------------
    const float T = std::clamp (tension, 0.0f, 1.0f);
    const float mass = std::clamp (mods.mass, 0.0f, 1.0f) - 0.5f;
    const float bright = std::clamp (mods.brightness, 0.0f, 1.0f) - 0.5f;
    const float loss = std::clamp (mods.loss, 0.0f, 1.0f) - 0.5f;
    const float inharm = std::clamp (mods.inharmonicity, 0.0f, 1.0f);

    const float stretch = tensionExponent (T, inharm);
    for (size_t i = 0; i < 4; ++i)
    {
        eff.nodeRatio[i] = std::pow (current.nodeRatio[i], stretch);
        eff.nodeDetuneCents[i] = current.nodeDetuneCents[i];
        eff.inject[i] = current.inject[i];
        eff.level[i] = current.level[i];
    }

    const float dispScale = (0.4f + 1.2f * T) * std::exp2 ((inharm - 0.5f) * 3.0f);
    eff.coreDispersion = std::min (0.6f, current.coreDispersion * dispScale);
    eff.nodeDispersion = std::min (0.6f, current.nodeDispersion * dispScale);

    const float decayMult = std::exp2 (-loss * 4.0f) * std::exp2 (mass * 1.2f);
    const float hfMult = std::exp2 (bright * 3.0f) * std::exp2 ((T - 0.5f) * 1.5f);
    eff.t60Low = current.t60Low * decayMult;
    eff.t60Mid = current.t60Mid * decayMult * std::sqrt (hfMult);
    eff.t60High = current.t60High * decayMult * hfMult;

    eff.couplingScale = current.couplingScale * std::exp2 (-mass);
    // Brighter = each loop keeps more of its own overtone series.
    const float selScale = std::exp2 (-bright * 2.0f);
    eff.coreSelectivity = std::clamp (current.coreSelectivity * selScale, 0.0f, 1.0f);
    eff.nodeSelectivity = std::clamp (current.nodeSelectivity * selScale, 0.0f, 1.0f);
    eff.coreLevel = current.coreLevel;
    eff.energyTuning = current.energyTuning * std::exp2 (-mass * 2.0f);
    eff.hardnessBias = current.hardnessBias - 0.3f * mass;
    eff.toneBias = current.toneBias + 0.4f * bright;
    eff.outputGain = current.outputGain;
    eff.exciterGainDb = current.exciterGainDb;
}

} // namespace arc

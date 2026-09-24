#include "Materials/MaterialProfile.h"

#include <algorithm>
#include <cmath>

namespace arc
{

namespace
{
// GLASS — thin shell (wine glass / bowl): sparse, high, slightly inharmonic modes with
// spacing that widens upward (1 : 2.63 : 4.45 : 6.55 : 9.4). Each ratio sits ~0.4 away
// from the CORE's harmonics: a node within ~1.5 % of a CORE harmonic forms a coupled
// doublet that beats at ~phi * f / pi (measured 31 Hz at C4 with 4.93 vs 5) — rough,
// not crystalline. High frequencies ring almost as long as low ones.
MaterialProfile makeGlass()
{
    MaterialProfile m;
    m.nodeRatio = { 2.63f, 4.45f, 6.55f, 9.40f };
    m.nodeDetuneCents = { 3.0f, -4.0f, 5.0f, -2.0f };
    m.coreDispersion = 0.04f;
    m.nodeDispersion = 0.10f;
    m.t60Low = 5.5f;
    m.t60Mid = 4.5f;
    m.t60High = 2.8f;
    m.couplingScale = 0.85f;
    m.coreSelectivity = 0.75f;
    m.nodeSelectivity = 0.85f;
    m.inject = { 0.55f, 0.45f, 0.35f, 0.25f };
    m.level = { 0.75f, 0.6f, 0.5f, 0.4f };
    m.coreLevel = 0.9f;
    m.energyTuning = 0.001f;
    m.hardnessBias = 0.15f;
    m.toneBias = 0.1f;
    m.outputGain = 1.0f;
    m.exciterGainDb = { 5.8f, -3.0f, 1.2f, -7.9f };
    return m;
}

// METAL — bell. Upper partials of a tuned bell relative to the prime: tierce 1.19
// (minor third), quint 1.5, nominal 2.0, undeciem 2.67. Every loop is strongly
// dispersive, so each node's own overtones are stretched into dense inharmonic
// clusters that beat against each other near 2f, 3f, 5f — but never at the
// fundamental. (A hum node at 0.5 was measured to split the fundamental into a
// -50/+51 cent pair: its loop's own 2nd overtone sits on f0. See DEVELOPMENT_LOG.)
MaterialProfile makeMetal()
{
    MaterialProfile m;
    m.nodeRatio = { 1.19f, 1.5f, 2.0f, 2.67f };
    m.nodeDetuneCents = { -6.0f, 4.0f, -3.0f, 7.0f };
    m.coreDispersion = 0.22f;
    m.nodeDispersion = 0.32f;
    m.t60Low = 9.0f;
    m.t60Mid = 6.0f;
    m.t60High = 1.6f;
    m.couplingScale = 1.2f;
    m.coreSelectivity = 0.45f;
    m.nodeSelectivity = 0.55f;
    m.inject = { 0.6f, 0.55f, 0.5f, 0.45f };
    m.level = { 0.7f, 0.8f, 0.7f, 0.65f };
    m.coreLevel = 0.9f;
    m.energyTuning = 0.002f;
    m.hardnessBias = 0.1f;
    m.toneBias = 0.0f;
    m.outputGain = 1.0f;
    m.exciterGainDb = { 4.5f, -3.6f, -0.7f, -11.1f };
    return m;
}

// WOOD — free wooden bar / body: the classic free-bar modes 1 : 2.756 : 5.404 : 8.933
// : 13.34 as short-lived body resonances over a nearly harmonic CORE (so a plucked CORE
// is a string on a wooden body). Fast high-frequency damping (steep T60 slope) and a
// short overall decay: warm, organic, low-mid weighted. (Tuned-bar ratios 2.0 / 3.93
// were measured to split against the CORE's 2nd / 4th harmonics.)
MaterialProfile makeWood()
{
    MaterialProfile m;
    m.nodeRatio = { 2.756f, 5.404f, 8.933f, 13.34f };
    m.nodeDetuneCents = { 2.0f, -3.0f, 4.0f, -4.0f };
    m.coreDispersion = 0.015f;
    m.nodeDispersion = 0.04f;
    m.t60Low = 1.5f;
    m.t60Mid = 0.45f;
    m.t60High = 0.07f;
    m.couplingScale = 0.75f;
    m.coreSelectivity = 0.6f;
    m.nodeSelectivity = 0.8f;
    m.inject = { 0.45f, 0.35f, 0.25f, 0.15f };
    m.level = { 0.7f, 0.55f, 0.4f, 0.3f };
    m.coreLevel = 1.0f;
    m.energyTuning = 0.004f;
    m.hardnessBias = -0.1f;
    m.toneBias = -0.1f;
    m.outputGain = 1.0f;
    m.exciterGainDb = { 10.8f, 2.7f, 0.3f, -7.2f };
    return m;
}

// MEMBRANE — circular membrane. Bessel-zero ratios (0,1)=1, (1,1)=1.594, (2,1)=2.136,
// (0,2)=2.296, (3,1)=2.653. Loops are dispersive (a membrane has no harmonic series)
// and HF decays fast so each loop behaves like one mode; strong energy-dependent
// tuning gives the characteristic pitch drop after a hard hit.
MaterialProfile makeMembrane()
{
    MaterialProfile m;
    m.nodeRatio = { 1.594f, 2.136f, 2.296f, 2.653f };
    m.nodeDetuneCents = { 0.0f, 0.0f, 0.0f, 0.0f };
    m.coreDispersion = 0.28f;
    m.nodeDispersion = 0.28f;
    m.t60Low = 1.6f;
    m.t60Mid = 0.35f;
    m.t60High = 0.08f;
    m.couplingScale = 0.9f;
    m.coreSelectivity = 0.8f;
    m.nodeSelectivity = 0.85f;
    m.inject = { 0.6f, 0.5f, 0.45f, 0.4f };
    m.level = { 0.8f, 0.7f, 0.6f, 0.5f };
    m.coreLevel = 1.0f;
    m.energyTuning = 0.04f;
    m.hardnessBias = 0.0f;
    m.toneBias = 0.0f;
    m.outputGain = 1.0f;
    m.exciterGainDb = { 11.2f, 3.4f, 0.7f, -7.1f };
    return m;
}

const MaterialProfile kProfiles[] = { makeGlass(), makeMetal(), makeWood(), makeMembrane() };
} // namespace

const MaterialProfile& materialProfile (MaterialType t) noexcept
{
    const int i = std::clamp (static_cast<int> (t), 0, static_cast<int> (MaterialType::count) - 1);
    return kProfiles[i];
}

float EffectiveMaterial::t60At (float f) const noexcept
{
    const float lf = std::log2 (std::max (f, 1.0f));
    const float l100 = std::log2 (100.0f), l1k = std::log2 (1000.0f), l8k = std::log2 (8000.0f);
    const float a = std::log2 (t60Low), b = std::log2 (t60Mid), c = std::log2 (t60High);
    float v;
    if (lf <= l1k)
        v = a + (b - a) * (lf - l100) / (l1k - l100);
    else
        v = b + (c - b) * (lf - l1k) / (l8k - l1k);
    // Extrapolate below 100 Hz with half the slope (bodies saturate at low frequency).
    if (lf < l100)
        v = a + 0.5f * (b - a) * (lf - l100) / (l1k - l100);
    return std::clamp (std::exp2 (v), 0.01f, 60.0f);
}

} // namespace arc

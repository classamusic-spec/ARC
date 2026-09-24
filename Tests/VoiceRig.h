#pragma once

// Drives one ArcVoice exactly like the engine does (MaterialEngine + VoiceControl per
// control block) so exciter / material / tension behaviour can be measured in isolation.

#include "Analysis.h"

#include <functional>

#include "Engine/ArcVoice.h"
#include "Engine/NetworkGeometry.h"
#include "Materials/MaterialEngine.h"

namespace arctest
{

struct RigSettings
{
    arc::MaterialType material = arc::MaterialType::metal;
    arc::MaterialModifiers mods;
    float tension = 0.5f;
    float coupling = 0.35f;
    arc::dsp::Topology topology = arc::dsp::Topology::ring;
    arc::ExciterType exciter = arc::ExciterType::strike;
    arc::ExciterParams ex;
    float excite = 0.6f;
    std::array<arc::NodeParams, 4> nodes {};
    float releaseDamping = 0.25f;
    bool compensation = true;
    bool quantise = false;
    int stages = 4;
    float pressure = 0.0f;
    float freeze = 0.0f;
};

struct RigResult
{
    Signal left, right, mono;
    std::vector<std::array<float, 5>> nodeEnergy;
    double coreFrequency = 0.0;
};

inline RigSettings defaultRig()
{
    RigSettings r;
    for (int i = 0; i < 4; ++i)
        r.nodes[static_cast<size_t> (i)].angle = arc::kDefaultNodeAngles[static_cast<size_t> (i)];
    return r;
}

inline arc::VoiceControl makeControl (const RigSettings& s, arc::MaterialEngine& me)
{
    arc::VoiceControl c;
    me.update (s.mods, s.tension);
    c.material = me.effective();
    std::array<arc::FieldPoint, 4> pos {};
    for (int i = 0; i < 4; ++i)
    {
        const auto& n = s.nodes[static_cast<size_t> (i)];
        c.nodeOffsetOctaves[static_cast<size_t> (i)] = arc::radiusToOctaves (n.radius, s.quantise);
        c.nodePan[static_cast<size_t> (i)] = arc::angleToPan (n.angle);
        c.nodeDecayMult[static_cast<size_t> (i)] = std::exp2 ((n.decay - 0.5f) * 4.0f);
        c.nodeDampMult[static_cast<size_t> (i)] = std::exp2 (-(n.damp - 0.5f) * 4.0f);
        c.nodeLevel[static_cast<size_t> (i)] = n.level;
        pos[static_cast<size_t> (i)] = arc::nodePosition (n.radius, n.angle);
    }
    std::array<float, arc::dsp::kNumEdges> ones {};
    ones.fill (1.0f);
    c.edgeTheta = arc::edgeGenerators (s.coupling, arc::edgeWeights (s.topology, s.nodes, pos), ones);
    c.exciterType = s.exciter;
    c.exciter = s.ex;
    c.excite = s.excite;
    c.releaseDamping = s.releaseDamping;
    c.couplingCompensation = s.compensation;
    c.dispersionStages = s.stages;
    c.freeze = s.freeze;
    return c;
}

/** Optional automation: called at every control block with the time in seconds; may
    modify the settings (material changes go through the MaterialEngine morph). */
using RigAutomation = std::function<void (double, RigSettings&)>;

inline RigResult renderVoice (const RigSettings& s0, int note, float velocity, double noteSeconds, double totalSeconds,
                              double sampleRate = 48000.0, uint32_t seed = 1, const RigAutomation& automation = {})
{
    constexpr int control = 16;
    RigSettings s = s0;
    arc::MaterialEngine me;
    me.setMaterial (s.material, true);
    me.prepare (sampleRate / control);
    arc::ArcVoice voice;
    voice.prepare (sampleRate, control);
    auto ctl = makeControl (s, me);
    voice.start (note, 1, velocity, seed, ctl);
    if (s.pressure > 0.0f)
        voice.setPressure (s.pressure);

    RigResult r;
    const int n = static_cast<int> (totalSeconds * sampleRate);
    const int off = static_cast<int> (noteSeconds * sampleRate);
    r.left.assign (static_cast<size_t> (n), 0.0f);
    r.right.assign (static_cast<size_t> (n), 0.0f);
    for (int i = 0; i < n; i += control)
    {
        if (i >= off && i - control < off)
            voice.release();
        if (automation)
        {
            automation (i / sampleRate, s);
            if (s.material != me.getMaterial())
                me.setMaterial (s.material, false);
        }
        ctl = makeControl (s, me);
        const int len = std::min (control, n - i);
        voice.render (r.left.data() + i, r.right.data() + i, len, ctl);
        r.nodeEnergy.push_back (voice.getNodeEnergy());
    }
    r.coreFrequency = voice.getCoreFrequency();
    r.mono.resize (static_cast<size_t> (n));
    for (size_t i = 0; i < r.mono.size(); ++i)
        r.mono[i] = 0.5f * (r.left[i] + r.right[i]);
    return r;
}

inline const char* materialName (arc::MaterialType m)
{
    switch (m)
    {
        case arc::MaterialType::glass:    return "glass";
        case arc::MaterialType::metal:    return "metal";
        case arc::MaterialType::wood:     return "wood";
        case arc::MaterialType::membrane: return "membrane";
        case arc::MaterialType::count:    break;
    }
    return "?";
}

inline const char* exciterName (arc::ExciterType e)
{
    switch (e)
    {
        case arc::ExciterType::strike: return "strike";
        case arc::ExciterType::pluck:  return "pluck";
        case arc::ExciterType::bow:    return "bow";
        case arc::ExciterType::air:    return "air";
        case arc::ExciterType::count:  break;
    }
    return "?";
}

} // namespace arctest

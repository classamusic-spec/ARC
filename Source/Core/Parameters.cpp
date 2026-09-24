#include "Core/Parameters.h"

namespace arc::params
{

namespace
{
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
using API = juce::AudioParameterInt;
using Range = juce::NormalisableRange<float>;

juce::ParameterID pid (const juce::String& id) { return { id, 1 }; }

std::unique_ptr<APF> unit (const juce::String& id, const juce::String& name, float def, const juce::String& label = {})
{
    return std::make_unique<APF> (pid (id), name, Range (0.0f, 1.0f, 0.0001f), def,
                                  juce::AudioParameterFloatAttributes()
                                      .withLabel (label)
                                      .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }));
}
} // namespace

juce::String nodeId (int node, const char* field)
{
    return juce::String ("arc.node") + juce::String::charToString (static_cast<juce::juce_wchar> ('A' + node)) + "." + field + ".v1";
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (unit (excite, "Excite", 0.6f));
    layout.add (unit (coupling, "Coupling", 0.35f));
    layout.add (unit (tension, "Tension", 0.5f));
    layout.add (unit (chaos, "Chaos", 0.1f));
    layout.add (std::make_unique<APC> (pid (topology), "Topology", juce::StringArray { "Star", "Ring", "Web", "Chain" }, 1));
    layout.add (std::make_unique<APB> (pid (quantise), "Quantize Nodes", false));

    layout.add (std::make_unique<APC> (pid (exciterType), "Exciter", juce::StringArray { "Strike", "Pluck", "Bow", "Air" }, 0));
    const ExciterParams ex;
    layout.add (unit (strikeHardness, "Strike Hardness", ex.strikeHardness));
    layout.add (unit (strikeLength, "Strike Length", ex.strikeLength));
    layout.add (unit (strikeTone, "Strike Tone", ex.strikeTone));
    layout.add (unit (pluckPosition, "Pluck Position", ex.pluckPosition));
    layout.add (unit (pluckDamp, "Pluck Damp", ex.pluckDamp));
    layout.add (unit (pluckTone, "Pluck Tone", ex.pluckTone));
    layout.add (unit (bowPressure, "Bow Pressure", ex.bowPressure));
    layout.add (unit (bowSpeed, "Bow Speed", ex.bowSpeed));
    layout.add (unit (bowFriction, "Bow Friction", ex.bowFriction));
    layout.add (unit (airFlow, "Air Flow", ex.airFlow));
    layout.add (unit (airTurbulence, "Air Turbulence", ex.airTurbulence));
    layout.add (unit (airTone, "Air Tone", ex.airTone));

    layout.add (std::make_unique<APC> (pid (materialType), "Material", juce::StringArray { "Glass", "Metal", "Wood", "Membrane" }, 1));
    layout.add (unit (mass, "Mass", 0.5f));
    layout.add (unit (brightness, "Brightness", 0.5f));
    layout.add (unit (loss, "Loss", 0.5f));
    layout.add (unit (inharmonicity, "Inharmonicity", 0.5f));

    const float defaultAngles[4] = { 0.375f, 0.625f, 0.125f, 0.875f };
    for (int n = 0; n < 4; ++n)
    {
        const juce::String nn = juce::String ("Node ") + juce::String::charToString (static_cast<juce::juce_wchar> ('A' + n)) + " ";
        layout.add (unit (nodeId (n, "radius"), nn + "Radius", 0.5f));
        layout.add (std::make_unique<APF> (pid (nodeId (n, "angle")), nn + "Angle", Range (0.0f, 1.0f, 0.0001f), defaultAngles[n],
                                           juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                                               [] (float v, int) { return juce::String (juce::roundToInt ((v - 0.5f) * 360.0f)) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")); })));
        layout.add (unit (nodeId (n, "decay"), nn + "Decay", 0.5f));
        layout.add (unit (nodeId (n, "damp"), nn + "Damp", 0.5f));
        layout.add (unit (nodeId (n, "level"), nn + "Level", 0.75f));
        layout.add (unit (nodeId (n, "link"), nn + "Link", 0.75f));
    }

    layout.add (std::make_unique<APB> (pid (freeze), "Freeze", false));
    layout.add (std::make_unique<APB> (pid (sync), "Sync", false));
    layout.add (unit (motionDepth, "Motion", 0.0f));
    layout.add (std::make_unique<APF> (pid (motionRate), "Motion Rate", Range (0.02f, 4.0f, 0.001f, 0.35f), 0.25f,
                                       juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<APC> (pid (motionDivision), "Motion Division",
                                       juce::StringArray { "1/4", "1/2", "1 Bar", "2 Bars", "4 Bars", "8 Bars" }, 2));
    layout.add (std::make_unique<APB> (pid (gesturePlay), "Gesture Play", true));

    layout.add (std::make_unique<API> (pid (polyphony), "Polyphony", 1, 16, 8));
    layout.add (std::make_unique<APC> (pid (voiceMode), "Voice Mode", juce::StringArray { "Poly", "Mono", "Legato" }, 0));
    layout.add (std::make_unique<APF> (pid (bendRange), "Bend Range", Range (0.0f, 24.0f, 1.0f), 2.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("st")));
    layout.add (unit (releaseDamping, "Release Damping", 0.25f));
    layout.add (std::make_unique<APF> (pid (glide), "Glide", Range (0.0f, 1.0f, 0.001f, 0.5f), 0.06f,
                                       juce::AudioParameterFloatAttributes().withLabel ("s")));
    layout.add (std::make_unique<APB> (pid (mpe), "MPE", false));
    layout.add (std::make_unique<APC> (pid (quality), "Quality", juce::StringArray { "Eco", "Normal", "High" }, 1));

    layout.add (std::make_unique<APF> (pid (masterOutput), "Master Output", Range (-60.0f, 6.0f, 0.01f, 2.5f), -3.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<APF> (pid (width), "Width", Range (0.0f, 1.5f, 0.001f), 1.0f));
    layout.add (unit (space, "Space", 0.12f));
    layout.add (unit (drive, "Drive", 0.0f));
    return layout;
}

std::atomic<float>* ParameterCache::get (juce::AudioProcessorValueTreeState& s, const juce::String& id)
{
    auto* p = s.getRawParameterValue (id);
    jassert (p != nullptr);
    return p;
}

ParameterCache::ParameterCache (juce::AudioProcessorValueTreeState& s)
{
    pExcite = get (s, excite);
    pCoupling = get (s, coupling);
    pTension = get (s, tension);
    pChaos = get (s, chaos);
    pTopology = get (s, topology);
    pQuantise = get (s, quantise);
    pExciter = get (s, exciterType);
    pStrikeHardness = get (s, strikeHardness);
    pStrikeLength = get (s, strikeLength);
    pStrikeTone = get (s, strikeTone);
    pPluckPosition = get (s, pluckPosition);
    pPluckDamp = get (s, pluckDamp);
    pPluckTone = get (s, pluckTone);
    pBowPressure = get (s, bowPressure);
    pBowSpeed = get (s, bowSpeed);
    pBowFriction = get (s, bowFriction);
    pAirFlow = get (s, airFlow);
    pAirTurbulence = get (s, airTurbulence);
    pAirTone = get (s, airTone);
    pMaterial = get (s, materialType);
    pMass = get (s, mass);
    pBrightness = get (s, brightness);
    pLoss = get (s, loss);
    pInharmonicity = get (s, inharmonicity);
    pFreeze = get (s, freeze);
    pSync = get (s, sync);
    pMotionDepth = get (s, motionDepth);
    pMotionRate = get (s, motionRate);
    pMotionDivision = get (s, motionDivision);
    pGesturePlay = get (s, gesturePlay);
    pPolyphony = get (s, polyphony);
    pVoiceMode = get (s, voiceMode);
    pBendRange = get (s, bendRange);
    pRelease = get (s, releaseDamping);
    pGlide = get (s, glide);
    pMpe = get (s, mpe);
    pQuality = get (s, quality);
    pMaster = get (s, masterOutput);
    pWidth = get (s, width);
    pSpace = get (s, space);
    pDrive = get (s, drive);
    for (int n = 0; n < 4; ++n)
        for (int f = 0; f < 6; ++f)
            pNode[static_cast<size_t> (n)][static_cast<size_t> (f)] = get (s, nodeId (n, nodeFields[f]));
}

void ParameterCache::fill (EngineParams& p) const noexcept
{
    auto v = [] (const std::atomic<float>* a) { return a->load (std::memory_order_relaxed); };
    auto i = [&] (const std::atomic<float>* a) { return static_cast<int> (std::lround (v (a))); };
    p.excite = v (pExcite);
    p.coupling = v (pCoupling);
    p.tension = v (pTension);
    p.chaos = v (pChaos);
    p.topology = static_cast<dsp::Topology> (std::clamp (i (pTopology), 0, 3));
    p.quantise = v (pQuantise) > 0.5f;
    p.exciter = static_cast<ExciterType> (std::clamp (i (pExciter), 0, 3));
    auto& e = p.exciterParams;
    e.strikeHardness = v (pStrikeHardness);
    e.strikeLength = v (pStrikeLength);
    e.strikeTone = v (pStrikeTone);
    e.pluckPosition = v (pPluckPosition);
    e.pluckDamp = v (pPluckDamp);
    e.pluckTone = v (pPluckTone);
    e.bowPressure = v (pBowPressure);
    e.bowSpeed = v (pBowSpeed);
    e.bowFriction = v (pBowFriction);
    e.airFlow = v (pAirFlow);
    e.airTurbulence = v (pAirTurbulence);
    e.airTone = v (pAirTone);
    p.material = static_cast<MaterialType> (std::clamp (i (pMaterial), 0, 3));
    p.materialMods.mass = v (pMass);
    p.materialMods.brightness = v (pBrightness);
    p.materialMods.loss = v (pLoss);
    p.materialMods.inharmonicity = v (pInharmonicity);
    for (size_t n = 0; n < 4; ++n)
    {
        auto& nd = p.nodes[n];
        nd.radius = v (pNode[n][0]);
        nd.angle = v (pNode[n][1]);
        nd.decay = v (pNode[n][2]);
        nd.damp = v (pNode[n][3]);
        nd.level = v (pNode[n][4]);
        nd.link = v (pNode[n][5]);
    }
    p.freeze = v (pFreeze) > 0.5f;
    p.sync = v (pSync) > 0.5f;
    p.motionDepth = v (pMotionDepth);
    p.motionRateHz = v (pMotionRate);
    p.motionDivision = static_cast<SyncDivision> (std::clamp (i (pMotionDivision), 0, 5));
    p.gesturePlay = v (pGesturePlay) > 0.5f;
    p.polyphony = std::clamp (i (pPolyphony), 1, 16);
    p.voiceMode = static_cast<VoiceMode> (std::clamp (i (pVoiceMode), 0, 2));
    p.bendRange = v (pBendRange);
    p.releaseDamping = v (pRelease);
    p.glideSeconds = v (pGlide);
    p.mpe = v (pMpe) > 0.5f;
    p.quality = static_cast<Quality> (std::clamp (i (pQuality), 0, 2));
    p.masterGainDb = v (pMaster);
    p.width = v (pWidth);
    p.space = v (pSpace);
    p.drive = v (pDrive);
}

} // namespace arc::params

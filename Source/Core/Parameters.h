#pragma once

// Host-automatable parameters. IDs are permanent (never display labels) and carry a
// version suffix; see docs/PARAMETERS.md.

#include <juce_audio_processors/juce_audio_processors.h>

#include "Engine/EngineParams.h"

namespace arc::params
{

// Performance macros
inline constexpr const char* excite = "arc.network.excite.v1";
inline constexpr const char* coupling = "arc.network.coupling.v1";
inline constexpr const char* tension = "arc.network.tension.v1";
inline constexpr const char* chaos = "arc.network.chaos.v1";
inline constexpr const char* topology = "arc.network.topology.v1";
inline constexpr const char* quantise = "arc.network.quantize.v1";

// Exciter
inline constexpr const char* exciterType = "arc.exciter.type.v1";
inline constexpr const char* strikeHardness = "arc.exciter.strike.hardness.v1";
inline constexpr const char* strikeLength = "arc.exciter.strike.length.v1";
inline constexpr const char* strikeTone = "arc.exciter.strike.tone.v1";
inline constexpr const char* pluckPosition = "arc.exciter.pluck.position.v1";
inline constexpr const char* pluckDamp = "arc.exciter.pluck.damp.v1";
inline constexpr const char* pluckTone = "arc.exciter.pluck.tone.v1";
inline constexpr const char* bowPressure = "arc.exciter.bow.pressure.v1";
inline constexpr const char* bowSpeed = "arc.exciter.bow.speed.v1";
inline constexpr const char* bowFriction = "arc.exciter.bow.friction.v1";
inline constexpr const char* airFlow = "arc.exciter.air.flow.v1";
inline constexpr const char* airTurbulence = "arc.exciter.air.turbulence.v1";
inline constexpr const char* airTone = "arc.exciter.air.tone.v1";

// Material
inline constexpr const char* materialType = "arc.material.type.v1";
inline constexpr const char* mass = "arc.material.mass.v1";
inline constexpr const char* brightness = "arc.material.brightness.v1";
inline constexpr const char* loss = "arc.material.loss.v1";
inline constexpr const char* inharmonicity = "arc.material.inharmonicity.v1";

// Utility / motion
inline constexpr const char* freeze = "arc.freeze.v1";
inline constexpr const char* sync = "arc.sync.v1";
inline constexpr const char* motionDepth = "arc.motion.depth.v1";
inline constexpr const char* motionRate = "arc.motion.rate.v1";
inline constexpr const char* motionDivision = "arc.motion.division.v1";
inline constexpr const char* gesturePlay = "arc.motion.play.v1";

// Voice
inline constexpr const char* polyphony = "arc.voice.polyphony.v1";
inline constexpr const char* voiceMode = "arc.voice.mode.v1";
inline constexpr const char* bendRange = "arc.voice.bendrange.v1";
inline constexpr const char* releaseDamping = "arc.voice.release.v1";
inline constexpr const char* glide = "arc.voice.glide.v1";
inline constexpr const char* mpe = "arc.voice.mpe.v1";
inline constexpr const char* quality = "arc.quality.v1";

// Output
inline constexpr const char* masterOutput = "arc.master.output.v1";
inline constexpr const char* width = "arc.fx.width.v1";
inline constexpr const char* space = "arc.fx.space.v1";
inline constexpr const char* drive = "arc.fx.drive.v1";

/** Per-node parameter id, e.g. nodeId (0, "radius") -> "arc.nodeA.radius.v1". */
juce::String nodeId (int node, const char* field);

inline constexpr const char* nodeFields[] = { "radius", "angle", "decay", "damp", "level", "link" };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/** Cached raw parameter pointers for allocation-free reads on the audio thread. */
class ParameterCache
{
public:
    explicit ParameterCache (juce::AudioProcessorValueTreeState& apvts);
    /** Reads every parameter into the engine snapshot (audio thread safe). */
    void fill (EngineParams& p) const noexcept;

private:
    std::atomic<float>* get (juce::AudioProcessorValueTreeState& s, const juce::String& id);

    std::atomic<float> *pExcite, *pCoupling, *pTension, *pChaos, *pTopology, *pQuantise;
    std::atomic<float> *pExciter, *pStrikeHardness, *pStrikeLength, *pStrikeTone, *pPluckPosition, *pPluckDamp,
        *pPluckTone, *pBowPressure, *pBowSpeed, *pBowFriction, *pAirFlow, *pAirTurbulence, *pAirTone;
    std::atomic<float> *pMaterial, *pMass, *pBrightness, *pLoss, *pInharmonicity;
    std::atomic<float> *pFreeze, *pSync, *pMotionDepth, *pMotionRate, *pMotionDivision, *pGesturePlay;
    std::atomic<float> *pPolyphony, *pVoiceMode, *pBendRange, *pRelease, *pGlide, *pMpe, *pQuality;
    std::atomic<float> *pMaster, *pWidth, *pSpace, *pDrive;
    std::array<std::array<std::atomic<float>*, 6>, 4> pNode {};
};

} // namespace arc::params

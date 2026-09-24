#pragma once

// The Resonance Field: ARC's central, interactive view of the network.
//
//  * geometry is the DSP's own: node radius / angle parameters (plus MOTION offsets
//    from telemetry) place the nodes; the connections drawn are the network's edges
//    for the active topology, with the coupling actually in use;
//  * light is energy: node halos follow each resonator's measured energy, curves carry
//    flow pulses whose speed and brightness follow the energy moving across each edge,
//    strikes send a pulse outward from the CORE, FREEZE frosts and stills the flows;
//  * TENSION changes the radial spacing of the field rings, CHAOS adds a small bounded
//    tremor, MOTION draws the recorded paths;
//  * the chamber header doubles as the instrument's caption: it shows help for
//    whatever is under the mouse.
//
// Interaction: drag a node (retune + move), Alt-drag or arm REC to record a gesture,
// double-click to reset, click a node / the CORE for its inspector, click empty space
// to close inspectors.

#include <array>
#include <functional>
#include <vector>

#include "Motion/Gesture.h"
#include "UI/Controls.h"
#include "UI/FieldModel.h"

class ArcAudioProcessor;

namespace arc::ui
{

class ResonanceField : public juce::Component, public juce::TooltipClient
{
public:
    enum class Target
    {
        none,
        core,
        node
    };
    struct Selection
    {
        Target target = Target::none;
        int node = -1;
        bool operator== (const Selection& o) const { return target == o.target && node == o.node; }
    };

    ResonanceField (ArcAudioProcessor& processor, FieldModel& model);
    ~ResonanceField() override;

    void setCaption (const juce::String& title, const juce::String& body);
    void setSelection (Selection s, bool notify);
    Selection getSelection() const noexcept { return selection; }
    std::function<void (Selection)> onSelectionChanged;

    void setRecordArmed (int node, bool armed);
    bool isRecordArmed (int node) const noexcept { return armedNode == node; }
    bool isRecording() const noexcept { return recordingNode >= 0; }
    std::function<void()> onGestureStateChanged;

    juce::Point<float> nodeCentre (int node) const;
    juce::Point<float> coreCentre() const { return centre; }
    float nodeRadiusPx() const noexcept { return nodeR; }
    float coreRadiusPx() const noexcept { return coreR; }
    const juce::Path& chamberPath() const noexcept { return chamber; }

    /** Renders what lies behind the field (the editor's chrome), in field coordinates.
        With it the field is opaque: its static layer includes the backdrop, so frames
        never repaint the canvas beneath and the cached blit needs no blending. */
    void setBackdrop (std::function<void (juce::Graphics&)> renderer);

    /** Advance animations by dt seconds (called by the editor's frame clock). */
    void advance (float dt, bool engineRunning);

    void paint (juce::Graphics&) override;
    void resized() override;
    bool hitTest (int x, int y) override;

    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    juce::String getTooltip() override;

private:
    struct NodeParams
    {
        juce::RangedAudioParameter* radius = nullptr;
        juce::RangedAudioParameter* angle = nullptr;
        juce::RangedAudioParameter* level = nullptr;
        juce::RangedAudioParameter* link = nullptr;
    };

    // Mapping between parameters and field coordinates.
    juce::Point<float> fieldToScreen (float radiusParam, float angleRadians) const;
    void screenToField (juce::Point<float> p, float& radiusParam, float& angleNorm) const;
    juce::Point<float> displayNodePosition (int node) const;
    int hitNode (juce::Point<float> p) const;
    bool hitCore (juce::Point<float> p) const;

    // Rendering
    void renderStatic (juce::Graphics& g);
    void ensureSprites (float physicalScale);
    void paintRings (juce::Graphics& g, float tension);
    void paintHeader (juce::Graphics& g);
    void paintSideReadouts (juce::Graphics& g);
    void paintConnections (juce::Graphics& g, const std::array<juce::Point<float>, 4>& pos);
    void paintMotionPaths (juce::Graphics& g, const std::array<juce::Point<float>, 4>& pos);
    void paintPulses (juce::Graphics& g);
    void paintNode (juce::Graphics& g, int node, juce::Point<float> p);
    void paintCore (juce::Graphics& g);
    void paintFreeze (juce::Graphics& g);

    float paramValue (const juce::String& id) const;

    ArcAudioProcessor& processor;
    FieldModel& model;
    std::array<NodeParams, 4> nodeParams;
    std::array<std::unique_ptr<juce::ParameterAttachment>, 4> radiusAttach, angleAttach;

    juce::Path chamber;
    juce::Point<float> centre;
    float rx = 1.0f, ry = 1.0f, nodeR = 20.0f, coreR = 40.0f;

    CachedLayer staticLayer, backdropLayer;
    std::function<void (juce::Graphics&)> backdrop;
    juce::Image nodeSprite, coreSprite, junctionSprite, haloSprite, coreHaloSprite, junctionHaloSprite;
    float spriteScale = 0.0f, cachedTension = -1.0f;

    Selection selection;
    int hoverNode = -1;
    bool hoverCore = false;

    // Dragging / recording
    int dragNode = -1;
    juce::Point<float> dragOffset;
    float dragRadius = 0.5f, dragAngle = 0.5f;
    int armedNode = -1, recordingNode = -1;
    double recordStart = 0.0;
    float recordStartRadius = 0.5f, recordStartAngle = 0.5f;
    std::vector<GestureSample> recordSamples;

    // Animation state
    double time = 0.0;
    bool running = false;
    std::array<float, 4> hoverAnim {}, selectAnim {};
    std::array<juce::Point<float>, 4> tremor {};
    std::array<juce::Point<float>, 4> tremorTarget {};
    float coreHover = 0.0f, coreSelect = 0.0f;
    juce::String captionTitle, captionBody, prevTitle, prevBody;
    float captionFade = 1.0f;
    juce::Random rng { 0x5EED };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonanceField)
};

} // namespace arc::ui

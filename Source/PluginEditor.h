#pragma once

// ARC editor. Composes the locked layout — header, EXCITER panel, Resonance Field,
// BODY / MATERIAL panel, performance strip — on a 1200 x 900 design canvas that is
// scaled uniformly (resizable 70 % .. 150 %, HiDPI-sharp: static art is cached at the
// physical pixel density). A display-synced clock drives telemetry smoothing and
// animation at up to 60 fps, throttling to ~15 fps when the network is silent and
// nobody is touching the UI. See docs/UI_SYSTEM.md.

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "UI/ArcLookAndFeel.h"
#include "UI/Controls.h"
#include "UI/FieldModel.h"
#include "UI/Header.h"
#include "UI/Inspectors.h"
#include "UI/PresetBrowser.h"
#include "UI/ResonanceField.h"
#include "UI/SidePanels.h"

namespace arc::ui
{

/** MOTION rate: free rate (Hz) or, with SYNC, the musical division. Drag / wheel. */
class MotionRateControl : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MotionRateControl (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void refresh();

private:
    bool synced() const;
    juce::AudioProcessorValueTreeState& state;
    juce::RangedAudioParameter& rate;
    juce::RangedAudioParameter& division;
    juce::ParameterAttachment rateAttach, divisionAttach;
    float dragStart = 0.0f;
    int dragStartDivision = 0;
};

/** Small live status block in the strip's right corner. */
class StatusReadout : public juce::Component
{
public:
    StatusReadout (ArcAudioProcessor& p, FieldModel& m) : processor (p), model (m) {}
    void paint (juce::Graphics&) override;

private:
    ArcAudioProcessor& processor;
    FieldModel& model;
};

/** The 1200 x 900 canvas: owns the cached machined-silver backdrop. */
class Canvas : public juce::Component
{
public:
    std::function<void (juce::Graphics&)> renderBackground;
    std::function<void()> onBackgroundClick;
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override
    {
        if (onBackgroundClick)
            onBackgroundClick();
    }
    void invalidateBackground() { layer.invalidate(); }

private:
    CachedLayer layer;
};

} // namespace arc::ui

class ArcAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ArcAudioProcessorEditor (ArcAudioProcessor&);
    ~ArcAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    /** Deterministic frame advance (tests / snapshots). */
    void advanceFrame (double dtSeconds);
    arc::ui::ResonanceField& getField() noexcept { return field; }
    void openPresetBrowser (bool open);
    arc::ui::PresetBrowser& getPresetBrowser() noexcept { return browser; }
    void selectNode (int node);
    void selectCore();
    void openSettings (bool open);
    void openPanelInspectors (bool exciterOpen, bool materialOpen);

private:
    void layoutCanvas();
    void renderBackground (juce::Graphics& g);
    void onFrame (double timestampSeconds);
    void closeOverlays (bool includingPanels);
    void updateCaption();
    void positionCards();
    void setUiSize (int index);

    ArcAudioProcessor& processor;
    arc::ui::ArcLookAndFeel lnf;
    arc::ui::FieldModel model;
    arc::ui::Canvas canvas;

    // Header
    arc::ui::PresetDisplay presetDisplay;
    arc::ui::IconButton gear { "Settings", arc::gfx::Icon::gear, arc::ui::colours::inkMuted, arc::ui::colours::cyanDim };
    arc::ui::LevelMeter meter;
    arc::ui::ArcKnob master { "", arc::ui::KnobStyle::master };

    // Panels and field
    arc::ui::ExciterPanel exciter;
    arc::ui::MaterialPanel material;
    arc::ui::ResonanceField field;

    // Performance strip
    arc::ui::ArcKnob motion { "Motion", arc::ui::KnobStyle::master };
    arc::ui::MotionRateControl motionRate;
    arc::ui::ArcKnob excite { "Excite", arc::ui::KnobStyle::macro }, coupling { "Coupling", arc::ui::KnobStyle::macro },
        tension { "Tension", arc::ui::KnobStyle::macro }, chaos { "Chaos", arc::ui::KnobStyle::macro };
    arc::ui::RoundButton freeze { "Freeze", arc::gfx::Icon::freeze, true }, random { "Random", arc::gfx::Icon::random, false },
        sync { "Sync", arc::gfx::Icon::sync, true };
    std::unique_ptr<arc::ui::IndexAttachment> freezeAttach, syncAttach;
    arc::ui::StatusReadout status;

    // Overlays
    arc::ui::NodeInspector nodeInspector;
    arc::ui::CoreInspector coreInspector;
    arc::ui::SettingsCard settings;
    arc::ui::PresetBrowser browser;

    std::unique_ptr<juce::VBlankAttachment> vblank;
    double lastFrameTime = -1.0, lastInteraction = 0.0, lastEngineBeat = -10.0, clock = 0.0;
    uint32_t lastBlockCounter = 0;
    juce::Point<float> lastMousePosition;
    bool dockPlaced = false, dockAtBottom = false;
    int frameCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArcAudioProcessorEditor)
};

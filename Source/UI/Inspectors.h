#pragma once

// Contextual inspectors on dark glass, floating over the Resonance Field:
//   NodeInspector  — RATIO / DECAY / DAMP / LEVEL / PAN / LINK + motion record / clear
//   CoreInspector  — network topology, quantised tuning, output space
//   SettingsCard   — voice mode, polyphony, glide, bend, release, MPE, quality

#include "UI/Controls.h"
#include "UI/FieldModel.h"

class ArcAudioProcessor;

namespace arc::ui
{

class ResonanceField;

class GlassCard : public juce::Component
{
public:
    GlassCard (const juce::String& title);
    void setTitle (const juce::String& t, const juce::String& sub = {});
    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Fade / rise animation driven by the editor's clock. */
    void setShown (bool shouldShow);
    bool isShown() const noexcept { return shown; }
    void advance (float dt);

protected:
    juce::Rectangle<int> content() const { return getLocalBounds().reduced (16, 0).withTrimmedTop (46).withTrimmedBottom (12); }
    juce::String title, subtitle;
    IconButton closeButton { "Close", gfx::Icon::close, colours::glassMuted, colours::cyanBright };
    float anim = 0.0f;
    bool shown = false;
};

class NodeInspector final : public GlassCard
{
public:
    NodeInspector (ArcAudioProcessor& p, FieldModel& m, ResonanceField& f);
    void setNode (int node);
    int getNode() const noexcept { return node; }
    void refresh(); // gesture state, ratio readout
    void resized() override;

private:
    ArcAudioProcessor& processor;
    FieldModel& model;
    ResonanceField& field;
    int node = -1;
    std::array<std::unique_ptr<ArcKnob>, 6> knobs;
    ChipToggle rec { "Rec", true };
    ChipToggle loop { "Loop", true };
    IconButton clear { "Clear motion", gfx::Icon::trash, colours::glassMuted, colours::cyanBright };
    bool hasGesture = false;
};

class CoreInspector final : public GlassCard
{
public:
    explicit CoreInspector (ArcAudioProcessor& p);
    void resized() override;
    void refresh();

private:
    SegmentedControl topology { { "Star", "Ring", "Web", "Chain" }, true };
    ChipToggle quantise { "Quantize", true };
    ArcKnob space { "Space", KnobStyle::glass }, width { "Width", KnobStyle::glass }, drive { "Drive", KnobStyle::glass };
};

class SettingsCard final : public GlassCard
{
public:
    explicit SettingsCard (ArcAudioProcessor& p);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    SegmentedControl voiceMode { { "Poly", "Mono", "Legato" }, true };
    SegmentedControl quality { { "Eco", "Normal", "High" }, true };
    ArcKnob polyphony { "Voices", KnobStyle::glass }, glide { "Glide", KnobStyle::glass }, bend { "Bend", KnobStyle::glass },
        release { "Release", KnobStyle::glass };
    ChipToggle mpe { "MPE", true };
    SegmentedControl uiSize { { "S", "M", "L", "XL" }, true };

public:
    /** Window size presets (S / M / L / XL); the editor applies them. */
    std::function<void (int)> onSizeChosen;
    void setSizeIndex (int i) { uiSize.setIndex (i, false); }
};

} // namespace arc::ui

#pragma once

// Header: the preset display (previous / name + tags / favourite / next, opening the
// browser) and the stereo output meter.

#include "Core/PresetManager.h"
#include "UI/Controls.h"
#include "UI/FieldModel.h"

namespace arc::ui
{

class PresetDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit PresetDisplay (PresetManager& presets);

    /** Refresh name / tags / modified / favourite (cheap; called ~10 Hz). */
    void refresh();
    std::function<void()> onOpenBrowser;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    PresetManager& presets;
    IconButton prev { "Previous", gfx::Icon::chevronLeft, colours::glassMuted, colours::cyanBright };
    IconButton next { "Next", gfx::Icon::chevronRight, colours::glassMuted, colours::cyanBright };
    IconButton heart { "Favourite", gfx::Icon::heart, colours::glassMuted, colours::cyanBright };
    juce::String name, tags;
    bool modified = false, favourite = false, hover = false;
};

class LevelMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit LevelMeter (FieldModel& m) : model (m) {}
    void paint (juce::Graphics&) override;

private:
    FieldModel& model;
};

} // namespace arc::ui

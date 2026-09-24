#pragma once

// EXCITER ("how it begins") and BODY / MATERIAL ("what it resonates as") panels.
// Four large selector tiles each; clicking a tile selects it and opens its inspector
// inline — the tiles glide into a compact stack and the detail knobs rise beneath them
// (contextual depth instead of permanent clutter). Clicking the selected tile again, or
// anywhere outside, closes it.

#include <array>

#include "UI/Controls.h"

namespace arc::ui
{

class SelectorPanel : public juce::Component
{
public:
    struct Option
    {
        juce::String name;
        gfx::Icon icon;
        juce::String help;
    };

    SelectorPanel (juce::AudioProcessorValueTreeState& state, const juce::String& choiceParamId, const juce::String& heading,
                   const juce::String& subheading, std::array<Option, 4> options, bool mirrored);
    ~SelectorPanel() override;

    void setInspectorOpen (bool open);
    bool isInspectorOpen() const noexcept { return inspectorOpen; }
    std::function<void (bool)> onInspectorToggled;

    /** Animation step (editor frame clock). */
    void advance (float dt);

    void paint (juce::Graphics&) override;
    void resized() override;

protected:
    /** Knobs shown in the inspector for the currently selected option. */
    virtual void updateInspectorFor (int option) = 0;
    void finishInit();
    std::vector<std::unique_ptr<ArcKnob>> knobs;
    juce::AudioProcessorValueTreeState& state;

private:
    void layoutContent();

    juce::String heading, subheading, paramId;
    std::array<std::unique_ptr<SelectorTile>, 4> tiles;
    std::unique_ptr<IndexAttachment> attachment;
    int selected = 0;
    bool inspectorOpen = false, mirrored = false;
    float openAnim = 0.0f;
};

class ExciterPanel final : public SelectorPanel
{
public:
    explicit ExciterPanel (juce::AudioProcessorValueTreeState& state);

private:
    void updateInspectorFor (int option) override;
    int attachedTo = -1;
};

class MaterialPanel final : public SelectorPanel
{
public:
    explicit MaterialPanel (juce::AudioProcessorValueTreeState& state);

private:
    void updateInspectorFor (int option) override;
    bool attached = false;
};

} // namespace arc::ui

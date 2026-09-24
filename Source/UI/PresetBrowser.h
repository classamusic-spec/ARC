#pragma once

// Preset browser: a dark-glass sheet that drops from the preset display over the
// chamber. Categories on the left, presets on the right; a click auditions (loads and
// keeps the browser open), a double-click or Enter loads and closes, Esc closes.
// Hovering a preset shows its description in the chamber caption. SAVE AS writes a
// user preset (name + category), user presets can be deleted.

#include "Core/PresetManager.h"
#include "UI/Inspectors.h"

namespace arc::ui
{

class PresetBrowser final : public GlassCard, public juce::TooltipClient
{
public:
    explicit PresetBrowser (PresetManager& pm);
    ~PresetBrowser() override;

    void refresh();
    void resized() override;
    void paint (juce::Graphics&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::String getTooltip() override;

private:
    struct CategoryList;
    struct PresetList;
    void selectCategory (int index);
    void rebuildVisible();
    void showSaveRow (bool show);

    PresetManager& presets;
    juce::StringArray categoryNames;
    int categoryIndex = 0;
    std::vector<int> visible; // indices into the preset manager

    std::unique_ptr<CategoryList> categoryList;
    std::unique_ptr<PresetList> presetList;
    juce::Viewport presetViewport;

    juce::TextButton saveButton { "Save as" }, initButton { "Init" }, deleteButton { "Delete" };
    juce::TextEditor nameEditor;
    juce::TextButton confirmSave { "Save" }, cancelSave { "Cancel" };
    SegmentedControl saveCategory { { "Pads", "Plucked", "Struck", "Bowed", "Air", "Drones", "User" }, true };
    bool saving = false;
};

} // namespace arc::ui

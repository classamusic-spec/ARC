#pragma once

// Preset browser: a dark-glass sheet that drops from the preset display over the
// chamber. Categories on the left (with how many presets each holds, or how many match
// the search), presets on the right under a search field. Typing anywhere in the browser
// searches: every word must appear in a preset's name, tags, description or category.
// A click auditions (loads and keeps the browser open), a double-click or Enter loads and
// closes, the arrow keys step through the list (also from the search field), Esc clears
// the search and then closes. Hovering a preset shows its description as a tooltip.
// SAVE AS writes a user preset (name + category), user presets can be deleted.
// Only the rows inside the viewport are painted, so the list costs the same at 400
// presets as at 40.

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
    void focusOfChildComponentChanged (FocusChangeType) override;
    juce::String getTooltip() override;

    // --- search and state (also used by the tests) ------------------------------------------
    void setSearchText (const juce::String& text);
    juce::String getSearchText() const { return searchBox.getText(); }
    void selectCategory (const juce::String& name);
    juce::String getSelectedCategory() const { return categoryNames[categoryIndex]; }
    int getNumVisible() const noexcept { return static_cast<int> (visible.size()); }
    int getVisiblePreset (int row) const noexcept;
    /** Presets in a category ("ALL", "FAVOURITES", a factory category, "USER") that match
        the current search (all of them when the search is empty). */
    int getCategoryCount (const juce::String& category) const;
    /** Rows drawn by the last paint of the list (the visible window, not the whole list). */
    int getLastPaintedRows() const noexcept;

private:
    struct CategoryList;
    struct PresetList;
    struct SearchKeys;
    void selectCategory (int index);
    void rebuildVisible();
    void rebuildSearchIndex();
    bool matches (int presetIndex) const;
    void step (int delta);
    void activate();
    void showSaveRow (bool show);
    juce::Rectangle<int> searchArea() const;

    PresetManager& presets;
    juce::StringArray categoryNames;
    std::vector<int> categoryCounts; // parallel to categoryNames, for the current search
    int categoryIndex = 0;
    std::vector<int> visible;           // indices into the preset manager
    std::vector<juce::String> haystack; // lower-case name, tags, description, category per preset
    juce::StringArray searchWords;

    std::unique_ptr<CategoryList> categoryList;
    std::unique_ptr<PresetList> presetList;
    std::unique_ptr<SearchKeys> searchKeys;
    juce::Viewport presetViewport;
    juce::TextEditor searchBox;
    IconButton clearSearch { "Clear search", gfx::Icon::close, colours::glassMuted, colours::cyanBright };

    juce::TextButton saveButton { "Save as" }, initButton { "Init" }, deleteButton { "Delete" };
    juce::TextEditor nameEditor;
    juce::TextButton confirmSave { "Save" }, cancelSave { "Cancel" };
    SegmentedControl saveCategory { { "Pads", "Plucked", "Struck", "Bowed", "Air", "Drones", "User" }, true };
    bool saving = false;
};

} // namespace arc::ui

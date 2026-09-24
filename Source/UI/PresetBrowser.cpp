#include "UI/PresetBrowser.h"

#include "Core/FactoryPresets.h"

namespace arc::ui
{

namespace
{
constexpr int kRowH = 38;
constexpr int kCatRowH = 26;

void styleTextButton (juce::TextButton& b)
{
    b.setColour (juce::TextButton::buttonColourId, colours::graphiteHigh);
    b.setColour (juce::TextButton::buttonOnColourId, colours::graphiteHigh.brighter (0.1f));
    b.setColour (juce::TextButton::textColourOffId, colours::glassText);
    b.setColour (juce::TextButton::textColourOnId, colours::cyanBright);
    b.setColour (juce::ComboBox::outlineColourId, colours::graphiteLine);
    b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
}
} // namespace

// ---------------------------------------------------------------------------------------
struct PresetBrowser::CategoryList final : juce::Component
{
    PresetBrowser& owner;
    int hover = -1;
    explicit CategoryList (PresetBrowser& o) : owner (o) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < owner.categoryNames.size(); ++i)
        {
            auto r = juce::Rectangle<float> (0.0f, (float) (i * kCatRowH), (float) getWidth(), (float) kCatRowH);
            const bool sel = i == owner.categoryIndex;
            if (sel)
            {
                g.setColour (colours::graphiteHigh);
                g.fillRoundedRectangle (r.reduced (0.0f, 2.0f), 5.0f);
                g.setColour (colours::cyan);
                g.fillRect (r.getX(), r.getY() + 7.0f, 2.0f, r.getHeight() - 14.0f);
            }
            else if (i == hover)
            {
                g.setColour (juce::Colours::white.withAlpha (0.04f));
                g.fillRoundedRectangle (r.reduced (0.0f, 2.0f), 5.0f);
            }
            g.setColour (sel ? colours::cyanBright : (i == hover ? colours::glassText : colours::glassMuted));
            drawTrackedText (g, owner.categoryNames[i], r.withTrimmedLeft (12.0f), Fonts::regular (10.5f, 0.2f),
                             juce::Justification::centredLeft);
        }
    }
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = e.y / kCatRowH;
        if (h != hover)
        {
            hover = h < owner.categoryNames.size() ? h : -1;
            repaint();
        }
    }
    void mouseExit (const juce::MouseEvent&) override
    {
        hover = -1;
        repaint();
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = e.y / kCatRowH;
        if (i < owner.categoryNames.size())
            owner.selectCategory (i);
    }
};

// ---------------------------------------------------------------------------------------
struct PresetBrowser::PresetList final : juce::Component
{
    PresetBrowser& owner;
    int hover = -1;
    explicit PresetList (PresetBrowser& o) : owner (o) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    juce::Rectangle<float> heartArea (juce::Rectangle<float> row) const { return row.removeFromRight (34.0f).reduced (9.0f, 11.0f); }

    void paint (juce::Graphics& g) override
    {
        auto& pm = owner.presets;
        for (size_t row = 0; row < owner.visible.size(); ++row)
        {
            const int idx = owner.visible[row];
            const auto& p = pm.getPreset (idx);
            auto r = juce::Rectangle<float> (0.0f, (float) row * kRowH, (float) getWidth(), (float) kRowH);
            const bool current = idx == pm.getCurrentIndex();
            if (current || (int) row == hover)
            {
                g.setColour (current ? colours::graphiteHigh : juce::Colours::white.withAlpha (0.04f));
                g.fillRoundedRectangle (r.reduced (2.0f, 2.0f), 6.0f);
            }
            if (current)
            {
                g.setColour (colours::cyan);
                g.fillRect (r.getX() + 2.0f, r.getY() + 10.0f, 2.0f, r.getHeight() - 20.0f);
            }
            auto text = r.withTrimmedLeft (14.0f).withTrimmedRight (40.0f);
            g.setColour (current ? colours::cyanBright : colours::glassText);
            g.setFont (Fonts::regular (14.0f, 0.03f));
            g.drawText (p.name, text.withHeight (22.0f).withY (r.getY() + 3.0f), juce::Justification::bottomLeft, true);
            g.setColour (colours::glassMuted);
            const auto meta = (owner.categoryIndex <= 1 ? p.category + juce::String::fromUTF8 ("  \xc2\xb7  ") : juce::String())
                              + p.tags.toUpperCase();
            drawTrackedText (g, meta.toUpperCase(), text.withTop (r.getY() + 24.0f).withHeight (11.0f), Fonts::regular (8.5f, 0.16f),
                             juce::Justification::centredLeft);
            const bool fav = pm.isFavourite (idx);
            gfx::drawIcon (g, fav ? gfx::Icon::heartFilled : gfx::Icon::heart, heartArea (r),
                           fav ? colours::cyan : colours::glassFaint.brighter (0.2f), 1.3f);
        }
        if (owner.visible.empty())
        {
            g.setColour (colours::glassMuted);
            drawTrackedText (g, owner.categoryIndex == 1 ? "NO FAVOURITES YET" : "NO PRESETS", getLocalBounds().toFloat().withHeight (60.0f),
                             Fonts::regular (10.5f, 0.25f), juce::Justification::centred);
        }
    }
    int rowAt (int y) const
    {
        const int r = y / kRowH;
        return r >= 0 && r < (int) owner.visible.size() ? r : -1;
    }
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = rowAt (e.y);
        if (h != hover)
        {
            hover = h;
            repaint();
        }
    }
    void mouseExit (const juce::MouseEvent&) override
    {
        hover = -1;
        repaint();
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int row = rowAt (e.y);
        if (row < 0)
            return;
        const int idx = owner.visible[(size_t) row];
        const auto r = juce::Rectangle<float> (0.0f, (float) row * kRowH, (float) getWidth(), (float) kRowH);
        if (heartArea (r).expanded (6.0f).contains (e.position))
        {
            owner.presets.setFavourite (idx, ! owner.presets.isFavourite (idx));
            if (owner.categoryIndex == 1)
                owner.rebuildVisible();
            repaint();
            return;
        }
        owner.presets.loadPreset (idx); // audition: stay open
        repaint();
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (rowAt (e.y) >= 0 && owner.onClose)
            owner.onClose();
    }
};

// ---------------------------------------------------------------------------------------
PresetBrowser::PresetBrowser (PresetManager& pm) : GlassCard ("Presets"), presets (pm)
{
    categoryList = std::make_unique<CategoryList> (*this);
    presetList = std::make_unique<PresetList> (*this);
    addAndMakeVisible (*categoryList);
    presetViewport.setViewedComponent (presetList.get(), false);
    presetViewport.setScrollBarsShown (true, false);
    presetViewport.setScrollBarThickness (8);
    addAndMakeVisible (presetViewport);

    for (auto* b : { &saveButton, &initButton, &deleteButton, &confirmSave, &cancelSave })
    {
        styleTextButton (*b);
        addAndMakeVisible (*b);
    }
    saveButton.setTooltip ("SAVE AS\nStore the current sound as a user preset.");
    initButton.setTooltip ("INIT\nThe default network: a clean starting point.");
    deleteButton.setTooltip ("DELETE\nRemove this user preset.");
    saveButton.onClick = [this] { showSaveRow (true); };
    initButton.onClick = [this]
    {
        presets.loadInit();
        refresh();
    };
    deleteButton.onClick = [this]
    {
        presets.deleteUserPreset (presets.getCurrentIndex());
        refresh();
    };
    confirmSave.onClick = [this]
    {
        const juce::StringArray cats { "PADS", "PLUCKED", "STRUCK", "BOWED", "AIR", "DRONES", "USER" };
        if (presets.saveUserPreset (nameEditor.getText(), cats[saveCategory.getIndex()]))
        {
            showSaveRow (false);
            refresh();
            selectCategory (categoryNames.indexOf ("USER"));
        }
        else
            nameEditor.setColour (juce::TextEditor::outlineColourId, colours::amber);
    };
    cancelSave.onClick = [this] { showSaveRow (false); };

    nameEditor.setFont (Fonts::regular (14.0f));
    nameEditor.setTextToShowWhenEmpty ("Preset name", colours::glassFaint);
    nameEditor.onReturnKey = [this] { confirmSave.triggerClick(); };
    nameEditor.onEscapeKey = [this] { showSaveRow (false); };
    addChildComponent (nameEditor);
    addChildComponent (saveCategory);
    saveCategory.setIndex (6, false);
    confirmSave.setVisible (false);
    cancelSave.setVisible (false);
    setWantsKeyboardFocus (true);
    refresh();
}

PresetBrowser::~PresetBrowser() = default;

void PresetBrowser::refresh()
{
    const auto currentCategory = categoryNames[categoryIndex];
    categoryNames = { "ALL", "FAVOURITES" };
    for (const auto& c : presets::categories())
        categoryNames.add (c);
    bool hasUser = false;
    for (int i = 0; i < presets.getNumPresets(); ++i)
        hasUser = hasUser || ! presets.getPreset (i).isFactory;
    if (hasUser)
        categoryNames.add ("USER");
    const int restored = categoryNames.indexOf (currentCategory);
    categoryIndex = restored >= 0 ? restored : 0;
    rebuildVisible();
    const int cur = presets.getCurrentIndex();
    deleteButton.setEnabled (cur >= 0 && ! presets.getPreset (cur).isFactory);
    setTitle ("Presets", juce::String (presets.getNumPresets()) + " PRESETS  " + juce::String::fromUTF8 ("\xc2\xb7") + "  CLICK TO AUDITION");
    categoryList->repaint();
}

void PresetBrowser::selectCategory (int index)
{
    categoryIndex = juce::jlimit (0, categoryNames.size() - 1, index);
    rebuildVisible();
    presetViewport.setViewPosition (0, 0);
    categoryList->repaint();
}

void PresetBrowser::rebuildVisible()
{
    visible.clear();
    const auto cat = categoryNames[categoryIndex];
    for (int i = 0; i < presets.getNumPresets(); ++i)
    {
        const auto& p = presets.getPreset (i);
        const bool match = cat == "ALL" || (cat == "FAVOURITES" && presets.isFavourite (i))
                           || (cat == "USER" && ! p.isFactory) || (p.isFactory && p.category == cat);
        if (match)
            visible.push_back (i);
    }
    presetList->setSize (juce::jmax (10, presetViewport.getWidth() - 10), juce::jmax (kRowH, (int) visible.size() * kRowH));
    presetList->repaint();
}

void PresetBrowser::showSaveRow (bool show)
{
    saving = show;
    nameEditor.setVisible (show);
    saveCategory.setVisible (show);
    confirmSave.setVisible (show);
    cancelSave.setVisible (show);
    saveButton.setVisible (! show);
    initButton.setVisible (! show);
    deleteButton.setVisible (! show);
    if (show)
    {
        nameEditor.setText (presets.getCurrentName() == "Init" ? juce::String() : presets.getCurrentName() + " 2", false);
        nameEditor.selectAll();
        nameEditor.grabKeyboardFocus();
    }
    resized();
}

void PresetBrowser::resized()
{
    GlassCard::resized();
    auto c = content();
    auto footer = c.removeFromBottom (saving ? 64 : 30);
    c.removeFromBottom (10);
    categoryList->setBounds (c.removeFromLeft (150).withHeight (juce::jmax (1, categoryNames.size() * kCatRowH)));
    c.removeFromLeft (12);
    presetViewport.setBounds (c);
    rebuildVisible();

    if (saving)
    {
        auto top = footer.removeFromTop (28);
        cancelSave.setBounds (top.removeFromRight (80));
        top.removeFromRight (6);
        confirmSave.setBounds (top.removeFromRight (80));
        top.removeFromRight (10);
        nameEditor.setBounds (top);
        footer.removeFromTop (8);
        saveCategory.setBounds (footer.removeFromTop (26));
    }
    else
    {
        saveButton.setBounds (footer.removeFromLeft (100));
        footer.removeFromLeft (8);
        initButton.setBounds (footer.removeFromLeft (70));
        deleteButton.setBounds (footer.removeFromRight (90));
    }
}

void PresetBrowser::paint (juce::Graphics& g)
{
    GlassCard::paint (g);
    // Divider between the category rail and the list.
    const auto c = content();
    g.setColour (colours::graphiteLine.withAlpha (0.7f));
    g.fillRect ((float) c.getX() + 156.0f, (float) c.getY(), 1.0f, (float) c.getHeight() - (saving ? 74.0f : 40.0f));
}

bool PresetBrowser::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (saving)
            showSaveRow (false);
        else if (onClose)
            onClose();
        return true;
    }
    if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey)
    {
        if (visible.empty())
            return true;
        const auto it = std::find (visible.begin(), visible.end(), presets.getCurrentIndex());
        int pos = it == visible.end() ? -1 : (int) std::distance (visible.begin(), it);
        pos = juce::jlimit (0, (int) visible.size() - 1, pos + (key == juce::KeyPress::downKey ? 1 : -1));
        presets.loadPreset (visible[(size_t) pos]);
        presetViewport.setViewPosition (0, juce::jmax (0, pos * kRowH - presetViewport.getHeight() / 2));
        presetList->repaint();
        return true;
    }
    if (key == juce::KeyPress::returnKey && onClose)
    {
        onClose();
        return true;
    }
    return false;
}

juce::String PresetBrowser::getTooltip()
{
    if (presetList->hover >= 0 && presetList->hover < (int) visible.size())
    {
        const auto& p = presets.getPreset (visible[(size_t) presetList->hover]);
        return p.name + "\n" + p.description;
    }
    return {};
}

} // namespace arc::ui

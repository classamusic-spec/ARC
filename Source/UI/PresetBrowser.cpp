#include "UI/PresetBrowser.h"

#include "Core/FactoryPresets.h"

namespace arc::ui
{

namespace
{
constexpr int kRowH = 38;
constexpr int kCatRowH = 26;
constexpr int kRailW = 164;
constexpr int kSearchH = 32;

void styleTextButton (juce::TextButton& b)
{
    b.setColour (juce::TextButton::buttonColourId, colours::graphiteHigh);
    b.setColour (juce::TextButton::buttonOnColourId, colours::graphiteHigh.brighter (0.1f));
    b.setColour (juce::TextButton::textColourOffId, colours::glassText);
    b.setColour (juce::TextButton::textColourOnId, colours::cyanBright);
    b.setColour (juce::ComboBox::outlineColourId, colours::graphiteLine);
    b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

const juce::String kMiddleDot = juce::String::fromUTF8 ("\xc2\xb7");
} // namespace

// ---------------------------------------------------------------------------------------
struct PresetBrowser::CategoryList final : juce::Component
{
    PresetBrowser& owner;
    int hover = -1;
    explicit CategoryList (PresetBrowser& o) : owner (o) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    void paint (juce::Graphics& g) override
    {
        const bool searching = ! owner.searchWords.isEmpty();
        for (int i = 0; i < owner.categoryNames.size(); ++i)
        {
            auto r = juce::Rectangle<float> (0.0f, (float) (i * kCatRowH), (float) getWidth(), (float) kCatRowH);
            const bool sel = i == owner.categoryIndex;
            const int count = i < (int) owner.categoryCounts.size() ? owner.categoryCounts[(size_t) i] : 0;
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
            // While searching, a category without a match fades back; the count says where
            // the matches are.
            const bool empty = searching && count == 0;
            g.setColour (sel ? colours::cyanBright : (empty ? colours::glassFaint : (i == hover ? colours::glassText : colours::glassMuted)));
            drawTrackedText (g, owner.categoryNames[i], r.withTrimmedLeft (12.0f).withTrimmedRight (34.0f), Fonts::regular (10.5f, 0.2f),
                             juce::Justification::centredLeft);
            g.setColour (sel ? colours::cyan : (empty ? colours::glassFaint.withAlpha (0.6f) : colours::glassFaint.brighter (0.25f)));
            drawTrackedText (g, juce::String (count), r.withTrimmedRight (10.0f), Fonts::regular (9.5f, 0.08f),
                             juce::Justification::centredRight);
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
    int lastPaintedRows = 0;
    explicit PresetList (PresetBrowser& o) : owner (o) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    juce::Rectangle<float> heartArea (juce::Rectangle<float> row) const { return row.removeFromRight (34.0f).reduced (9.0f, 11.0f); }

    void paint (juce::Graphics& g) override
    {
        auto& pm = owner.presets;
        const bool showCategory = owner.categoryIndex <= 1 || ! owner.searchWords.isEmpty();
        // Only the rows the viewport shows: the list is ~15000 px tall with the whole library.
        const auto clip = g.getClipBounds();
        const int first = juce::jmax (0, clip.getY() / kRowH);
        const int last = juce::jmin ((int) owner.visible.size() - 1, clip.getBottom() / kRowH);
        lastPaintedRows = juce::jmax (0, last - first + 1);
        for (int row = first; row <= last; ++row)
        {
            const int idx = owner.visible[(size_t) row];
            const auto& p = pm.getPreset (idx);
            auto r = juce::Rectangle<float> (0.0f, (float) row * kRowH, (float) getWidth(), (float) kRowH);
            const bool current = idx == pm.getCurrentIndex();
            if (current || row == hover)
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
            const auto meta = (showCategory ? p.category + "  " + kMiddleDot + "  " : juce::String()) + p.tags;
            drawTrackedText (g, meta.toUpperCase(), text.withTop (r.getY() + 24.0f).withHeight (11.0f), Fonts::regular (8.5f, 0.16f),
                             juce::Justification::centredLeft);
            const bool fav = pm.isFavourite (idx);
            gfx::drawIcon (g, fav ? gfx::Icon::heartFilled : gfx::Icon::heart, heartArea (r),
                           fav ? colours::cyan : colours::glassFaint.brighter (0.2f), 1.3f);
        }
        if (owner.visible.empty())
        {
            const auto area = getLocalBounds().toFloat().withHeight (70.0f);
            if (! owner.searchWords.isEmpty())
            {
                g.setColour (colours::glassMuted);
                drawTrackedText (g, "NOTHING MATCHES", area.withHeight (40.0f), Fonts::regular (10.5f, 0.25f), juce::Justification::centredBottom);
                g.setColour (colours::glassFaint.brighter (0.2f));
                drawTrackedText (g, "TRY A MATERIAL, A MOOD OR AN INSTRUMENT", area.withTrimmedTop (44.0f), Fonts::regular (8.5f, 0.2f),
                                 juce::Justification::centredTop);
            }
            else
            {
                g.setColour (colours::glassMuted);
                drawTrackedText (g, owner.categoryIndex == 1 ? "NO FAVOURITES YET" : "NO PRESETS", area.withHeight (60.0f),
                                 Fonts::regular (10.5f, 0.25f), juce::Justification::centred);
            }
        }
    }
    int rowAt (int y) const
    {
        const int r = y / kRowH;
        return r >= 0 && r < (int) owner.visible.size() ? r : -1;
    }
    void repaintRow (int row)
    {
        if (row >= 0)
            repaint (0, row * kRowH, getWidth(), kRowH);
    }
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = rowAt (e.y);
        if (h != hover)
        {
            repaintRow (hover);
            hover = h;
            repaintRow (hover);
        }
    }
    void mouseExit (const juce::MouseEvent&) override
    {
        repaintRow (hover);
        hover = -1;
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
            owner.rebuildVisible(); // favourites count (and list, when showing them)
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
/** The search field keeps the list keys: arrows step through the results, Enter loads and
    closes, Esc clears the search (then closes). */
struct PresetBrowser::SearchKeys final : juce::KeyListener
{
    PresetBrowser& owner;
    explicit SearchKeys (PresetBrowser& o) : owner (o) {}

    bool keyPressed (const juce::KeyPress& key, juce::Component*) override
    {
        if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey)
        {
            owner.step (key == juce::KeyPress::downKey ? 1 : -1);
            return true;
        }
        if (key == juce::KeyPress::returnKey)
        {
            owner.activate();
            return true;
        }
        if (key == juce::KeyPress::escapeKey)
        {
            if (owner.searchBox.isEmpty())
            {
                if (owner.onClose)
                    owner.onClose();
            }
            else
                owner.setSearchText ({});
            return true;
        }
        return false;
    }
};

// ---------------------------------------------------------------------------------------
PresetBrowser::PresetBrowser (PresetManager& pm) : GlassCard ("Presets"), presets (pm)
{
    categoryList = std::make_unique<CategoryList> (*this);
    presetList = std::make_unique<PresetList> (*this);
    searchKeys = std::make_unique<SearchKeys> (*this);
    addAndMakeVisible (*categoryList);
    presetViewport.setViewedComponent (presetList.get(), false);
    presetViewport.setScrollBarsShown (true, false);
    presetViewport.setScrollBarThickness (8);
    presetViewport.setSingleStepSizes (kRowH, kRowH);
    addAndMakeVisible (presetViewport);

    // The search field draws no box of its own: the browser paints it (focus ring, icon).
    searchBox.setFont (Fonts::regular (13.5f, 0.02f));
    searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    searchBox.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    searchBox.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    searchBox.setColour (juce::TextEditor::textColourId, colours::glassText);
    searchBox.setColour (juce::TextEditor::highlightColourId, colours::cyanDim.withAlpha (0.6f));
    searchBox.setColour (juce::TextEditor::highlightedTextColourId, colours::ice);
    searchBox.setColour (juce::CaretComponent::caretColourId, colours::cyan);
    searchBox.setIndents (0, 7);
    searchBox.setSelectAllWhenFocused (true);
    searchBox.setTitle ("Search presets");
    searchBox.setTooltip ("SEARCH\nEvery word must match a name, tag, description or category: try \"glass bell\", \"dark drone\", \"kalimba\".");
    searchBox.addKeyListener (searchKeys.get());
    searchBox.onTextChange = [this] { setSearchText (searchBox.getText()); };
    addAndMakeVisible (searchBox);

    clearSearch.setTooltip ("Clear the search");
    clearSearch.onClick = [this]
    {
        setSearchText ({});
        searchBox.grabKeyboardFocus();
    };
    addChildComponent (clearSearch);

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

PresetBrowser::~PresetBrowser() { searchBox.removeKeyListener (searchKeys.get()); }

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
    rebuildSearchIndex();
    rebuildVisible();
    const int cur = presets.getCurrentIndex();
    deleteButton.setEnabled (cur >= 0 && ! presets.getPreset (cur).isFactory);
    const auto total = juce::String (presets.getNumPresets());
    setTitle ("Presets", total + " PRESETS  " + kMiddleDot + "  CLICK TO AUDITION  " + kMiddleDot + "  TYPE TO SEARCH");
    searchBox.setTextToShowWhenEmpty ("Search " + total + " presets: a name, material, mood or instrument", colours::glassFaint.brighter (0.15f));
    resized();
}

void PresetBrowser::rebuildSearchIndex()
{
    haystack.clear();
    haystack.reserve ((size_t) presets.getNumPresets());
    for (int i = 0; i < presets.getNumPresets(); ++i)
    {
        const auto& p = presets.getPreset (i);
        haystack.push_back ((p.name + " " + p.tags + " " + p.description + " " + p.category).toLowerCase());
    }
}

bool PresetBrowser::matches (int presetIndex) const
{
    if (searchWords.isEmpty())
        return true;
    const auto& text = haystack[(size_t) presetIndex];
    for (const auto& w : searchWords)
        if (! text.contains (w))
            return false;
    return true;
}

void PresetBrowser::setSearchText (const juce::String& text)
{
    if (searchBox.getText() != text)
        searchBox.setText (text, false); // no onTextChange: we are already handling it
    searchWords = juce::StringArray::fromTokens (text.toLowerCase(), " \t,;", "\"");
    searchWords.removeEmptyStrings();
    clearSearch.setVisible (text.isNotEmpty());
    rebuildVisible();
    presetViewport.setViewPosition (0, 0);
    repaint (searchArea().expanded (4));
}

void PresetBrowser::selectCategory (int index)
{
    categoryIndex = juce::jlimit (0, categoryNames.size() - 1, index);
    rebuildVisible();
    presetViewport.setViewPosition (0, 0);
}

void PresetBrowser::selectCategory (const juce::String& name)
{
    const int i = categoryNames.indexOf (name);
    if (i >= 0)
        selectCategory (i);
}

int PresetBrowser::getVisiblePreset (int row) const noexcept
{
    return row >= 0 && row < (int) visible.size() ? visible[(size_t) row] : -1;
}

int PresetBrowser::getCategoryCount (const juce::String& category) const
{
    const int i = categoryNames.indexOf (category);
    return i >= 0 && i < (int) categoryCounts.size() ? categoryCounts[(size_t) i] : 0;
}

int PresetBrowser::getLastPaintedRows() const noexcept { return presetList->lastPaintedRows; }

void PresetBrowser::rebuildVisible()
{
    visible.clear();
    categoryCounts.assign ((size_t) categoryNames.size(), 0);
    const auto cat = categoryNames[categoryIndex];
    const int favouritesRow = categoryNames.indexOf ("FAVOURITES"), userRow = categoryNames.indexOf ("USER");
    for (int i = 0; i < presets.getNumPresets(); ++i)
    {
        if (! matches (i))
            continue;
        const auto& p = presets.getPreset (i);
        const bool fav = presets.isFavourite (i);
        ++categoryCounts[0];
        if (fav && favouritesRow >= 0)
            ++categoryCounts[(size_t) favouritesRow];
        const int row = p.isFactory ? categoryNames.indexOf (p.category) : userRow;
        if (row >= 0)
            ++categoryCounts[(size_t) row];

        const bool inCategory = cat == "ALL" || (cat == "FAVOURITES" && fav) || (cat == "USER" && ! p.isFactory)
                                || (p.isFactory && p.category == cat);
        if (inCategory)
            visible.push_back (i);
    }
    presetList->hover = -1;
    presetList->setSize (juce::jmax (10, presetViewport.getWidth() - 10), juce::jmax (kRowH, (int) visible.size() * kRowH));
    presetList->repaint();
    categoryList->repaint();
    repaint (searchArea().expanded (4));
}

void PresetBrowser::step (int delta)
{
    if (visible.empty())
        return;
    const auto it = std::find (visible.begin(), visible.end(), presets.getCurrentIndex());
    int pos = it == visible.end() ? (delta > 0 ? -1 : (int) visible.size()) : (int) std::distance (visible.begin(), it);
    pos = juce::jlimit (0, (int) visible.size() - 1, pos + delta);
    presets.loadPreset (visible[(size_t) pos]);
    // Keep the loaded row in view without jumping the list around.
    const int top = presetViewport.getViewPositionY(), h = presetViewport.getViewHeight();
    if (pos * kRowH < top)
        presetViewport.setViewPosition (0, pos * kRowH);
    else if ((pos + 1) * kRowH > top + h)
        presetViewport.setViewPosition (0, (pos + 1) * kRowH - h);
    presetList->repaint();
}

void PresetBrowser::activate()
{
    // Enter loads the highlighted result (the first one if the current preset is filtered
    // out) and closes.
    if (! visible.empty() && std::find (visible.begin(), visible.end(), presets.getCurrentIndex()) == visible.end())
        presets.loadPreset (visible.front());
    if (onClose)
        onClose();
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

juce::Rectangle<int> PresetBrowser::searchArea() const
{
    auto c = content();
    c.removeFromLeft (kRailW + 12);
    return c.removeFromTop (kSearchH);
}

void PresetBrowser::resized()
{
    GlassCard::resized();
    auto c = content();
    auto footer = c.removeFromBottom (saving ? 64 : 30);
    c.removeFromBottom (10);
    categoryList->setBounds (c.removeFromLeft (kRailW).withHeight (juce::jmax (1, categoryNames.size() * kCatRowH)));
    c.removeFromLeft (12);

    auto search = c.removeFromTop (kSearchH);
    clearSearch.setBounds (search.removeFromRight (30).withSizeKeepingCentre (20, 20));
    search.removeFromRight (34); // match count
    search.removeFromLeft (34);  // magnifier
    searchBox.setBounds (search.reduced (0, 3));
    c.removeFromTop (8);
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
    g.fillRect ((float) c.getX() + (float) kRailW + 6.0f, (float) c.getY(), 1.0f, (float) c.getHeight() - (saving ? 74.0f : 40.0f));

    // Search field: a recessed glass well; the ring lights up while it has the keyboard.
    const auto s = searchArea().toFloat();
    const bool focused = searchBox.hasKeyboardFocus (true);
    g.setColour (colours::chamberDeep.withAlpha (0.55f));
    g.fillRoundedRectangle (s, 8.0f);
    g.setColour (focused ? colours::cyanDim : colours::graphiteLine);
    g.drawRoundedRectangle (s.reduced (0.5f), 8.0f, 1.0f);
    gfx::drawIcon (g, gfx::Icon::search, juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ s.getX() + 18.0f, s.getCentreY() }),
                   focused || ! searchWords.isEmpty() ? colours::cyan : colours::glassMuted, 1.4f);
    if (! searchWords.isEmpty())
    {
        const auto countArea = s.withTrimmedRight (30.0f).removeFromRight (34.0f);
        g.setColour (visible.empty() ? colours::amber : colours::glassMuted);
        drawTrackedText (g, juce::String ((int) visible.size()), countArea, Fonts::regular (10.0f, 0.1f), juce::Justification::centredRight);
    }
}

void PresetBrowser::focusOfChildComponentChanged (FocusChangeType)
{
    repaint (searchArea().expanded (4));
}

bool PresetBrowser::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (saving)
            showSaveRow (false);
        else if (! searchBox.isEmpty())
            setSearchText ({});
        else if (onClose)
            onClose();
        return true;
    }
    if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey)
    {
        step (key == juce::KeyPress::downKey ? 1 : -1);
        return true;
    }
    if (key == juce::KeyPress::returnKey)
    {
        activate();
        return true;
    }
    // Type to search: the first printable key moves into the search field.
    const auto ch = key.getTextCharacter();
    if (! saving && ch >= ' ' && ! key.getModifiers().isCommandDown() && ! key.getModifiers().isCtrlDown())
    {
        searchBox.grabKeyboardFocus();
        searchBox.setText (searchBox.getText() + juce::String::charToString (ch), true);
        searchBox.moveCaretToEnd();
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

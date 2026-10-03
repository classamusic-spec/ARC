#include "UI/Header.h"

namespace arc::ui
{

PresetDisplay::PresetDisplay (PresetManager& p) : presets (p)
{
    prev.setTooltip ("PREVIOUS PRESET\nStep back through the library (left arrow key).");
    next.setTooltip ("NEXT PRESET\nStep forward through the library (right arrow key).");
    heart.setTooltip ("FAVOURITE\nMark this preset; favourites have their own list in the browser.");
    prev.onClick = [this] { presets.loadPrevious(); refresh(); };
    next.onClick = [this] { presets.loadNext(); refresh(); };
    heart.onClick = [this]
    {
        const int i = presets.getCurrentIndex();
        if (i >= 0)
            presets.setFavourite (i, ! presets.isFavourite (i));
        refresh();
    };
    for (auto* b : { &prev, &next, &heart })
    {
        b->setStroke (1.5f);
        addAndMakeVisible (*b);
    }
    setTooltip ("PRESETS\nClick to browse the library, audition, favourite and save.");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    refresh();
}

void PresetDisplay::refresh()
{
    const int i = presets.getCurrentIndex();
    const auto newName = presets.getCurrentName();
    juce::String newTags;
    if (i >= 0)
    {
        auto list = juce::StringArray::fromTokens (presets.getPreset (i).tags, ",", "");
        list.trim();
        list.removeEmptyStrings();
        while (list.size() > 3)
            list.remove (3);
        newTags = list.joinIntoString (juce::String::fromUTF8 ("  \xc2\xb7  ")).toUpperCase();
    }
    else
        newTags = "UNSAVED";
    const bool mod = presets.isModified();
    const bool fav = i >= 0 && presets.isFavourite (i);
    if (newName != name || newTags != tags || mod != modified || fav != favourite)
    {
        name = newName;
        tags = newTags;
        modified = mod;
        favourite = fav;
        heart.setIcon (favourite ? gfx::Icon::heartFilled : gfx::Icon::heart);
        heart.setColours (favourite ? colours::cyan : colours::glassMuted, colours::cyanBright);
        repaint();
    }
}

void PresetDisplay::resized()
{
    auto r = getLocalBounds();
    const int cap = r.getHeight();
    prev.setBounds (r.removeFromLeft (cap).reduced (6));
    next.setBounds (r.removeFromRight (cap).reduced (6));
    heart.setBounds (r.removeFromRight (cap - 12).reduced (8, 12));
}

void PresetDisplay::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float radius = b.getHeight() * 0.26f;
    const float cap = b.getHeight();

    // Graphite display with darker end caps (the chevron keys).
    juce::ColourGradient face (colours::graphiteHigh, b.getX(), b.getY(), colours::chamber, b.getX(), b.getBottom(), false);
    g.setGradientFill (face);
    g.fillRoundedRectangle (b, radius);
    {
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip;
        clip.addRoundedRectangle (b, radius);
        g.reduceClipRegion (clip);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillRect (b.withWidth (cap));
        g.fillRect (b.withLeft (b.getRight() - cap));
        g.setColour (colours::graphiteLine.withAlpha (0.8f));
        g.fillRect (b.getX() + cap, b.getY() + 8.0f, 1.0f, b.getHeight() - 16.0f);
        g.fillRect (b.getRight() - cap, b.getY() + 8.0f, 1.0f, b.getHeight() - 16.0f);
        // Glass: a soft sheen falling from the top edge (no hard gloss line).
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (hover ? 0.085f : 0.06f), b.getX(), b.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), b.getX(), b.getY() + b.getHeight() * 0.62f, false));
        g.fillRect (b);
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (b.reduced (0.5f), radius, 1.0f);
    // The lit top edge of the glass.
    juce::ColourGradient edge (juce::Colours::white.withAlpha (0.0f), b.getX() + radius, 0.0f, juce::Colours::white.withAlpha (0.0f),
                               b.getRight() - radius, 0.0f, false);
    edge.addColour (0.5, juce::Colours::white.withAlpha (hover ? 0.2f : 0.14f));
    g.setGradientFill (edge);
    g.fillRect (b.getX() + radius, b.getY() + 1.0f, b.getWidth() - 2.0f * radius, 1.0f);

    const auto text = b.reduced (cap + 10.0f, 0.0f);
    g.setColour (hover ? colours::cyanBright : colours::cyanBright.interpolatedWith (colours::glassText, 0.35f));
    const auto nameFont = Fonts::regular (b.getHeight() * 0.34f, 0.06f);
    drawTrackedText (g, name, text.withHeight (b.getHeight() * 0.58f).withY (b.getY() + b.getHeight() * 0.1f), nameFont,
                     juce::Justification::centredBottom);
    if (modified)
    {
        const float w = textWidth (nameFont, name);
        const auto dot = juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ text.getCentreX() + w * 0.5f + 9.0f,
                                                                            b.getY() + b.getHeight() * 0.44f });
        g.setColour (colours::cyan);
        g.fillEllipse (dot);
    }
    g.setColour (colours::glassMuted);
    drawTrackedText (g, tags, text.withTop (b.getY() + b.getHeight() * 0.62f).withHeight (b.getHeight() * 0.24f),
                     Fonts::regular (b.getHeight() * 0.17f, 0.26f), juce::Justification::centredTop);
}

void PresetDisplay::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && onOpenBrowser)
        onOpenBrowser();
}

// ---------------------------------------------------------------------------------------
void LevelMeter::paint (juce::Graphics& g)
{
    // Two slim bars in recessed slots: a continuous signal-light fill (cyan, ice near full
    // scale, amber over it), finely segmented, with a peak-hold tick.
    const auto b = getLocalBounds().toFloat();
    const float labelW = 13.0f, barH = 5.0f, gap = 5.0f;
    const float top = b.getY() + (b.getHeight() - (2.0f * barH + gap)) * 0.5f;
    const float slotX = b.getX() + labelW, slotW = b.getWidth() - labelW;
    auto toPos = [] (float lin)
    {
        const float db = 20.0f * std::log10 (lin + 1.0e-9f);
        return juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 51.0f); // -48 .. +3 dBFS
    };
    auto dbPos = [] (float db) { return (db + 48.0f) / 51.0f; };
    const float x0 = slotX + 1.0f, span = slotW - 2.0f;
    juce::ColourGradient fill (colours::cyanDim, x0, 0.0f, colours::amber, x0 + span, 0.0f, false);
    fill.addColour (dbPos (-18.0f), colours::cyan);
    fill.addColour (dbPos (-3.0f), colours::cyanBright);
    fill.addColour (dbPos (0.0f) - 0.002, colours::ice);
    fill.addColour (dbPos (0.0f), colours::amber);

    const std::array<float, 2> levels { model.meterL, model.meterR };
    const std::array<float, 2> holds { model.holdL, model.holdR };
    for (int ch = 0; ch < 2; ++ch)
    {
        const float y = top + (float) ch * (barH + gap);
        const auto slot = juce::Rectangle<float> (slotX, y, slotW, barH);
        g.setColour (colours::inkMuted);
        drawTrackedText (g, ch == 0 ? "L" : "R", { b.getX(), y - 3.0f, labelW, barH + 6.0f }, Fonts::label (8.5f),
                         juce::Justification::centredLeft);

        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillRoundedRectangle (slot.translated (0.0f, 0.9f), barH * 0.5f);
        g.setColour (colours::graphite);
        g.fillRoundedRectangle (slot, barH * 0.5f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRect (slot.reduced (barH * 0.5f, 0.0f).withHeight (1.0f));

        const float level = toPos (levels[(size_t) ch]);
        const auto inner = slot.reduced (1.0f);
        if (level > 0.002f)
        {
            const auto lit = inner.withWidth (juce::jmax (inner.getHeight(), span * level));
            g.setGradientFill (fill);
            g.fillRoundedRectangle (lit, inner.getHeight() * 0.5f);
            g.setColour (colours::graphite.withAlpha (0.6f));
            for (float x = inner.getX() + 3.0f; x < lit.getRight() - 1.0f; x += 3.0f)
                g.fillRect (x, lit.getY(), 0.8f, lit.getHeight());
        }
        const float hold = toPos (holds[(size_t) ch]);
        if (hold > 0.01f)
        {
            g.setColour (hold > dbPos (0.0f) ? colours::amber : colours::ice);
            g.fillRect (x0 + span * hold - 0.7f, inner.getY() - 0.5f, 1.4f, inner.getHeight() + 1.0f);
        }
    }
}

} // namespace arc::ui

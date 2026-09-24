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
        // Glass sheen
        g.setColour (juce::Colours::white.withAlpha (hover ? 0.07f : 0.045f));
        g.fillRect (b.withHeight (b.getHeight() * 0.45f));
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (b.reduced (0.5f), radius, 1.0f);

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
    const auto b = getLocalBounds().toFloat();
    const float labelW = 14.0f;
    const float barH = juce::jmin (8.0f, (b.getHeight() - 6.0f) * 0.5f);
    const std::array<float, 2> levels { model.meterL, model.meterR };
    const std::array<float, 2> holds { model.holdL, model.holdR };
    const char* names[2] = { "L", "R" };
    const int segments = 34;
    for (int ch = 0; ch < 2; ++ch)
    {
        const float y = b.getY() + (b.getHeight() - 2.0f * barH - 6.0f) * 0.5f + (float) ch * (barH + 6.0f);
        g.setColour (colours::inkMuted);
        g.setFont (Fonts::regular (10.0f));
        g.drawText (names[ch], juce::Rectangle<float> (b.getX(), y - 2.0f, labelW, barH + 4.0f), juce::Justification::centredLeft);
        const auto bar = juce::Rectangle<float> (b.getX() + labelW, y, b.getWidth() - labelW, barH);
        // Recessed track
        g.setColour (colours::graphite);
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawLine (bar.getX() + 1.0f, bar.getBottom() + 0.8f, bar.getRight() - 1.0f, bar.getBottom() + 0.8f, 0.8f);

        // dB scale -48 .. +3 dBFS
        auto toX = [&] (float lin)
        {
            const float db = 20.0f * std::log10 (lin + 1.0e-9f);
            return juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 51.0f);
        };
        const float level = toX (levels[(size_t) ch]);
        const float hold = toX (holds[(size_t) ch]);
        const float segW = (bar.getWidth() - 4.0f) / (float) segments;
        for (int s = 0; s < segments; ++s)
        {
            const float pos = ((float) s + 0.5f) / (float) segments;
            const auto seg = juce::Rectangle<float> (bar.getX() + 2.0f + segW * (float) s, bar.getY() + 1.5f, segW - 1.2f, bar.getHeight() - 3.0f);
            const bool lit = pos <= level;
            const bool hot = pos > 0.94f;
            if (lit)
                g.setColour (hot ? colours::amber : colours::cyan.interpolatedWith (colours::cyanBright, pos));
            else
                g.setColour (colours::graphiteLine.withAlpha (0.55f));
            g.fillRect (seg);
        }
        if (hold > 0.01f)
        {
            g.setColour (colours::ice);
            g.fillRect (bar.getX() + 2.0f + (bar.getWidth() - 4.0f) * hold - 1.0f, bar.getY() + 1.0f, 1.6f, bar.getHeight() - 2.0f);
        }
    }
}

} // namespace arc::ui

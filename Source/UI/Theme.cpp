#include "UI/Theme.h"

#if ARC_HAS_BINARY_DATA
    #include "BinaryData.h"
#endif

namespace arc::ui
{

namespace
{
juce::Typeface::Ptr loadTypeface (Fonts::Weight w)
{
#if ARC_HAS_BINARY_DATA
    switch (w)
    {
        case Fonts::Weight::light:
            return juce::Typeface::createSystemTypefaceFor (ArcBinary::JostLight_ttf, (size_t) ArcBinary::JostLight_ttfSize);
        case Fonts::Weight::regular:
            return juce::Typeface::createSystemTypefaceFor (ArcBinary::JostRegular_ttf, (size_t) ArcBinary::JostRegular_ttfSize);
        case Fonts::Weight::medium:
            return juce::Typeface::createSystemTypefaceFor (ArcBinary::JostMedium_ttf, (size_t) ArcBinary::JostMedium_ttfSize);
    }
#endif
    juce::ignoreUnused (w);
    return nullptr;
}

juce::Typeface::Ptr typeface (Fonts::Weight w)
{
    static juce::Typeface::Ptr cache[3] = { loadTypeface (Fonts::Weight::light), loadTypeface (Fonts::Weight::regular),
                                            loadTypeface (Fonts::Weight::medium) };
    return cache[static_cast<int> (w)];
}
} // namespace

juce::Font Fonts::get (Weight w, float height, float tracking)
{
    auto options = juce::FontOptions().withHeight (height).withKerningFactor (tracking);
    if (auto tf = typeface (w))
        options = options.withTypeface (tf);
    else
        options = options.withStyle (w == Weight::medium ? "Bold" : "Regular");
    return juce::Font (options);
}

float textWidth (const juce::Font& font, const juce::String& text)
{
    // Shaping is the costly part of measuring text (JUCE 8), and the same labels are drawn
    // and measured every frame, so measured widths are remembered (bounded: the cache is
    // simply emptied when it grows past a few thousand strings).
    const juce::SharedResourcePointer<SharedUiCaches> caches;
    const auto key = font.getTypefaceName() + "|" + font.getTypefaceStyle() + "|" + juce::String (font.getHeight(), 3) + "|"
                     + juce::String (font.getExtraKerningFactor(), 4) + "|" + text;
    {
        const juce::SpinLock::ScopedLockType sl (caches->lock);
        if (const auto it = caches->textWidths.find (key); it != caches->textWidths.end())
            return it->second;
    }
    const float w = juce::GlyphArrangement::getStringWidth (font, text);
    const juce::SpinLock::ScopedLockType sl (caches->lock);
    if (caches->textWidths.size() > 4096)
        caches->textWidths.clear();
    caches->textWidths.emplace (key, w);
    return w;
}

juce::Font fitFont (juce::Font font, const juce::String& text, float maxWidth, float minHeight)
{
    // Tracking is part of the measured width, and the trailing space after the last glyph
    // does not need to fit. Width scales with height (tracking is relative to it too), so
    // the fitting height is solved for directly, then corrected for hinting.
    auto width = [&] (const juce::Font& f) { return textWidth (f, text) - f.getExtraKerningFactor() * f.getHeight(); };
    float w = width (font);
    for (int i = 0; i < 4 && w > maxWidth + 0.5f && font.getHeight() > minHeight; ++i)
    {
        font = font.withHeight (juce::jmax (minHeight, font.getHeight() * maxWidth / w * 0.995f));
        w = width (font);
    }
    return font;
}

void drawTrackedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, const juce::Font& requested,
                      juce::Justification just)
{
    // A label never loses letters: text that would not fit shrinks (to 70 % at most)
    // instead of being cut ("CHAI" for CHAIN in a narrow segment).
    const auto font = fitFont (requested, text, area.getWidth(), requested.getHeight() * 0.7f);
    // Tracking adds space after every glyph, including the last: shift centred text by
    // half of that so it sits optically centred.
    const float trailing = font.getExtraKerningFactor() * font.getHeight();
    auto a = area;
    if (just.testFlags (juce::Justification::horizontallyCentred))
        a = a.translated (trailing * 0.5f, 0.0f);
    else if (just.testFlags (juce::Justification::right))
        a = a.translated (trailing, 0.0f);
    g.setFont (font);
    g.drawText (text, a, just, false);
}

} // namespace arc::ui

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
    return juce::GlyphArrangement::getStringWidth (font, text);
}

void drawTrackedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, const juce::Font& font,
                      juce::Justification just)
{
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

#include "UI/ArcLookAndFeel.h"

#include "Graphics/SilverSurface.h"

namespace arc::ui
{

namespace
{
const juce::Identifier kStyleId ("arcKnobStyle");
}

ArcLookAndFeel::ArcLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, colours::graphite);
    setColour (juce::PopupMenu::textColourId, colours::glassText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::graphiteHigh);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::cyanBright);
    setColour (juce::TooltipWindow::backgroundColourId, colours::graphite);
    setColour (juce::TooltipWindow::textColourId, colours::glassText);
    setColour (juce::TextEditor::textColourId, colours::glassText);
    setColour (juce::TextEditor::highlightColourId, colours::cyan.withAlpha (0.3f));
    setColour (juce::TextEditor::highlightedTextColourId, colours::ice);
    setColour (juce::CaretComponent::caretColourId, colours::cyan);
    setColour (juce::Label::textColourId, colours::ink);
    setColour (juce::ScrollBar::thumbColourId, colours::glassFaint);
    setColour (juce::ResizableWindow::backgroundColourId, colours::chassisMid);
}

void ArcLookAndFeel::setKnobStyle (juce::Slider& s, KnobStyle style)
{
    s.getProperties().set (kStyleId, static_cast<int> (style));
}

KnobStyle ArcLookAndFeel::getKnobStyle (const juce::Slider& s)
{
    return static_cast<KnobStyle> (static_cast<int> (s.getProperties().getWithDefault (kStyleId, static_cast<int> (KnobStyle::small))));
}

void ArcLookAndFeel::paintKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float proportion, KnobStyle style, bool hover,
                                bool dragging, bool bipolar, bool enabled)
{
    const auto area = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), bounds.getHeight()),
                                                    juce::jmin (bounds.getWidth(), bounds.getHeight()));
    const auto c = area.getCentre();
    const float outer = area.getWidth() * 0.5f;
    const bool onGlass = style == KnobStyle::glass;

    // Geometry per style: dot ring radius, silver ring radius, face radius.
    const float dotR = outer * (style == KnobStyle::macro ? 0.93f : 0.92f);
    const float ringR = outer * (style == KnobStyle::macro ? 0.74f : 0.72f);
    const float faceR = ringR * (style == KnobStyle::macro ? 0.8f : 0.76f);
    const int numDots = style == KnobStyle::macro ? 41 : style == KnobStyle::master ? 29 : 23;
    const float dotSize = outer * (style == KnobStyle::macro ? 0.04f : 0.047f);
    const float alpha = enabled ? 1.0f : 0.4f;

    // --- segmented value arc --------------------------------------------------------
    const float start = kRotaryStart, end = kRotaryEnd;
    const float valueAngle = start + proportion * (end - start);
    const float centreAngle = start + 0.5f * (end - start);
    for (int i = 0; i < numDots; ++i)
    {
        const float t = (float) i / (float) (numDots - 1);
        const float a = start + t * (end - start);
        const bool lit = bipolar ? ((a >= juce::jmin (centreAngle, valueAngle) - 1.0e-4f) && (a <= juce::jmax (centreAngle, valueAngle) + 1.0e-4f))
                                 : a <= valueAngle + 1.0e-4f;
        const juce::Point<float> p (c.x + dotR * std::sin (a), c.y - dotR * std::cos (a));
        if (lit)
        {
            // Brighter near the pointer: energy gathers at the value.
            const float nearValue = 1.0f - juce::jlimit (0.0f, 1.0f, std::abs (a - valueAngle) / 1.6f);
            const float glow = (0.35f + 0.65f * nearValue) * alpha;
            g.setColour (colours::cyan.withAlpha (0.24f * glow));
            g.fillEllipse (juce::Rectangle<float> (dotSize * 3.4f, dotSize * 3.4f).withCentre (p));
            g.setColour (colours::cyanBright.interpolatedWith (colours::cyan, 1.0f - nearValue).withAlpha (glow));
            g.fillEllipse (juce::Rectangle<float> (dotSize * 1.5f, dotSize * 1.5f).withCentre (p));
        }
        else
        {
            g.setColour ((onGlass ? colours::glassFaint : colours::inkMuted).withAlpha (0.6f * alpha));
            g.fillEllipse (juce::Rectangle<float> (dotSize, dotSize).withCentre (p));
        }
    }

    // --- contact shadow ----------------------------------------------------------------
    if (! onGlass)
        gfx::paintContactShadow (g, juce::Rectangle<float> (ringR * 2.3f, ringR * 2.3f).withCentre (c.translated (0.0f, ringR * 0.18f)), 0.35f);

    // --- machined silver ring -----------------------------------------------------------
    {
        const auto ring = juce::Rectangle<float> (ringR * 2.0f, ringR * 2.0f).withCentre (c);
        // Polished chrome: bright top-left, a dark band, a second reflection, dark lower edge.
        juce::ColourGradient metal (juce::Colour (0xfffcfdfe), ring.getX(), ring.getY(), juce::Colour (0xff6f7780), ring.getRight(),
                                    ring.getBottom(), false);
        metal.addColour (0.3, juce::Colour (0xffdde1e6));
        metal.addColour (0.56, juce::Colour (0xff959da6));
        metal.addColour (0.78, juce::Colour (0xffdbdfe4));
        if (onGlass)
        {
            metal = juce::ColourGradient (juce::Colour (0xff6d7680), ring.getX(), ring.getY(), juce::Colour (0xff262c33), ring.getRight(),
                                          ring.getBottom(), false);
            metal.addColour (0.4, juce::Colour (0xff4a525c));
        }
        g.setGradientFill (metal);
        g.fillEllipse (ring);
        g.setColour (juce::Colours::white.withAlpha (onGlass ? 0.15f : 0.85f));
        g.drawEllipse (ring.reduced (0.6f), 0.9f);
        g.setColour (juce::Colours::black.withAlpha (onGlass ? 0.35f : 0.42f));
        g.drawEllipse (ring, 1.0f);
    }

    // --- graphite face -------------------------------------------------------------------
    {
        const auto face = juce::Rectangle<float> (faceR * 2.0f, faceR * 2.0f).withCentre (c);
        juce::ColourGradient fg (juce::Colour (0xff3a4047), face.getCentreX() - faceR * 0.35f, face.getY() + faceR * 0.2f,
                                 juce::Colour (0xff07090b), face.getCentreX() + faceR * 0.5f, face.getBottom(), true);
        fg.addColour (0.45, juce::Colour (0xff171b20));
        g.setGradientFill (fg);
        g.fillEllipse (face);
        // Fine concentric machining (very faint).
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        for (float rr = faceR * 0.3f; rr < faceR; rr += faceR * 0.09f)
            g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (c), 0.6f);
        // Soft top reflection
        juce::ColourGradient refl (juce::Colours::white.withAlpha (hover ? 0.16f : 0.1f), c.x, face.getY(),
                                   juce::Colours::white.withAlpha (0.0f), c.x, c.y, false);
        g.setGradientFill (refl);
        g.fillEllipse (face.reduced (faceR * 0.08f).withTrimmedBottom (faceR * 0.9f));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (face, 1.0f);
        if (dragging || hover)
        {
            g.setColour (colours::cyan.withAlpha (dragging ? 0.45f : 0.22f));
            g.drawEllipse (face.expanded (0.8f), 1.0f);
        }
    }

    // --- pointer ---------------------------------------------------------------------------
    {
        const float a = valueAngle;
        const float r0 = faceR * 0.6f, r1 = faceR * 0.94f;
        const juce::Point<float> p0 (c.x + r0 * std::sin (a), c.y - r0 * std::cos (a));
        const juce::Point<float> p1 (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a));
        const float w = juce::jmax (1.6f, faceR * (style == KnobStyle::macro ? 0.045f : 0.07f));
        g.setColour (colours::cyan.withAlpha (0.25f * alpha));
        g.drawLine ({ p0, p1 }, w * 2.6f);
        g.setColour (juce::Colour (0xfff2f7fa).withAlpha (alpha));
        g.drawLine ({ p0, p1 }, w);
    }
}

void ArcLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos, float, float,
                                       juce::Slider& s)
{
    paintKnob (g, juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height), pos, getKnobStyle (s),
               s.isMouseOverOrDragging() && s.isEnabled(), s.isMouseButtonDown(),
               static_cast<bool> (s.getProperties().getWithDefault ("bipolar", false)), s.isEnabled());
}

// --- popup menus ----------------------------------------------------------------------
void ArcLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setGradientFill (juce::ColourGradient (colours::graphiteHigh, 0.0f, 0.0f, colours::graphite, 0.0f, (float) height, false));
    g.fillRect (r);
    g.setColour (colours::cyan.withAlpha (0.25f));
    g.drawRect (r, 1.0f);
}

void ArcLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                        bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                        const juce::String& shortcutKeyText, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (colours::graphiteLine);
        g.fillRect (area.reduced (10, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    auto r = area.toFloat().reduced (4.0f, 1.0f);
    if (isHighlighted && isActive)
    {
        g.setColour (colours::graphiteHigh.brighter (0.15f));
        g.fillRoundedRectangle (r, 4.0f);
    }
    auto textArea = r.reduced (12.0f, 0.0f);
    if (isTicked)
    {
        g.setColour (colours::cyan);
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ r.getX() + 7.0f, r.getCentreY() }));
    }
    g.setColour (! isActive ? colours::glassFaint : isHighlighted ? colours::cyanBright : colours::glassText);
    g.setFont (getPopupMenuFont());
    g.drawText (text, textArea, juce::Justification::centredLeft, true);
    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (colours::glassMuted);
        g.drawText (shortcutKeyText, textArea, juce::Justification::centredRight, true);
    }
    if (hasSubMenu)
    {
        juce::Path arrow;
        const float ax = r.getRight() - 10.0f, ay = r.getCentreY();
        arrow.startNewSubPath (ax - 3.0f, ay - 4.0f);
        arrow.lineTo (ax + 1.0f, ay);
        arrow.lineTo (ax - 3.0f, ay + 4.0f);
        g.setColour (colours::glassMuted);
        g.strokePath (arrow, juce::PathStrokeType (1.2f));
    }
}

juce::Font ArcLookAndFeel::getPopupMenuFont() { return Fonts::regular (14.0f, 0.04f); }

void ArcLookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    g.setColour (colours::glassMuted);
    drawTrackedText (g, name.toUpperCase(), area.toFloat().reduced (12.0f, 0.0f), Fonts::label (10.5f), juce::Justification::bottomLeft);
}

// --- tooltips -----------------------------------------------------------------------------
juce::Rectangle<int> ArcLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                                       juce::Rectangle<int> parentArea)
{
    const auto lines = juce::StringArray::fromLines (tipText);
    const auto title = lines[0], body = lines.size() > 1 ? lines[1] : juce::String();
    const float w = juce::jmax (textWidth (Fonts::label (11.0f), title.toUpperCase()),
                                juce::jmin (260.0f, textWidth (Fonts::regular (13.0f), body))) + 24.0f;
    const int h = body.isNotEmpty() ? (body.length() > 40 ? 62 : 46) : 26;
    return juce::Rectangle<int> (screenPos.x - (int) w / 2, screenPos.y + 22, (int) w, h).constrainedWithin (parentArea);
}

void ArcLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (colours::graphite.withAlpha (0.96f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (colours::cyan.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    const auto lines = juce::StringArray::fromLines (text);
    auto area = r.reduced (12.0f, 6.0f);
    g.setColour (colours::cyanBright);
    drawTrackedText (g, lines[0].toUpperCase(), area.removeFromTop (14.0f), Fonts::label (11.0f), juce::Justification::centredLeft);
    if (lines.size() > 1)
    {
        g.setColour (colours::glassText);
        g.setFont (Fonts::regular (13.0f));
        g.drawFittedText (lines[1], area.toNearestInt(), juce::Justification::topLeft, 2, 1.0f);
    }
}

// --- text editors, scroll bars, labels ---------------------------------------------------
void ArcLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& e)
{
    // A bare editor (transparent background) sits in a well its parent draws (preset search).
    if (e.findColour (juce::TextEditor::backgroundColourId).isTransparent())
        return;
    g.setColour (colours::chamberDeep);
    g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, 5.0f);
}

void ArcLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& e)
{
    if (e.findColour (juce::TextEditor::outlineColourId).isTransparent())
        return;
    g.setColour (e.hasKeyboardFocus (true) ? colours::cyan.withAlpha (0.8f) : colours::graphiteLine);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f, 5.0f, 1.0f);
}

void ArcLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height, bool vertical,
                                    int thumbStart, int thumbSize, bool over, bool down)
{
    if (thumbSize <= 0)
        return;
    const auto thumb = vertical ? juce::Rectangle<float> ((float) x + (float) width * 0.35f, (float) thumbStart, (float) width * 0.3f, (float) thumbSize)
                                : juce::Rectangle<float> ((float) thumbStart, (float) y + (float) height * 0.35f, (float) thumbSize, (float) height * 0.3f);
    g.setColour ((down ? colours::cyan : over ? colours::glassMuted : colours::glassFaint).withAlpha (0.9f));
    g.fillRoundedRectangle (thumb, juce::jmin (thumb.getWidth(), thumb.getHeight()) * 0.5f);
}

void ArcLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const float radius = r.getHeight() * 0.5f;
    g.setColour (down ? colours::graphiteHigh.brighter (0.15f) : (highlighted ? colours::graphiteHigh.brighter (0.07f) : colours::graphiteHigh));
    g.fillRoundedRectangle (r, radius);
    g.setColour (highlighted && b.isEnabled() ? colours::cyan.withAlpha (0.7f) : colours::graphiteLine);
    g.drawRoundedRectangle (r, radius, 1.0f);
}

void ArcLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool highlighted, bool)
{
    g.setColour (! b.isEnabled() ? colours::glassFaint : (highlighted ? colours::cyanBright : colours::glassText));
    drawTrackedText (g, b.getButtonText().toUpperCase(), b.getLocalBounds().toFloat(), Fonts::regular (10.5f, 0.22f),
                     juce::Justification::centred);
}

void ArcLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool over, bool dragging)
{
    // Three machined dots along the diagonal.
    for (int i = 0; i < 3; ++i)
    {
        const float x = (float) w - 5.0f - (float) i * 4.5f, y = (float) h - 5.0f - (float) (2 - i) * 0.0f;
        for (int j = 0; j <= i; ++j)
        {
            const auto p = juce::Point<float> (x + (float) j * 4.5f, y - (float) j * 4.5f);
            g.setColour ((dragging ? colours::cyan : over ? colours::inkMuted : colours::inkFaint).withAlpha (0.9f));
            g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre (p));
        }
    }
}

juce::Font ArcLookAndFeel::getLabelFont (juce::Label&) { return Fonts::regular (14.0f); }
juce::Font ArcLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return Fonts::label (juce::jmin (13.0f, (float) buttonHeight * 0.42f));
}

} // namespace arc::ui

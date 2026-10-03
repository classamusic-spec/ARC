#include "UI/Controls.h"

#include "Graphics/SilverSurface.h"

namespace arc::ui
{

juce::String percent (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; }

namespace
{
/** Soft light around a shape from a few widening strokes, drawn before the shape itself
    (which covers their inner halves). No blur: a blurred shadow costs about a millisecond
    per paint, and lit controls repaint while they animate. */
void paintGlow (juce::Graphics& g, const juce::Path& shape, juce::Colour c, float strength)
{
    constexpr int steps = 6;
    for (int i = steps; i >= 1; --i)
    {
        const float k = (float) i / (float) steps;
        g.setColour (c.withAlpha (strength * 0.16f * (1.0f - k) * (1.0f - k) + strength * 0.012f));
        g.strokePath (shape, juce::PathStrokeType (2.0f + 12.0f * k));
    }
}
} // namespace

// ---------------------------------------------------------------------------------------
IndexAttachment::IndexAttachment (juce::RangedAudioParameter& p, std::function<void (int)> onParameterChange, juce::UndoManager* um)
    : param (p),
      attachment (p, [this, cb = std::move (onParameterChange)] (float plain) { if (cb) cb (juce::roundToInt (plain)); }, um)
{
    attachment.sendInitialUpdate();
}

void IndexAttachment::set (int index) { attachment.setValueAsCompleteGesture ((float) index); }

int IndexAttachment::get() const { return juce::roundToInt (param.convertFrom0to1 (param.getValue())); }

// ---------------------------------------------------------------------------------------
ArcKnob::ArcKnob (const juce::String& n, KnobStyle s) : style (s), name (n)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (ArcLookAndFeel::kRotaryStart, ArcLookAndFeel::kRotaryEnd, true);
    slider.setMouseDragSensitivity (s == KnobStyle::macro ? 320 : 240);
    slider.setVelocityBasedMode (false);
    slider.setScrollWheelEnabled (true);
    ArcLookAndFeel::setKnobStyle (slider, s);
    slider.onValueChange = [this] { repaint(); };
    slider.onDragStart = [this] { repaint(); };
    slider.onDragEnd = [this] { repaint(); };
    slider.onHoverChange = [this] { repaint(); };
    addAndMakeVisible (slider);
    if (s == KnobStyle::glass)
        labelColour = colours::glassMuted;
}

void ArcKnob::attach (juce::AudioProcessorValueTreeState& state, const juce::String& paramId)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    if (auto* p = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void ArcKnob::setHelp (const juce::String& title, const juce::String& text) { slider.setTooltip (title + "\n" + text); }

juce::String ArcKnob::currentText()
{
    if (slider.isMouseOverOrDragging())
    {
        if (formatter)
            return formatter (slider.getValue());
        return slider.getTextFromValue (slider.getValue());
    }
    return name;
}

void ArcKnob::paint (juce::Graphics& g)
{
    const bool active = slider.isMouseOverOrDragging();
    const float fontSize = style == KnobStyle::macro ? 14.5f : style == KnobStyle::master ? 10.5f : 10.5f;
    auto labelArea = getLocalBounds().toFloat().withTop ((float) slider.getBottom() + (style == KnobStyle::macro ? 2.0f : 0.0f));
    const auto colour = active ? (style == KnobStyle::glass ? colours::cyanBright : colours::cyanDim.darker (0.25f)) : labelColour;
    g.setColour (colour);
    const auto font = style == KnobStyle::macro ? Fonts::get (Fonts::Weight::medium, fontSize, 0.18f) : Fonts::label (fontSize);
    drawTrackedText (g, currentText().toUpperCase(), labelArea, font, juce::Justification::centredTop);
}

void ArcKnob::resized()
{
    auto r = getLocalBounds();
    const int labelH = style == KnobStyle::macro ? 26 : 16;
    const int side = juce::jmin (r.getWidth(), r.getHeight() - labelH);
    slider.setBounds (r.removeFromTop (side).withSizeKeepingCentre (side, side));
}

// ---------------------------------------------------------------------------------------
SelectorTile::SelectorTile (const juce::String& name, gfx::Icon i) : juce::Button (name), icon (i)
{
    setClickingTogglesState (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SelectorTile::setSelectedState (bool s)
{
    selected = s;
    selectedAnim = s ? 1.0f : 0.0f;
    repaint();
}

void SelectorTile::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const float radius = juce::jmin (10.0f, r.getHeight() * 0.2f);
    if (down)
        r = r.translated (0.0f, 0.6f);

    if (selected)
    {
        // Lit from within: soft cyan bloom, graphite face, one crisp cyan edge.
        juce::Path shape;
        shape.addRoundedRectangle (r, radius);
        paintGlow (g, shape, colours::cyan, over ? 1.25f : 1.0f);
        juce::ColourGradient face (colours::graphiteHigh, r.getX(), r.getY(), colours::chamber, r.getX(), r.getBottom(), false);
        g.setGradientFill (face);
        g.fillPath (shape);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), r.getX(), r.getCentreY(), false));
        g.fillRoundedRectangle (r.reduced (1.5f), radius - 1.0f);
        g.setColour (colours::cyan.withAlpha (over ? 1.0f : 0.9f));
        g.drawRoundedRectangle (r, radius, 1.2f);
    }
    else
    {
        // Raised satin key: a soft shadow below, top lip catching the light.
        g.setColour (juce::Colour (0xff3a4048).withAlpha (0.08f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f).expanded (0.5f), radius + 0.5f);
        g.setColour (juce::Colour (0xff3a4048).withAlpha (0.1f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.0f), radius);
        juce::ColourGradient face (colours::panelFace.brighter (over ? 0.17f : 0.12f), r.getX(), r.getY(),
                                   colours::panelFaceLow.brighter (over ? 0.07f : 0.02f), r.getX(), r.getBottom(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (r, radius);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (1.0f).translated (0.0f, 0.5f), radius - 1.0f, 1.0f);
        g.setColour ((over ? colours::inkFaint : colours::bevelDark).withAlpha (over ? 0.9f : 0.75f));
        g.drawRoundedRectangle (r, radius, 1.0f);
    }

    // Content: icon + label (full), or a smaller centred icon + label (compact).
    const float h = r.getHeight();
    const float iconSize = juce::jlimit (14.0f, 32.0f, h * (0.54f - 0.12f * compact));
    const float iconX = r.getX() + juce::jmax (18.0f, r.getWidth() * 0.2f) - iconSize * 0.5f;
    const auto iconArea = juce::Rectangle<float> (iconX, r.getCentreY() - iconSize * 0.5f, iconSize, iconSize);
    const auto iconColour = selected ? colours::cyanBright : (over ? colours::ink : colours::inkSoft);
    if (selected)
    {
        gfx::drawIcon (g, icon, iconArea, colours::cyan.withAlpha (0.12f), 6.5f);
        gfx::drawIcon (g, icon, iconArea, colours::cyan.withAlpha (0.3f), 3.4f);
    }
    gfx::drawIcon (g, icon, iconArea, iconColour, selected ? 1.5f : 1.3f);

    auto textArea = r.withLeft (iconArea.getRight() + juce::jmax (12.0f, r.getWidth() * 0.1f)).withTrimmedRight (30.0f);
    g.setColour (selected ? colours::glassText : colours::ink);
    drawTrackedText (g, getName().toUpperCase(), textArea, Fonts::label (juce::jlimit (10.0f, 13.5f, h * 0.25f)),
                     juce::Justification::centredLeft);

    // Status LED.
    const auto led = juce::Point<float> (r.getRight() - juce::jmax (14.0f, h * 0.3f), r.getCentreY());
    if (selected)
    {
        g.setGradientFill (juce::ColourGradient (colours::cyan.withAlpha (0.55f), led.x, led.y, colours::cyan.withAlpha (0.0f),
                                                 led.x + 7.0f, led.y, true));
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (led));
        g.setColour (colours::cyanBright);
        g.fillEllipse (juce::Rectangle<float> (4.6f, 4.6f).withCentre (led));
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillEllipse (juce::Rectangle<float> (4.6f, 4.6f).withCentre (led.translated (0.0f, 0.7f)));
        g.setColour (colours::inkFaint.withAlpha (0.55f));
        g.fillEllipse (juce::Rectangle<float> (4.6f, 4.6f).withCentre (led));
    }
}

// ---------------------------------------------------------------------------------------
RoundButton::RoundButton (const juce::String& label, gfx::Icon i, bool isToggle) : juce::Button (label), icon (i)
{
    setClickingTogglesState (isToggle);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void RoundButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    const auto b = getLocalBounds().toFloat();
    const float labelH = 22.0f;
    const float d = juce::jmin (b.getWidth(), b.getHeight() - labelH) - 12.0f; // room for the shadow
    const auto circle = juce::Rectangle<float> (d, d).withCentre ({ b.getCentreX(), b.getY() + 4.0f + d * 0.5f });
    const bool on = getToggleState() || activity > 0.05f;
    const float glow = juce::jmax (getToggleState() ? 1.0f : 0.0f, activity);

    if (glow > 0.01f)
    {
        // Engaged: the key glows from its seat.
        juce::Path disc;
        disc.addEllipse (circle);
        paintGlow (g, disc, colours::cyan, 1.3f * glow);
    }
    ArcLookAndFeel::paintKeyCap (g, circle, down);
    const auto face = circle.reduced (d * 0.065f);
    if (over && ! down)
    {
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.fillEllipse (face);
    }
    if (on)
    {
        g.setColour (colours::cyan.withAlpha (0.45f + 0.55f * glow));
        g.drawEllipse (face, 1.3f);
    }

    const auto iconArea = face.reduced (d * 0.24f).translated (0.0f, down ? 0.5f : 0.0f);
    if (on)
        gfx::drawIcon (g, icon, iconArea, colours::cyan.withAlpha (0.25f * glow), 3.4f);
    gfx::drawIcon (g, icon, iconArea, on ? colours::cyanDim.interpolatedWith (colours::cyan, glow) : (over ? colours::ink : colours::inkSoft), 1.35f);

    g.setColour (on ? colours::cyanDim.darker (0.2f) : colours::ink);
    drawTrackedText (g, getName().toUpperCase(), b.withTop (b.getBottom() - labelH + 4.0f), Fonts::label (11.5f),
                     juce::Justification::centredTop);
}

// ---------------------------------------------------------------------------------------
SegmentedControl::SegmentedControl (juce::StringArray opts, bool onGlass) : options (std::move (opts)), glass (onGlass)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SegmentedControl::attach (juce::RangedAudioParameter& p)
{
    attachment = std::make_unique<IndexAttachment> (p, [this] (int i) { setIndex (i, false); });
}

void SegmentedControl::setIndex (int i, bool notify)
{
    i = juce::jlimit (0, options.size() - 1, i);
    if (i == index && ! notify)
        return;
    index = i;
    repaint();
    if (notify)
    {
        if (attachment != nullptr)
            attachment->set (i);
        if (onChange)
            onChange (i);
    }
}

SegmentedControl::Layout SegmentedControl::layout() const
{
    Layout l;
    const auto r = getLocalBounds().toFloat().reduced (2.5f);
    const int n = juce::jmax (1, options.size());
    // Content-sized segments: a long label (CHAIN, LEGATO) gets the room it needs, short
    // ones share the rest; the font shrinks only if even that cannot fit.
    auto font = Fonts::label (juce::jlimit (8.5f, 11.0f, r.getHeight() * 0.5f));
    std::vector<float> widths ((size_t) n, 0.0f);
    for (int pass = 0; pass < 12; ++pass)
    {
        float total = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            widths[(size_t) i] = textWidth (font, options[i].toUpperCase()) - font.getExtraKerningFactor() * font.getHeight();
            total += widths[(size_t) i];
        }
        const float pad = (r.getWidth() - total) / (float) n;
        if (pad >= 8.0f || font.getHeight() <= 7.5f)
        {
            float x = r.getX();
            for (int i = 0; i < n; ++i)
            {
                const float w = widths[(size_t) i] + juce::jmax (0.0f, pad);
                l.segments.emplace_back (x, r.getY(), i == n - 1 ? r.getRight() - x : w, r.getHeight());
                x += w;
            }
            break;
        }
        font = font.withHeight (font.getHeight() - 0.5f);
    }
    l.font = font;
    return l;
}

int SegmentedControl::segmentAt (juce::Point<int> p) const
{
    if (options.isEmpty() || ! getLocalBounds().contains (p))
        return -1;
    const auto l = layout();
    for (size_t i = 0; i < l.segments.size(); ++i)
        if (p.x < (int) std::ceil (l.segments[i].getRight()))
            return (int) i;
    return options.size() - 1;
}

void SegmentedControl::mouseDown (const juce::MouseEvent& e)
{
    const int s = segmentAt (e.getPosition());
    if (s >= 0)
        setIndex (s, true);
}

void SegmentedControl::mouseMove (const juce::MouseEvent& e)
{
    const int s = segmentAt (e.getPosition());
    if (s != hover)
    {
        hover = s;
        repaint();
    }
}

void SegmentedControl::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
    repaint();
}

void SegmentedControl::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    const float radius = r.getHeight() * 0.5f;
    g.setColour (glass ? colours::chamberDeep.withAlpha (0.8f) : colours::panelFaceLow);
    g.fillRoundedRectangle (r, radius);
    g.setColour (glass ? colours::graphiteLine : colours::hairline);
    g.drawRoundedRectangle (r, radius, 1.0f);

    const auto l = layout();
    for (int i = 0; i < options.size() && i < (int) l.segments.size(); ++i)
    {
        const auto seg = l.segments[(size_t) i];
        const bool sel = i == index;
        if (sel)
        {
            g.setColour (glass ? colours::graphiteHigh : colours::graphite);
            g.fillRoundedRectangle (seg, seg.getHeight() * 0.5f);
            g.setColour (colours::cyan.withAlpha (0.8f));
            g.drawRoundedRectangle (seg, seg.getHeight() * 0.5f, 1.0f);
        }
        else if (i == hover)
        {
            g.setColour ((glass ? juce::Colours::white : juce::Colours::white).withAlpha (glass ? 0.06f : 0.5f));
            g.fillRoundedRectangle (seg, seg.getHeight() * 0.5f);
        }
        g.setColour (sel ? colours::cyanBright : (glass ? colours::glassMuted : colours::inkSoft));
        drawTrackedText (g, options[i].toUpperCase(), seg, l.font, juce::Justification::centred);
    }
}

// ---------------------------------------------------------------------------------------
ChipToggle::ChipToggle (const juce::String& label, bool onGlass) : juce::Button (label), glass (onGlass)
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ChipToggle::attach (juce::RangedAudioParameter& p)
{
    attachment = std::make_unique<IndexAttachment> (p, [this] (int i) { setToggleState (i != 0, juce::dontSendNotification); });
    onClick = [this] { if (attachment) attachment->set (getToggleState() ? 1 : 0); };
}

void ChipToggle::paintButton (juce::Graphics& g, bool over, bool)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    const float radius = r.getHeight() * 0.5f;
    const bool on = getToggleState();
    g.setColour (on ? (glass ? colours::graphiteHigh : colours::graphite) : (glass ? colours::chamberDeep.withAlpha (0.7f) : colours::panelFace));
    g.fillRoundedRectangle (r, radius);
    g.setColour (on ? colours::cyan.withAlpha (0.85f) : (over ? colours::inkFaint : (glass ? colours::graphiteLine : colours::hairline)));
    g.drawRoundedRectangle (r, radius, 1.0f);
    // status dot
    const auto dot = juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ r.getX() + radius + 2.0f, r.getCentreY() });
    g.setColour (on ? colours::cyan : (glass ? colours::glassFaint : colours::inkFaint));
    g.fillEllipse (dot);
    g.setColour (on ? colours::cyanBright : (glass ? colours::glassMuted : colours::inkSoft));
    drawTrackedText (g, getName().toUpperCase(), r.withTrimmedLeft (radius + 6.0f), Fonts::label (juce::jlimit (8.5f, 11.0f, r.getHeight() * 0.45f)),
                     juce::Justification::centred);
}

// ---------------------------------------------------------------------------------------
IconButton::IconButton (const juce::String& name, gfx::Icon i, juce::Colour n, juce::Colour h)
    : juce::Button (name), icon (i), normal (n), hoverColour (h)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void IconButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (getHeight() * 0.22f);
    if (down)
        r = r.translated (0.0f, 0.5f);
    const auto c = (over || getToggleState()) ? hoverColour : normal;
    if (over)
        gfx::drawIcon (g, icon, r, c.withAlpha (0.25f), stroke * 2.6f);
    gfx::drawIcon (g, icon, r, c, stroke);
}

} // namespace arc::ui

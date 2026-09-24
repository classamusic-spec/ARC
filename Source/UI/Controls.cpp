#include "UI/Controls.h"

#include "Graphics/SilverSurface.h"

namespace arc::ui
{

juce::String percent (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; }

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
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    const float radius = juce::jmin (10.0f, r.getHeight() * 0.2f);
    if (down)
        r = r.translated (0.0f, 0.6f);

    if (selected)
    {
        // Outer cyan halo
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (colours::cyan.withAlpha (0.05f * (float) i));
            g.drawRoundedRectangle (r.expanded ((float) (4 - i) * 1.2f), radius + (float) (4 - i), 1.6f);
        }
        juce::ColourGradient face (colours::graphiteHigh, r.getX(), r.getY(), colours::chamber, r.getX(), r.getBottom(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (r, radius);
        // Inner top sheen, fading down
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.08f), r.getX(), r.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), r.getX(), r.getCentreY(), false));
        g.fillRoundedRectangle (r.reduced (2.0f), radius - 1.0f);
        g.setColour (colours::cyan.withAlpha (over ? 1.0f : 0.85f));
        g.drawRoundedRectangle (r, radius, 1.3f);
    }
    else
    {
        juce::ColourGradient face (colours::panelFace.brighter (over ? 0.16f : 0.1f), r.getX(), r.getY(),
                                   colours::panelFaceLow.brighter (over ? 0.06f : 0.0f), r.getX(), r.getBottom(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (r, radius);
        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.drawRoundedRectangle (r.reduced (1.0f).translated (0.0f, 0.5f), radius - 1.0f, 1.0f);
        g.setColour ((over ? colours::inkMuted : colours::bevelDark).withAlpha (0.9f));
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

    auto textArea = r.withLeft (iconArea.getRight() + juce::jmax (12.0f, r.getWidth() * 0.1f));
    g.setColour (selected ? colours::glassText : colours::ink);
    drawTrackedText (g, getName().toUpperCase(), textArea, Fonts::label (juce::jlimit (10.0f, 13.5f, h * 0.25f)),
                     juce::Justification::centredLeft);
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
    auto circle = juce::Rectangle<float> (d, d).withCentre ({ b.getCentreX(), b.getY() + 4.0f + d * 0.5f });
    const bool on = getToggleState() || activity > 0.05f;
    const float glow = juce::jmax (getToggleState() ? 1.0f : 0.0f, activity);

    gfx::paintContactShadow (g, circle.expanded (d * 0.07f).translated (0.0f, d * 0.05f), 0.3f);
    if (glow > 0.0f)
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (colours::cyan.withAlpha (0.07f * (float) i * glow));
            g.drawEllipse (circle.expanded ((float) (4 - i) * 1.6f), 2.0f);
        }

    // Silver rim
    juce::ColourGradient rim (juce::Colour (0xfff8f9fa), circle.getX(), circle.getY(), juce::Colour (0xff939aa2), circle.getRight(),
                              circle.getBottom(), false);
    g.setGradientFill (rim);
    g.fillEllipse (circle);
    // Face
    const auto face = circle.reduced (d * 0.07f);
    juce::ColourGradient fg (down ? colours::panelFaceLow : colours::panelFace.brighter (over ? 0.18f : 0.12f), face.getX(), face.getY(),
                             down ? colours::panelFace.brighter (0.05f) : colours::panelFaceLow, face.getX(), face.getBottom(), false);
    g.setGradientFill (fg);
    g.fillEllipse (face);
    g.setColour (on ? colours::cyan.withAlpha (0.4f + 0.6f * glow) : colours::bevelDark.withAlpha (0.8f));
    g.drawEllipse (face, on ? 1.4f : 0.9f);

    const auto iconArea = face.reduced (d * 0.25f);
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

int SegmentedControl::segmentAt (juce::Point<int> p) const
{
    if (options.isEmpty() || ! getLocalBounds().contains (p))
        return -1;
    return juce::jlimit (0, options.size() - 1, p.x * options.size() / juce::jmax (1, getWidth()));
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

    const float w = r.getWidth() / (float) juce::jmax (1, options.size());
    for (int i = 0; i < options.size(); ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);
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
        drawTrackedText (g, options[i].toUpperCase(), seg, Fonts::label (juce::jlimit (8.5f, 11.0f, seg.getHeight() * 0.48f)),
                         juce::Justification::centred);
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

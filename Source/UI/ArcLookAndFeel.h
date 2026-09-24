#pragma once

// ARC LookAndFeel: knobs (graphite face, machined silver ring, segmented ice-cyan
// arc, minimal pointer), popup menus and tooltips in dark glass, text editors,
// scroll bars. Custom controls (Controls.h) draw themselves; this class covers the
// JUCE widgets ARC reuses.

#include "UI/Theme.h"

namespace arc::ui
{

enum class KnobStyle
{
    macro,  // bottom strip performance knobs
    master, // header output knob
    small,  // inspectors on silver
    glass   // inspectors on dark glass cards
};

class ArcLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ArcLookAndFeel();

    static void setKnobStyle (juce::Slider& s, KnobStyle style);
    static KnobStyle getKnobStyle (const juce::Slider& s);

    /** Paints a complete ARC knob (also used by previews / snapshots). */
    static void paintKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float proportion, KnobStyle style, bool hover,
                           bool dragging, bool bipolar = false, bool enabled = true);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                            bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>& area, const juce::String& sectionName) override;

    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override;

    void fillTextEditorBackground (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int width, int height, juce::TextEditor&) override;

    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical,
                        int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour, bool highlighted,
                               bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    static constexpr float kRotaryStart = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float kRotaryEnd = juce::MathConstants<float>::pi * 2.75f;
};

} // namespace arc::ui

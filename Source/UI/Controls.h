#pragma once

// ARC controls. Every control carries its help text as a standard tooltip string
// ("TITLE\nDescription"): the Resonance Field's caption shows it on hover, and hosts /
// screen readers get it through the normal TooltipClient interface.

#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Graphics/Icons.h"
#include "UI/ArcLookAndFeel.h"

namespace arc::ui
{

/** Binds an integer index (choice / bool / int parameter) to a callback-driven control. */
class IndexAttachment
{
public:
    IndexAttachment (juce::RangedAudioParameter& p, std::function<void (int)> onParameterChange, juce::UndoManager* um = nullptr);
    void set (int index);           // from the UI (one complete gesture)
    int get() const;
    juce::RangedAudioParameter& parameter() noexcept { return param; }

private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
};

// ---------------------------------------------------------------------------------------
/** Rotary knob + label. Shows the value in place of the name while hovered / dragged. */
class ArcKnob : public juce::Component
{
public:
    ArcKnob (const juce::String& name, KnobStyle style);

    void attach (juce::AudioProcessorValueTreeState& state, const juce::String& paramId);
    void setHelp (const juce::String& title, const juce::String& text);
    void setValueFormatter (std::function<juce::String (double)> f) { formatter = std::move (f); }
    void setBipolar (bool b) { slider.getProperties().set ("bipolar", b); }
    void setLabelColour (juce::Colour c) { labelColour = c; }
    void setName (const juce::String& n) { name = n; repaint(); }

    juce::Slider& getSlider() noexcept { return slider; }
    juce::Rectangle<int> getKnobBounds() const { return slider.getBounds(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String currentText();

    struct KnobSlider : juce::Slider
    {
        std::function<void()> onHoverChange;
        void mouseEnter (const juce::MouseEvent& e) override { juce::Slider::mouseEnter (e); if (onHoverChange) onHoverChange(); }
        void mouseExit (const juce::MouseEvent& e) override { juce::Slider::mouseExit (e); if (onHoverChange) onHoverChange(); }
    };

    KnobSlider slider;
    KnobStyle style;
    juce::String name;
    juce::Colour labelColour { colours::ink };
    std::function<juce::String (double)> formatter;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// ---------------------------------------------------------------------------------------
/** Large selector tile (EXCITER / MATERIAL): icon + name, graphite + cyan when selected. */
class SelectorTile : public juce::Button
{
public:
    SelectorTile (const juce::String& name, gfx::Icon icon);
    void setSelectedState (bool s);
    void setCompact (float amount) { compact = amount; repaint(); } // 0 = full, 1 = compact row
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    gfx::Icon icon;
    float selectedAnim = 0.0f, compact = 0.0f;
    bool selected = false;
};

// ---------------------------------------------------------------------------------------
/** Round machined button with icon and label below (FREEZE / RANDOM / SYNC). */
class RoundButton : public juce::Button
{
public:
    RoundButton (const juce::String& label, gfx::Icon icon, bool isToggle);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    void setActivity (float a) { activity = a; repaint(); } // extra glow (e.g. FREEZE amount)

    std::function<void (const juce::MouseEvent&)> onClickWithMods;

    void mouseUp (const juce::MouseEvent& e) override
    {
        const bool inside = e.mouseWasClicked() && getLocalBounds().contains (e.getPosition());
        juce::Button::mouseUp (e);
        if (inside && onClickWithMods)
            onClickWithMods (e);
    }

private:
    gfx::Icon icon;
    float activity = 0.0f;
};

// ---------------------------------------------------------------------------------------
/** Segmented switch (TOPOLOGY, VOICE MODE, QUALITY). Silver or dark-glass variant. */
class SegmentedControl : public juce::Component, public juce::SettableTooltipClient
{
public:
    SegmentedControl (juce::StringArray options, bool onGlass);
    void attach (juce::RangedAudioParameter& p);
    void setIndex (int i, bool notify);
    int getIndex() const noexcept { return index; }
    std::function<void (int)> onChange;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    int segmentAt (juce::Point<int> p) const;
    juce::StringArray options;
    bool glass;
    int index = 0, hover = -1;
    std::unique_ptr<IndexAttachment> attachment;
};

// ---------------------------------------------------------------------------------------
/** Small pill toggle (QUANTIZE, MPE, LOOP ...). */
class ChipToggle : public juce::Button
{
public:
    ChipToggle (const juce::String& label, bool onGlass);
    void attach (juce::RangedAudioParameter& p);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    bool glass;
    std::unique_ptr<IndexAttachment> attachment;
};

// ---------------------------------------------------------------------------------------
/** Borderless icon button (chevrons, heart, gear, record ...). */
class IconButton : public juce::Button
{
public:
    IconButton (const juce::String& name, gfx::Icon icon, juce::Colour normal, juce::Colour hover);
    void setIcon (gfx::Icon i) { icon = i; repaint(); }
    void setColours (juce::Colour n, juce::Colour h) { normal = n; hoverColour = h; repaint(); }
    void setStroke (float s) { stroke = s; }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    gfx::Icon icon;
    juce::Colour normal, hoverColour;
    float stroke = 1.4f;
};

/** Formats a unit parameter as a percentage. */
juce::String percent (double v);

} // namespace arc::ui

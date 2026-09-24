#pragma once

// ARC's vector icon set. Every icon is a thin-stroke line drawing in a unit box, scaled
// into the requested rectangle — crisp at any scale, one visual weight throughout.

#include <juce_gui_basics/juce_gui_basics.h>

namespace arc::gfx
{

enum class Icon
{
    strike,   // concentric rings
    pluck,    // string with two slashes
    bow,      // sustained wave
    air,      // spiral
    glass,    // faceted gem
    metal,    // hexagon in a ring
    wood,     // growth rings
    membrane, // stretched ring
    freeze,   // snowflake
    random,   // die
    sync,     // circular arrows
    motion,   // orbit (triangle mark with an orbit)
    heart,
    heartFilled,
    chevronLeft,
    chevronRight,
    gear,
    record,
    close,
    save,
    trash,
    search
};

void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> area, juce::Colour colour, float strokeWidth = 1.4f);

/** The ARC logotype: A (as an open lambda), R, C in a single thin stroke weight. */
void drawLogo (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, float strokeWidth);

/** The CORE emblem: three spokes at 120 degrees (the network, abstracted). */
void drawCoreEmblem (juce::Graphics& g, juce::Point<float> centre, float radius, juce::Colour colour, float strokeWidth,
                     float rotation = 0.0f);

} // namespace arc::gfx

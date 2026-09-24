#pragma once

// Machined-silver rendering: satin chassis, raised plates, recessed trays, the chrome
// bezel of the resonance chamber, engraved grooves. Restrained on purpose — gradients,
// thin specular lines, soft shadows; no photographic scratches. All of it is static
// artwork, drawn once into the editor's cached layer.

#include <juce_gui_basics/juce_gui_basics.h>

namespace arc::gfx
{

/** Superellipse ("squircle") outline — the shape of the resonance chamber. */
juce::Path superellipse (juce::Rectangle<float> r, float exponent, int segments = 160);

/** Fine horizontal brushing, tiled over silver surfaces at low alpha. */
const juce::Image& brushedTexture();

/** Full chassis: satin gradient, brushing, soft top light, outer frame bevel. */
void paintChassis (juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius);

/** A raised machined plate (side panels, bottom strip) with drop shadow and bevel. */
void paintRaisedPlate (juce::Graphics& g, const juce::Path& shape, float shadowRadius = 14.0f);

/** A recessed tray (around displays): inner shadow and darker rim. */
void paintRecessedTray (juce::Graphics& g, const juce::Path& shape);

/** Chrome bezel between an outer and inner outline (the chamber ring). */
void paintChromeBezel (juce::Graphics& g, const juce::Path& outer, const juce::Path& inner);

/** Engraved groove: dark line with a light line below / right. */
void paintGroove (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b);

/** Soft elliptical drop shadow (under knobs and round buttons). */
void paintContactShadow (juce::Graphics& g, juce::Rectangle<float> r, float alpha);

} // namespace arc::gfx

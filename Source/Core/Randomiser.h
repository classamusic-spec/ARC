#pragma once

// RANDOM: musical randomisation. Never a uniform draw over every parameter:
//   mutate()     (click)       small Gaussian steps around the current sound, inside
//                              musical windows, keeping exciter / material / topology;
//   regenerate() (SHIFT-click) a new network drawn from weighted, material-aware
//                              distributions (harmonic or interval node tunings,
//                              moderate COUPLING / TENSION, mostly low CHAOS).
// Operates on plain parameter values; performance / setup parameters are untouched.

#include <map>

#include <juce_core/juce_core.h>

namespace arc::randomiser
{

using ValueMap = std::map<juce::String, float>;

void mutate (ValueMap& values, juce::Random& rng);
void regenerate (ValueMap& values, juce::Random& rng);

} // namespace arc::randomiser

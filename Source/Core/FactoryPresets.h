#pragma once

// The factory preset library: plain data, built once on the message thread.
// A preset lists only the parameters it changes (plain, denormalised values); every
// other sound parameter takes its default. See docs/PRESETS in docs/PARAMETERS.md.

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace arc::presets
{

struct PresetDefinition
{
    std::string name;
    std::string category;
    std::string tags;        // comma separated
    std::string description; // one sentence, shown in the preset browser
    uint32_t seed = 0;       // CHAOS / MOTION random-walk seed (deterministic recall)
    std::vector<std::pair<std::string, float>> values; // parameter id -> plain value
    std::array<std::string, 4> gestures;               // serialised node gestures ("" = none)
    int foldedTunings = 0;   // design diagnostic: chord tones moved by octaves to fit a node's reach
    std::string tuningNotes; // ... and where they went ("B 3 -> 1.5")
};

/** Category display order. */
const std::vector<std::string>& categories();

/** All factory presets, grouped by category in display order. */
const std::vector<PresetDefinition>& factoryPresets();

} // namespace arc::presets

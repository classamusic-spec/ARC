#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

// Phase 1 placeholder editor. The ARC silver UI is built in Phase 11,
// after the resonant engine is proven.
class ArcAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ArcAudioProcessorEditor (ArcAudioProcessor&);
    void paint (juce::Graphics&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArcAudioProcessorEditor)
};

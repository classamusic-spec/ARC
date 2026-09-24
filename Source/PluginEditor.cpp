#include "PluginEditor.h"

ArcAudioProcessorEditor::ArcAudioProcessorEditor (ArcAudioProcessor& p)
    : AudioProcessorEditor (p)
{
    setSize (600, 400);
}

void ArcAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffd9dde2));
    g.setColour (juce::Colour (0xff1a2129));
    g.setFont (28.0f);
    g.drawText ("ARC", getLocalBounds().removeFromTop (200), juce::Justification::centred);
    g.setFont (14.0f);
    g.drawText ("RESONANT NETWORK SYNTHESIZER - ENGINE BOOTSTRAP", getLocalBounds(), juce::Justification::centred);
}

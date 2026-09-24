#include "ArcTest.h"

#include "PluginProcessor.h"

TEST_CASE ("smoke", "processor renders MIDI note")
{
    ArcAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 10);

    double energy = 0.0;
    for (int block = 0; block < 20; ++block)
    {
        proc.processBlock (buffer, midi);
        midi.clear();
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                energy += buffer.getSample (ch, i) * buffer.getSample (ch, i);
    }

    MEASURE ("energy", energy);
    CHECK (energy > 1.0e-3);
    CHECK (std::isfinite (energy));
}

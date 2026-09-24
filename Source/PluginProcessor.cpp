#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "arc.master.output.v1", 1 }, "Master Output",
        juce::NormalisableRange<float> (-60.0f, 6.0f, 0.01f), -6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    return layout;
}
} // namespace

ArcAudioProcessor::ArcAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ARC", createLayout())
{
    masterParam = apvts.getRawParameterValue ("arc.master.output.v1");
}

void ArcAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

void ArcAudioProcessor::releaseResources() {}

bool ArcAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ArcAudioProcessor::handleMidiMessage (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.reset();
}

void ArcAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    engine.setMasterGain (juce::Decibels::decibelsToGain (masterParam->load(), -60.0f));

    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // Scratch for mono layouts: render right into left's tail-free twin.
    float monoScratch[512];

    int position = 0;
    auto renderTo = [&] (int end)
    {
        while (position < end)
        {
            const int chunk = right != nullptr ? end - position : juce::jmin (end - position, 512);
            if (right != nullptr)
                engine.render (left + position, right + position, chunk);
            else
            {
                juce::FloatVectorOperations::clear (monoScratch, chunk);
                engine.render (left + position, monoScratch, chunk);
                juce::FloatVectorOperations::add (left + position, monoScratch, chunk);
                juce::FloatVectorOperations::multiply (left + position, 0.5f, chunk);
            }
            position += chunk;
        }
    };

    for (const auto metadata : midi)
    {
        renderTo (juce::jlimit (0, numSamples, metadata.samplePosition));
        handleMidiMessage (metadata.getMessage());
    }
    renderTo (numSamples);
}

juce::AudioProcessorEditor* ArcAudioProcessor::createEditor()
{
    return new ArcAudioProcessorEditor (*this);
}

void ArcAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ArcAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArcAudioProcessor();
}

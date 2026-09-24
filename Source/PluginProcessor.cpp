#include "PluginProcessor.h"
#include "PluginEditor.h"

ArcAudioProcessor::ArcAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ARC", arc::params::createLayout()),
      paramCache (apvts)
{
    paramCache.fill (engineParams);
    engine.setParameters (engineParams);
}

void ArcAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    paramCache.fill (engineParams);
    engine.setParameters (engineParams);
    engine.prepare (sampleRate, samplesPerBlock);
    monoScratch.assign (static_cast<size_t> (std::max (samplesPerBlock, 32)), 0.0f);
}

void ArcAudioProcessor::releaseResources() {}

bool ArcAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ArcAudioProcessor::handleMidiMessage (const juce::MidiMessage& m) noexcept
{
    const int ch = m.getChannel();
    if (m.isNoteOn())
        engine.noteOn (ch, m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (ch, m.getNoteNumber());
    else if (m.isPitchWheel())
        engine.pitchBend (ch, static_cast<float> (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isChannelPressure())
        engine.channelPressure (ch, static_cast<float> (m.getChannelPressureValue()) / 127.0f);
    else if (m.isAftertouch())
        engine.polyPressure (ch, m.getNoteNumber(), static_cast<float> (m.getAfterTouchValue()) / 127.0f);
    else if (m.isController())
        engine.controller (ch, m.getControllerNumber(), static_cast<float> (m.getControllerValue()) / 127.0f);
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.allNotesOff (m.isAllSoundOff());
}

void ArcAudioProcessor::updateTransport() noexcept
{
    arc::TransportInfo t;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            t.valid = true;
            if (auto bpm = pos->getBpm())
                t.bpm = *bpm;
            if (auto ppq = pos->getPpqPosition())
                t.ppqPosition = *ppq;
            t.playing = pos->getIsPlaying();
            if (auto sig = pos->getTimeSignature())
                t.beatsPerBar = sig->numerator * 4.0 / std::max (1, sig->denominator);
        }
    engine.setTransport (t);
}

void ArcAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    paramCache.fill (engineParams);
    engine.setParameters (engineParams);
    updateTransport();

    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer (0);
    const bool stereo = buffer.getNumChannels() > 1;
    auto* right = stereo ? buffer.getWritePointer (1) : nullptr;
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    int position = 0;
    auto renderTo = [&] (int end)
    {
        while (position < end)
        {
            const int chunk = stereo ? end - position
                                     : juce::jmin (end - position, static_cast<int> (monoScratch.size()));
            if (stereo)
                engine.render (left + position, right + position, chunk);
            else
            {
                engine.render (left + position, monoScratch.data(), chunk);
                juce::FloatVectorOperations::add (left + position, monoScratch.data(), chunk);
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

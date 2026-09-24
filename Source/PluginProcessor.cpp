#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Core/FactoryPresets.h"

ArcAudioProcessor::ArcAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ARC", arc::params::createLayout()),
      paramCache (apvts)
{
    paramCache.fill (engineParams);
    engine.setParameters (engineParams);
    engine.postSeed (seed.load());

    presets = std::make_unique<arc::PresetManager> (*this);
    // Open on the signature sound (factory preset 0).
    presets->loadPreset (0);
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
    syncGesturesToEngine();

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

// -------------------------------------------------------------------------------------
// Gestures / seed
// -------------------------------------------------------------------------------------
void ArcAudioProcessor::setGesture (int node, const arc::Gesture& g)
{
    if (node < 0 || node > 3)
        return;
    {
        const juce::SpinLock::ScopedLockType lock (gestureLock);
        gestureStore[static_cast<size_t> (node)] = g;
    }
    gestureSerial[static_cast<size_t> (node)].fetch_add (1, std::memory_order_release);
}

arc::Gesture ArcAudioProcessor::getGesture (int node) const
{
    if (node < 0 || node > 3)
        return {};
    const juce::SpinLock::ScopedLockType lock (gestureLock);
    return gestureStore[static_cast<size_t> (node)];
}

void ArcAudioProcessor::syncGesturesToEngine() noexcept
{
    for (size_t n = 0; n < 4; ++n)
    {
        const auto serial = gestureSerial[n].load (std::memory_order_acquire);
        if (serial == appliedGestureSerial[n])
            continue;
        const juce::SpinLock::ScopedTryLockType lock (gestureLock);
        if (! lock.isLocked())
            return; // being written right now: pick it up next block
        engine.setGestureNow (static_cast<int> (n), gestureStore[n]);
        appliedGestureSerial[n] = serial;
    }
}

void ArcAudioProcessor::setSeed (uint32_t newSeed)
{
    seed.store (newSeed, std::memory_order_relaxed);
    engine.postSeed (newSeed);
}

// -------------------------------------------------------------------------------------
// Programs (factory presets)
// -------------------------------------------------------------------------------------
int ArcAudioProcessor::getNumPrograms()
{
    return static_cast<int> (arc::presets::factoryPresets().size());
}

int ArcAudioProcessor::getCurrentProgram()
{
    const int i = presets != nullptr ? presets->getCurrentIndex() : 0;
    return i >= 0 && i < getNumPrograms() ? i : 0;
}

void ArcAudioProcessor::setCurrentProgram (int index)
{
    if (presets != nullptr && index >= 0 && index < getNumPrograms() && index != presets->getCurrentIndex())
        presets->loadPreset (index);
}

const juce::String ArcAudioProcessor::getProgramName (int index)
{
    const auto& list = arc::presets::factoryPresets();
    return index >= 0 && index < static_cast<int> (list.size()) ? juce::String (list[static_cast<size_t> (index)].name)
                                                                 : juce::String();
}

// -------------------------------------------------------------------------------------
// State: parameters + gestures + seed + preset metadata + version
// -------------------------------------------------------------------------------------
void ArcAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("ARC_STATE");
    state.setProperty ("version", stateVersion, nullptr);
    state.setProperty ("pluginVersion", ARC_VERSION_STRING, nullptr);
    state.setProperty ("seed", juce::String (static_cast<juce::int64> (getSeed())), nullptr);
    state.appendChild (apvts.copyState(), nullptr);

    juce::ValueTree gestures ("GESTURES");
    for (int n = 0; n < 4; ++n)
    {
        const auto g = getGesture (n);
        if (g.valid)
        {
            juce::ValueTree gt ("GESTURE");
            gt.setProperty ("node", n, nullptr);
            gt.setProperty ("data", juce::String (g.serialise()), nullptr);
            gestures.appendChild (gt, nullptr);
        }
    }
    state.appendChild (gestures, nullptr);

    if (presets != nullptr)
        state.appendChild (presets->getState(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ArcAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;
    const auto tree = juce::ValueTree::fromXml (*xml);

    if (tree.hasType (apvts.state.getType()))
    {
        apvts.replaceState (tree); // pre-release state: parameters only
        return;
    }
    if (! tree.hasType ("ARC_STATE"))
        return;

    const auto parameters = tree.getChildWithName (apvts.state.getType());
    if (parameters.isValid())
        apvts.replaceState (parameters);

    std::array<arc::Gesture, 4> restored {};
    const auto gestures = tree.getChildWithName ("GESTURES");
    for (const auto& gt : gestures)
    {
        const int node = gt.getProperty ("node", -1);
        if (node >= 0 && node < 4)
            restored[static_cast<size_t> (node)] = arc::Gesture::deserialise (gt.getProperty ("data").toString().toStdString());
    }
    for (int n = 0; n < 4; ++n)
        setGesture (n, restored[static_cast<size_t> (n)]);

    setSeed (static_cast<uint32_t> (tree.getProperty ("seed", "0").toString().getLargeIntValue()));

    // Preset metadata belongs to the message thread; hosts may restore from elsewhere.
    const auto presetState = tree.getChildWithName ("PRESET").createCopy();
    if (juce::MessageManager::existsAndIsCurrentThread() || juce::MessageManager::getInstanceWithoutCreating() == nullptr)
        presets->restoreState (presetState);
    else
        juce::MessageManager::callAsync ([weak = juce::WeakReference<ArcAudioProcessor> (this), presetState]
                                         {
                                             if (weak != nullptr)
                                                 weak->presets->restoreState (presetState);
                                         });
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ArcAudioProcessor();
}

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/Parameters.h"
#include "Engine/ArcEngine.h"

class ArcAudioProcessor final : public juce::AudioProcessor
{
public:
    ArcAudioProcessor();
    ~ArcAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ARC"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }
    arc::ArcEngine& getEngine() noexcept { return engine; }
    arc::Telemetry& getTelemetry() noexcept { return engine.getTelemetry(); }

private:
    void handleMidiMessage (const juce::MidiMessage& m) noexcept;
    void updateTransport() noexcept;

    juce::AudioProcessorValueTreeState apvts;
    arc::params::ParameterCache paramCache;
    arc::EngineParams engineParams;
    arc::ArcEngine engine;
    std::vector<float> monoScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArcAudioProcessor)
};

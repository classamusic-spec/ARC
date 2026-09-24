#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Core/Parameters.h"
#include "Core/PresetManager.h"
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

    // Host programs = factory presets (message thread).
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }
    arc::ArcEngine& getEngine() noexcept { return engine; }
    arc::Telemetry& getTelemetry() noexcept { return engine.getTelemetry(); }
    arc::PresetManager& getPresetManager() noexcept { return *presets; }

    // --- node gestures and CHAOS / MOTION seed (any non-audio thread) -----------------
    /** Replaces node n's gesture (invalid gesture = clear); reaches the engine at the
        start of the next block. */
    void setGesture (int node, const arc::Gesture& g);
    arc::Gesture getGesture (int node) const;
    void setSeed (uint32_t seed);
    uint32_t getSeed() const noexcept { return seed.load (std::memory_order_relaxed); }

    static constexpr int stateVersion = 1;

    /** Last editor width used with this instance (restored with the DAW state). */
    int getEditorWidth() const noexcept { return editorWidth.load (std::memory_order_relaxed); }
    void setEditorWidth (int w) noexcept { editorWidth.store (w, std::memory_order_relaxed); }

private:
    void handleMidiMessage (const juce::MidiMessage& m) noexcept;
    void updateTransport() noexcept;
    void syncGesturesToEngine() noexcept;

    juce::AudioProcessorValueTreeState apvts;
    arc::params::ParameterCache paramCache;
    arc::EngineParams engineParams;
    arc::ArcEngine engine;
    std::vector<float> monoScratch;

    // Gestures: message-side store, copied to the engine by the audio thread with a
    // try-lock (never blocks) when a node's serial changes.
    std::array<arc::Gesture, 4> gestureStore {};
    mutable juce::SpinLock gestureLock;
    std::array<std::atomic<uint32_t>, 4> gestureSerial {};
    std::array<uint32_t, 4> appliedGestureSerial {};
    std::atomic<uint32_t> seed { 0xA2C1u };
    std::atomic<int> editorWidth { 1080 };

    std::unique_ptr<arc::PresetManager> presets;

    JUCE_DECLARE_WEAK_REFERENCEABLE (ArcAudioProcessor)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArcAudioProcessor)
};

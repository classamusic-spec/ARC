// Phase 13-14 — realtime safety and threading.
//
//  * allocation detector: global operator new / delete are hooked for this test binary;
//    while a thread-local flag is set (the audio callback), any allocation is counted.
//  * threading stress: an audio thread runs processBlock continuously while the message
//    thread loads presets, randomises, records gestures, saves / restores state and reads
//    telemetry — the ThreadSanitizer build turns any data race into a failure.

#include "ArcTest.h"

#include <atomic>
#include <cstdlib>
#include <new>
#include <thread>

#include "Core/Parameters.h"
#include "PluginProcessor.h"
#include "UI/FieldModel.h"

namespace
{
thread_local bool inAudioCallback = false;
std::atomic<long> audioAllocations { 0 };
} // namespace

#if ! defined(__SANITIZE_ADDRESS__) && ! defined(__SANITIZE_THREAD__)
// Sanitizers install their own allocators; the detector runs in the normal build.
void* operator new (std::size_t n)
{
    if (inAudioCallback)
        audioAllocations.fetch_add (1, std::memory_order_relaxed);
    if (void* p = std::malloc (n == 0 ? 1 : n))
        return p;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t n)
{
    if (inAudioCallback)
        audioAllocations.fetch_add (1, std::memory_order_relaxed);
    if (void* p = std::malloc (n == 0 ? 1 : n))
        return p;
    throw std::bad_alloc();
}
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }
    #define ARC_ALLOCATION_DETECTOR 1
#endif

TEST_CASE ("realtime", "processBlock never allocates")
{
#if ARC_ALLOCATION_DETECTOR
    ArcAudioProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    midi.ensureSize (4096);
    auto& pm = proc.getPresetManager();
    long worst = 0;
    int blocks = 0;
    for (int preset = 0; preset < pm.getNumPresets(); preset += 3)
    {
        pm.loadPreset (preset); // message-thread work between blocks
        proc.setGesture (preset % 4, arc::orbitGesture (2.0f, 4.0f, 1.0f, 0.01f));
        for (int b = 0; b < 40; ++b, ++blocks)
        {
            midi.clear();
            if (b % 10 == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 40 + (b + preset) % 36, 0.8f), b);
            if (b % 10 == 7)
                midi.addEvent (juce::MidiMessage::noteOff (1, 40 + (b - 7 + preset) % 36), 3);
            midi.addEvent (juce::MidiMessage::pitchWheel (1, 8192 + (b % 7) * 300), 100);
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, b % 20 < 10 ? 127 : 0), 200);
            midi.addEvent (juce::MidiMessage::channelPressureChange (1, b % 128), 300);
            if (b == 20)
            {
                // Automation of discrete / structural parameters between blocks.
                for (const char* id : { arc::params::topology, arc::params::exciterType, arc::params::materialType, arc::params::quality,
                                        arc::params::voiceMode, arc::params::freeze, arc::params::sync })
                {
                    auto* p = proc.getValueTreeState().getParameter (id);
                    p->setValueNotifyingHost (std::fmod (p->getValue() + 0.37f, 1.0f));
                }
            }
            buffer.clear();
            const long before = audioAllocations.load();
            inAudioCallback = true;
            proc.processBlock (buffer, midi);
            inAudioCallback = false;
            worst = std::max (worst, audioAllocations.load() - before);
        }
    }
    MEASURE ("blocksChecked", blocks);
    MEASURE ("allocationsInProcessBlock", (double) audioAllocations.load());
    MEASURE ("worstBlock", (double) worst);
    CHECK (audioAllocations.load() == 0);
#else
    MEASURE ("skippedUnderSanitizer", 1);
#endif
}

TEST_CASE ("realtime", "audio and message threads run concurrently without races")
{
    ArcAudioProcessor proc;
    proc.prepareToPlay (48000.0, 256);
    std::atomic<bool> stop { false };
    std::atomic<int> audioBlocks { 0 };
    std::thread audio ([&]
                       {
                           juce::AudioBuffer<float> buffer (2, 256);
                           juce::MidiBuffer midi;
                           int b = 0;
                           while (! stop.load())
                           {
                               midi.clear();
                               if (b % 20 == 0)
                                   midi.addEvent (juce::MidiMessage::noteOn (1, 48 + (b / 20) % 24, 0.8f), 0);
                               if (b % 20 == 12)
                                   midi.addEvent (juce::MidiMessage::noteOff (1, 48 + (b / 20) % 24), 0);
                               buffer.clear();
                               proc.processBlock (buffer, midi);
                               ++b;
                               audioBlocks.store (b);
                           }
                       });

    arc::ui::FieldModel model;
    auto& pm = proc.getPresetManager();
    juce::Random rng (42);
    const auto end = juce::Time::getMillisecondCounterHiRes() + 2500.0;
    int iterations = 0;
    while (juce::Time::getMillisecondCounterHiRes() < end)
    {
        switch (iterations % 6)
        {
            case 0: pm.loadPreset (rng.nextInt (pm.getNumPresets())); break;
            case 1: pm.randomise (rng.nextBool(), rng); break;
            case 2: proc.setGesture (rng.nextInt (4), arc::swayGesture (1.0f + rng.nextFloat(), 0.0f, 0.8f, 0.02f)); break;
            case 3:
            {
                juce::MemoryBlock state;
                proc.getStateInformation (state);
                proc.setStateInformation (state.getData(), (int) state.getSize());
                break;
            }
            case 4:
                proc.getValueTreeState().getParameter (arc::params::coupling)->setValueNotifyingHost (rng.nextFloat());
                break;
            default: model.update (proc.getTelemetry(), 1.0f / 60.0f, false); break;
        }
        ++iterations;
        juce::Thread::sleep (1);
    }
    stop.store (true);
    audio.join();
    MEASURE ("messageThreadOperations", iterations);
    MEASURE ("audioBlocks", audioBlocks.load());
    CHECK (audioBlocks.load() > 100);
    CHECK (proc.getEngine().nonFiniteVoiceResets == 0);
}

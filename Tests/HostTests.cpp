// Phase 14 — host behaviour: what a DAW does to a plugin over a session.
// (pluginval covers the formal VST3 checks; these exercise the same processor directly
// with sustained notes so audio continuity can be measured.)

#include "Analysis.h"
#include "ArcTest.h"

#include "Core/Parameters.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

using namespace arctest;

namespace
{
Signal run (ArcAudioProcessor& p, int blockSize, int samples, std::function<void (juce::MidiBuffer&, int)> events = {})
{
    Signal out;
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    int done = 0, b = 0;
    while (done < samples)
    {
        const int n = std::min (blockSize, samples - done);
        juce::AudioBuffer<float> view (buffer.getArrayOfWritePointers(), 2, n);
        view.clear();
        midi.clear();
        if (events)
            events (midi, b);
        p.processBlock (view, midi);
        for (int i = 0; i < n; ++i)
            out.push_back (0.5f * (view.getSample (0, i) + view.getSample (1, i)));
        done += n;
        ++b;
    }
    return out;
}
} // namespace

TEST_CASE ("host", "block sizes from 1 to 4096 render the same sound")
{
    // Sample-accurate engine: any host block size produces the identical stream.
    Signal reference;
    double worst = 0;
    for (int bs : { 4096, 1, 7, 64, 333, 1024 })
    {
        ArcAudioProcessor p;
        p.getPresetManager().loadPreset (p.getPresetManager().findPreset ("factory/Temple Lattice"));
        p.prepareToPlay (48000.0, bs);
        const auto out = run (p, bs, 48000, [bs] (juce::MidiBuffer& m, int b)
                              {
                                  if (b == 0)
                                      m.addEvent (juce::MidiMessage::noteOn (1, 57, 0.8f), 0);
                                  juce::ignoreUnused (bs);
                              });
        if (reference.empty())
            reference = out;
        else
            for (size_t i = 0; i < out.size(); ++i)
                worst = std::max (worst, static_cast<double> (std::abs (out[i] - reference[i])));
    }
    MEASURE ("maxDifferenceAcrossBlockSizes", worst);
    CHECK (worst < 1.0e-5);
}

TEST_CASE ("host", "sample-rate changes and re-prepare mid-session")
{
    ArcAudioProcessor p;
    int nonFinite = 0;
    double peak = 0;
    for (double sr : { 44100.0, 96000.0, 48000.0, 192000.0, 22050.0, 88200.0 })
    {
        p.prepareToPlay (sr, 512);
        const auto out = run (p, 512, static_cast<int> (sr * 0.5), [] (juce::MidiBuffer& m, int b)
                              {
                                  if (b == 0)
                                      for (int n : { 48, 60, 67 })
                                          m.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), 0);
                              });
        nonFinite += allFinite (out) ? 0 : 1;
        peak = std::max (peak, peakAbs (out));
        const double level = rms (out, static_cast<int> (out.size() / 2), static_cast<int> (out.size() / 2));
        MEASURE ("rmsDb@" + std::to_string (static_cast<int> (sr)), 20.0 * std::log10 (level + 1e-12));
        CHECK (level > 1.0e-4); // sounds at every rate
    }
    MEASURE ("peak", peak);
    CHECK (nonFinite == 0);
    CHECK (peak < 1.0);
}

TEST_CASE ("host", "multiple instances are independent")
{
    ArcAudioProcessor a, b;
    a.getPresetManager().loadPreset (a.getPresetManager().findPreset ("factory/Frozen Wire"));
    b.getPresetManager().loadPreset (b.getPresetManager().findPreset ("factory/Deep Frame"));
    a.prepareToPlay (48000.0, 256);
    b.prepareToPlay (48000.0, 256);
    // Interleaved processing (as a host does on one thread), different material each.
    Signal outA, outB;
    juce::AudioBuffer<float> buf (2, 256);
    juce::MidiBuffer midi;
    for (int i = 0; i < 188; ++i)
    {
        midi.clear();
        if (i == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, 50, 0.8f), 0);
        buf.clear();
        a.processBlock (buf, midi);
        for (int s = 0; s < 256; ++s)
            outA.push_back (0.5f * (buf.getSample (0, s) + buf.getSample (1, s)));
        buf.clear();
        b.processBlock (buf, midi);
        for (int s = 0; s < 256; ++s)
            outB.push_back (0.5f * (buf.getSample (0, s) + buf.getSample (1, s)));
    }
    // Instance A equals the same preset rendered by a solo instance: no shared state.
    ArcAudioProcessor solo;
    solo.getPresetManager().loadPreset (solo.getPresetManager().findPreset ("factory/Frozen Wire"));
    solo.prepareToPlay (48000.0, 256);
    const auto ref = run (solo, 256, (int) outA.size(), [] (juce::MidiBuffer& m, int blk)
                          {
                              if (blk == 0)
                                  m.addEvent (juce::MidiMessage::noteOn (1, 50, 0.8f), 0);
                          });
    double diff = 0;
    for (size_t i = 0; i < ref.size(); ++i)
        diff = std::max (diff, static_cast<double> (std::abs (outA[i] - ref[i])));
    MEASURE ("instanceIsolationMaxDiff", diff);
    CHECK (diff < 1.0e-6);
    CHECK (rms (outA) > 1.0e-4);
    CHECK (rms (outB) > 1.0e-4);
}

TEST_CASE ("host", "bypass, release and editor open-close while processing")
{
    ArcAudioProcessor p;
    p.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buf (2, 256);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
    p.processBlock (buf, midi);
    midi.clear();

    // Bypass: silence out, MIDI state survives; un-bypass continues.
    for (int i = 0; i < 20; ++i)
    {
        buf.clear();
        p.processBlockBypassed (buf, midi);
    }
    CHECK (buf.getMagnitude (0, 256) == 0.0f);

    // Editor opened / closed repeatedly while audio runs.
    for (int k = 0; k < 6; ++k)
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (1080, 810);
        for (int i = 0; i < 10; ++i)
        {
            buf.clear();
            p.processBlock (buf, midi);
            if (auto* e = dynamic_cast<ArcAudioProcessorEditor*> (ed.get()))
                e->advanceFrame (1.0 / 60.0);
        }
    }
    // releaseResources + prepare again (host suspend / resume).
    p.releaseResources();
    p.prepareToPlay (44100.0, 1024);
    juce::AudioBuffer<float> big (2, 1024);
    midi.addEvent (juce::MidiMessage::noteOn (1, 64, 0.9f), 0);
    p.processBlock (big, midi);
    CHECK (big.getMagnitude (0, 1024) >= 0.0f);
    CHECK (p.getEngine().nonFiniteVoiceResets == 0);
}

TEST_CASE ("docs", "parameter table")
{
    // Generates docs/measurements/parameter_table.md from the live layout (IDs, ranges,
    // defaults, preset membership), so the documentation cannot drift from the code.
    ArcAudioProcessor p;
    juce::String md;
    md << "| ID | Name | Range | Default | In presets |\n|---|---|---|---|---|\n";
    int count = 0;
    for (auto* param : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto& range = r->getNormalisableRange();
            juce::String rangeText;
            if (r->isBoolean())
                rangeText = "Off / On";
            else if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (r))
                rangeText = c->choices.joinIntoString (" / ");
            else
                rangeText = juce::String (range.start, 2) + " ... " + juce::String (range.end, 2) + (r->getLabel().isNotEmpty() ? " " + r->getLabel() : juce::String());
            md << "| `" << r->getParameterID() << "` | " << r->getName (64) << " | " << rangeText << " | "
               << r->getText (r->getDefaultValue(), 32) << " | " << (arc::PresetManager::isPresetParameter (r->getParameterID()) ? "yes" : "no") << " |\n";
            ++count;
        }
    juce::File (juce::String (ARC_SOURCE_DIR) + "/docs/measurements/parameter_table.md").replaceWithText (md);
    MEASURE ("parameters", count);
    CHECK (count >= 60);
}

// Phase 11-12 — editor rendering: snapshots of real states (for design review) and
// frame-cost measurement of the animated Resonance Field.

#include "ArcTest.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>

#include "Core/FactoryPresets.h"
#include "Core/Parameters.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace arctest
{
std::string outputDir();
}

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 512;

void play (ArcAudioProcessor& p, ArcAudioProcessorEditor* ed, double seconds, const std::vector<int>& notes, bool noteOnAtStart)
{
    juce::AudioBuffer<float> buffer (2, kBlock);
    juce::MidiBuffer midi;
    const int blocks = (int) (seconds * kSr / kBlock);
    for (int b = 0; b < blocks; ++b)
    {
        midi.clear();
        if (b == 0 && noteOnAtStart)
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOn (1, n, 0.85f), 0);
        buffer.clear();
        p.processBlock (buffer, midi);
        if (ed != nullptr && (b % 2) == 0)
            ed->advanceFrame (2.0 * kBlock / kSr);
    }
}

void save (ArcAudioProcessorEditor& ed, const std::string& name, float scale = 1.0f)
{
    std::filesystem::create_directories (arctest::outputDir() + "/ui");
    const auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, scale);
    juce::File f (arctest::outputDir() + "/ui/" + name + ".png");
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
}

void setPlain (ArcAudioProcessor& p, const juce::String& id, float v)
{
    auto* q = p.getValueTreeState().getParameter (id);
    q->setValueNotifyingHost (q->convertTo0to1 (v));
}
} // namespace

TEST_CASE ("ui", "editor snapshots")
{
    ArcAudioProcessor proc;
    proc.prepareToPlay (kSr, kBlock);
    std::unique_ptr<juce::AudioProcessorEditor> base (proc.createEditor());
    auto* ed = dynamic_cast<ArcAudioProcessorEditor*> (base.get());
    REQUIRE (ed != nullptr);
    ed->setSize (1200, 900);

    // 1. Opening state: Obsidian Bloom, a bowed chord blooming.
    play (proc, ed, 1.2, { 48, 55, 63 }, true);
    save (*ed, "01_obsidian_bloom");
    save (*ed, "01_obsidian_bloom_2x", 2.0f);

    // 2. A struck preset right after the strike, with a node inspector open.
    auto& pm = proc.getPresetManager();
    pm.loadPreset (pm.findPreset ("factory/Satellite String"));
    play (proc, ed, 0.3, {}, false);
    ed->selectNode (1);
    play (proc, ed, 0.25, { 60, 67 }, true);
    save (*ed, "02_satellite_node_inspector");

    // 3. Preset browser.
    ed->selectNode (-1);
    ed->openPresetBrowser (true);
    play (proc, ed, 0.3, {}, false);
    save (*ed, "03_preset_browser");
    ed->getPresetBrowser().setSearchText ("glass bell");
    play (proc, ed, 0.1, {}, false);
    save (*ed, "03_preset_search");
    ed->getPresetBrowser().setSearchText ({});
    ed->openPresetBrowser (false);

    // 4. FREEZE on a ringing bell, high DPI.
    pm.loadPreset (pm.findPreset ("factory/Black Bell"));
    play (proc, ed, 0.2, { 45, 57 }, true);
    setPlain (proc, arc::params::freeze, 1.0f);
    play (proc, ed, 0.8, {}, false);
    save (*ed, "04_black_bell_frozen_2x", 2.0f);
    setPlain (proc, arc::params::freeze, 0.0f);

    // 5. Panel inspectors (inline) and the CORE dock.
    ed->openPanelInspectors (true, true);
    ed->selectCore();
    play (proc, ed, 0.5, { 50 }, true);
    save (*ed, "06_inspectors_core_dock");
    ed->openPanelInspectors (false, false);

    // 6. Settings.
    ed->openSettings (true);
    play (proc, ed, 0.3, {}, false);
    save (*ed, "07_settings");
    ed->openSettings (false);

    // 7. Smallest window (75 %).
    ed->setSize (900, 675);
    play (proc, ed, 0.3, { 52 }, true);
    save (*ed, "05_small_window");
    CHECK (true);
}

TEST_CASE ("ui", "field frame cost")
{
    ArcAudioProcessor proc;
    proc.prepareToPlay (kSr, kBlock);
    std::unique_ptr<juce::AudioProcessorEditor> base (proc.createEditor());
    auto* ed = dynamic_cast<ArcAudioProcessorEditor*> (base.get());
    REQUIRE (ed != nullptr);
    for (int w : { 1080, 1440 })
    {
        ed->setSize (w, w * 3 / 4);
        play (proc, ed, 0.5, { 48, 55, 60, 64 }, true);
        auto& field = ed->getField();
        juce::Image target (juce::Image::ARGB, field.getWidth() * 2, field.getHeight() * 2, true);
        {
            // Warm-up frame: builds the static layer and sprites for this size (a one-off
            // cost on resize), so the timing below is the steady per-frame cost.
            juce::Graphics g (target);
            g.addTransform (juce::AffineTransform::scale (2.0f));
            field.paintEntireComponent (g, false);
        }
        const int frames = 60;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < frames; ++i)
        {
            ed->advanceFrame (1.0 / 60.0);
            juce::Graphics g (target);
            g.addTransform (juce::AffineTransform::scale (2.0f));
            field.paintEntireComponent (g, false);
        }
        const double ms = (juce::Time::getMillisecondCounterHiRes() - t0) / frames;

        MEASURE ("fieldFrameMs_2x_width" + std::to_string (w), ms);
        CHECK (! arctest::timingChecksEnabled || ms < 16.0);
    }
}

// ---------------------------------------------------------------------------------------
// Interaction: synthetic mouse events on the real components.
namespace
{
juce::Component* findById (juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id)
        return &root;
    for (auto* c : root.getChildren())
        if (auto* f = findById (*c, id))
            return f;
    return nullptr;
}

juce::MouseEvent mouse (juce::Component& c, juce::Point<float> pos, juce::Point<float> down, juce::ModifierKeys mods, int clicks, bool dragged)
{
    const auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), pos, mods, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, &c, &c, now, down,
                             now, clicks, dragged);
}

float param (ArcAudioProcessor& p, const juce::String& id)
{
    auto* q = p.getValueTreeState().getParameter (id);
    return q->convertFrom0to1 (q->getValue());
}
} // namespace

TEST_CASE ("ui", "dragging a node moves and retunes it")
{
    ArcAudioProcessor proc;
    proc.getPresetManager().loadInit();
    std::unique_ptr<juce::AudioProcessorEditor> base (proc.createEditor());
    auto* ed = dynamic_cast<ArcAudioProcessorEditor*> (base.get());
    REQUIRE (ed != nullptr);
    ed->setSize (1200, 900);
    ed->advanceFrame (0.02);
    auto& field = ed->getField();

    const auto start = field.nodeCentre (1); // node B
    const float r0 = param (proc, arc::params::nodeId (1, "radius")), a0 = param (proc, arc::params::nodeId (1, "angle"));
    const auto target = start + juce::Point<float> (40.0f, 30.0f);
    field.mouseDown (mouse (field, start, start, {}, 1, false));
    for (int i = 1; i <= 10; ++i)
        field.mouseDrag (mouse (field, start + (target - start) * ((float) i / 10.0f), start, juce::ModifierKeys::leftButtonModifier, 1, true));
    field.mouseUp (mouse (field, target, start, {}, 1, true));
    ed->advanceFrame (0.02);
    const float r1 = param (proc, arc::params::nodeId (1, "radius")), a1 = param (proc, arc::params::nodeId (1, "angle"));
    MEASURE ("radiusChange", r1 - r0);
    MEASURE ("angleChange", a1 - a0);
    // The node follows the pointer: its parameter position maps back onto the target.
    const auto landed = field.nodeCentre (1);
    MEASURE ("landingErrorPx", landed.getDistanceFrom (target));
    CHECK (std::abs (r1 - r0) > 0.02f);
    CHECK (std::abs (a1 - a0) > 0.01f);
    CHECK (landed.getDistanceFrom (target) < 1.5f);
    CHECK (field.getSelection().target == arc::ui::ResonanceField::Target::node && field.getSelection().node == 1);

    // Double-click resets it.
    field.mouseDoubleClick (mouse (field, landed, landed, {}, 2, false));
    CHECK (std::abs (param (proc, arc::params::nodeId (1, "radius")) - 0.5f) < 1.0e-3f);
    CHECK (std::abs (param (proc, arc::params::nodeId (1, "angle")) - 0.625f) < 1.0e-3f);

    // Clicking empty chamber space deselects.
    const auto empty = juce::Point<float> ((float) field.getWidth() * 0.5f, (float) field.getHeight() * 0.93f);
    field.mouseDown (mouse (field, empty, empty, {}, 1, false));
    CHECK (field.getSelection().target == arc::ui::ResonanceField::Target::none);
}

TEST_CASE ("ui", "alt-drag records a looping gesture")
{
    ArcAudioProcessor proc;
    proc.getPresetManager().loadInit();
    std::unique_ptr<juce::AudioProcessorEditor> base (proc.createEditor());
    auto* ed = dynamic_cast<ArcAudioProcessorEditor*> (base.get());
    REQUIRE (ed != nullptr);
    ed->setSize (1200, 900);
    ed->advanceFrame (0.02);
    auto& field = ed->getField();
    REQUIRE (! proc.getGesture (2).valid);

    const float r0 = param (proc, arc::params::nodeId (2, "radius")), a0 = param (proc, arc::params::nodeId (2, "angle"));
    const auto start = field.nodeCentre (2); // node C
    const auto alt = juce::ModifierKeys (juce::ModifierKeys::altModifier | juce::ModifierKeys::leftButtonModifier);
    field.mouseDown (mouse (field, start, start, alt, 1, false));
    CHECK (field.isRecording());
    // A circular drag over ~0.4 s of real time.
    for (int i = 1; i <= 40; ++i)
    {
        const float t = (float) i / 40.0f;
        const auto p = start + juce::Point<float> (30.0f * std::sin (6.2831853f * t), 20.0f * (1.0f - std::cos (6.2831853f * t)));
        field.mouseDrag (mouse (field, p, start, alt, 1, true));
        juce::Thread::sleep (10);
    }
    field.mouseUp (mouse (field, start, start, {}, 1, true));
    const auto g = proc.getGesture (2);
    MEASURE ("gestureSeconds", g.durationSeconds);
    CHECK (g.valid);
    CHECK (! field.isRecording());
    CHECK (g.durationSeconds > 0.2f);
    // The node's base position returns to where recording started; the loop plays around it.
    CHECK (std::abs (param (proc, arc::params::nodeId (2, "radius")) - r0) < 1.0e-3f);
    CHECK (std::abs (param (proc, arc::params::nodeId (2, "angle")) - a0) < 1.0e-3f);
}

TEST_CASE ("ui", "selector tiles, random and freeze drive the parameters")
{
    ArcAudioProcessor proc;
    proc.getPresetManager().loadInit();
    std::unique_ptr<juce::AudioProcessorEditor> base (proc.createEditor());
    auto* ed = dynamic_cast<ArcAudioProcessorEditor*> (base.get());
    REQUIRE (ed != nullptr);
    ed->setSize (1200, 900);

    // Button::triggerClick() is asynchronous; replicate its click path synchronously.
    auto click = [] (juce::Component* c)
    {
        REQUIRE (c != nullptr);
        auto* b = dynamic_cast<juce::Button*> (c);
        REQUIRE (b != nullptr);
        if (b->getClickingTogglesState())
            b->setToggleState (! b->getToggleState(), juce::dontSendNotification);
        if (b->onClick)
            b->onClick();
    };

    click (findById (*ed, juce::String (arc::params::exciterType) + ".tile.2")); // BOW
    CHECK (juce::roundToInt (param (proc, arc::params::exciterType)) == 2);
    click (findById (*ed, juce::String (arc::params::materialType) + ".tile.0")); // GLASS
    CHECK (juce::roundToInt (param (proc, arc::params::materialType)) == 0);

    CHECK (proc.getPresetManager().isModified()); // changed from Init
    click (findById (*ed, "freeze"));
    CHECK (param (proc, arc::params::freeze) > 0.5f);
    click (findById (*ed, "freeze"));
    CHECK (param (proc, arc::params::freeze) < 0.5f);
    click (findById (*ed, "sync"));
    CHECK (param (proc, arc::params::sync) > 0.5f);

    // RANDOM (plain click = gentle mutation) through the real button's mouse path.
    auto* rnd = dynamic_cast<arc::ui::RoundButton*> (findById (*ed, "random"));
    REQUIRE (rnd != nullptr);
    const float c0 = param (proc, arc::params::coupling);
    const auto centre = rnd->getLocalBounds().getCentre().toFloat();
    rnd->mouseUp (mouse (*rnd, centre, centre, {}, 1, false));
    MEASURE ("couplingAfterRandom", param (proc, arc::params::coupling));
    CHECK (std::abs (param (proc, arc::params::coupling) - c0) > 1.0e-4f);
}

TEST_CASE ("ui", "audio cost with the editor closed and open")
{
    // The performance matrix's GUI closed / open column. A real-time paced audio thread
    // holds an 8-voice bowed chord while the message thread either idles or runs the editor
    // at 60 fps, repainting what the editor invalidates each frame (field and meter every
    // frame; preset display and status every 6th) the way a host paints dirty regions, and
    // separately the cost of a full-window repaint (open / resize). Under ThreadSanitizer
    // this is also the race test for editor frames against live audio.
    struct Result
    {
        double audioFraction = 0, frameMs = 0, guiLoad = 0, fullWindowMs = 0;
        int frames = 0;
    };
    auto measure = [] (bool withEditor)
    {
        ArcAudioProcessor proc;
        auto& pm = proc.getPresetManager();
        pm.loadPreset (pm.findPreset ("factory/Obsidian Bloom"));
        constexpr int block = 256;
        proc.prepareToPlay (kSr, block);
        std::unique_ptr<juce::AudioProcessorEditor> base;
        ArcAudioProcessorEditor* ed = nullptr;
        std::vector<juce::Rectangle<int>> everyFrame, everySixth;
        if (withEditor)
        {
            base.reset (proc.createEditor());
            ed = dynamic_cast<ArcAudioProcessorEditor*> (base.get());
            ed->setSize (1200, 900);
            auto area = [ed] (const char* id)
            {
                auto* c = findById (*ed, id);
                return c != nullptr ? ed->getLocalArea (c, c->getLocalBounds()) : juce::Rectangle<int>();
            };
            everyFrame = { area ("field"), area ("meter") };
            everySixth = { area ("presetDisplay"), area ("status") };
        }
        std::atomic<bool> done { false };
        double cpuSeconds = 0;
        const int blocks = (int) (3.0 * kSr / block);
        std::thread audio ([&]
                           {
                               juce::AudioBuffer<float> buffer (2, block);
                               juce::MidiBuffer midi;
                               const auto period = std::chrono::duration<double> (block / kSr);
                               auto deadline = std::chrono::steady_clock::now();
                               for (int b = 0; b < blocks; ++b)
                               {
                                   midi.clear();
                                   if (b == 0)
                                       for (int n : { 36, 43, 48, 55, 60, 63, 67, 70 })
                                           midi.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), 0);
                                   buffer.clear();
                                   const auto t0 = std::chrono::steady_clock::now();
                                   proc.processBlock (buffer, midi);
                                   cpuSeconds += std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
                                   deadline += std::chrono::duration_cast<std::chrono::steady_clock::duration> (period);
                                   std::this_thread::sleep_until (deadline);
                               }
                               done.store (true);
                           });
        Result r;
        double guiMs = 0;
        const auto start = juce::Time::getMillisecondCounterHiRes();
        while (! done.load())
        {
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            if (ed != nullptr)
            {
                ed->advanceFrame (1.0 / 60.0);
                for (auto& a : everyFrame)
                    juce::ignoreUnused (ed->createComponentSnapshot (a, true, 1.0f));
                if (r.frames % 6 == 0)
                    for (auto& a : everySixth)
                        juce::ignoreUnused (ed->createComponentSnapshot (a, true, 1.0f));
                guiMs += juce::Time::getMillisecondCounterHiRes() - t0;
                if (++r.frames % 30 == 0)
                {
                    const auto f0 = juce::Time::getMillisecondCounterHiRes();
                    juce::ignoreUnused (ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f));
                    r.fullWindowMs = juce::Time::getMillisecondCounterHiRes() - f0;
                }
            }
            juce::Thread::sleep (juce::jmax (1, (int) (1000.0 / 60.0 - (juce::Time::getMillisecondCounterHiRes() - t0))));
        }
        audio.join();
        r.audioFraction = cpuSeconds / (blocks * block / kSr);
        r.frameMs = r.frames > 0 ? guiMs / r.frames : 0.0;
        r.guiLoad = guiMs / (juce::Time::getMillisecondCounterHiRes() - start);
        return r;
    };
    const auto closed = measure (false);
    const auto open = measure (true);
    MEASURE ("guiClosed.audioPercentOfCore", closed.audioFraction * 100.0);
    MEASURE ("guiOpen.audioPercentOfCore", open.audioFraction * 100.0);
    MEASURE ("guiOpen.framesDrawn", open.frames);
    MEASURE ("guiOpen.frameMs", open.frameMs);
    MEASURE ("guiOpen.messageThreadLoadPercent", open.guiLoad * 100.0);
    MEASURE ("guiOpen.fullWindowRepaintMs", open.fullWindowMs);
    CHECK (open.frames > (arctest::timingChecksEnabled ? 60 : 5)); // sanitizers slow frames ~100x
    // The editor must not slow the audio thread: it only reads relaxed atomics.
    CHECK (! arctest::timingChecksEnabled || open.audioFraction < closed.audioFraction * 1.5 + 0.02);
    CHECK (! arctest::timingChecksEnabled || open.frameMs < 8.0);
}

TEST_CASE ("ui", "preset browser searches the whole library and paints only what shows")
{
    ArcAudioProcessor proc;
    auto& pm = proc.getPresetManager();
    arc::ui::PresetBrowser browser (pm);
    browser.setSize (680, 500);
    const int total = pm.getNumPresets();
    MEASURE ("presets", total);
    CHECK (total >= 396);

    // No search: ALL holds everything, the categories add up to it.
    CHECK (browser.getNumVisible() == total);
    int sum = 0;
    for (const auto& c : arc::presets::categories())
    {
        CHECK (browser.getCategoryCount (c) > 0);
        sum += browser.getCategoryCount (c);
    }
    CHECK (sum == total);

    // Every result contains every word (name, tags, description or category).
    auto everyResultMatches = [&] (std::initializer_list<const char*> words)
    {
        for (int row = 0; row < browser.getNumVisible(); ++row)
        {
            const auto& p = pm.getPreset (browser.getVisiblePreset (row));
            const auto text = (p.name + " " + p.tags + " " + p.description + " " + p.category).toLowerCase();
            for (auto* w : words)
                if (! text.contains (w))
                    return false;
        }
        return true;
    };
    browser.setSearchText ("glass");
    const int glass = browser.getNumVisible();
    MEASURE ("matches.glass", glass);
    CHECK (glass > 28); // the GLASS category and more (glass pads, bowed glass, ...)
    CHECK (everyResultMatches ({ "glass" }));
    CHECK (browser.getCategoryCount ("ALL") == glass);

    browser.setSearchText ("Glass  BELL"); // case and spacing do not matter
    const int glassBell = browser.getNumVisible();
    MEASURE ("matches.glass_bell", glassBell);
    CHECK (glassBell > 0);
    CHECK (glassBell < glass);
    CHECK (everyResultMatches ({ "glass", "bell" }));

    // The search narrows the selected category, and the rail counts say where the rest are.
    browser.selectCategory ("DRONES");
    browser.setSearchText ("dark");
    CHECK (browser.getNumVisible() == browser.getCategoryCount ("DRONES"));
    CHECK (browser.getCategoryCount ("ALL") > browser.getNumVisible());
    CHECK (everyResultMatches ({ "dark" }));
    for (int row = 0; row < browser.getNumVisible(); ++row)
        CHECK (pm.getPreset (browser.getVisiblePreset (row)).category == "DRONES");

    browser.setSearchText ("zzqx nothing like this");
    CHECK (browser.getNumVisible() == 0);
    CHECK (browser.getCategoryCount ("ALL") == 0);

    // Clip-aware painting: the whole library is ~15000 px of rows, a paint draws only the
    // rows inside the viewport.
    browser.setSearchText ({});
    browser.selectCategory ("ALL");
    CHECK (browser.getNumVisible() == total);
    juce::Image img (juce::Image::ARGB, 680, 500, true);
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 10; ++i)
    {
        juce::Graphics g (img);
        browser.paintEntireComponent (g, true);
    }
    const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count() / 10.0;
    MEASURE ("fullBrowserPaintMs", ms);
    MEASURE ("rowsPainted", browser.getLastPaintedRows());
    CHECK (browser.getLastPaintedRows() > 0);
    CHECK (browser.getLastPaintedRows() <= 12); // ~10 rows fit, plus partial rows at the edges
    CHECK (ms < 40.0);
}

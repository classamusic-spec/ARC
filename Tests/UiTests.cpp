// Phase 11-12 — editor rendering: snapshots of real states (for design review) and
// frame-cost measurement of the animated Resonance Field.

#include "ArcTest.h"

#include <filesystem>

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
        CHECK (ms < 16.0);
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

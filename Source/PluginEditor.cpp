#include "PluginEditor.h"

#include "Core/Parameters.h"
#include "Graphics/Icons.h"
#include "Graphics/SilverSurface.h"

using namespace arc::ui;

namespace
{
// Layout of the 1200 x 900 design canvas (derived from the locked reference).
namespace L
{
const juce::Rectangle<int> logo { 38, 60, 132, 34 };
const juce::Rectangle<int> tagline { 206, 62, 170, 32 };
const juce::Rectangle<int> preset { 394, 44, 412, 56 };
const juce::Rectangle<int> gear { 824, 59, 28, 28 };
const juce::Rectangle<int> meter { 866, 60, 150, 36 };
const juce::Rectangle<int> master { 1030, 38, 80, 96 };
const juce::Rectangle<int> masterLabel { 1112, 64, 72, 30 };
const juce::Rectangle<int> leftPanel { 23, 153, 263, 442 };
const juce::Rectangle<int> rightPanel { 914, 153, 263, 442 };
const juce::Rectangle<int> field { 276, 126, 648, 468 };
const juce::Rectangle<int> strip { 23, 628, 1154, 218 };
const juce::Rectangle<int> motion { 44, 648, 92, 100 };
const juce::Rectangle<int> motionRate { 40, 756, 100, 22 };
const int macroX[4] = { 246, 412, 578, 744 };
const juce::Rectangle<int> macroSize { 0, 648, 150, 180 };
const int buttonX[3] = { 882, 954, 1026 };
const juce::Rectangle<int> buttonSize { 0, 670, 66, 96 };
const juce::Rectangle<int> status { 1082, 676, 86, 96 };
const float dividers[3] = { 158.0f, 832.0f, 1070.0f };
} // namespace L

} // namespace

// ---------------------------------------------------------------------------------------
namespace arc::ui
{

void Canvas::paint (juce::Graphics& g)
{
    if (renderBackground)
        layer.draw (g, getLocalBounds(), renderBackground);
}

// ---------------------------------------------------------------------------------------
MotionRateControl::MotionRateControl (juce::AudioProcessorValueTreeState& s)
    : state (s), rate (*s.getParameter (arc::params::motionRate)), division (*s.getParameter (arc::params::motionDivision)),
      rateAttach (rate, [this] (float) { repaint(); }), divisionAttach (division, [this] (float) { repaint(); })
{
    setTooltip ("MOTION RATE\nDrift speed; with SYNC on, one cycle per musical division. Drag or scroll.");
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

bool MotionRateControl::synced() const { return state.getRawParameterValue (arc::params::sync)->load() > 0.5f; }

void MotionRateControl::refresh() { repaint(); }

void MotionRateControl::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    juce::String value;
    if (synced())
    {
        const juce::StringArray names { "1/4", "1/2", "1 BAR", "2 BARS", "4 BARS", "8 BARS" };
        value = names[juce::roundToInt (division.convertFrom0to1 (division.getValue()))];
    }
    else
    {
        const float hz = rate.convertFrom0to1 (rate.getValue());
        value = hz < 1.0f ? juce::String (hz, 2) + " HZ" : juce::String (hz, 1) + " HZ";
    }
    const bool over = isMouseOverOrDragging();
    g.setColour (over ? colours::cyanDim : colours::inkMuted);
    drawTrackedText (g, (synced() ? "SYNC  " : "RATE  ") + value, b, Fonts::regular (10.0f, 0.2f), juce::Justification::centred);
}

void MotionRateControl::mouseDown (const juce::MouseEvent&)
{
    dragStart = rate.getValue();
    dragStartDivision = juce::roundToInt (division.convertFrom0to1 (division.getValue()));
    (synced() ? divisionAttach : rateAttach).beginGesture();
}

void MotionRateControl::mouseDrag (const juce::MouseEvent& e)
{
    const float dy = (float) -e.getDistanceFromDragStartY();
    if (synced())
        divisionAttach.setValueAsPartOfGesture ((float) juce::jlimit (0, 5, dragStartDivision + juce::roundToInt (dy / 18.0f)));
    else
        rateAttach.setValueAsPartOfGesture (rate.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, dragStart + dy / 220.0f)));
}

void MotionRateControl::mouseUp (const juce::MouseEvent&)
{
    rateAttach.endGesture();
    divisionAttach.endGesture();
}

void MotionRateControl::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (synced())
    {
        const int cur = juce::roundToInt (division.convertFrom0to1 (division.getValue()));
        divisionAttach.setValueAsCompleteGesture ((float) juce::jlimit (0, 5, cur + (w.deltaY > 0 ? 1 : -1)));
    }
    else
        rateAttach.setValueAsCompleteGesture (rate.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, rate.getValue() + w.deltaY * 0.05f)));
}

void MotionRateControl::mouseDoubleClick (const juce::MouseEvent&)
{
    if (synced())
        divisionAttach.setValueAsCompleteGesture (division.convertFrom0to1 (division.getDefaultValue()));
    else
        rateAttach.setValueAsCompleteGesture (rate.convertFrom0to1 (rate.getDefaultValue()));
}

// ---------------------------------------------------------------------------------------
void StatusReadout::paint (juce::Graphics& g)
{
    auto& s = processor.getValueTreeState();
    const juce::StringArray modes { "POLY", "MONO", "LEGATO" };
    const int mode = juce::roundToInt (s.getRawParameterValue (arc::params::voiceMode)->load());
    const int poly = juce::roundToInt (s.getRawParameterValue (arc::params::polyphony)->load());
    const bool synced = s.getRawParameterValue (arc::params::sync)->load() > 0.5f;
    juce::StringArray lines;
    lines.add (mode == 0 ? "POLY " + juce::String (poly) : modes[mode]);
    lines.add (juce::String (model.activeVoices) + (model.activeVoices == 1 ? " VOICE" : " VOICES"));
    lines.add (synced ? juce::String (juce::roundToInt (model.bpm)) + " BPM" : "CPU " + juce::String (juce::roundToInt (model.cpu * 100.0f)) + " %");
    auto b = getLocalBounds().toFloat();
    float y = b.getY();
    for (auto& l : lines)
    {
        g.setColour (colours::inkSoft);
        drawTrackedText (g, l, { b.getX(), y, b.getWidth(), 16.0f }, Fonts::regular (10.0f, 0.22f), juce::Justification::centredLeft);
        y += 18.0f;
    }
    g.setColour (colours::cyan.withAlpha (0.7f));
    g.fillRect (b.getX(), y + 8.0f, 42.0f, 1.2f);
}

} // namespace arc::ui

// ---------------------------------------------------------------------------------------
ArcAudioProcessorEditor::ArcAudioProcessorEditor (ArcAudioProcessor& p)
    : AudioProcessorEditor (p), processor (p), presetDisplay (p.getPresetManager()), meter (model), exciter (p.getValueTreeState()),
      material (p.getValueTreeState()), field (p, model), motionRate (p.getValueTreeState()), status (p, model),
      nodeInspector (p, model, field), coreInspector (p), settings (p), browser (p.getPresetManager())
{
    setLookAndFeel (&lnf);
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
    auto& state = processor.getValueTreeState();

    addAndMakeVisible (canvas);
    canvas.renderBackground = [this] (juce::Graphics& g) { renderBackground (g); };
    canvas.onBackgroundClick = [this] { closeOverlays (true); };
    field.setBackdrop ([this] (juce::Graphics& g)
                       {
                           g.addTransform (juce::AffineTransform::translation ((float) -field.getX(), (float) -field.getY()));
                           renderBackground (g);
                       });

    // Header
    presetDisplay.onOpenBrowser = [this] { openPresetBrowser (! browser.isShown()); };
    gear.setTooltip ("SETTINGS\nVoice mode, polyphony, glide, bend, release, MPE, quality, window size.");
    gear.onClick = [this]
    {
        const bool show = ! settings.isShown();
        closeOverlays (false);
        settings.setShown (show);
        positionCards();
    };
    meter.setTooltip ("OUTPUT\nStereo peak level (dBFS).");
    master.attach (state, arc::params::masterOutput);
    master.setValueFormatter ([] (double v) { return juce::String (v, 1) + " DB"; });
    master.setHelp ("MASTER OUTPUT", "Final level. Not stored in presets.");

    // Performance strip
    motion.attach (state, arc::params::motionDepth);
    motion.setValueFormatter (percent);
    motion.setHelp ("MOTION", "The network drifts by itself: nodes wander, retune and re-couple.");
    excite.attach (state, arc::params::excite);
    excite.setValueFormatter (percent);
    excite.setHelp ("EXCITE", "How much energy the exciter puts into the network.");
    coupling.attach (state, arc::params::coupling);
    coupling.setValueFormatter (percent);
    coupling.setHelp ("COUPLING", "Controls how strongly energy travels between resonators.");
    tension.attach (state, arc::params::tension);
    tension.setValueFormatter (percent);
    tension.setHelp ("TENSION", "Changes the frequency relationship between resonant modes.");
    tension.setBipolar (true);
    chaos.attach (state, arc::params::chaos);
    chaos.setValueFormatter (percent);
    chaos.setHelp ("CHAOS", "Introduces controlled irregularity into the network.");

    freeze.setTooltip ("FREEZE\nSuspend the sounding network: its energy stops decaying (F). New notes still play.");
    freezeAttach = std::make_unique<IndexAttachment> (*state.getParameter (arc::params::freeze),
                                                      [this] (int v) { freeze.setToggleState (v != 0, juce::dontSendNotification); });
    freeze.onClick = [this] { freezeAttach->set (freeze.getToggleState() ? 1 : 0); };
    sync.setTooltip ("SYNC\nMotion and recorded gestures follow the host tempo.");
    syncAttach = std::make_unique<IndexAttachment> (*state.getParameter (arc::params::sync),
                                                    [this] (int v) { sync.setToggleState (v != 0, juce::dontSendNotification); motionRate.refresh(); });
    sync.onClick = [this] { syncAttach->set (sync.getToggleState() ? 1 : 0); };
    random.setTooltip ("RANDOM\nClick: a gentle variation of this sound (R). Shift-click: a new network.");
    random.onClickWithMods = [this] (const juce::MouseEvent& e)
    {
        processor.getPresetManager().randomise (e.mods.isShiftDown());
        presetDisplay.refresh();
    };

    field.setComponentID ("field");
    random.setComponentID ("random");
    freeze.setComponentID ("freeze");
    sync.setComponentID ("sync");
    presetDisplay.setComponentID ("presetDisplay");
    meter.setComponentID ("meter");
    status.setComponentID ("status");
    for (auto* c : std::initializer_list<juce::Component*> { &presetDisplay, &gear, &meter, &master, &exciter, &material, &field, &motion,
                                                             &motionRate, &excite, &coupling, &tension, &chaos, &freeze, &random, &sync, &status })
        canvas.addAndMakeVisible (*c);

    // Overlays
    for (auto* card : std::initializer_list<GlassCard*> { &nodeInspector, &coreInspector, &settings, &browser })
    {
        canvas.addChildComponent (*card);
        card->onClose = [this, card]
        {
            card->setShown (false);
            if (card == &nodeInspector || card == &coreInspector)
                field.setSelection ({}, false);
        };
    }
    field.onSelectionChanged = [this] (ResonanceField::Selection s)
    {
        browser.setShown (false);
        settings.setShown (false);
        const bool wasDocked = nodeInspector.isShown() || coreInspector.isShown();
        nodeInspector.setShown (s.target == ResonanceField::Target::node);
        coreInspector.setShown (s.target == ResonanceField::Target::core);
        if (s.target == ResonanceField::Target::node)
            nodeInspector.setNode (s.node);
        dockPlaced = wasDocked && s.target != ResonanceField::Target::none;
        positionCards();
        dockPlaced = s.target != ResonanceField::Target::none;
        if (s.target == ResonanceField::Target::none)
        {
            exciter.setInspectorOpen (false);
            material.setInspectorOpen (false);
        }
        positionCards();
    };
    field.onGestureStateChanged = [this] { nodeInspector.refresh(); };
    settings.onSizeChosen = [this] (int i) { setUiSize (i); };

    // Size: restore the last one used with this instance.
    setResizable (true, true);
    const int w = juce::jlimit (900, 1800, processor.getEditorWidth());
    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio ((double) kBaseWidth / (double) kBaseHeight);
        c->setSizeLimits (900, 675, 1800, 1350);
    }
    setSize (w, w * kBaseHeight / kBaseWidth);
    setWantsKeyboardFocus (true);

    vblank = std::make_unique<juce::VBlankAttachment> (this, [this] (double t) { onFrame (t); });
}

ArcAudioProcessorEditor::~ArcAudioProcessorEditor()
{
    vblank.reset();
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void ArcAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll (colours::chassisEdge); }

void ArcAudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / (float) kBaseWidth;
    canvas.setBounds (0, 0, kBaseWidth, kBaseHeight);
    canvas.setTransform (juce::AffineTransform::scale (scale));
    processor.setEditorWidth (getWidth());
    settings.setSizeIndex (getWidth() < 990 ? 0 : getWidth() < 1170 ? 1 : getWidth() < 1350 ? 2 : 3);
    layoutCanvas();
}

void ArcAudioProcessorEditor::layoutCanvas()
{
    presetDisplay.setBounds (L::preset);
    gear.setBounds (L::gear);
    meter.setBounds (L::meter);
    master.setBounds (L::master);
    exciter.setBounds (L::leftPanel);
    material.setBounds (L::rightPanel);
    field.setBounds (L::field);
    motion.setBounds (L::motion);
    motionRate.setBounds (L::motionRate);
    ArcKnob* macros[4] = { &excite, &coupling, &tension, &chaos };
    for (int i = 0; i < 4; ++i)
        macros[i]->setBounds (L::macroSize.withX (L::macroX[i] - L::macroSize.getWidth() / 2));
    RoundButton* buttons[3] = { &freeze, &random, &sync };
    for (int i = 0; i < 3; ++i)
        buttons[i]->setBounds (L::buttonSize.withX (L::buttonX[i] - L::buttonSize.getWidth() / 2));
    status.setBounds (L::status);
    nodeInspector.setSize (452, 92);
    coreInspector.setSize (452, 92);
    settings.setSize (380, 214);
    browser.setSize (680, 500);
    positionCards();
    canvas.invalidateBackground();
}

void ArcAudioProcessorEditor::positionCards()
{
    // Node and CORE inspectors share a dock in the quiet band at the top of the chamber,
    // so the network stays visible (and playable) while it is inspected.
    // The band is chosen when the dock opens (top, unless nodes crowd it more than the
    // bottom) and then stays put, even while MOTION moves the nodes.
    const auto fb = field.getBounds();
    const int h = nodeInspector.getHeight();
    const auto top = juce::Rectangle<int> (nodeInspector.getWidth(), h).withCentre ({ fb.getCentreX(), fb.getY() + 18 + h / 2 });
    const auto bottom = top.withY (fb.getBottom() - 18 - h);
    if (! dockPlaced)
    {
        auto crowding = [&] (juce::Rectangle<int> band)
        {
            float c = 0.0f;
            for (int n = 0; n < 4; ++n)
            {
                const auto p = (field.nodeCentre (n) + fb.getPosition().toFloat()).toInt();
                if (band.expanded ((int) field.nodeRadiusPx()).contains (p))
                    c += n == field.getSelection().node ? 3.0f : 1.0f;
            }
            return c;
        };
        dockAtBottom = crowding (bottom) < crowding (top);
    }
    const auto dock = dockAtBottom ? bottom : top;
    nodeInspector.setBounds (dock);
    coreInspector.setBounds (dock);
    settings.setTopLeftPosition (L::gear.getRight() - settings.getWidth() + 40, L::preset.getBottom() + 12);
    browser.setTopLeftPosition (L::preset.getCentreX() - browser.getWidth() / 2, L::preset.getBottom() + 10);
}

// ---------------------------------------------------------------------------------------
void ArcAudioProcessorEditor::renderBackground (juce::Graphics& g)
{
    const auto full = juce::Rectangle<float> (0.0f, 0.0f, (float) kBaseWidth, (float) kBaseHeight);
    arc::gfx::paintChassis (g, full, 22.0f);

    // Chamber outlines
    const auto fb = L::field.toFloat();
    const auto bezelOuter = arc::gfx::superellipse (fb.expanded (24.0f, 19.0f), 3.6f);
    const auto bezelInner = arc::gfx::superellipse (fb, 3.6f);
    const auto cutout = arc::gfx::superellipse (fb.expanded (36.0f, 30.0f), 3.6f);

    // Side plates, machined around the bezel.
    auto plate = [&] (juce::Rectangle<int> r)
    {
        juce::Path p;
        p.addRoundedRectangle (r.toFloat(), 24.0f);
        juce::Graphics::ScopedSaveState s (g);
        juce::Path outside;
        outside.addRectangle (full);
        outside.addPath (cutout);
        outside.setUsingNonZeroWinding (false);
        g.reduceClipRegion (outside);
        arc::gfx::paintRaisedPlate (g, p, 14.0f);
    };
    plate (L::leftPanel);
    plate (L::rightPanel);

    // Performance strip plate and its engraved dividers.
    {
        juce::Path p;
        p.addRoundedRectangle (L::strip.toFloat(), 28.0f);
        arc::gfx::paintRaisedPlate (g, p, 16.0f);
        for (float x : L::dividers)
            arc::gfx::paintGroove (g, { x, (float) L::strip.getY() + 42.0f }, { x, (float) L::strip.getBottom() - 42.0f });
    }

    // Preset tray
    {
        juce::Path tray;
        tray.addRoundedRectangle (L::preset.toFloat().expanded (7.0f, 6.0f), 18.0f);
        arc::gfx::paintRecessedTray (g, tray);
    }

    // The chrome ring of the resonance chamber (drawn over the plates' inner edges).
    arc::gfx::paintChromeBezel (g, bezelOuter, bezelInner);

    // Header typography
    arc::gfx::drawLogo (g, L::logo.toFloat(), colours::ink, 2.2f);
    arc::gfx::paintGroove (g, { 192.0f, 56.0f }, { 192.0f, 100.0f });
    g.setColour (colours::inkSoft);
    drawTrackedText (g, "RESONANT NETWORK", L::tagline.toFloat().withHeight (15.0f), Fonts::regular (10.5f, 0.3f), juce::Justification::centredLeft);
    drawTrackedText (g, "SYNTHESIZER", L::tagline.toFloat().withTrimmedTop (17.0f).withHeight (15.0f), Fonts::regular (10.5f, 0.3f),
                     juce::Justification::centredLeft);
    drawTrackedText (g, "MASTER", L::masterLabel.toFloat().withHeight (14.0f), Fonts::regular (10.5f, 0.3f), juce::Justification::centredLeft);
    drawTrackedText (g, "OUTPUT", L::masterLabel.toFloat().withTrimmedTop (16.0f).withHeight (14.0f), Fonts::regular (10.5f, 0.3f),
                     juce::Justification::centredLeft);

}

// ---------------------------------------------------------------------------------------
void ArcAudioProcessorEditor::onFrame (double t)
{
    if (lastFrameTime < 0.0)
        lastFrameTime = t;
    const double dt = juce::jlimit (0.0, 0.1, t - lastFrameTime);
    lastFrameTime = t;

    // Throttle to ~15 fps when silent and idle.
    const bool interacting = juce::Desktop::getInstance().getMainMouseSource().isDragging() || (clock - lastInteraction) < 2.0;
    if (! interacting && model.isQuiet() && (++frameCounter % 4) != 0)
    {
        clock += dt;
        return;
    }
    advanceFrame (dt);
}

void ArcAudioProcessorEditor::advanceFrame (double dtSeconds)
{
    const float dt = (float) dtSeconds;
    clock += dtSeconds;
    auto& telemetry = processor.getTelemetry();
    const auto beat = arc::Telemetry::load (telemetry.blockCounter);
    if (beat != lastBlockCounter)
    {
        lastBlockCounter = beat;
        lastEngineBeat = clock;
    }
    const bool running = (clock - lastEngineBeat) < 0.3;
    model.update (telemetry, dt, model.freeze > 0.5f);

    field.advance (dt, running);
    field.repaint();
    meter.repaint();
    exciter.advance (dt);
    material.advance (dt);
    for (auto* card : std::initializer_list<GlassCard*> { &nodeInspector, &coreInspector, &settings, &browser })
        card->advance (dt);
    freeze.setActivity (model.freeze);

    if ((++frameCounter % 6) == 0)
    {
        presetDisplay.refresh();
        nodeInspector.refresh();
        coreInspector.refresh();
        status.repaint();
    }
    updateCaption();
}

void ArcAudioProcessorEditor::updateCaption()
{
    juce::String tip;
    auto mouse = juce::Desktop::getInstance().getMainMouseSource();
    if (auto* c = mouse.getComponentUnderMouse(); c != nullptr && (c == this || isParentOf (c)))
    {
        for (auto* comp = c; comp != nullptr && comp != this; comp = comp->getParentComponent())
            if (auto* tc = dynamic_cast<juce::TooltipClient*> (comp))
            {
                tip = tc->getTooltip();
                if (tip.isNotEmpty())
                    break;
            }
        if (mouse.getScreenPosition() != lastMousePosition)
        {
            lastMousePosition = mouse.getScreenPosition();
            lastInteraction = clock;
        }
    }
    const auto lines = juce::StringArray::fromLines (tip);
    field.setCaption (lines[0], lines.size() > 1 ? lines[1] : juce::String());
}

// ---------------------------------------------------------------------------------------
void ArcAudioProcessorEditor::closeOverlays (bool includingPanels)
{
    browser.setShown (false);
    settings.setShown (false);
    nodeInspector.setShown (false);
    coreInspector.setShown (false);
    field.setSelection ({}, false);
    if (includingPanels)
    {
        exciter.setInspectorOpen (false);
        material.setInspectorOpen (false);
    }
}

void ArcAudioProcessorEditor::openPresetBrowser (bool open)
{
    closeOverlays (false);
    browser.setShown (open);
    if (open)
    {
        browser.refresh();
        positionCards();
        browser.grabKeyboardFocus();
    }
}

void ArcAudioProcessorEditor::selectNode (int node)
{
    field.setSelection ({ ResonanceField::Target::node, node }, true);
}

void ArcAudioProcessorEditor::selectCore()
{
    field.setSelection ({ ResonanceField::Target::core, -1 }, true);
}

void ArcAudioProcessorEditor::openSettings (bool open)
{
    closeOverlays (false);
    settings.setShown (open);
    positionCards();
}

void ArcAudioProcessorEditor::openPanelInspectors (bool exciterOpen, bool materialOpen)
{
    exciter.setInspectorOpen (exciterOpen);
    material.setInspectorOpen (materialOpen);
}

bool ArcAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    auto& presets = processor.getPresetManager();
    if (key == juce::KeyPress::escapeKey)
    {
        closeOverlays (true);
        return true;
    }
    if (key == juce::KeyPress::leftKey)
    {
        presets.loadPrevious();
        presetDisplay.refresh();
        return true;
    }
    if (key == juce::KeyPress::rightKey)
    {
        presets.loadNext();
        presetDisplay.refresh();
        return true;
    }
    const auto c = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
    if (c == 'f')
    {
        freeze.triggerClick();
        return true;
    }
    if (c == 'r')
    {
        presets.randomise (key.getModifiers().isShiftDown());
        presetDisplay.refresh();
        return true;
    }
    return false;
}

void ArcAudioProcessorEditor::setUiSize (int index)
{
    const int widths[4] = { 900, 1080, 1260, 1440 };
    const int w = widths[juce::jlimit (0, 3, index)];
    setSize (w, w * kBaseHeight / kBaseWidth);
}

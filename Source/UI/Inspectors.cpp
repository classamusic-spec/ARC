#include "UI/Inspectors.h"

#include "Core/Parameters.h"
#include "PluginProcessor.h"
#include "UI/ResonanceField.h"

namespace arc::ui
{

// ---------------------------------------------------------------------------------------
GlassCard::GlassCard (const juce::String& t) : title (t)
{
    closeButton.onClick = [this] { if (onClose) onClose(); };
    closeButton.setTooltip ("CLOSE\nEsc also closes inspectors.");
    addAndMakeVisible (closeButton);
    setVisible (false);
    setAlwaysOnTop (true);
}

void GlassCard::setTitle (const juce::String& t, const juce::String& sub)
{
    title = t;
    subtitle = sub;
    repaint();
}

void GlassCard::setShown (bool s)
{
    shown = s;
    if (s)
        setVisible (true);
}

void GlassCard::advance (float dt)
{
    const float target = shown ? 1.0f : 0.0f;
    if (std::abs (anim - target) < 1.0e-3f)
    {
        if (! shown && isVisible())
            setVisible (false);
        return;
    }
    anim = smoothTowards (anim, target, dt, 0.06f);
    setAlpha (anim);
    setTransform (juce::AffineTransform::translation (0.0f, (1.0f - easeOutCubic (anim)) * 8.0f));
    if (! shown && anim < 0.01f)
    {
        anim = 0.0f;
        setVisible (false);
    }
}

void GlassCard::resized() { closeButton.setBounds (getWidth() - 34, 10, 24, 24); }

void GlassCard::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (1.0f);
    // Shadow, glass body, fine cyan hairline, top sheen.
    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 18, { 0, 6 }).drawForRectangle (g, b.toNearestInt());
    juce::ColourGradient body (juce::Colour (0xff1d232b), b.getX(), b.getY(), juce::Colour (0xff0b0f14), b.getX(), b.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (b, 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRoundedRectangle (b.withHeight (juce::jmin (40.0f, b.getHeight() * 0.45f)).reduced (1.0f), 11.0f);
    g.setColour (colours::cyan.withAlpha (0.35f));
    g.drawRoundedRectangle (b, 12.0f, 1.0f);

    g.setColour (colours::glassText);
    drawTrackedText (g, title.toUpperCase(), { b.getX() + 16.0f, b.getY() + 10.0f, b.getWidth() - 60.0f, 18.0f },
                     Fonts::regular (13.0f, 0.28f), juce::Justification::centredLeft);
    if (subtitle.isNotEmpty())
    {
        g.setColour (colours::cyan.withAlpha (0.9f));
        drawTrackedText (g, subtitle.toUpperCase(), { b.getX() + 16.0f, b.getY() + 27.0f, b.getWidth() - 60.0f, 13.0f },
                         Fonts::regular (9.5f, 0.2f), juce::Justification::centredLeft);
    }
}

// ---------------------------------------------------------------------------------------
NodeInspector::NodeInspector (ArcAudioProcessor& p, FieldModel& m, ResonanceField& f)
    : GlassCard ("Node"), processor (p), model (m), field (f)
{
    const char* names[6] = { "Ratio", "Decay", "Damp", "Level", "Pan", "Link" };
    for (size_t i = 0; i < 6; ++i)
    {
        knobs[i] = std::make_unique<ArcKnob> (names[i], KnobStyle::glass);
        addAndMakeVisible (*knobs[i]);
    }
    rec.setTooltip ("RECORD MOTION\nArm, then drag this node: its path loops when you let go (or Alt-drag any node).");
    rec.onClick = [this] { field.setRecordArmed (node, rec.getToggleState()); };
    clear.setTooltip ("CLEAR MOTION\nRemove this node's recorded gesture.");
    clear.onClick = [this]
    {
        if (node >= 0)
            processor.setGesture (node, {});
        refresh();
    };
    loop.setTooltip ("LOOP MOTION\nPlay recorded gestures (all nodes).");
    loop.attach (*processor.getValueTreeState().getParameter (params::gesturePlay));
    addAndMakeVisible (rec);
    addAndMakeVisible (loop);
    addAndMakeVisible (clear);
}

void NodeInspector::setNode (int n)
{
    if (n == node)
        return;
    node = n;
    if (n < 0)
        return;
    auto& state = processor.getValueTreeState();
    const char* fields[6] = { "radius", "decay", "damp", "level", "angle", "link" };
    for (size_t i = 0; i < 6; ++i)
        knobs[i]->attach (state, params::nodeId (n, fields[i]));

    knobs[0]->setValueFormatter ([] (double v)
                                 {
                                     const double st = (v - 0.5) * 24.0;
                                     return (st >= 0 ? "+" : "") + juce::String (st, 1) + " ST";
                                 });
    knobs[0]->setHelp ("RATIO", "Retunes the node up to an octave around its material ratio (drag the node for the same).");
    knobs[1]->setValueFormatter ([] (double v) { return juce::String::charToString (0x00d7) + juce::String (std::exp2 ((v - 0.5) * 4.0), 2); });
    knobs[1]->setHelp ("DECAY", "How long this resonator rings, relative to the material.");
    knobs[2]->setValueFormatter ([] (double v) { return juce::String::charToString (0x00d7) + juce::String (std::exp2 (-(v - 0.5) * 4.0), 2); });
    knobs[2]->setHelp ("DAMP", "High-frequency damping of this resonator.");
    knobs[3]->setValueFormatter (percent);
    knobs[3]->setHelp ("LEVEL", "How much of this resonator you hear.");
    knobs[4]->setValueFormatter ([] (double v) { return juce::String (juce::roundToInt ((v - 0.5) * 360.0)) + juce::String::charToString (0x00b0); });
    knobs[4]->setHelp ("PAN", "The node's angle: stereo position and closeness to its neighbours.");
    knobs[4]->setBipolar (true);
    knobs[5]->setValueFormatter (percent);
    knobs[5]->setHelp ("LINK", "How strongly this node couples into the network.");
    refresh();
}

void NodeInspector::refresh()
{
    if (node < 0)
        return;
    const char* names[4] = { "A", "B", "C", "D" };
    const float ratio = model.nodeRatio[(size_t) node];
    setTitle (juce::String ("Node ") + names[node], "RATIO " + juce::String::charToString (0x00d7) + juce::String (ratio, 2));
    hasGesture = processor.getGesture (node).valid;
    clear.setEnabled (hasGesture);
    clear.setAlpha (hasGesture ? 1.0f : 0.35f);
    rec.setToggleState (field.isRecordArmed (node) || (field.isRecording() && field.isRecordArmed (node)), juce::dontSendNotification);
}

void NodeInspector::resized()
{
    // Dock layout: title block on the left, six knobs in a row.
    GlassCard::resized();
    closeButton.setBounds (getWidth() - 30, 8, 22, 22);
    auto r = getLocalBounds().reduced (12, 8);
    auto left = r.removeFromLeft (104);
    r.removeFromRight (22);
    auto chips = left.removeFromBottom (22);
    rec.setBounds (chips.removeFromLeft (46));
    chips.removeFromLeft (4);
    loop.setBounds (chips.removeFromLeft (50));
    clear.setBounds (juce::Rectangle<int> (22, 22).withCentre ({ r.getRight() + 10, getHeight() - 20 }));
    const int kw = r.getWidth() / 6;
    for (int i = 0; i < 6; ++i)
        knobs[(size_t) i]->setBounds (r.getX() + i * kw, r.getY(), kw, r.getHeight());
}

// ---------------------------------------------------------------------------------------
CoreInspector::CoreInspector (ArcAudioProcessor& p) : GlassCard ("Network")
{
    auto& state = p.getValueTreeState();
    topology.attach (*state.getParameter (params::topology));
    topology.onChange = [this] (int) { refresh(); };
    topology.setTooltip ("TOPOLOGY\nWhich resonators exchange energy: STAR (core only), RING, WEB (all), CHAIN (node to node).");
    quantise.attach (*state.getParameter (params::quantise));
    quantise.setTooltip ("QUANTIZE\nSnap node tuning to semitones.");
    space.attach (state, params::space);
    space.setValueFormatter (percent);
    space.setHelp ("SPACE", "A small, quiet ambience around the dry network.");
    width.attach (state, params::width);
    width.setValueFormatter (percent);
    width.setHelp ("WIDTH", "Stereo spread of the nodes.");
    drive.attach (state, params::drive);
    drive.setValueFormatter (percent);
    drive.setHelp ("DRIVE", "Soft output saturation.");
    quantise.onStateChange = [this] { refresh(); };
    for (auto* c : std::initializer_list<juce::Component*> { &topology, &quantise, &space, &width, &drive })
        addAndMakeVisible (*c);
    refresh();
}

void CoreInspector::refresh()
{
    const juce::StringArray names { "STAR", "RING", "WEB", "CHAIN" };
    setTitle ("Network", names[topology.getIndex()] + (quantise.getToggleState() ? juce::String::fromUTF8 ("  \xc2\xb7  SEMITONES") : juce::String()));
}

void CoreInspector::resized()
{
    GlassCard::resized();
    closeButton.setBounds (getWidth() - 30, 8, 22, 22);
    auto r = getLocalBounds().reduced (12, 8);
    auto left = r.removeFromLeft (104);
    r.removeFromRight (22);
    quantise.setBounds (left.removeFromBottom (22).withWidth (92));
    auto knobsArea = r.removeFromRight (r.getWidth() * 45 / 100);
    const int kw = knobsArea.getWidth() / 3;
    space.setBounds (knobsArea.removeFromLeft (kw));
    width.setBounds (knobsArea.removeFromLeft (kw));
    drive.setBounds (knobsArea);
    topology.setBounds (r.reduced (6, 0).withSizeKeepingCentre (r.getWidth() - 12, 24));
}

// ---------------------------------------------------------------------------------------
SettingsCard::SettingsCard (ArcAudioProcessor& p) : GlassCard ("Settings")
{
    auto& state = p.getValueTreeState();
    setTitle ("Settings", juce::String::fromUTF8 ("PLAYING  \xc2\xb7  QUALITY"));
    voiceMode.attach (*state.getParameter (params::voiceMode));
    voiceMode.setTooltip ("VOICE MODE\nPOLY chords, MONO single notes, LEGATO glides one network between notes.");
    quality.attach (*state.getParameter (params::quality));
    quality.setTooltip ("QUALITY\nNetwork dispersion detail: ECO saves CPU, HIGH for rendering.");
    polyphony.attach (state, params::polyphony);
    polyphony.setValueFormatter ([] (double v) { return juce::String (juce::roundToInt (v)); });
    polyphony.setHelp ("VOICES", "Maximum simultaneous notes (about 1 % CPU each).");
    glide.attach (state, params::glide);
    glide.setValueFormatter ([] (double v) { return juce::String (juce::roundToInt (v * 1000.0)) + " MS"; });
    glide.setHelp ("GLIDE", "Portamento time in MONO / LEGATO.");
    bend.attach (state, params::bendRange);
    bend.setValueFormatter ([] (double v) { return juce::String (juce::roundToInt (v)) + " ST"; });
    bend.setHelp ("BEND RANGE", "Pitch-bend range in semitones.");
    release.attach (state, params::releaseDamping);
    release.setValueFormatter (percent);
    release.setHelp ("RELEASE", "How firmly the network is damped when a key is released.");
    mpe.attach (*state.getParameter (params::mpe));
    mpe.setTooltip ("MPE\nPer-note pitch, pressure and timbre (lower zone).");
    uiSize.setTooltip ("WINDOW SIZE\nS 75 %, M 90 %, L 105 %, XL 120 % (or drag the corner).");
    uiSize.onChange = [this] (int i) { if (onSizeChosen) onSizeChosen (i); };
    for (auto* c : std::initializer_list<juce::Component*> { &voiceMode, &quality, &polyphony, &glide, &bend, &release, &mpe, &uiSize })
        addAndMakeVisible (*c);
}

void SettingsCard::resized()
{
    GlassCard::resized();
    auto c = content();
    auto row = c.removeFromTop (24);
    voiceMode.setBounds (row.removeFromLeft (c.getWidth() * 3 / 5));
    row.removeFromLeft (10);
    mpe.setBounds (row);
    c.removeFromTop (10);
    auto knobsRow = c.removeFromTop (78);
    const int kw = knobsRow.getWidth() / 4;
    polyphony.setBounds (knobsRow.removeFromLeft (kw));
    glide.setBounds (knobsRow.removeFromLeft (kw));
    bend.setBounds (knobsRow.removeFromLeft (kw));
    release.setBounds (knobsRow);
    c.removeFromTop (8);
    auto bottomRow = c.removeFromTop (24);
    uiSize.setBounds (bottomRow.removeFromRight (120));
    bottomRow.removeFromRight (10);
    quality.setBounds (bottomRow);
}

void SettingsCard::paint (juce::Graphics& g)
{
    GlassCard::paint (g);
    g.setColour (colours::glassFaint);
    drawTrackedText (g, juce::String ("ARC ") + ARC_VERSION_STRING + juce::String::fromUTF8 ("  \xc2\xb7  JOST TYPEFACE (SIL OFL 1.1)"),
                     getLocalBounds().toFloat().removeFromBottom (18.0f).reduced (16.0f, 0.0f), Fonts::regular (8.5f, 0.2f),
                     juce::Justification::centredLeft);
}

} // namespace arc::ui

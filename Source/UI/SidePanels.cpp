#include "UI/SidePanels.h"

#include "Core/Parameters.h"

namespace arc::ui
{

SelectorPanel::SelectorPanel (juce::AudioProcessorValueTreeState& s, const juce::String& choiceParamId, const juce::String& h,
                              const juce::String& sub, std::array<Option, 4> options, bool mirror)
    : state (s), heading (h), subheading (sub), mirrored (mirror)
{
    for (size_t i = 0; i < 4; ++i)
    {
        tiles[i] = std::make_unique<SelectorTile> (options[i].name, options[i].icon);
        tiles[i]->setTooltip (options[i].name.toUpperCase() + "\n" + options[i].help);
        tiles[i]->onClick = [this, i]
        {
            const int idx = (int) i;
            if (idx == selected)
                setInspectorOpen (! inspectorOpen);
            else
            {
                attachment->set (idx);
                setInspectorOpen (true);
            }
        };
        addAndMakeVisible (*tiles[i]);
    }
    paramId = choiceParamId;
    for (size_t i = 0; i < 4; ++i)
        tiles[i]->setComponentID (choiceParamId + ".tile." + juce::String ((int) i));
}

void SelectorPanel::finishInit()
{
    // Called by the derived constructor: the attachment's initial update calls the
    // (pure virtual) updateInspectorFor.
    attachment = std::make_unique<IndexAttachment> (*state.getParameter (paramId), [this] (int i)
                                                    {
                                                        selected = juce::jlimit (0, 3, i);
                                                        for (size_t k = 0; k < 4; ++k)
                                                            tiles[k]->setSelectedState ((int) k == selected);
                                                        updateInspectorFor (selected);
                                                        layoutContent();
                                                    });
}

SelectorPanel::~SelectorPanel() = default;

void SelectorPanel::setInspectorOpen (bool open)
{
    if (open == inspectorOpen)
        return;
    inspectorOpen = open;
    if (onInspectorToggled)
        onInspectorToggled (open);
}

void SelectorPanel::advance (float dt)
{
    const float target = inspectorOpen ? 1.0f : 0.0f;
    if (std::abs (openAnim - target) < 1.0e-3f)
    {
        if (! juce::approximatelyEqual (openAnim, target))
        {
            openAnim = target;
            layoutContent();
        }
        return;
    }
    openAnim = smoothTowards (openAnim, target, dt, 0.07f);
    layoutContent();
}

void SelectorPanel::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float x = b.getX() + 40.0f;
    g.setColour (colours::ink);
    drawTrackedText (g, heading, { x, b.getY() + 40.0f, b.getWidth() - 60.0f, 22.0f }, Fonts::regular (16.5f, 0.3f),
                     juce::Justification::centredLeft);
    g.setColour (colours::inkMuted);
    drawTrackedText (g, subheading, { x, b.getY() + 63.0f, b.getWidth() - 60.0f, 14.0f }, Fonts::regular (10.0f, 0.24f),
                     juce::Justification::centredLeft);
    // Accent rule: a short cyan-tinted stroke over a long hairline.
    const float ry = b.getY() + 92.0f;
    g.setColour (colours::hairline);
    g.fillRect (x, ry, 150.0f, 1.0f);
    g.setColour (colours::cyanDim.withAlpha (0.55f));
    g.fillRect (x, ry - 0.5f, 26.0f, 2.0f);

    if (openAnim > 0.01f && ! knobs.empty())
    {
        // Inspector well: a fine engraved line above the knobs.
        const float top = (float) knobs.front()->getY() - 12.0f;
        g.setColour (colours::hairline.withAlpha (openAnim));
        g.fillRect (x - 12.0f, top, b.getWidth() - 56.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.8f * openAnim));
        g.fillRect (x - 12.0f, top + 1.0f, b.getWidth() - 56.0f, 1.0f);
    }
}

void SelectorPanel::resized() { layoutContent(); }

void SelectorPanel::layoutContent()
{
    const auto b = getLocalBounds();
    const float t = easeOutCubic (openAnim);
    const int left = 26, right = b.getWidth() - 30;
    const int top = 118;
    const float tileH = juce::jmap (t, 55.0f, 40.0f);
    const float gap = juce::jmap (t, 11.0f, 6.0f);
    float y = (float) top;
    for (auto& tile : tiles)
    {
        tile->setBounds (left, juce::roundToInt (y), right - left, juce::roundToInt (tileH));
        tile->setCompact (t);
        y += tileH + gap;
    }

    // Inspector knobs rise from below the tiles.
    const int n = (int) knobs.size();
    if (n == 0)
        return;
    const int knobW = (right - left) / n;
    const int knobH = 74;
    const int inspectorTop = juce::roundToInt (y) + 16 + juce::roundToInt ((1.0f - t) * 30.0f);
    for (int i = 0; i < n; ++i)
    {
        auto& k = knobs[(size_t) i];
        k->setBounds (left + i * knobW, inspectorTop, knobW, knobH);
        k->setAlpha (t);
        k->setVisible (t > 0.02f);
        k->setInterceptsMouseClicks (t > 0.5f, t > 0.5f);
    }
    repaint();
}

// ---------------------------------------------------------------------------------------
ExciterPanel::ExciterPanel (juce::AudioProcessorValueTreeState& s)
    : SelectorPanel (s, params::exciterType, "EXCITER", "HOW IT BEGINS",
                     { { { "Strike", gfx::Icon::strike, "A mallet hits the CORE: bells, bars, drums." },
                         { "Pluck", gfx::Icon::pluck, "A string is drawn and released: harps, kalimbas, wires." },
                         { "Bow", gfx::Icon::bow, "Continuous friction sustains the network: bowed metal, glass, wood." },
                         { "Air", gfx::Icon::air, "Breath drives the CORE: flutes, pipes, blown glass, wind." } } },
                     false)
{
    for (int i = 0; i < 3; ++i)
    {
        knobs.push_back (std::make_unique<ArcKnob> ("", KnobStyle::small));
        addChildComponent (*knobs.back());
    }
    finishInit();
}

void ExciterPanel::updateInspectorFor (int option)
{
    if (option == attachedTo || knobs.size() != 3)
        return;
    attachedTo = option;
    struct K
    {
        const char* id;
        const char* name;
        const char* help;
    };
    static const K table[4][3] = {
        { { params::strikeHardness, "Hardness", "Mallet hardness: soft felt to hard metal. Harder = brighter, shorter contact." },
          { params::strikeLength, "Length", "Contact time of the strike." },
          { params::strikeTone, "Tone", "Spectral tilt of the impact." } },
        { { params::pluckPosition, "Position", "Where the string is plucked: near the end = bright and thin, middle = round." },
          { params::pluckDamp, "Damp", "Palm mute: shortens the ring of the whole network." },
          { params::pluckTone, "Tone", "Sharpness of the release." } },
        { { params::bowPressure, "Pressure", "Bow force: light = airy flautando, heavy = pressed and bright." },
          { params::bowSpeed, "Speed", "Bow velocity: sets the loudness of the sustained tone." },
          { params::bowFriction, "Friction", "Rosin grip and bow noise." } },
        { { params::airFlow, "Flow", "Breath pressure: from breath noise to a speaking tone." },
          { params::airTurbulence, "Turbulence", "Amount of breath noise in the jet." },
          { params::airTone, "Tone", "Brightness of the breath." } },
    };
    for (size_t i = 0; i < 3; ++i)
    {
        const auto& k = table[juce::jlimit (0, 3, option)][i];
        knobs[i]->setName (k.name);
        knobs[i]->attach (state, k.id);
        knobs[i]->setValueFormatter (percent);
        knobs[i]->setHelp (juce::String (k.name).toUpperCase(), k.help);
    }
}

// ---------------------------------------------------------------------------------------
MaterialPanel::MaterialPanel (juce::AudioProcessorValueTreeState& s)
    : SelectorPanel (s, params::materialType, "BODY / MATERIAL", "WHAT IT RESONATES AS",
                     { { { "Glass", gfx::Icon::glass, "Bright, clean, long high-frequency resonance; widely spaced modes." },
                         { "Metal", gfx::Icon::metal, "Dense, shimmering, bell-like partials with slow beating." },
                         { "Wood", gfx::Icon::wood, "Warm, fast-decaying, strongly damped highs: bars and bodies." },
                         { "Membrane", gfx::Icon::membrane, "Drum-like modes that bend slightly with strike energy." } } },
                     true)
{
    finishInit();
}

void MaterialPanel::updateInspectorFor (int)
{
    if (attached)
        return;
    attached = true;
    struct D
    {
        const char* id;
        const char* name;
        const char* help;
    };
    static const D defs[4] = { { params::mass, "Mass", "Heavier bodies ring longer, couple less and bend less with energy." },
                               { params::brightness, "Bright", "High-frequency decay and how far each resonator keeps its overtones." },
                               { params::loss, "Loss", "Overall damping of the body." },
                               { params::inharmonicity, "Inharm", "Stiffness: stretches the partials away from the harmonic series." } };
    for (auto& d : defs)
    {
        knobs.push_back (std::make_unique<ArcKnob> (d.name, KnobStyle::small));
        knobs.back()->attach (state, d.id);
        knobs.back()->setValueFormatter (percent);
        knobs.back()->setHelp (juce::String (d.name).toUpperCase(), d.help);
        addChildComponent (*knobs.back());
    }
}

} // namespace arc::ui

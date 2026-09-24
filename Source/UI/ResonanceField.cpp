#include "UI/ResonanceField.h"

#include "Core/Parameters.h"
#include "Engine/EngineParams.h"
#include "Graphics/Icons.h"
#include "Graphics/SilverSurface.h"
#include "PluginProcessor.h"
#include "Synthesis/CouplingMatrix.h"

namespace arc::ui
{

namespace
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;
constexpr float kInnerRadius = 0.45f; // display radius at radius param 0
constexpr float kRadiusSpan = 0.52f;  // ... up to 0.97 at radius param 1
constexpr float kJunction = 0.76f;    // ring junctions sit inside the node midpoints: the
                                      // ring edges bow inward, towards the CORE
const char* const kNodeNames[4] = { "A", "B", "C", "D" };

float fract (float x) { return x - std::floor (x); }

/** Procedural sphere: cracked obsidian (nodes / core) or polished steel (junctions). */
juce::Image makeSphere (int size, uint32_t seed, int cells, bool metal, juce::Colour rimTint, float rimAmount)
{
    size = juce::jmax (4, size);
    juce::Image img (juce::Image::ARGB, size, size, true);
    juce::Random r (static_cast<juce::int64> (seed));
    std::vector<juce::Point<float>> sites;
    std::vector<float> shade;
    for (int i = 0; i < cells; ++i)
    {
        sites.emplace_back (r.nextFloat() * 2.2f - 1.1f, r.nextFloat() * 2.2f - 1.1f);
        shade.push_back (r.nextFloat());
    }
    const float lx = -0.45f, ly = -0.6f, lz = 0.66f;
    const float ll = std::sqrt (lx * lx + ly * ly + lz * lz);
    const float hx = lx / ll, hy = ly / ll, hz = lz / ll + 1.0f;
    const float hl = std::sqrt (hx * hx + hy * hy + hz * hz);

    juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
    const float half = (float) size * 0.5f;
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const float nx = ((float) x + 0.5f - half) / half, ny = ((float) y + 0.5f - half) / half;
            const float r2 = nx * nx + ny * ny;
            if (r2 >= 1.0f + 2.0f / half)
                continue;
            const float d = std::sqrt (r2);
            const float cover = juce::jlimit (0.0f, 1.0f, (1.0f - d) * half + 0.5f);
            const float nz = std::sqrt (juce::jmax (0.0f, 1.0f - juce::jmin (1.0f, r2)));
            const float diff = juce::jmax (0.0f, (nx * lx + ny * ly + nz * lz) / ll);
            const float spec = std::pow (juce::jmax (0.0f, (nx * hx + ny * hy + nz * hz) / hl), metal ? 26.0f : 42.0f);
            const float fres = std::pow (1.0f - nz, 2.4f);

            float cr, cg, cb;
            if (metal)
            {
                const float v = 0.18f + 0.7f * diff + 0.25f * (1.0f - ny) * 0.5f;
                cr = v * 0.93f; cg = v * 0.96f; cb = v;
                cr += spec * 0.9f; cg += spec * 0.9f; cb += spec * 0.95f;
            }
            else
            {
                // Texture coordinates bent around the sphere.
                const float k = 0.62f + 0.38f * nz;
                const float tx = nx / k, ty = ny / k;
                float d1 = 1e9f, d2 = 1e9f;
                int nearest = 0;
                for (size_t i = 0; i < sites.size(); ++i)
                {
                    const float dx = tx - sites[i].x, dy = ty - sites[i].y;
                    const float dd = dx * dx + dy * dy;
                    if (dd < d1)
                    {
                        d2 = d1;
                        d1 = dd;
                        nearest = (int) i;
                    }
                    else if (dd < d2)
                        d2 = dd;
                }
                const float edge = juce::jlimit (0.0f, 1.0f, (std::sqrt (d2) - std::sqrt (d1)) / 0.045f);
                const float crack = (1.0f - edge) * (1.0f - edge);
                const float cell = 0.085f + 0.05f * shade[(size_t) nearest];
                const float lit = 0.3f + 0.95f * diff;
                cr = cell * lit + crack * (0.13f + 0.26f * diff);
                cg = cell * lit * 1.05f + crack * (0.15f + 0.29f * diff);
                cb = cell * lit * 1.18f + crack * (0.18f + 0.33f * diff);
                cr += spec * 0.38f; cg += spec * 0.4f; cb += spec * 0.44f;
            }
            cr += rimTint.getFloatRed() * fres * rimAmount;
            cg += rimTint.getFloatGreen() * fres * rimAmount;
            cb += rimTint.getFloatBlue() * fres * rimAmount;
            const auto c = juce::Colour::fromFloatRGBA (juce::jlimit (0.0f, 1.0f, cr), juce::jlimit (0.0f, 1.0f, cg),
                                                        juce::jlimit (0.0f, 1.0f, cb), cover);
            data.setPixelColour (x, y, c);
        }
    return img;
}

juce::Image makeHalo (int size, juce::Colour c)
{
    juce::Image img (juce::Image::ARGB, size, size, true);
    juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
    const float half = (float) size * 0.5f;
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const float nx = ((float) x + 0.5f - half) / half, ny = ((float) y + 0.5f - half) / half;
            const float d = std::sqrt (nx * nx + ny * ny);
            if (d >= 1.0f)
                continue;
            const float a = std::pow (1.0f - d, 2.2f);
            const float core = std::pow (juce::jmax (0.0f, 1.0f - d * 2.2f), 2.0f);
            data.setPixelColour (x, y, c.interpolatedWith (juce::Colours::white, core * 0.5f).withAlpha (juce::jlimit (0.0f, 1.0f, a)));
        }
    return img;
}

void strokeGlow (juce::Graphics& g, const juce::Path& p, juce::Colour c, float width, float base, float glow)
{
    if (glow > 0.03f)
    {
        g.setColour (c.withAlpha (0.09f * glow));
        g.strokePath (p, juce::PathStrokeType (width * 4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (c.interpolatedWith (colours::ice, 0.5f * glow).withAlpha (juce::jlimit (0.0f, 1.0f, base + 0.6f * glow)));
    g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

/** Blits a sprite rendered at `pixelScale` device pixels per logical unit, centred on c,
    1:1 in device space (no resampling) with the position snapped to device pixels. */
void blit (juce::Graphics& g, const juce::Image& img, juce::Point<float> c, float pixelScale, float alpha = 1.0f)
{
    if (! img.isValid() || alpha <= 0.002f)
        return;
    const float x = std::round (c.x * pixelScale - (float) img.getWidth() * 0.5f);
    const float y = std::round (c.y * pixelScale - (float) img.getHeight() * 0.5f);
    g.setOpacity (juce::jmin (1.0f, alpha));
    g.drawImageTransformed (img, juce::AffineTransform::translation (x, y).scaled (1.0f / pixelScale));
    g.setOpacity (1.0f);
}

juce::String ratioText (float ratio)
{
    return juce::String::charToString (0x00d7) + juce::String (ratio, ratio < 10.0f ? 2 : 1);
}
} // namespace

// ---------------------------------------------------------------------------------------
ResonanceField::ResonanceField (ArcAudioProcessor& p, FieldModel& m) : processor (p), model (m)
{
    auto& state = processor.getValueTreeState();
    for (int n = 0; n < 4; ++n)
    {
        auto& np = nodeParams[(size_t) n];
        np.radius = state.getParameter (params::nodeId (n, "radius"));
        np.angle = state.getParameter (params::nodeId (n, "angle"));
        np.level = state.getParameter (params::nodeId (n, "level"));
        np.link = state.getParameter (params::nodeId (n, "link"));
        radiusAttach[(size_t) n] = std::make_unique<juce::ParameterAttachment> (*np.radius, [this] (float) { repaint(); });
        angleAttach[(size_t) n] = std::make_unique<juce::ParameterAttachment> (*np.angle, [this] (float) { repaint(); });
    }
    setOpaque (false);
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

ResonanceField::~ResonanceField() = default;

float ResonanceField::paramValue (const juce::String& id) const
{
    if (auto* v = processor.getValueTreeState().getRawParameterValue (id))
        return v->load (std::memory_order_relaxed);
    return 0.0f;
}

// ---------------------------------------------------------------------------------------
void ResonanceField::resized()
{
    const auto b = getLocalBounds().toFloat();
    chamber = gfx::superellipse (b, 3.6f);
    centre = b.getCentre().translated (0.0f, b.getHeight() * 0.03f);
    backdropLayer.invalidate();
    rx = b.getWidth() * 0.47f;
    ry = b.getHeight() * 0.44f;
    nodeR = b.getWidth() * 0.052f;
    coreR = b.getWidth() * 0.094f;
    staticLayer.invalidate();
    spriteScale = 0.0f;
}

bool ResonanceField::hitTest (int x, int y) { return chamber.contains ((float) x, (float) y); }

juce::Point<float> ResonanceField::fieldToScreen (float radiusParam, float angle) const
{
    const float dr = kInnerRadius + kRadiusSpan * juce::jlimit (0.0f, 1.0f, radiusParam);
    return { centre.x + dr * std::sin (angle) * rx, centre.y - dr * std::cos (angle) * ry };
}

void ResonanceField::screenToField (juce::Point<float> p, float& radiusParam, float& angleNorm) const
{
    const float nx = (p.x - centre.x) / rx, ny = (p.y - centre.y) / ry;
    const float d = std::sqrt (nx * nx + ny * ny);
    radiusParam = juce::jlimit (0.0f, 1.0f, (d - kInnerRadius) / kRadiusSpan);
    const float angle = std::atan2 (nx, -ny);
    angleNorm = fract (angle / twoPi + 0.5f);
}

juce::Point<float> ResonanceField::displayNodePosition (int n) const
{
    const auto u = (size_t) n;
    juce::Point<float> p;
    if (dragNode == n)
        p = fieldToScreen (dragRadius, normToAngle (dragAngle));
    else if (running)
        p = fieldToScreen (model.nodeRadius[u], model.nodeAngle[u]);
    else
        p = fieldToScreen (nodeParams[u].radius->convertFrom0to1 (nodeParams[u].radius->getValue()),
                           normToAngle (nodeParams[u].angle->convertFrom0to1 (nodeParams[u].angle->getValue())));
    return p + tremor[u];
}

juce::Point<float> ResonanceField::nodeCentre (int node) const { return displayNodePosition (node); }

int ResonanceField::hitNode (juce::Point<float> p) const
{
    int best = -1;
    float bestD = nodeR * 1.35f;
    for (int n = 0; n < 4; ++n)
    {
        const float d = p.getDistanceFrom (displayNodePosition (n));
        if (d < bestD)
        {
            bestD = d;
            best = n;
        }
    }
    return best;
}

bool ResonanceField::hitCore (juce::Point<float> p) const { return p.getDistanceFrom (centre) < coreR * 1.05f; }

// ---------------------------------------------------------------------------------------
void ResonanceField::setCaption (const juce::String& title, const juce::String& body)
{
    if (title == captionTitle && body == captionBody)
        return;
    prevTitle = captionTitle;
    prevBody = captionBody;
    captionTitle = title;
    captionBody = body;
    captionFade = 0.0f;
}

void ResonanceField::setSelection (Selection s, bool notify)
{
    if (s == selection)
        return;
    selection = s;
    if (notify && onSelectionChanged)
        onSelectionChanged (selection);
    repaint();
}

void ResonanceField::setRecordArmed (int node, bool armed)
{
    armedNode = armed ? node : (armedNode == node ? -1 : armedNode);
    if (onGestureStateChanged)
        onGestureStateChanged();
    repaint();
}

void ResonanceField::advance (float dt, bool engineRunning)
{
    time += dt;
    running = engineRunning;
    captionFade = juce::jmin (1.0f, captionFade + dt / 0.2f);
    const float chaos = paramValue (params::chaos);
    for (size_t n = 0; n < 4; ++n)
    {
        const bool hov = hoverNode == (int) n || dragNode == (int) n;
        const bool sel = selection.target == Target::node && selection.node == (int) n;
        hoverAnim[n] = smoothTowards (hoverAnim[n], hov ? 1.0f : 0.0f, dt, 0.08f);
        selectAnim[n] = smoothTowards (selectAnim[n], sel ? 1.0f : 0.0f, dt, 0.1f);
        // CHAOS: a small, bounded, smooth tremor of the drawn nodes.
        if (rng.nextFloat() < dt * 3.0f)
            tremorTarget[n] = { (rng.nextFloat() * 2.0f - 1.0f) * chaos * nodeR * 0.12f,
                                (rng.nextFloat() * 2.0f - 1.0f) * chaos * nodeR * 0.12f };
        tremor[n] = { smoothTowards (tremor[n].x, tremorTarget[n].x, dt, 0.25f), smoothTowards (tremor[n].y, tremorTarget[n].y, dt, 0.25f) };
    }
    coreHover = smoothTowards (coreHover, hoverCore ? 1.0f : 0.0f, dt, 0.08f);
    coreSelect = smoothTowards (coreSelect, selection.target == Target::core ? 1.0f : 0.0f, dt, 0.1f);
}

// ---------------------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------------------
void ResonanceField::ensureSprites (float scale)
{
    if (std::abs (scale - spriteScale) < 0.01f && nodeSprite.isValid())
        return;
    spriteScale = scale;
    // Everything at device resolution: blitted 1:1 each frame, never resampled.
    nodeSprite = makeSphere (juce::roundToInt (nodeR * 2.0f * scale), 0xA11CEu, 22, false, colours::cyan, 0.55f);
    coreSprite = makeSphere (juce::roundToInt (coreR * 2.0f * scale), 0xC0DEu, 38, false, colours::cyan, 0.35f);
    junctionSprite = makeSphere (juce::roundToInt (nodeR * 0.62f * scale), 0x5EEDu, 0, true, colours::cyan, 0.15f);
    haloSprite = makeHalo (juce::roundToInt (nodeR * 3.6f * scale), colours::cyan);
    coreHaloSprite = makeHalo (juce::roundToInt (coreR * 3.4f * scale), colours::cyan);
    junctionHaloSprite = makeHalo (juce::roundToInt (nodeR * 1.8f * scale), colours::cyan);
}

void ResonanceField::setBackdrop (std::function<void (juce::Graphics&)> renderer)
{
    backdrop = std::move (renderer);
    setOpaque (backdrop != nullptr);
    staticLayer.setOpaque (backdrop != nullptr);
    backdropLayer.invalidate();
    staticLayer.invalidate();
}

void ResonanceField::renderStatic (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    if (backdrop)
        backdropLayer.draw (g, getLocalBounds(), [this] (juce::Graphics& bg) { backdrop (bg); });
    juce::ColourGradient bg (juce::Colour (0xff111922), centre.x, centre.y, colours::chamberDeep, b.getRight(), b.getBottom(), true);
    bg.addColour (0.55, juce::Colour (0xff0a0f14));
    g.setGradientFill (bg);
    g.fillPath (chamber);

    // Deep blue bloom behind the core.
    juce::ColourGradient bloom (juce::Colour (0xff17324a).withAlpha (0.55f), centre.x, centre.y,
                                juce::Colour (0xff17324a).withAlpha (0.0f), centre.x + rx * 0.75f, centre.y, true);
    g.setGradientFill (bloom);
    g.fillEllipse (juce::Rectangle<float> (rx * 1.5f, ry * 1.5f).withCentre (centre));

    paintRings (g, cachedTension);

    // Recessed edge: the chamber sinks into the chrome.
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (chamber, juce::PathStrokeType (10.0f));
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.strokePath (chamber, juce::PathStrokeType (26.0f));

    // Glass reflection: a faint broad arc across the upper left.
    {
        juce::Path refl;
        refl.addCentredArc (b.getX() + b.getWidth() * 0.62f, b.getY() + b.getHeight() * 0.95f, b.getWidth() * 0.62f,
                            b.getHeight() * 0.9f, 0.0f, -1.45f, -0.35f, true);
        g.setColour (juce::Colours::white.withAlpha (0.028f));
        g.strokePath (refl, juce::PathStrokeType (b.getHeight() * 0.1f));
    }
}

void ResonanceField::paintRings (juce::Graphics& g, float tension)
{
    const float inner = 0.3f * (1.1f - 0.2f * tension);
    const float mid = 0.64f * (0.94f + 0.12f * tension);
    const float outer = 0.98f * (0.97f + 0.06f * tension);

    auto ellipse = [&] (float k) { return juce::Rectangle<float> (rx * 2.0f * k, ry * 2.0f * k).withCentre (centre); };
    g.setColour (colours::glassFaint.withAlpha (0.42f));
    g.drawEllipse (ellipse (inner), 0.8f);
    g.setColour (colours::glassFaint.withAlpha (0.22f));
    g.drawEllipse (ellipse (outer), 0.7f);

    // Dotted mid ring
    const int dots = 132;
    for (int i = 0; i < dots; ++i)
    {
        const float a = twoPi * (float) i / (float) dots;
        const juce::Point<float> p (centre.x + mid * rx * std::sin (a), centre.y - mid * ry * std::cos (a));
        g.setColour (colours::glassMuted.withAlpha (0.35f));
        g.fillEllipse (juce::Rectangle<float> (1.4f, 1.4f).withCentre (p));
    }

    // Crosshair with fading ends
    const float ext = 1.02f;
    auto line = [&] (juce::Point<float> a, juce::Point<float> c2)
    {
        juce::ColourGradient cg (colours::glassMuted.withAlpha (0.0f), a.x, a.y, colours::glassMuted.withAlpha (0.0f), c2.x, c2.y, false);
        cg.addColour (0.18, colours::glassMuted.withAlpha (0.35f));
        cg.addColour (0.5, colours::glassMuted.withAlpha (0.12f));
        cg.addColour (0.82, colours::glassMuted.withAlpha (0.35f));
        g.setGradientFill (cg);
        g.drawLine ({ a, c2 }, 0.8f);
    };
    line ({ centre.x, centre.y - ry * ext }, { centre.x, centre.y + ry * ext });
    line ({ centre.x - rx * ext, centre.y }, { centre.x + rx * ext, centre.y });
    // Axis end glints (cyan), as in the reference.
    for (auto p : { juce::Point<float> (centre.x, centre.y - ry * 0.99f), juce::Point<float> (centre.x, centre.y + ry * 0.99f) })
    {
        g.setColour (colours::cyan.withAlpha (0.12f));
        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (p));
        g.setColour (colours::cyanBright.withAlpha (0.55f));
        g.fillEllipse (juce::Rectangle<float> (2.2f, 2.2f).withCentre (p));
    }
}

void ResonanceField::paintHeader (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto titleArea = juce::Rectangle<float> (b.getX(), b.getY() + b.getHeight() * 0.055f, b.getWidth(), 22.0f);
    const auto bodyArea = titleArea.translated (0.0f, 24.0f).withHeight (16.0f);
    const auto defaultTitle = juce::String ("RESONANCE FIELD");
    const auto defaultBody = juce::String::fromUTF8 ("DRAG NODES  \xc2\xb7  SHAPE RESONANCE  \xc2\xb7  CREATE MOTION");
    const auto frozenBody = juce::String::fromUTF8 ("FROZEN  \xc2\xb7  THE NETWORK HOLDS ITS ENERGY");

    auto draw = [&] (const juce::String& t, const juce::String& body, float alpha)
    {
        if (alpha <= 0.001f)
            return;
        const auto title = t.isNotEmpty() ? t : defaultTitle;
        const auto sub = t.isNotEmpty() ? body : (model.freeze > 0.5f ? frozenBody : defaultBody);
        g.setColour (colours::glassText.withAlpha (0.92f * alpha));
        drawTrackedText (g, title.toUpperCase(), titleArea, Fonts::regular (17.0f, 0.3f), juce::Justification::centred);
        g.setColour (colours::glassMuted.withAlpha (0.95f * alpha));
        drawTrackedText (g, sub.toUpperCase(), bodyArea, Fonts::regular (10.0f, 0.2f), juce::Justification::centred);
    };
    const float f = easeOutCubic (captionFade);
    draw (prevTitle, prevBody, 1.0f - f);
    draw (captionTitle, captionBody, f);
}

void ResonanceField::paintSideReadouts (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float lineH = 18.0f;

    // Left: live network state (the reference's STABILITY / FLOW / HARMONICS, made real).
    float flow = 0.0f;
    for (auto e : model.edgeGlow)
        flow = juce::jmax (flow, e);
    float harmonic = 0.0f;
    for (auto r : model.nodeRatio)
    {
        const float nearest = juce::jmax (1.0f, std::round (r * 2.0f) * 0.5f);
        harmonic += 1.0f - juce::jlimit (0.0f, 1.0f, std::abs (r - nearest) / 0.08f);
    }
    harmonic *= 0.25f;
    const float stability = 1.0f - paramValue (params::chaos);
    const std::array<std::pair<const char*, float>, 3> rows { { { "STABILITY", stability }, { "FLOW", flow }, { "HARMONICS", harmonic } } };
    const float lx = b.getX() + b.getWidth() * 0.07f;
    float y = centre.y - lineH * 1.5f;
    for (auto& [label, value] : rows)
    {
        g.setColour (colours::glassMuted.withAlpha (0.8f));
        drawTrackedText (g, label, { lx, y, 90.0f, lineH }, Fonts::regular (9.5f, 0.22f), juce::Justification::centredLeft);
        for (int i = 0; i < 5; ++i)
        {
            const bool lit = value > ((float) i + 0.5f) / 5.0f;
            const auto dot = juce::Rectangle<float> (3.0f, 3.0f).withCentre ({ lx + 72.0f + (float) i * 6.5f, y + lineH * 0.5f });
            g.setColour (lit ? colours::cyan.withAlpha (0.85f) : colours::glassFaint.withAlpha (0.6f));
            g.fillEllipse (dot);
        }
        y += lineH;
    }

    // Right: the hovered / selected node, or interaction hints.
    int info = hoverNode >= 0 ? hoverNode : (selection.target == Target::node ? selection.node : -1);
    const float rxPos = b.getRight() - b.getWidth() * 0.07f - 110.0f;
    y = centre.y - lineH * 1.5f;
    std::array<juce::String, 3> lines;
    if (info >= 0)
    {
        const auto u = (size_t) info;
        lines = { juce::String ("NODE ") + kNodeNames[u], "RATIO  " + ratioText (model.nodeRatio[u]),
                  "LEVEL  " + percent (nodeParams[u].level->getValue()) };
    }
    else if (hoverCore || selection.target == Target::core)
        lines = { "CORE", "THE NOTE YOU PLAY", juce::String::fromUTF8 ("CLICK  \xc2\xb7  NETWORK") };
    else
        lines = { juce::String::fromUTF8 ("DRAG  \xc2\xb7  RETUNE"), juce::String::fromUTF8 ("ALT-DRAG  \xc2\xb7  RECORD"),
                  juce::String::fromUTF8 ("CLICK  \xc2\xb7  INSPECT") };
    for (auto& l : lines)
    {
        g.setColour (colours::glassMuted.withAlpha (info >= 0 ? 0.95f : 0.7f));
        drawTrackedText (g, l, { rxPos, y, 110.0f, lineH },
                         Fonts::regular (9.5f, 0.22f), juce::Justification::centredRight);
        y += lineH;
    }
}

void ResonanceField::paintMotionPaths (juce::Graphics& g, const std::array<juce::Point<float>, 4>& pos)
{
    juce::ignoreUnused (pos);
    for (int n = 0; n < 4; ++n)
    {
        const auto u = (size_t) n;
        const float baseR = nodeParams[u].radius->convertFrom0to1 (nodeParams[u].radius->getValue());
        const float baseA = normToAngle (nodeParams[u].angle->convertFrom0to1 (nodeParams[u].angle->getValue()));
        if (recordingNode == n && recordSamples.size() > 1)
        {
            juce::Path live;
            for (size_t i = 0; i < recordSamples.size(); ++i)
            {
                const auto p = fieldToScreen (recordSamples[i].radius, recordSamples[i].angle);
                if (i == 0)
                    live.startNewSubPath (p);
                else
                    live.lineTo (p);
            }
            strokeGlow (g, live, colours::amber, 1.4f, 0.55f, 0.6f);
            continue;
        }
        const auto gesture = processor.getGesture (n);
        if (! gesture.valid)
            continue;
        juce::Path path;
        const int steps = 96;
        for (int i = 0; i <= steps; ++i)
        {
            float dr, da;
            gesture.sample ((double) i / steps, dr, da);
            const auto p = fieldToScreen (baseR + dr, baseA + da);
            if (i == 0)
                path.startNewSubPath (p);
            else
                path.lineTo (p);
        }
        juce::Path dashed;
        const float dashes[] = { 2.0f, 5.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, path, dashes, 2);
        g.setColour (colours::cyan.withAlpha (0.45f));
        g.fillPath (dashed);
    }
}

void ResonanceField::paintConnections (juce::Graphics& g, const std::array<juce::Point<float>, 4>& pos)
{
    const auto topology = static_cast<dsp::Topology> (juce::roundToInt (paramValue (params::topology)));
    const auto mask = dsp::topologyMask (topology);
    const bool frozen = model.freeze > 0.5f;

    auto nodePos = [&] (int idx) { return idx == 0 ? centre : pos[(size_t) (idx - 1)]; };
    auto radiusOf = [&] (int idx) { return idx == 0 ? coreR : nodeR; };

    for (int e = 0; e < dsp::kNumEdges; ++e)
    {
        const auto u = (size_t) e;
        const float strength = model.edgeStrength[u];
        const bool inTopology = mask[u] > 0.0f;
        if (! inTopology && strength < 0.02f)
            continue;
        const auto& edge = dsp::kEdges[u];
        const auto a = nodePos (edge.a), b = nodePos (edge.b);
        const float glow = model.edgeGlow[u] * (frozen ? 0.6f : 1.0f);
        const float base = 0.1f + 0.4f * juce::jlimit (0.0f, 1.0f, strength * 1.6f);

        if (e < 4)
        {
            // Spoke: a lens of two strands, CORE -> node.
            const auto d = b - a;
            const float len = d.getDistanceFromOrigin();
            if (len < 1.0f)
                continue;
            const auto dir = d / len;
            const juce::Point<float> perp (-dir.y, dir.x);
            const auto p0 = a + dir * (radiusOf (edge.a) * 0.92f), p3 = b - dir * (radiusOf (edge.b) * 0.95f);
            for (float side : { -1.0f, 1.0f })
            {
                const float bend = side * len * (0.1f + 0.05f * strength);
                juce::Path strand;
                strand.startNewSubPath (p0);
                strand.cubicTo (p0 + dir * (len * 0.3f) + perp * bend, p3 - dir * (len * 0.3f) + perp * bend, p3);
                strokeGlow (g, strand, colours::cyan, 1.1f, base * 0.8f, glow);
            }
        }
        else if (e < 8)
        {
            // Ring edge through its junction point.
            const auto mid = (a + b) * 0.5f;
            const auto j = centre + (mid - centre) * kJunction;
            const auto control = j * 2.0f - mid;
            const auto dirA = (control - a) / juce::jmax (1.0f, control.getDistanceFrom (a));
            const auto dirB = (control - b) / juce::jmax (1.0f, control.getDistanceFrom (b));
            juce::Path arc;
            arc.startNewSubPath (a + dirA * nodeR);
            arc.quadraticTo (control, b + dirB * nodeR);
            strokeGlow (g, arc, colours::cyan, 1.0f, inTopology ? base * 0.75f : base * 0.35f, glow);
        }
        else
        {
            // Cross edge: passes behind the CORE.
            juce::Path cross;
            const auto d = (b - a) / juce::jmax (1.0f, b.getDistanceFrom (a));
            cross.startNewSubPath (a + d * nodeR);
            cross.lineTo (b - d * nodeR);
            strokeGlow (g, cross, colours::blue, 0.9f, base * 0.5f, glow * 0.7f);
        }

        // Flow pulses travelling along the edge (spokes: outward along the lens axis;
        // ring edges: along the arc through the junction).
        if (glow > 0.06f && ! frozen)
        {
            const auto mid = (a + b) * 0.5f;
            const auto control = (centre + (mid - centre) * kJunction) * 2.0f - mid;
            for (int k = 0; k < 2; ++k)
            {
                const float t = fract (model.flowPhase[u] + 0.5f * (float) k);
                juce::Point<float> p;
                if (e >= 4 && e < 8)
                {
                    const float s1 = 1.0f - t;
                    p = a * (s1 * s1) + control * (2.0f * s1 * t) + b * (t * t);
                }
                else
                    p = a + (b - a) * t;
                const float fade = std::sin (t * juce::MathConstants<float>::pi);
                g.setColour (colours::cyan.withAlpha (0.18f * glow * fade));
                g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (p));
                g.setColour (colours::ice.withAlpha (0.85f * glow * fade));
                g.fillEllipse (juce::Rectangle<float> (2.6f, 2.6f).withCentre (p));
            }
        }
    }

    // Junctions (ring-edge midpoints): steel beads that light with the ring's flow.
    for (int e = 4; e < 8; ++e)
    {
        const auto& edge = dsp::kEdges[(size_t) e];
        const auto mid = (nodePos (edge.a) + nodePos (edge.b)) * 0.5f;
        const auto j = centre + (mid - centre) * kJunction;
        const float glow = model.edgeGlow[(size_t) e] * (mask[(size_t) e] > 0.0f ? 1.0f : 0.3f);
        if (glow > 0.02f)
            blit (g, junctionHaloSprite, j, spriteScale, 0.6f * glow);
        blit (g, junctionSprite, j, spriteScale, mask[(size_t) e] > 0.0f ? 1.0f : 0.55f);
    }
}

void ResonanceField::paintPulses (juce::Graphics& g)
{
    for (auto& p : model.pulses)
    {
        const float life = 1.3f;
        if (p.age >= life)
            continue;
        const float t = p.age / life;
        const float k = coreR / rx + (1.0f - coreR / rx) * easeOutCubic (t);
        const float alpha = p.strength * (1.0f - t) * (1.0f - t);
        const auto r = juce::Rectangle<float> (rx * 2.0f * k, ry * 2.0f * k).withCentre (centre);
        g.setColour (colours::cyan.withAlpha (0.12f * alpha));
        g.drawEllipse (r, 6.0f);
        g.setColour (colours::cyanBright.withAlpha (0.55f * alpha));
        g.drawEllipse (r, 1.1f);
    }
}

void ResonanceField::paintNode (juce::Graphics& g, int n, juce::Point<float> p)
{
    const auto u = (size_t) n;
    const float glow = model.nodeGlow[u + 1];
    const float hover = hoverAnim[u], sel = selectAnim[u];
    const float size = nodeR * 2.0f;

    blit (g, haloSprite, p, spriteScale, 0.08f + 0.8f * glow);
    blit (g, nodeSprite, p, spriteScale);

    // Rim light
    const auto rim = juce::Rectangle<float> (size, size).withCentre (p);
    g.setColour (colours::cyan.withAlpha (0.28f + 0.6f * glow + 0.2f * hover));
    g.drawEllipse (rim.reduced (0.6f), 1.2f);
    g.setColour (colours::cyan.withAlpha (0.08f + 0.2f * glow));
    g.drawEllipse (rim.expanded (2.5f), 3.0f);

    // Label
    g.setColour (colours::glassText.withAlpha (0.96f));
    g.setFont (Fonts::regular (nodeR * 0.62f));
    g.drawText (kNodeNames[u], rim.translated (0.0f, -nodeR * 0.02f), juce::Justification::centred, false);

    // Selection: a fine ring with four ticks.
    if (sel > 0.01f)
    {
        const float rr = nodeR * (1.38f + 0.1f * (1.0f - sel));
        g.setColour (colours::cyanBright.withAlpha (0.85f * sel));
        g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (p), 1.0f);
        for (int k = 0; k < 4; ++k)
        {
            const float a = (float) k * juce::MathConstants<float>::halfPi + (float) time * 0.25f;
            const juce::Point<float> d (std::sin (a), -std::cos (a));
            g.drawLine ({ p + d * (rr + 2.0f), p + d * (rr + 7.0f) }, 1.2f);
        }
    }

    // Gesture badge: an orbiting spark shows the node carries a recorded motion.
    if (processor.getGesture (n).valid)
    {
        const float a = twoPi * processor.getEngine().getMotion().gesturePhase (n);
        const float rr = nodeR * 1.18f;
        const juce::Point<float> sp (p.x + rr * std::sin (a), p.y - rr * std::cos (a));
        g.setColour (colours::cyan.withAlpha (0.25f));
        g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (sp));
        g.setColour (colours::ice);
        g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (sp));
    }

    // Recording (armed: breathing amber ring; recording: solid).
    if (armedNode == n || recordingNode == n)
    {
        const float pulse = recordingNode == n ? 1.0f : 0.5f + 0.5f * std::sin ((float) time * 5.0f);
        g.setColour (colours::amber.withAlpha (0.35f + 0.55f * pulse));
        g.drawEllipse (rim.expanded (6.0f), 1.6f);
    }
}

void ResonanceField::paintCore (juce::Graphics& g)
{
    const float glow = model.nodeGlow[0];
    blit (g, coreHaloSprite, centre, spriteScale, 0.16f + 0.75f * glow);
    const float size = coreR * 2.0f;
    blit (g, coreSprite, centre, spriteScale);
    const auto rim = juce::Rectangle<float> (size, size).withCentre (centre);
    g.setColour (colours::cyan.withAlpha (0.18f + 0.5f * glow + 0.15f * coreHover));
    g.drawEllipse (rim.reduced (0.6f), 1.3f);

    gfx::drawCoreEmblem (g, centre, coreR * 0.34f, colours::cyan.withAlpha (0.2f + 0.3f * glow), 5.0f);
    gfx::drawCoreEmblem (g, centre, coreR * 0.34f, colours::glassText.withAlpha (0.85f), 2.0f);

    if (coreSelect > 0.01f)
    {
        g.setColour (colours::cyanBright.withAlpha (0.8f * coreSelect));
        g.drawEllipse (rim.expanded (7.0f), 1.0f);
    }
}

void ResonanceField::paintFreeze (juce::Graphics& g)
{
    const float f = model.freeze;
    if (f < 0.01f)
        return;
    g.setColour (colours::ice.withAlpha (0.045f * f));
    g.fillPath (chamber);
    // A crystalline ring around the CORE: the network's energy, suspended.
    const float rr = coreR * 1.55f;
    juce::Path crystal;
    for (int i = 0; i < 24; ++i)
    {
        const float a = twoPi * (float) i / 24.0f;
        const float len = (i % 2 == 0 ? 7.0f : 3.5f) * f;
        const juce::Point<float> d (std::sin (a), -std::cos (a));
        crystal.startNewSubPath (centre + d * rr);
        crystal.lineTo (centre + d * (rr + len));
    }
    g.setColour (colours::ice.withAlpha (0.65f * f));
    g.strokePath (crystal, juce::PathStrokeType (1.0f));
    g.setColour (colours::ice.withAlpha (0.3f * f));
    g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (centre), 1.0f);
}

void ResonanceField::paint (juce::Graphics& g)
{
    ensureSprites (juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor()));
    g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);

    // Static layer (chamber, rings, crosshair, recess) follows TENSION in 1/200 steps.
    const float tension = std::round (paramValue (params::tension) * 200.0f) / 200.0f;
    if (! juce::approximatelyEqual (tension, cachedTension))
    {
        cachedTension = tension;
        staticLayer.invalidate();
    }

    // The static layer is transparent outside the chamber: blit it unclipped (fast
    // path), then clip the live content to the chamber.
    staticLayer.draw (g, getLocalBounds(), [this] (juce::Graphics& sg) { renderStatic (sg); });
    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (chamber);
    paintPulses (g);

    std::array<juce::Point<float>, 4> pos;
    for (int n = 0; n < 4; ++n)
        pos[(size_t) n] = displayNodePosition (n);

    paintMotionPaths (g, pos);
    paintConnections (g, pos);
    paintCore (g);
    for (int n = 0; n < 4; ++n)
        paintNode (g, n, pos[(size_t) n]);
    paintFreeze (g);
    paintHeader (g);
    paintSideReadouts (g);
}

// ---------------------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------------------
void ResonanceField::mouseMove (const juce::MouseEvent& e)
{
    const int n = hitNode (e.position);
    const bool c = n < 0 && hitCore (e.position);
    if (n != hoverNode || c != hoverCore)
    {
        hoverNode = n;
        hoverCore = c;
        setMouseCursor (n >= 0 ? juce::MouseCursor::DraggingHandCursor
                               : c ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }
}

void ResonanceField::mouseExit (const juce::MouseEvent&)
{
    hoverNode = -1;
    hoverCore = false;
}

void ResonanceField::mouseDown (const juce::MouseEvent& e)
{
    const int n = hitNode (e.position);
    if (n >= 0)
    {
        const auto u = (size_t) n;
        // Grab offset from where the node is drawn *now* (before dragNode switches the
        // display to the drag values).
        dragOffset = displayNodePosition (n) - e.position;
        dragRadius = nodeParams[u].radius->convertFrom0to1 (nodeParams[u].radius->getValue());
        dragAngle = nodeParams[u].angle->convertFrom0to1 (nodeParams[u].angle->getValue());
        dragNode = n;
        radiusAttach[u]->beginGesture();
        angleAttach[u]->beginGesture();
        if (e.mods.isAltDown() || armedNode == n)
        {
            recordingNode = n;
            recordStart = juce::Time::getMillisecondCounterHiRes() * 0.001;
            recordStartRadius = dragRadius;
            recordStartAngle = dragAngle;
            recordSamples.clear();
            recordSamples.push_back ({ 0.0, dragRadius, normToAngle (dragAngle) });
            if (onGestureStateChanged)
                onGestureStateChanged();
        }
        setSelection ({ Target::node, n }, true);
        return;
    }
    if (hitCore (e.position))
    {
        setSelection (selection.target == Target::core ? Selection {} : Selection { Target::core, -1 }, true);
        return;
    }
    setSelection ({}, true);
}

void ResonanceField::mouseDrag (const juce::MouseEvent& e)
{
    if (dragNode < 0)
        return;
    const auto u = (size_t) dragNode;
    float r, a;
    screenToField (e.position + dragOffset, r, a);
    if (e.mods.isShiftDown())
    {
        // Fine: move a tenth as far.
        r = dragRadius + 0.1f * (r - dragRadius);
        float da = a - dragAngle;
        da -= std::round (da);
        a = fract (dragAngle + 0.1f * da);
    }
    dragRadius = r;
    dragAngle = a;
    radiusAttach[u]->setValueAsPartOfGesture (r);
    angleAttach[u]->setValueAsPartOfGesture (a);
    if (recordingNode == dragNode)
        recordSamples.push_back ({ juce::Time::getMillisecondCounterHiRes() * 0.001 - recordStart, r, normToAngle (a) });
    repaint();
}

void ResonanceField::mouseUp (const juce::MouseEvent&)
{
    if (dragNode < 0)
        return;
    const auto u = (size_t) dragNode;
    radiusAttach[u]->endGesture();
    angleAttach[u]->endGesture();

    if (recordingNode == dragNode)
    {
        const bool synced = paramValue (params::sync) > 0.5f;
        const auto g = buildGesture (recordSamples, synced ? model.bpm / 60.0 : 0.0, model.beatsPerBar);
        if (g.valid)
        {
            // The loop plays around where recording started.
            radiusAttach[u]->setValueAsCompleteGesture (recordStartRadius);
            angleAttach[u]->setValueAsCompleteGesture (recordStartAngle);
            processor.setGesture (dragNode, g);
        }
        recordingNode = -1;
        armedNode = -1;
        recordSamples.clear();
        if (onGestureStateChanged)
            onGestureStateChanged();
    }
    dragNode = -1;
    repaint();
}

void ResonanceField::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int n = hitNode (e.position);
    if (n < 0)
        return;
    const auto u = (size_t) n;
    radiusAttach[u]->setValueAsCompleteGesture (nodeParams[u].radius->convertFrom0to1 (nodeParams[u].radius->getDefaultValue()));
    angleAttach[u]->setValueAsCompleteGesture (nodeParams[u].angle->convertFrom0to1 (nodeParams[u].angle->getDefaultValue()));
}

juce::String ResonanceField::getTooltip()
{
    if (hoverNode >= 0)
        return juce::String ("NODE ") + kNodeNames[(size_t) hoverNode]
               + juce::String::fromUTF8 ("\nDRAG TO MOVE AND RETUNE  \xc2\xb7  ALT-DRAG RECORDS MOTION  \xc2\xb7  DOUBLE-CLICK RESETS");
    if (hoverCore)
        return juce::String::fromUTF8 ("CORE\nTHE DRIVEN RESONATOR  \xc2\xb7  CLICK FOR NETWORK AND SPACE");
    return {};
}

} // namespace arc::ui

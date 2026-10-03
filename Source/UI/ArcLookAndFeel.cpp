#include "UI/ArcLookAndFeel.h"

#include "Graphics/SilverSurface.h"

#include <algorithm>
#include <vector>

namespace arc::ui
{

namespace
{
const juce::Identifier kStyleId ("arcKnobStyle");
}

ArcLookAndFeel::ArcLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, colours::graphite);
    setColour (juce::PopupMenu::textColourId, colours::glassText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::graphiteHigh);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::cyanBright);
    setColour (juce::TooltipWindow::backgroundColourId, colours::graphite);
    setColour (juce::TooltipWindow::textColourId, colours::glassText);
    setColour (juce::TextEditor::textColourId, colours::glassText);
    setColour (juce::TextEditor::highlightColourId, colours::cyan.withAlpha (0.3f));
    setColour (juce::TextEditor::highlightedTextColourId, colours::ice);
    setColour (juce::CaretComponent::caretColourId, colours::cyan);
    setColour (juce::Label::textColourId, colours::ink);
    setColour (juce::ScrollBar::thumbColourId, colours::glassFaint);
    setColour (juce::ResizableWindow::backgroundColourId, colours::chassisMid);
}

void ArcLookAndFeel::setKnobStyle (juce::Slider& s, KnobStyle style)
{
    s.getProperties().set (kStyleId, static_cast<int> (style));
}

KnobStyle ArcLookAndFeel::getKnobStyle (const juce::Slider& s)
{
    return static_cast<KnobStyle> (static_cast<int> (s.getProperties().getWithDefault (kStyleId, static_cast<int> (KnobStyle::small))));
}

namespace
{
float smoothstep (float edge0, float edge1, float x) noexcept
{
    const float t = juce::jlimit (0.0f, 1.0f, (x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

float hash01 (int i) noexcept
{
    auto h = (uint32_t) i * 2654435761u;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return (float) (h & 0xffffu) / 65535.0f;
}

struct KnobGeometry
{
    float ringR, faceR, trackR, trackW;
};

KnobGeometry knobGeometry (KnobStyle style, float outer) noexcept
{
    if (style == KnobStyle::macro)
        return { outer * 0.74f, outer * 0.74f * 0.8f, outer * 0.9f, juce::jmax (2.0f, outer * 0.04f) };
    if (style == KnobStyle::master)
        return { outer * 0.72f, outer * 0.72f * 0.76f, outer * 0.895f, juce::jmax (1.8f, outer * 0.06f) };
    return { outer * 0.72f, outer * 0.72f * 0.76f, outer * 0.895f, juce::jmax (1.6f, outer * 0.064f) };
}

/** The knob's static body at device resolution: contact shadow, turned chrome ring
    (conic reflections, concentric turning marks), dark anodized face. Rendered per pixel
    once per style and size, then blitted 1:1. */
juce::Image renderKnobBody (KnobStyle style, int ringDiameterPx)
{
    const bool glass = style == KnobStyle::glass;
    const float R = (float) ringDiameterPx * 0.5f;
    const float faceFrac = style == KnobStyle::macro ? 0.8f : 0.76f;
    const float extent = glass ? 1.1f : 1.32f; // room for the shadow
    const int size = (int) std::ceil (R * 2.0f * extent) + 2;
    juce::Image img (juce::Image::ARGB, size, size, true);
    juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
    const float half = (float) size * 0.5f;
    const float px = 1.0f / R;           // one device pixel, in ring radii
    const float keyLight = -0.785f;      // reflections line up with the upper-left key light
    const float halfPi = juce::MathConstants<float>::halfPi;

    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const float dx = ((float) x + 0.5f - half) / R, dy = ((float) y + 0.5f - half) / R;
            const float r = std::sqrt (dx * dx + dy * dy);

            // Shadow: soft, below (on silver); a tight dark halo on glass.
            const float sdy = dy - (glass ? 0.05f : 0.12f);
            const float rs = std::sqrt (dx * dx + sdy * sdy);
            const float shadow = glass ? 0.32f * smoothstep (1.06f, 0.92f, rs) : 0.4f * smoothstep (1.26f, 0.7f, rs);
            const float cover = juce::jlimit (0.0f, 1.0f, (1.0f - r) * R + 0.5f);
            if (cover <= 0.0f && shadow <= 0.002f)
                continue;

            float L = 0.0f, faceMix = 0.0f;
            if (cover > 0.0f)
            {
                const float theta = std::atan2 (dx, -dy);
                const float up = -dy; // +1 at the top
                const float lobe = std::pow (std::abs (std::cos (theta - keyLight)), 10.0f);
                const float lobe2 = std::pow (std::abs (std::cos (theta - keyLight - halfPi)), 16.0f);
                const float turning = hash01 ((int) (r * R)) - 0.5f; // concentric marks, 1 px apart

                // Turned chrome ring.
                const float rp = juce::jmax (0.0f, (r - faceFrac) / (1.0f - faceFrac));
                const float lip = 1.0f - smoothstep (0.0f, 2.2f * px / (1.0f - faceFrac), rp); // dark seat of the face
                float ring = glass ? 0.24f + 0.22f * lobe * (0.6f + 0.4f * up) + 0.05f * lobe2 + 0.06f * up
                                   : 0.6f + 0.32f * lobe * (0.65f + 0.35f * up) + 0.1f * lobe2 + 0.1f * up;
                ring += (glass ? 0.02f : 0.035f) * turning * (1.0f - lip);
                ring -= 0.12f * std::pow (rp, 5.0f); // rolls off at the rim
                ring += 0.12f * juce::jmax (0.0f, up) * smoothstep (0.6f, 0.9f, rp) * (1.0f - smoothstep (0.9f, 1.0f, rp));
                ring *= 1.0f - 0.8f * lip;
                ring *= 1.0f - (glass ? 0.3f : 0.45f) * smoothstep (1.0f - 1.5f * px, 1.0f, r); // crisp outer edge

                // Dark anodized face, turned like the ring.
                const float rf = juce::jmin (1.0f, r / faceFrac);
                const float edge = smoothstep (1.0f - 3.0f * px / faceFrac, 1.0f, rf);
                float face = 0.068f + 0.045f * lobe * (0.55f + 0.45f * up);
                const float hx = dx / (0.62f * faceFrac), hy = (dy + 0.48f * faceFrac) / (0.34f * faceFrac);
                face += 0.07f * std::exp (-(hx * hx + hy * hy) * 1.6f); // soft top highlight
                face += 0.016f * turning * (1.0f - edge);
                face -= 0.035f * std::pow (rf, 8.0f);
                face += 0.07f * juce::jmax (0.0f, -up) * edge; // lower bevel catches light

                faceMix = juce::jlimit (0.0f, 1.0f, (faceFrac - r) * R + 0.5f); // anti-aliased seat
                L = ring + (face - ring) * faceMix;
            }
            // Body over shadow (ring tinted cool, face a touch bluer).
            const float tr = 0.965f + (0.92f - 0.965f) * faceMix, tg = 0.98f + (0.97f - 0.98f) * faceMix, tb = 1.0f + 0.08f * faceMix;
            const float sa = shadow * (1.0f - cover);
            const float a = cover + sa;
            const float shade = glass ? 0.0f : 0.17f;
            const auto c = juce::Colour::fromFloatRGBA (juce::jlimit (0.0f, 1.0f, (L * tr * cover + shade * sa) / a),
                                                        juce::jlimit (0.0f, 1.0f, (L * tg * cover + shade * 1.08f * sa) / a),
                                                        juce::jlimit (0.0f, 1.0f, (L * tb * cover + shade * 1.2f * sa) / a), a);
            data.setPixelColour (x, y, c);
        }
    return img;
}

/** A round machined key (FREEZE / RANDOM / SYNC): chrome bevel around a satin
    aluminium cap, turned like the knobs; pressed, the cap reads concave. */
juce::Image renderButtonCap (bool pressed, int diameterPx)
{
    const float R = (float) diameterPx * 0.5f;
    const float capFrac = 0.85f;
    const int size = (int) std::ceil (R * 2.0f * 1.3f) + 2;
    juce::Image img (juce::Image::ARGB, size, size, true);
    juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
    const float half = (float) size * 0.5f;
    const float px = 1.0f / R;
    const float keyLight = -0.785f;
    const float halfPi = juce::MathConstants<float>::halfPi;
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
        {
            const float dx = ((float) x + 0.5f - half) / R, dy = ((float) y + 0.5f - half) / R;
            const float r = std::sqrt (dx * dx + dy * dy);
            const float sdy = dy - (pressed ? 0.06f : 0.12f);
            const float shadow = (pressed ? 0.3f : 0.36f) * smoothstep (pressed ? 1.12f : 1.24f, 0.72f, std::sqrt (dx * dx + sdy * sdy));
            const float cover = juce::jlimit (0.0f, 1.0f, (1.0f - r) * R + 0.5f);
            if (cover <= 0.0f && shadow <= 0.002f)
                continue;
            float L = 0.0f;
            if (cover > 0.0f)
            {
                const float theta = std::atan2 (dx, -dy);
                const float up = -dy;
                const float lobe = std::pow (std::abs (std::cos (theta - keyLight)), 10.0f);
                const float lobe2 = std::pow (std::abs (std::cos (theta - keyLight - halfPi)), 14.0f);
                const float turning = hash01 ((int) (r * R) + 31) - 0.5f;

                // Chrome bevel.
                const float rp = juce::jmax (0.0f, (r - capFrac) / (1.0f - capFrac));
                const float lip = 1.0f - smoothstep (0.0f, 2.0f * px / (1.0f - capFrac), rp);
                float bevel = 0.66f + 0.28f * lobe * (0.65f + 0.35f * up) + 0.12f * up + 0.03f * turning * (1.0f - lip);
                bevel -= 0.14f * std::pow (rp, 4.0f);
                bevel *= 1.0f - 0.38f * lip;
                bevel *= 1.0f - 0.4f * smoothstep (1.0f - 1.5f * px, 1.0f, r);

                // Satin cap.
                const float rc = juce::jmin (1.0f, r / capFrac);
                const float tilt = pressed ? -up : up; // pressed: the light falls the other way
                float cap = (pressed ? 0.75f : 0.8f) + 0.13f * lobe * (0.6f + 0.4f * tilt) + 0.04f * lobe2 + 0.05f * tilt
                            + 0.018f * turning * (1.0f - smoothstep (1.0f - 3.0f * px / capFrac, 1.0f, rc));
                const float hx = dx / (0.6f * capFrac), hy = (dy + (pressed ? -0.45f : 0.45f) * capFrac) / (0.35f * capFrac);
                cap += 0.05f * std::exp (-(hx * hx + hy * hy) * 1.6f);
                cap -= 0.05f * std::pow (rc, 10.0f);

                L = bevel + (cap - bevel) * juce::jlimit (0.0f, 1.0f, (capFrac - r) * R + 0.5f);
            }
            const float sa = shadow * (1.0f - cover);
            const float a = cover + sa;
            const auto c = juce::Colour::fromFloatRGBA (juce::jlimit (0.0f, 1.0f, (L * 0.965f * cover + 0.17f * sa) / a),
                                                        juce::jlimit (0.0f, 1.0f, (L * 0.98f * cover + 0.18f * sa) / a),
                                                        juce::jlimit (0.0f, 1.0f, (L * cover + 0.2f * sa) / a), a);
            data.setPixelColour (x, y, c);
        }
    return img;
}

/** Bodies are shared by every control of a kind and size (a few dozen pixels each way);
    the cache keeps the 32 most recently used, so live resizing cannot grow it.
    Kinds 0..3 are the knob styles, 4 / 5 the round key cap up / pressed. Returned by
    value (images are reference counted), so a body outlives the cache if it must. */
juce::Image bodySprite (int kind, int diameterPx)
{
    const juce::SharedResourcePointer<SharedUiCaches> caches;
    auto& bodies = caches->bodies;
    {
        const juce::SpinLock::ScopedLockType sl (caches->lock);
        for (size_t i = 0; i < bodies.size(); ++i)
            if (bodies[i].kind == kind && bodies[i].diameter == diameterPx)
            {
                if (i + 1 != bodies.size())
                    std::rotate (bodies.begin() + (std::ptrdiff_t) i, bodies.begin() + (std::ptrdiff_t) i + 1, bodies.end());
                return bodies.back().image;
            }
    }
    auto image = kind < 4 ? renderKnobBody (static_cast<KnobStyle> (kind), diameterPx) : renderButtonCap (kind == 5, diameterPx);
    const juce::SpinLock::ScopedLockType sl (caches->lock);
    if (bodies.size() >= 32)
        bodies.erase (bodies.begin());
    bodies.push_back ({ kind, diameterPx, image });
    return image;
}

/** Blits a cached body centred on c, 1:1 in device pixels. */
void blitBody (juce::Graphics& g, int kind, juce::Point<float> c, float diameter, float alpha)
{
    const float scale = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const auto body = bodySprite (kind, juce::jmax (8, juce::roundToInt (diameter * scale)));
    const float x = std::round (c.x * scale - (float) body.getWidth() * 0.5f);
    const float y = std::round (c.y * scale - (float) body.getHeight() * 0.5f);
    g.setOpacity (alpha);
    g.drawImageTransformed (body, juce::AffineTransform::translation (x, y).scaled (1.0f / scale));
    g.setOpacity (1.0f);
}
} // namespace

void ArcLookAndFeel::paintKeyCap (juce::Graphics& g, juce::Rectangle<float> circle, bool pressed)
{
    blitBody (g, pressed ? 5 : 4, circle.getCentre(), circle.getWidth(), 1.0f);
}

void ArcLookAndFeel::paintKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float proportion, KnobStyle style, bool hover,
                                bool dragging, bool bipolar, bool enabled)
{
    const auto area = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), bounds.getHeight()),
                                                    juce::jmin (bounds.getWidth(), bounds.getHeight()));
    const auto c = area.getCentre();
    const float outer = area.getWidth() * 0.5f;
    const bool onGlass = style == KnobStyle::glass;
    const auto geo = knobGeometry (style, outer);
    const float alpha = enabled ? 1.0f : 0.4f;

    const float start = kRotaryStart, end = kRotaryEnd;
    const float valueAngle = start + juce::jlimit (0.0f, 1.0f, proportion) * (end - start);
    const float centreAngle = start + 0.5f * (end - start);
    auto onTrack = [&] (float angle, float radius) { return juce::Point<float> (c.x + radius * std::sin (angle), c.y - radius * std::cos (angle)); };

    // --- engraved track ------------------------------------------------------------------
    const juce::PathStrokeType stroke (geo.trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    {
        juce::Path track;
        track.addCentredArc (c.x, c.y, geo.trackR, geo.trackR, 0.0f, start, end, true);
        if (onGlass)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f * alpha));
            g.strokePath (track, stroke, juce::AffineTransform::translation (0.0f, 0.8f));
            g.setColour (colours::chamberDeep.withAlpha (0.9f * alpha));
            g.strokePath (track, stroke);
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (0.8f * alpha));
            g.strokePath (track, stroke, juce::AffineTransform::translation (0.0f, 0.9f));
            g.setColour (colours::inkMuted.withAlpha (0.32f * alpha));
            g.strokePath (track, stroke);
        }
        if (bipolar)
        {
            // Zero mark at twelve o'clock.
            g.setColour ((onGlass ? colours::glassMuted : colours::inkMuted).withAlpha (0.8f * alpha));
            g.drawLine ({ onTrack (centreAngle, geo.trackR + geo.trackW * 1.3f), onTrack (centreAngle, geo.trackR + geo.trackW * 2.6f) },
                        juce::jmax (1.0f, geo.trackW * 0.45f));
        }
    }

    // --- luminous value arc ----------------------------------------------------------------
    const float a0 = bipolar ? juce::jmin (centreAngle, valueAngle) : start;
    const float a1 = bipolar ? juce::jmax (centreAngle, valueAngle) : valueAngle;
    if (a1 - a0 > 0.002f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, geo.trackR, geo.trackR, 0.0f, a0, a1, true);
        g.setColour (colours::cyan.withAlpha ((onGlass ? 0.16f : 0.12f) * alpha));
        g.strokePath (arc, juce::PathStrokeType (geo.trackW * 3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (colours::cyan.withAlpha (0.26f * alpha));
        g.strokePath (arc, juce::PathStrokeType (geo.trackW * 1.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (colours::cyan.withAlpha (alpha));
        g.strokePath (arc, stroke);
        g.setColour (colours::cyanBright.withAlpha (0.75f * alpha));
        g.strokePath (arc, juce::PathStrokeType (geo.trackW * 0.38f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    {
        // The value point itself glows.
        const auto e = onTrack (valueAngle, geo.trackR);
        const float gr = geo.trackW * 2.8f;
        g.setGradientFill (juce::ColourGradient (colours::cyan.withAlpha (0.5f * alpha), e.x, e.y, colours::cyan.withAlpha (0.0f),
                                                 e.x + gr, e.y, true));
        g.fillEllipse (juce::Rectangle<float> (gr * 2.0f, gr * 2.0f).withCentre (e));
        g.setColour (colours::ice.withAlpha (alpha));
        g.fillEllipse (juce::Rectangle<float> (geo.trackW * 1.2f, geo.trackW * 1.2f).withCentre (e));
    }

    // --- body: shadow, turned ring, anodized face (cached sprite) -------------------------------
    // Disabled, the body stays solid and recedes under a veil of its surroundings (a
    // translucent body would let the panel show through the cap).
    blitBody (g, static_cast<int> (style), c, geo.ringR * 2.0f, 1.0f);
    if (! enabled)
    {
        g.setColour ((onGlass ? colours::graphite : colours::panelFace).withAlpha (0.55f));
        g.fillEllipse (juce::Rectangle<float> (geo.ringR * 2.0f, geo.ringR * 2.0f).withCentre (c));
    }
    const auto face = juce::Rectangle<float> (geo.faceR * 2.0f, geo.faceR * 2.0f).withCentre (c);
    if (hover || dragging)
    {
        g.setColour (juce::Colours::white.withAlpha (dragging ? 0.05f : 0.035f));
        g.fillEllipse (face.reduced (geo.faceR * 0.06f));
        g.setColour (colours::cyan.withAlpha (dragging ? 0.55f : 0.3f));
        g.drawEllipse (face.expanded (0.6f), 1.0f);
    }

    // --- pointer ---------------------------------------------------------------------------------
    {
        const float w = juce::jmax (1.6f, geo.faceR * (style == KnobStyle::macro ? 0.05f : 0.075f));
        juce::Path pointer;
        pointer.startNewSubPath (onTrack (valueAngle, geo.faceR * 0.56f));
        pointer.lineTo (onTrack (valueAngle, geo.faceR * 0.9f));
        g.setColour (colours::cyan.withAlpha (0.28f * alpha));
        g.strokePath (pointer, juce::PathStrokeType (w * 2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xfff4f8fb).withAlpha (alpha));
        g.strokePath (pointer, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void ArcLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos, float, float,
                                       juce::Slider& s)
{
    paintKnob (g, juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height), pos, getKnobStyle (s),
               s.isMouseOverOrDragging() && s.isEnabled(), s.isMouseButtonDown(),
               static_cast<bool> (s.getProperties().getWithDefault ("bipolar", false)), s.isEnabled());
}

// --- popup menus ----------------------------------------------------------------------
void ArcLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setGradientFill (juce::ColourGradient (colours::graphiteHigh, 0.0f, 0.0f, colours::graphite, 0.0f, (float) height, false));
    g.fillRect (r);
    g.setColour (colours::cyan.withAlpha (0.25f));
    g.drawRect (r, 1.0f);
}

void ArcLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                        bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                        const juce::String& shortcutKeyText, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (colours::graphiteLine);
        g.fillRect (area.reduced (10, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    auto r = area.toFloat().reduced (4.0f, 1.0f);
    if (isHighlighted && isActive)
    {
        g.setColour (colours::graphiteHigh.brighter (0.15f));
        g.fillRoundedRectangle (r, 4.0f);
    }
    auto textArea = r.reduced (12.0f, 0.0f);
    if (isTicked)
    {
        g.setColour (colours::cyan);
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre ({ r.getX() + 7.0f, r.getCentreY() }));
    }
    g.setColour (! isActive ? colours::glassFaint : isHighlighted ? colours::cyanBright : colours::glassText);
    g.setFont (getPopupMenuFont());
    g.drawText (text, textArea, juce::Justification::centredLeft, true);
    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (colours::glassMuted);
        g.drawText (shortcutKeyText, textArea, juce::Justification::centredRight, true);
    }
    if (hasSubMenu)
    {
        juce::Path arrow;
        const float ax = r.getRight() - 10.0f, ay = r.getCentreY();
        arrow.startNewSubPath (ax - 3.0f, ay - 4.0f);
        arrow.lineTo (ax + 1.0f, ay);
        arrow.lineTo (ax - 3.0f, ay + 4.0f);
        g.setColour (colours::glassMuted);
        g.strokePath (arrow, juce::PathStrokeType (1.2f));
    }
}

juce::Font ArcLookAndFeel::getPopupMenuFont() { return Fonts::regular (14.0f, 0.04f); }

void ArcLookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& name)
{
    g.setColour (colours::glassMuted);
    drawTrackedText (g, name.toUpperCase(), area.toFloat().reduced (12.0f, 0.0f), Fonts::label (10.5f), juce::Justification::bottomLeft);
}

// --- tooltips -----------------------------------------------------------------------------
juce::Rectangle<int> ArcLookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                                       juce::Rectangle<int> parentArea)
{
    const auto lines = juce::StringArray::fromLines (tipText);
    const auto title = lines[0], body = lines.size() > 1 ? lines[1] : juce::String();
    const float w = juce::jmax (textWidth (Fonts::label (11.0f), title.toUpperCase()),
                                juce::jmin (260.0f, textWidth (Fonts::regular (13.0f), body))) + 24.0f;
    const int h = body.isNotEmpty() ? (body.length() > 40 ? 62 : 46) : 26;
    return juce::Rectangle<int> (screenPos.x - (int) w / 2, screenPos.y + 22, (int) w, h).constrainedWithin (parentArea);
}

void ArcLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (colours::graphite.withAlpha (0.96f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (colours::cyan.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    const auto lines = juce::StringArray::fromLines (text);
    auto area = r.reduced (12.0f, 6.0f);
    g.setColour (colours::cyanBright);
    drawTrackedText (g, lines[0].toUpperCase(), area.removeFromTop (14.0f), Fonts::label (11.0f), juce::Justification::centredLeft);
    if (lines.size() > 1)
    {
        g.setColour (colours::glassText);
        g.setFont (Fonts::regular (13.0f));
        g.drawFittedText (lines[1], area.toNearestInt(), juce::Justification::topLeft, 2, 1.0f);
    }
}

// --- text editors, scroll bars, labels ---------------------------------------------------
void ArcLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& e)
{
    // A bare editor (transparent background) sits in a well its parent draws (preset search).
    if (e.findColour (juce::TextEditor::backgroundColourId).isTransparent())
        return;
    g.setColour (colours::chamberDeep);
    g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, 5.0f);
}

void ArcLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& e)
{
    if (e.findColour (juce::TextEditor::outlineColourId).isTransparent())
        return;
    g.setColour (e.hasKeyboardFocus (true) ? colours::cyan.withAlpha (0.8f) : colours::graphiteLine);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f, 5.0f, 1.0f);
}

void ArcLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height, bool vertical,
                                    int thumbStart, int thumbSize, bool over, bool down)
{
    if (thumbSize <= 0)
        return;
    const auto thumb = vertical ? juce::Rectangle<float> ((float) x + (float) width * 0.35f, (float) thumbStart, (float) width * 0.3f, (float) thumbSize)
                                : juce::Rectangle<float> ((float) thumbStart, (float) y + (float) height * 0.35f, (float) thumbSize, (float) height * 0.3f);
    g.setColour ((down ? colours::cyan : over ? colours::glassMuted : colours::glassFaint).withAlpha (0.9f));
    g.fillRoundedRectangle (thumb, juce::jmin (thumb.getWidth(), thumb.getHeight()) * 0.5f);
}

void ArcLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const float radius = r.getHeight() * 0.5f;
    g.setColour (down ? colours::graphiteHigh.brighter (0.15f) : (highlighted ? colours::graphiteHigh.brighter (0.07f) : colours::graphiteHigh));
    g.fillRoundedRectangle (r, radius);
    g.setColour (highlighted && b.isEnabled() ? colours::cyan.withAlpha (0.7f) : colours::graphiteLine);
    g.drawRoundedRectangle (r, radius, 1.0f);
}

void ArcLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool highlighted, bool)
{
    g.setColour (! b.isEnabled() ? colours::glassFaint : (highlighted ? colours::cyanBright : colours::glassText));
    drawTrackedText (g, b.getButtonText().toUpperCase(), b.getLocalBounds().toFloat(), Fonts::regular (10.5f, 0.22f),
                     juce::Justification::centred);
}

void ArcLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool over, bool dragging)
{
    // Three machined dots along the diagonal.
    for (int i = 0; i < 3; ++i)
    {
        const float x = (float) w - 5.0f - (float) i * 4.5f, y = (float) h - 5.0f - (float) (2 - i) * 0.0f;
        for (int j = 0; j <= i; ++j)
        {
            const auto p = juce::Point<float> (x + (float) j * 4.5f, y - (float) j * 4.5f);
            g.setColour ((dragging ? colours::cyan : over ? colours::inkMuted : colours::inkFaint).withAlpha (0.9f));
            g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre (p));
        }
    }
}

juce::Font ArcLookAndFeel::getLabelFont (juce::Label&) { return Fonts::regular (14.0f); }
juce::Font ArcLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return Fonts::label (juce::jmin (13.0f, (float) buttonHeight * 0.42f));
}

} // namespace arc::ui

#pragma once

// ARC design system: palette, typography, metrics. One place for every visual constant,
// so the whole instrument stays consistent (see docs/UI_SYSTEM.md).
//
// Art direction (locked reference): machined satin silver (~70 %), a dark graphite
// resonance chamber (~20 %) and ice-cyan signal light (~10 %). Restrained blue and a
// trace of violet are secondary energy colours; amber is reserved for recording.

#include <cmath>

#include <juce_gui_basics/juce_gui_basics.h>

namespace arc::ui
{

// Base design size. The editor scales this canvas uniformly (4:3).
inline constexpr int kBaseWidth = 1200;
inline constexpr int kBaseHeight = 900;

namespace colours
{
// Silver chassis
inline const juce::Colour chassisHigh { 0xffe3e7ec };
inline const juce::Colour chassisMid { 0xffcbd0d7 };
inline const juce::Colour chassisLow { 0xffb0b7bf };
inline const juce::Colour chassisEdge { 0xff808891 };
inline const juce::Colour panelFace { 0xffd8dde3 };
inline const juce::Colour panelFaceLow { 0xffc7cdd4 };
inline const juce::Colour bevelLight { 0xfff8fafc };
inline const juce::Colour bevelDark { 0xff8a929b };
inline const juce::Colour hairline { 0xffa6adb5 };

// Text on silver
inline const juce::Colour ink { 0xff1c2229 };
inline const juce::Colour inkSoft { 0xff353c45 };
inline const juce::Colour inkMuted { 0xff5d6570 };
inline const juce::Colour inkFaint { 0xff828a94 };

// Graphite (chamber, selected controls, displays)
inline const juce::Colour chamberDeep { 0xff05070a };
inline const juce::Colour chamber { 0xff0b0f14 };
inline const juce::Colour chamberLift { 0xff161b22 };
inline const juce::Colour graphite { 0xff1b2026 };
inline const juce::Colour graphiteHigh { 0xff2c323a };
inline const juce::Colour graphiteLine { 0xff39414b };

// Signal
inline const juce::Colour cyan { 0xff45cdf2 };
inline const juce::Colour cyanBright { 0xff9eeaff };
inline const juce::Colour cyanDim { 0xff1d7fa3 };
inline const juce::Colour ice { 0xffd8f6ff };
inline const juce::Colour blue { 0xff4f7fd6 };
inline const juce::Colour violet { 0xff8a7ad6 };
inline const juce::Colour amber { 0xffe9b45c };

// Text on graphite
inline const juce::Colour glassText { 0xffdbe6ee };
inline const juce::Colour glassMuted { 0xff7f8c98 };
inline const juce::Colour glassFaint { 0xff4d5864 };
} // namespace colours

/** Embedded Jost (SIL OFL 1.1) with graceful fallback to the platform sans. */
class Fonts
{
public:
    enum class Weight
    {
        light,
        regular,
        medium
    };

    static juce::Font get (Weight w, float height, float tracking = 0.0f);
    static juce::Font light (float height, float tracking = 0.0f) { return get (Weight::light, height, tracking); }
    static juce::Font regular (float height, float tracking = 0.0f) { return get (Weight::regular, height, tracking); }
    static juce::Font medium (float height, float tracking = 0.0f) { return get (Weight::medium, height, tracking); }

    /** Wide-tracked small caps used for panel headings and labels. */
    static juce::Font label (float height) { return get (Weight::regular, height, 0.18f); }
    static juce::Font heading (float height) { return get (Weight::regular, height, 0.24f); }
};

/** Easing and small maths helpers shared by animated components. */
inline float smoothTowards (float current, float target, float dtSeconds, float timeConstant) noexcept
{
    const float k = 1.0f - std::exp (-dtSeconds / std::max (1.0e-4f, timeConstant));
    return current + (target - current) * k;
}

inline float easeOutCubic (float t) noexcept
{
    t = juce::jlimit (0.0f, 1.0f, t);
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}

/** Width of a single line of text in `font` (tracking included). */
float textWidth (const juce::Font& font, const juce::String& text);

/** Draws text with explicit letter spacing, centred or left-aligned in `area`. */
void drawTrackedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, const juce::Font& font,
                      juce::Justification just);

/**
    Cached raster layer rendered at the physical pixel density of the target context,
    so static artwork stays sharp at any editor scale / display scale. `render` draws in
    logical (component) coordinates; the cache is rebuilt when size or scale changes, or
    when invalidate() is called.
*/
class CachedLayer
{
public:
    void invalidate() noexcept { valid = false; }
    /** Opaque layers use an RGB image: blitting skips alpha blending entirely. */
    void setOpaque (bool b) noexcept
    {
        if (b != opaque)
        {
            opaque = b;
            valid = false;
        }
    }

    template <typename RenderFn>
    void draw (juce::Graphics& g, juce::Rectangle<int> logicalArea, RenderFn&& render)
    {
        const float scale = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        const int w = juce::roundToInt ((float) logicalArea.getWidth() * scale);
        const int h = juce::roundToInt ((float) logicalArea.getHeight() * scale);
        if (w <= 0 || h <= 0)
            return;
        if (! valid || image.getWidth() != w || image.getHeight() != h || ! juce::approximatelyEqual (cachedScale, scale))
        {
            image = juce::Image (opaque ? juce::Image::RGB : juce::Image::ARGB, w, h, true);
            juce::Graphics ig (image);
            // Newest transform applies first: logical -> area-relative -> physical pixels.
            ig.addTransform (juce::AffineTransform::scale (scale));
            ig.addTransform (juce::AffineTransform::translation ((float) -logicalArea.getX(), (float) -logicalArea.getY()));
            render (ig);
            cachedScale = scale;
            valid = true;
        }
        g.drawImageTransformed (image, juce::AffineTransform::scale (1.0f / scale)
                                           .translated ((float) logicalArea.getX(), (float) logicalArea.getY()));
    }

private:
    juce::Image image;
    float cachedScale = 0.0f;
    bool valid = false, opaque = false;
};

} // namespace arc::ui

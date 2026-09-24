#include "Graphics/SilverSurface.h"

#include "UI/Theme.h"

namespace arc::gfx
{

using namespace arc::ui;

juce::Path superellipse (juce::Rectangle<float> r, float n, int segments)
{
    juce::Path p;
    const auto c = r.getCentre();
    const float a = r.getWidth() * 0.5f, b = r.getHeight() * 0.5f;
    for (int i = 0; i < segments; ++i)
    {
        const float t = juce::MathConstants<float>::twoPi * (float) i / (float) segments;
        const float ct = std::cos (t), st = std::sin (t);
        const float x = a * std::copysign (std::pow (std::abs (ct), 2.0f / n), ct);
        const float y = b * std::copysign (std::pow (std::abs (st), 2.0f / n), st);
        if (i == 0)
            p.startNewSubPath (c.x + x, c.y + y);
        else
            p.lineTo (c.x + x, c.y + y);
    }
    p.closeSubPath();
    return p;
}

const juce::Image& brushedTexture()
{
    static const juce::Image tex = []
    {
        constexpr int w = 512, h = 512;
        juce::Image img (juce::Image::ARGB, w, h, true);
        juce::Random rng (0xA2C1);
        juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);
        // Each row: a slowly varying streak value plus fine grain, smeared horizontally.
        for (int y = 0; y < h; ++y)
        {
            float streak = rng.nextFloat() * 2.0f - 1.0f;
            float v = 0.0f;
            for (int x = 0; x < w; ++x)
            {
                if (rng.nextInt (64) == 0)
                    streak = 0.7f * streak + 0.3f * (rng.nextFloat() * 2.0f - 1.0f);
                v = 0.92f * v + 0.08f * (streak + 0.6f * (rng.nextFloat() * 2.0f - 1.0f));
                const auto level = (juce::uint8) juce::jlimit (0, 255, (int) (128.0f + 180.0f * v));
                data.setPixelColour (x, y, juce::Colour (level, level, level, (juce::uint8) 255));
            }
        }
        return img;
    }();
    return tex;
}

namespace
{
void tileTexture (juce::Graphics& g, const juce::Path& clip, float alpha)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (clip);
    g.setTiledImageFill (brushedTexture(), 0, 0, alpha);
    g.fillPath (clip);
}
} // namespace

void paintChassis (juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius)
{
    // Outer frame (darker satin) and the main plate inset by a machined bevel.
    g.setGradientFill (juce::ColourGradient (colours::chassisLow.brighter (0.1f), bounds.getX(), bounds.getY(),
                                             colours::chassisEdge, bounds.getX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, cornerRadius + 6.0f);

    const auto plate = bounds.reduced (7.0f);
    juce::Path platePath;
    platePath.addRoundedRectangle (plate, cornerRadius);

    juce::ColourGradient face (colours::chassisHigh, plate.getX(), plate.getY(), colours::chassisLow, plate.getX(),
                               plate.getBottom(), false);
    face.addColour (0.12, colours::chassisHigh.darker (0.02f));
    face.addColour (0.55, colours::chassisMid);
    face.addColour (0.82, colours::chassisMid.darker (0.04f));
    g.setGradientFill (face);
    g.fillPath (platePath);

    tileTexture (g, platePath, 0.045f);

    // Broad soft top light (satin sheen).
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (platePath);
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.55f), plate.getCentreX(), plate.getY() - plate.getHeight() * 0.1f,
                                    juce::Colours::white.withAlpha (0.0f), plate.getCentreX(), plate.getY() + plate.getHeight() * 0.55f,
                                    true);
        g.setGradientFill (sheen);
        g.fillRect (plate);
    }

    // Bevel: light top-left edge, dark bottom-right edge.
    g.setColour (colours::bevelLight.withAlpha (0.9f));
    g.strokePath (platePath, juce::PathStrokeType (1.2f), juce::AffineTransform::translation (0.0f, 0.8f));
    g.setColour (colours::chassisEdge.withAlpha (0.55f));
    g.strokePath (platePath, juce::PathStrokeType (1.0f));
}

void paintRaisedPlate (juce::Graphics& g, const juce::Path& shape, float shadowRadius)
{
    const auto r = shape.getBounds();
    // Drop shadow
    juce::DropShadow (juce::Colour (0xff4a5058).withAlpha (0.22f), (int) shadowRadius, { 0, (int) (shadowRadius * 0.35f) })
        .drawForPath (g, shape);

    juce::ColourGradient face (colours::panelFace.brighter (0.25f), r.getX(), r.getY(), colours::panelFaceLow, r.getX(),
                               r.getBottom(), false);
    face.addColour (0.5, colours::panelFace);
    g.setGradientFill (face);
    g.fillPath (shape);
    tileTexture (g, shape, 0.035f);

    // Sheen from the top-left
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (shape);
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.45f), r.getX(), r.getY(), juce::Colours::white.withAlpha (0.0f),
                                    r.getX() + r.getWidth() * 0.6f, r.getY() + r.getHeight() * 0.6f, false);
        g.setGradientFill (sheen);
        g.fillRect (r);
    }

    // Machined edge: bright top lip, darker lower lip.
    g.setColour (colours::bevelLight);
    g.strokePath (shape, juce::PathStrokeType (1.4f), juce::AffineTransform::translation (0.0f, 1.0f));
    g.setColour (colours::bevelDark.withAlpha (0.6f));
    g.strokePath (shape, juce::PathStrokeType (1.0f));
}

void paintRecessedTray (juce::Graphics& g, const juce::Path& shape)
{
    const auto r = shape.getBounds();
    juce::ColourGradient face (colours::chassisLow.darker (0.08f), r.getX(), r.getY(), colours::chassisMid.brighter (0.05f),
                               r.getX(), r.getBottom(), false);
    g.setGradientFill (face);
    g.fillPath (shape);
    // Inner shadow along the top edge.
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (shape);
        juce::DropShadow (juce::Colours::black.withAlpha (0.35f), 8, { 0, 3 }).drawForPath (g, [&]
        {
            juce::Path outside;
            outside.addRectangle (r.expanded (30.0f));
            outside.setUsingNonZeroWinding (false);
            outside.addPath (shape);
            return outside;
        }());
    }
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.strokePath (shape, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.0f, 1.0f));
    g.setColour (colours::bevelDark.withAlpha (0.8f));
    g.strokePath (shape, juce::PathStrokeType (0.9f));
}

void paintChromeBezel (juce::Graphics& g, const juce::Path& outer, const juce::Path& inner)
{
    const auto r = outer.getBounds();
    juce::Path ring (outer);
    ring.setUsingNonZeroWinding (false);
    ring.addPath (inner);

    // Soft shadow of the whole bezel onto the chassis.
    juce::DropShadow (juce::Colour (0xff3c4249).withAlpha (0.35f), 18, { 0, 6 }).drawForPath (g, outer);

    // Banded chrome reflections (vertical), as a polished ring catches the room.
    juce::ColourGradient chrome (juce::Colour (0xfff7f8f9), r.getX(), r.getY(), juce::Colour (0xffd5d9de), r.getX(), r.getBottom(), false);
    chrome.addColour (0.10, juce::Colour (0xffc2c7cd));
    chrome.addColour (0.22, juce::Colour (0xffeef0f2));
    chrome.addColour (0.48, juce::Colour (0xffa9b0b8));
    chrome.addColour (0.62, juce::Colour (0xffe5e8eb));
    chrome.addColour (0.84, juce::Colour (0xff9aa1a9));
    chrome.addColour (0.94, juce::Colour (0xffeceef0));
    g.setGradientFill (chrome);
    g.fillPath (ring);

    // Horizontal sheen across the ring (left/right bright flanks).
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (ring);
        juce::ColourGradient flank (juce::Colours::white.withAlpha (0.55f), r.getX(), r.getCentreY(),
                                    juce::Colours::white.withAlpha (0.0f), r.getX() + r.getWidth() * 0.18f, r.getCentreY(), false);
        g.setGradientFill (flank);
        g.fillRect (r);
        juce::ColourGradient flankR (juce::Colours::white.withAlpha (0.0f), r.getRight() - r.getWidth() * 0.18f, r.getCentreY(),
                                     juce::Colours::white.withAlpha (0.45f), r.getRight(), r.getCentreY(), false);
        g.setGradientFill (flankR);
        g.fillRect (r);

        // Specular hot spots (top-left, bottom-right).
        auto spot = [&] (float cx, float cy, float rad, float alpha)
        {
            juce::ColourGradient sg (juce::Colours::white.withAlpha (alpha), cx, cy, juce::Colours::white.withAlpha (0.0f), cx + rad,
                                     cy, true);
            g.setGradientFill (sg);
            g.fillEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
        };
        spot (r.getX() + r.getWidth() * 0.16f, r.getY() + r.getHeight() * 0.1f, r.getWidth() * 0.12f, 0.75f);
        spot (r.getRight() - r.getWidth() * 0.14f, r.getBottom() - r.getHeight() * 0.1f, r.getWidth() * 0.1f, 0.55f);
        spot (r.getCentreX(), r.getBottom() - r.getHeight() * 0.02f, r.getWidth() * 0.2f, 0.35f);
    }

    // Lips: bright outer edge, dark inner lip where chrome meets the chamber.
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.strokePath (outer, juce::PathStrokeType (1.3f));
    g.setColour (colours::chassisEdge.withAlpha (0.7f));
    g.strokePath (outer, juce::PathStrokeType (0.8f), juce::AffineTransform::translation (0.0f, 1.2f));
    g.setColour (juce::Colour (0xff1e2329));
    g.strokePath (inner, juce::PathStrokeType (2.2f));
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.strokePath (inner, juce::PathStrokeType (0.8f), juce::AffineTransform::translation (0.0f, -1.6f));
}

void paintGroove (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b)
{
    const bool vertical = std::abs (a.x - b.x) < std::abs (a.y - b.y);
    const juce::Point<float> off = vertical ? juce::Point<float> (1.0f, 0.0f) : juce::Point<float> (0.0f, 1.0f);
    g.setColour (colours::bevelDark.withAlpha (0.75f));
    g.drawLine ({ a, b }, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawLine ({ a + off, b + off }, 1.0f);
}

void paintContactShadow (juce::Graphics& g, juce::Rectangle<float> r, float alpha)
{
    juce::ColourGradient sg (juce::Colour (0xff2a2f36).withAlpha (alpha), r.getCentreX(), r.getCentreY(),
                             juce::Colour (0xff2a2f36).withAlpha (0.0f), r.getRight(), r.getCentreY(), true);
    g.setGradientFill (sg);
    g.fillEllipse (r);
}

} // namespace arc::gfx

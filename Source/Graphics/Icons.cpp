#include "Graphics/Icons.h"

namespace arc::gfx
{

namespace
{
constexpr float pi = juce::MathConstants<float>::pi;
constexpr float twoPi = juce::MathConstants<float>::twoPi;

/** Maps unit coordinates (0..1) into the target area (square, centred). */
struct Box
{
    juce::Rectangle<float> r;
    float x (float u) const { return r.getX() + u * r.getWidth(); }
    float y (float v) const { return r.getY() + v * r.getHeight(); }
    juce::Point<float> p (float u, float v) const { return { x (u), y (v) }; }
    float s (float d) const { return d * r.getWidth(); }
};

juce::Rectangle<float> square (juce::Rectangle<float> a)
{
    const float side = juce::jmin (a.getWidth(), a.getHeight());
    return a.withSizeKeepingCentre (side, side);
}

void addCircle (juce::Path& p, const Box& b, float cx, float cy, float rad)
{
    p.addEllipse (b.x (cx - rad), b.y (cy - rad), b.s (2.0f * rad), b.s (2.0f * rad));
}
} // namespace

void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> area, juce::Colour colour, float strokeWidth)
{
    const Box b { square (area) };
    juce::Path p;
    bool fill = false;
    const juce::PathStrokeType stroke (strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    switch (icon)
    {
        case Icon::strike:
            addCircle (p, b, 0.5f, 0.5f, 0.44f);
            addCircle (p, b, 0.5f, 0.5f, 0.31f);
            addCircle (p, b, 0.5f, 0.5f, 0.18f);
            break;

        case Icon::pluck:
            p.startNewSubPath (b.p (0.02f, 0.5f));
            p.lineTo (b.p (0.98f, 0.5f));
            p.startNewSubPath (b.p (0.3f, 0.78f));
            p.lineTo (b.p (0.48f, 0.22f));
            p.startNewSubPath (b.p (0.52f, 0.78f));
            p.lineTo (b.p (0.7f, 0.22f));
            break;

        case Icon::bow:
        {
            const int n = 48;
            for (int i = 0; i <= n; ++i)
            {
                const float t = (float) i / (float) n;
                const float env = std::sin (pi * t);
                const auto pt = b.p (0.02f + 0.96f * t, 0.5f - 0.2f * env * std::sin (twoPi * 1.5f * t));
                if (i == 0)
                    p.startNewSubPath (pt);
                else
                    p.lineTo (pt);
            }
            break;
        }

        case Icon::air:
        {
            const int n = 90;
            for (int i = 0; i <= n; ++i)
            {
                const float t = (float) i / (float) n;
                const float ang = 2.6f * twoPi * t - pi * 0.5f;
                const float rad = 0.04f + 0.42f * t;
                const auto pt = b.p (0.5f + rad * std::cos (ang), 0.5f + rad * std::sin (ang));
                if (i == 0)
                    p.startNewSubPath (pt);
                else
                    p.lineTo (pt);
            }
            break;
        }

        case Icon::glass:
            // Faceted gem: outline + girdle + facets.
            p.startNewSubPath (b.p (0.5f, 0.04f));
            p.lineTo (b.p (0.9f, 0.42f));
            p.lineTo (b.p (0.5f, 0.96f));
            p.lineTo (b.p (0.1f, 0.42f));
            p.closeSubPath();
            p.startNewSubPath (b.p (0.1f, 0.42f));
            p.lineTo (b.p (0.9f, 0.42f));
            p.startNewSubPath (b.p (0.32f, 0.42f));
            p.lineTo (b.p (0.5f, 0.04f));
            p.lineTo (b.p (0.68f, 0.42f));
            p.startNewSubPath (b.p (0.32f, 0.42f));
            p.lineTo (b.p (0.5f, 0.96f));
            p.lineTo (b.p (0.68f, 0.42f));
            break;

        case Icon::metal:
        {
            addCircle (p, b, 0.5f, 0.5f, 0.46f);
            for (int i = 0; i <= 6; ++i)
            {
                const float ang = twoPi * (float) i / 6.0f + pi / 6.0f;
                const auto pt = b.p (0.5f + 0.3f * std::cos (ang), 0.5f + 0.3f * std::sin (ang));
                if (i == 0)
                    p.startNewSubPath (pt);
                else
                    p.lineTo (pt);
            }
            addCircle (p, b, 0.5f, 0.5f, 0.1f);
            break;
        }

        case Icon::wood:
        {
            // Growth rings, slightly eccentric and wavy.
            for (int ring = 0; ring < 4; ++ring)
            {
                const float base = 0.1f + 0.11f * (float) ring;
                const int n = 60;
                for (int i = 0; i <= n; ++i)
                {
                    const float ang = twoPi * (float) i / (float) n;
                    const float wob = 1.0f + 0.07f * std::sin (3.0f * ang + (float) ring) + 0.05f * std::sin (5.0f * ang);
                    const float rad = base * wob;
                    const auto pt = b.p (0.46f + rad * std::cos (ang) * 1.05f, 0.52f + rad * std::sin (ang) * 0.95f);
                    if (i == 0)
                        p.startNewSubPath (pt);
                    else
                        p.lineTo (pt);
                }
                p.closeSubPath();
            }
            break;
        }

        case Icon::membrane:
            addCircle (p, b, 0.5f, 0.5f, 0.44f);
            addCircle (p, b, 0.5f, 0.5f, 0.36f);
            break;

        case Icon::freeze:
            for (int i = 0; i < 6; ++i)
            {
                const float ang = twoPi * (float) i / 6.0f;
                const float c = std::cos (ang), s = std::sin (ang);
                p.startNewSubPath (b.p (0.5f, 0.5f));
                p.lineTo (b.p (0.5f + 0.45f * c, 0.5f + 0.45f * s));
                // Barbs
                const float bx = 0.5f + 0.3f * c, by = 0.5f + 0.3f * s;
                for (float side : { -1.0f, 1.0f })
                {
                    const float a2 = ang + side * 0.75f;
                    p.startNewSubPath (b.p (bx, by));
                    p.lineTo (b.p (bx + 0.13f * std::cos (a2), by + 0.13f * std::sin (a2)));
                }
            }
            break;

        case Icon::random:
        {
            // Isometric die with pips.
            const auto top = b.p (0.5f, 0.06f), left = b.p (0.08f, 0.28f), right = b.p (0.92f, 0.28f), mid = b.p (0.5f, 0.5f);
            const auto bl = b.p (0.08f, 0.72f), br = b.p (0.92f, 0.72f), bot = b.p (0.5f, 0.94f);
            p.startNewSubPath (top);
            p.lineTo (right);
            p.lineTo (br);
            p.lineTo (bot);
            p.lineTo (bl);
            p.lineTo (left);
            p.closeSubPath();
            p.startNewSubPath (left);
            p.lineTo (mid);
            p.lineTo (right);
            p.startNewSubPath (mid);
            p.lineTo (bot);
            juce::Path pips;
            auto pip = [&] (float u, float v) { pips.addEllipse (b.x (u) - b.s (0.035f), b.y (v) - b.s (0.035f), b.s (0.07f), b.s (0.07f)); };
            pip (0.5f, 0.28f);
            pip (0.28f, 0.52f);
            pip (0.72f, 0.52f);
            pip (0.72f, 0.74f);
            g.setColour (colour);
            g.fillPath (pips);
            break;
        }

        case Icon::sync:
        {
            // Two arcs with arrowheads, chasing each other.
            for (int k = 0; k < 2; ++k)
            {
                const float a0 = pi * (float) k + 0.35f, a1 = a0 + pi - 0.7f;
                juce::Path arc;
                arc.addCentredArc (b.x (0.5f), b.y (0.5f), b.s (0.4f), b.s (0.4f), 0.0f, a0, a1, true);
                p.addPath (arc);
                // Arrowhead at a1 (tangent direction clockwise).
                const float ex = 0.5f + 0.4f * std::sin (a1), ey = 0.5f - 0.4f * std::cos (a1);
                const float tx = std::cos (a1), ty = std::sin (a1);
                const float nx = -ty, ny = tx;
                p.startNewSubPath (b.p (ex, ey));
                p.lineTo (b.p (ex - 0.14f * tx + 0.09f * nx, ey - 0.14f * ty + 0.09f * ny));
                p.startNewSubPath (b.p (ex, ey));
                p.lineTo (b.p (ex - 0.14f * tx - 0.09f * nx, ey - 0.14f * ty - 0.09f * ny));
            }
            break;
        }

        case Icon::motion:
        {
            // An orbit around a point: the MOTION mark.
            juce::Path orbit;
            orbit.addEllipse (b.x (0.06f), b.y (0.3f), b.s (0.88f), b.s (0.4f));
            p.addPath (orbit, juce::AffineTransform::rotation (-0.45f, b.x (0.5f), b.y (0.5f)));
            addCircle (p, b, 0.5f, 0.5f, 0.07f);
            juce::Path dot;
            const float ang = -0.45f;
            const float ox = 0.5f + 0.44f * std::cos (ang), oy = 0.5f + 0.44f * std::sin (ang) * 0.46f;
            dot.addEllipse (b.x (ox) - b.s (0.06f), b.y (oy) - b.s (0.06f), b.s (0.12f), b.s (0.12f));
            g.setColour (colour);
            g.fillPath (dot);
            break;
        }

        case Icon::heart:
        case Icon::heartFilled:
            p.startNewSubPath (b.p (0.5f, 0.88f));
            p.cubicTo (b.p (0.12f, 0.6f), b.p (0.02f, 0.36f), b.p (0.2f, 0.18f));
            p.cubicTo (b.p (0.34f, 0.05f), b.p (0.48f, 0.14f), b.p (0.5f, 0.28f));
            p.cubicTo (b.p (0.52f, 0.14f), b.p (0.66f, 0.05f), b.p (0.8f, 0.18f));
            p.cubicTo (b.p (0.98f, 0.36f), b.p (0.88f, 0.6f), b.p (0.5f, 0.88f));
            p.closeSubPath();
            fill = icon == Icon::heartFilled;
            break;

        case Icon::chevronLeft:
            p.startNewSubPath (b.p (0.64f, 0.14f));
            p.lineTo (b.p (0.32f, 0.5f));
            p.lineTo (b.p (0.64f, 0.86f));
            break;

        case Icon::chevronRight:
            p.startNewSubPath (b.p (0.36f, 0.14f));
            p.lineTo (b.p (0.68f, 0.5f));
            p.lineTo (b.p (0.36f, 0.86f));
            break;

        case Icon::gear:
        {
            const int teeth = 8;
            for (int i = 0; i <= teeth * 4; ++i)
            {
                const float ang = twoPi * (float) i / (float) (teeth * 4);
                const bool outer = (i % 4) == 1 || (i % 4) == 2;
                const float rad = outer ? 0.46f : 0.36f;
                const auto pt = b.p (0.5f + rad * std::cos (ang), 0.5f + rad * std::sin (ang));
                if (i == 0)
                    p.startNewSubPath (pt);
                else
                    p.lineTo (pt);
            }
            p.closeSubPath();
            addCircle (p, b, 0.5f, 0.5f, 0.14f);
            break;
        }

        case Icon::record:
            addCircle (p, b, 0.5f, 0.5f, 0.3f);
            fill = true;
            break;

        case Icon::close:
            p.startNewSubPath (b.p (0.2f, 0.2f));
            p.lineTo (b.p (0.8f, 0.8f));
            p.startNewSubPath (b.p (0.8f, 0.2f));
            p.lineTo (b.p (0.2f, 0.8f));
            break;

        case Icon::save:
            p.startNewSubPath (b.p (0.5f, 0.1f));
            p.lineTo (b.p (0.5f, 0.64f));
            p.startNewSubPath (b.p (0.28f, 0.44f));
            p.lineTo (b.p (0.5f, 0.66f));
            p.lineTo (b.p (0.72f, 0.44f));
            p.startNewSubPath (b.p (0.12f, 0.7f));
            p.lineTo (b.p (0.12f, 0.9f));
            p.lineTo (b.p (0.88f, 0.9f));
            p.lineTo (b.p (0.88f, 0.7f));
            break;

        case Icon::trash:
            p.startNewSubPath (b.p (0.14f, 0.24f));
            p.lineTo (b.p (0.86f, 0.24f));
            p.startNewSubPath (b.p (0.38f, 0.24f));
            p.lineTo (b.p (0.42f, 0.1f));
            p.lineTo (b.p (0.58f, 0.1f));
            p.lineTo (b.p (0.62f, 0.24f));
            p.startNewSubPath (b.p (0.24f, 0.24f));
            p.lineTo (b.p (0.3f, 0.92f));
            p.lineTo (b.p (0.7f, 0.92f));
            p.lineTo (b.p (0.76f, 0.24f));
            break;

        case Icon::search:
            addCircle (p, b, 0.42f, 0.42f, 0.28f);
            p.startNewSubPath (b.p (0.62f, 0.62f));
            p.lineTo (b.p (0.9f, 0.9f));
            break;
    }

    g.setColour (colour);
    if (fill)
        g.fillPath (p);
    else
        g.strokePath (p, stroke);
}

void drawLogo (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, float strokeWidth)
{
    // Proportions from the locked reference: wide, thin, geometric. Height = cap height.
    const float h = area.getHeight();
    const float w = h * 0.86f; // glyph width
    const float gap = h * 0.36f;
    const float x0 = area.getX(), y0 = area.getY(), y1 = area.getBottom();
    juce::Path p;
    // A as an open lambda (no crossbar)
    p.startNewSubPath (x0, y1);
    p.lineTo (x0 + w * 0.5f, y0);
    p.lineTo (x0 + w, y1);
    // R
    const float rx = x0 + w + gap;
    const float bowl = h * 0.52f;
    p.startNewSubPath (rx, y1);
    p.lineTo (rx, y0);
    p.lineTo (rx + w * 0.52f, y0);
    p.addCentredArc (rx + w * 0.52f, y0 + bowl * 0.5f, bowl * 0.5f, bowl * 0.5f, 0.0f, 0.0f, juce::MathConstants<float>::pi, false);
    p.lineTo (rx, y0 + bowl);
    p.startNewSubPath (rx + w * 0.46f, y0 + bowl);
    p.lineTo (rx + w * 0.86f, y1);
    // C
    const float cx = rx + w + gap + h * 0.5f;
    // Opening to the right (JUCE arcs: clockwise from 12 o'clock).
    const float halfPi = juce::MathConstants<float>::halfPi;
    p.addCentredArc (cx, y0 + h * 0.5f, h * 0.5f, h * 0.5f, 0.0f, halfPi + 0.62f, halfPi + juce::MathConstants<float>::twoPi - 0.62f, true);

    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (strokeWidth, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
}

void drawCoreEmblem (juce::Graphics& g, juce::Point<float> c, float radius, juce::Colour colour, float strokeWidth, float rotation)
{
    juce::Path p;
    for (int i = 0; i < 3; ++i)
    {
        const float ang = rotation + juce::MathConstants<float>::twoPi * (float) i / 3.0f + juce::MathConstants<float>::pi;
        p.startNewSubPath (c);
        p.lineTo (c.x + radius * std::sin (ang), c.y - radius * std::cos (ang));
    }
    g.setColour (colour);
    g.strokePath (p, juce::PathStrokeType (strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

} // namespace arc::gfx

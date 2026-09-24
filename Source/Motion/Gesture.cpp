#include "Motion/Gesture.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace arc
{

namespace
{
constexpr double kPi = 3.14159265358979323846;
}

void Gesture::sample (double phase, float& dr, float& da) const noexcept
{
    if (! valid)
    {
        dr = da = 0.0f;
        return;
    }
    phase -= std::floor (phase);
    const double x = phase * kPoints;
    const int i0 = static_cast<int> (x) % kPoints;
    const int i1 = (i0 + 1) % kPoints;
    const float f = static_cast<float> (x - std::floor (x));
    dr = dRadius[static_cast<size_t> (i0)] + f * (dRadius[static_cast<size_t> (i1)] - dRadius[static_cast<size_t> (i0)]);
    // Angles are periodic: interpolate along the shorter arc so orbits pass through +-pi
    // (and wrap from the last point to the first) without spinning back.
    const float a0 = dAngle[static_cast<size_t> (i0)];
    const float step = wrapAngle (dAngle[static_cast<size_t> (i1)] - a0);
    da = wrapAngle (a0 + f * step);
}

float wrapAngle (float a) noexcept
{
    constexpr float twoPi = 6.2831853f;
    return a - twoPi * std::floor ((a + 3.14159265f) / twoPi);
}

double divisionBeats (int division, double beatsPerBar) noexcept
{
    switch (division)
    {
        case 0: return 1.0;
        case 1: return 2.0;
        case 2: return beatsPerBar;
        case 3: return 2.0 * beatsPerBar;
        case 4: return 4.0 * beatsPerBar;
        case 5: return 8.0 * beatsPerBar;
        default: break;
    }
    return beatsPerBar;
}

Gesture buildGesture (const std::vector<GestureSample>& raw, double beatsPerSecond, double beatsPerBar)
{
    Gesture g;
    if (raw.size() < 2)
        return g;
    const double t0 = raw.front().timeSeconds;
    const double duration = raw.back().timeSeconds - t0;
    if (duration < 0.05)
        return g;

    // Unwrap the angle so a drag across +-pi is continuous.
    std::vector<double> ang (raw.size());
    ang[0] = raw[0].angle;
    for (size_t i = 1; i < raw.size(); ++i)
    {
        double d = raw[i].angle - raw[i - 1].angle;
        while (d > kPi)
            d -= 2.0 * kPi;
        while (d < -kPi)
            d += 2.0 * kPi;
        ang[i] = ang[i - 1] + d;
    }

    // Uniform resampling over the recorded time.
    std::array<double, Gesture::kPoints> r {}, a {};
    size_t k = 0;
    for (int p = 0; p < Gesture::kPoints; ++p)
    {
        const double t = t0 + duration * p / Gesture::kPoints;
        while (k + 1 < raw.size() - 1 && raw[k + 1].timeSeconds < t)
            ++k;
        const double ta = raw[k].timeSeconds, tb = raw[k + 1].timeSeconds;
        const double f = tb > ta ? std::clamp ((t - ta) / (tb - ta), 0.0, 1.0) : 0.0;
        r[static_cast<size_t> (p)] = static_cast<double> (raw[k].radius) + f * static_cast<double> (raw[k + 1].radius - raw[k].radius);
        a[static_cast<size_t> (p)] = ang[k] + f * (ang[k + 1] - ang[k]);
    }

    // Light smoothing (removes pointer jitter), 5-tap binomial, open ends.
    auto smooth = [] (std::array<double, Gesture::kPoints>& v)
    {
        auto src = v;
        for (int i = 0; i < Gesture::kPoints; ++i)
        {
            double acc = 0, w = 0;
            const double taps[5] = { 1, 4, 6, 4, 1 };
            for (int j = -2; j <= 2; ++j)
            {
                const int idx = std::clamp (i + j, 0, Gesture::kPoints - 1);
                acc += taps[j + 2] * src[static_cast<size_t> (idx)];
                w += taps[j + 2];
            }
            v[static_cast<size_t> (i)] = acc / w;
        }
    };
    smooth (r);
    smooth (a);

    // Loop closure: cross-fade the last 12 % back to the start so the cycle is seamless.
    // The angle closes modulo whole turns: a drag that circles the core becomes an orbit
    // instead of unwinding at the end of every cycle.
    const double turns = std::round ((a[Gesture::kPoints - 1] - a[0]) / (2.0 * kPi));
    const double angleEnd = a[0] + 2.0 * kPi * turns;
    const int fade = Gesture::kPoints / 8;
    for (int i = 0; i < fade; ++i)
    {
        const int idx = Gesture::kPoints - fade + i;
        const double w = static_cast<double> (i + 1) / (fade + 1);
        const double sm = w * w * (3.0 - 2.0 * w);
        r[static_cast<size_t> (idx)] += sm * (r[0] - r[static_cast<size_t> (idx)]);
        a[static_cast<size_t> (idx)] += sm * (angleEnd - a[static_cast<size_t> (idx)]);
    }

    // Offsets from the start position: radius bounded, angle wrapped to (-pi, pi].
    for (int i = 0; i < Gesture::kPoints; ++i)
    {
        g.dRadius[static_cast<size_t> (i)] = static_cast<float> (std::clamp (r[static_cast<size_t> (i)] - r[0], -1.0, 1.0));
        g.dAngle[static_cast<size_t> (i)] = wrapAngle (static_cast<float> (a[static_cast<size_t> (i)] - a[0]));
    }

    g.durationSeconds = static_cast<float> (duration);
    if (beatsPerSecond > 0.0)
    {
        const double beats = duration * beatsPerSecond;
        double best = divisionBeats (0, beatsPerBar);
        for (int d = 0; d < 6; ++d)
        {
            const double cand = divisionBeats (d, beatsPerBar);
            if (std::abs (std::log2 (cand / beats)) < std::abs (std::log2 (best / beats)))
                best = cand;
        }
        g.durationBeats = static_cast<float> (best);
    }
    g.valid = true;
    return g;
}

Gesture orbitGesture (float seconds, float beats, float turns, float radiusWobble)
{
    Gesture g;
    const float wobbleCycles = 2.0f * std::max (1.0f, std::round (std::abs (turns)));
    for (int i = 0; i < Gesture::kPoints; ++i)
    {
        const double t = static_cast<double> (i) / Gesture::kPoints;
        g.dAngle[static_cast<size_t> (i)] = wrapAngle (static_cast<float> (2.0 * kPi * static_cast<double> (turns) * t));
        g.dRadius[static_cast<size_t> (i)] = radiusWobble * static_cast<float> (std::sin (2.0 * kPi * static_cast<double> (wobbleCycles) * t));
    }
    g.durationSeconds = std::max (0.05f, seconds);
    g.durationBeats = std::max (0.0f, beats);
    g.valid = true;
    return g;
}

Gesture swayGesture (float seconds, float beats, float angleSwing, float radiusSwing)
{
    Gesture g;
    for (int i = 0; i < Gesture::kPoints; ++i)
    {
        const double t = static_cast<double> (i) / Gesture::kPoints;
        g.dAngle[static_cast<size_t> (i)] = angleSwing * static_cast<float> (std::sin (2.0 * kPi * t));
        g.dRadius[static_cast<size_t> (i)] = radiusSwing * static_cast<float> (std::sin (4.0 * kPi * t));
    }
    g.durationSeconds = std::max (0.05f, seconds);
    g.durationBeats = std::max (0.0f, beats);
    g.valid = true;
    return g;
}

std::string Gesture::serialise() const
{
    if (! valid)
        return {};
    std::ostringstream os;
    os.precision (9); // max_digits10: text round trip is exact (DAW recall is bit-identical)
    os << "v1 " << durationSeconds << " " << durationBeats;
    for (int i = 0; i < kPoints; ++i)
        os << " " << dRadius[static_cast<size_t> (i)] << " " << dAngle[static_cast<size_t> (i)];
    return os.str();
}

Gesture Gesture::deserialise (const std::string& text)
{
    Gesture g;
    std::istringstream is (text);
    std::string version;
    is >> version;
    if (version != "v1")
        return g;
    is >> g.durationSeconds >> g.durationBeats;
    for (int i = 0; i < kPoints; ++i)
        is >> g.dRadius[static_cast<size_t> (i)] >> g.dAngle[static_cast<size_t> (i)];
    if (! is.fail() && std::isfinite (g.durationSeconds) && g.durationSeconds > 0.0f)
    {
        g.valid = true;
        for (int i = 0; i < kPoints; ++i)
        {
            auto& r = g.dRadius[static_cast<size_t> (i)];
            auto& a = g.dAngle[static_cast<size_t> (i)];
            if (! std::isfinite (r) || ! std::isfinite (a))
                g.valid = false;
            r = std::clamp (r, -1.0f, 1.0f);
            a = wrapAngle (a);
        }
    }
    return g;
}

} // namespace arc

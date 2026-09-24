#pragma once

// Bank of decaying complex phasors ("modal resonators").
//
// Each mode is z[n] = p z[n-1] + x[n] with p = r e^{j w}, output Im(z) * gain.
// This form is numerically robust in single precision at very long decays and
// low frequencies (unlike direct-form biquads), is exactly tuned (no
// interpolation), and has frequency-independent cost. Used for material body
// modes; compared against waveguide loops in docs/RESONATOR_DESIGN.md.

#include <array>

#include "Synthesis/DspCore.h"

namespace arc::dsp
{

class ModalBank
{
public:
    static constexpr int kMaxModes = 32;

    void setSampleRate (double sr) noexcept { sampleRate = sr; }

    void setNumModes (int n) noexcept { numModes = clamp (n, 0, kMaxModes); }
    int getNumModes() const noexcept { return numModes; }

    /** Sets mode i. Frequencies above 0.48 fs are silenced (gain 0). */
    void setMode (int i, double frequencyHz, double t60Seconds, float gain) noexcept
    {
        const auto idx = static_cast<size_t> (clamp (i, 0, kMaxModes - 1));
        const bool valid = frequencyHz > 0.0 && frequencyHz < 0.48 * sampleRate;
        const double w = kTwoPi * frequencyHz / sampleRate;
        const double r = valid ? std::pow (10.0, -3.0 / (std::max (t60Seconds, 1.0e-4) * sampleRate)) : 0.0;
        pr[idx] = static_cast<float> (r * std::cos (w));
        pi[idx] = static_cast<float> (r * std::sin (w));
        g[idx] = valid ? gain : 0.0f;
    }

    void reset() noexcept
    {
        re.fill (0.0f);
        im.fill (0.0f);
    }

    inline float process (float x) noexcept
    {
        float out = 0.0f;
        for (int m = 0; m < numModes; ++m)
        {
            const auto i = static_cast<size_t> (m);
            const float zr = re[i], zi = im[i];
            const float nr = pr[i] * zr - pi[i] * zi + x;
            const float ni = pr[i] * zi + pi[i] * zr;
            re[i] = nr;
            im[i] = ni;
            out += g[i] * ni;
        }
        return out;
    }

    /** Sum of squared mode amplitudes (instantaneous energy proxy). */
    float energy() const noexcept
    {
        float e = 0.0f;
        for (int m = 0; m < numModes; ++m)
        {
            const auto i = static_cast<size_t> (m);
            e += re[i] * re[i] + im[i] * im[i];
        }
        return e;
    }

private:
    std::array<float, kMaxModes> re {}, im {}, pr {}, pi {}, g {};
    int numModes = 0;
    double sampleRate = 48000.0;
};

} // namespace arc::dsp

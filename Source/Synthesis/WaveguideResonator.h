#pragma once

// A single recirculating waveguide loop: the resonator primitive every ARC
// network node is built from.
//
//      in ──►(+)──► [ delay line: k + frac ] ──► loss G(z) ──► dispersion A(z) ──► y
//             ▲                                                                   │
//             └───────────────────────── (loop / network scattering) ◄────────────┘
//
// Tuning: the integer + fractional line delay is chosen so the *total* loop
// phase delay at the fundamental (line + loss + dispersion + interpolator)
// equals fs / f0, so dispersion and damping never detune the fundamental.
//
// Interpolation: first-order allpass (Thiran-type) designed for an exact phase
// delay at f0. Chosen after the measured comparison in docs/RESONATOR_DESIGN.md:
// unity magnitude (no fraction-dependent HF damping, lossless when frozen),
// best tuning, lowest cost. Coefficients are ramped per sample between control
// updates and the integer tap uses hysteresis to minimise switching transients.

#include "Synthesis/FractionalDelay.h"
#include "Synthesis/LoopFilters.h"

namespace arc::dsp
{

inline constexpr int kMaxDispersionStages = 6;

struct ResonatorSettings
{
    double frequency = 220.0;       // fundamental, Hz
    double t60Fundamental = 3.0;    // seconds, at the fundamental
    double t60High = 1.0;           // seconds, at hfReference
    double hfReference = 5000.0;    // Hz
    double dispersion = 0.0;        // 0..0.6, fraction of period (see AllpassChain)
    int dispersionStages = 4;
    double loopSelectivity = 0.0;   // in-loop modal selectivity (dissipative off-resonance;
                                    // used only where a sustained drive feeds the loop)
    Interpolation interpolation = Interpolation::thiran1;
};

inline constexpr double kSelectivityQ = 3.0;

/** Coefficients computed at control rate; applied to the loop per sample. */
struct LoopCoefficients
{
    FractionalDelaySetting delay;
    float lossB0 = 0.99f;
    float lossA1 = 0.0f;
    float dispA = 0.0f;
    int dispStages = 0;
    double lineDelay = 0.0;      // samples delay requested from line + interpolator at f0
    double totalDelay = 0.0;     // intended loop delay fs / f0
    double lossDelay = 0.0;      // phase delay of the loss filter at f0
    double dispersionDelay = 0.0;// phase delay of the dispersion chain at f0
    SelectivityStage::Coeffs selectivity {}; // zero phase at f0 (no tuning term)
};

/** Response of the loop's filters (fractional allpass, loss, dispersion) including
    the integer line delay, at w: magnitude and total phase delay in samples. */
void loopResponse (const LoopCoefficients& c, double w, double& magnitude, double& phaseDelaySamples) noexcept;

/** Computes loop coefficients (control rate, allocation free).
    preferredK: integer tap of the previous design (-1 = none) for hysteresis. */
LoopCoefficients designLoop (const ResonatorSettings& s, double sampleRate, int maxLineDelay,
                             int preferredK = -1, int lockedStages = -1) noexcept;

/** Cheap re-tune for small frequency changes: keeps the loss and dispersion
    coefficients (and their phase delays) of `base`, recomputes only the line delay
    and the fractional allpass for the new frequency. */
LoopCoefficients retuneLoop (const LoopCoefficients& base, double frequency, double sampleRate, int maxLineDelay,
                             Interpolation interpolation, int preferredK) noexcept;

class WaveguideResonator
{
public:
    void prepare (double sampleRate, double minFrequency);
    void reset() noexcept;

    /** Applies new coefficients. When the integer tap is unchanged the
        interpolator parameter is ramped linearly over rampSamples. */
    void setCoefficients (const LoopCoefficients& c, int rampSamples = 0) noexcept;

    void configure (const ResonatorSettings& s, int rampSamples = 0) noexcept
    {
        interpolation = s.interpolation;
        setCoefficients (designLoop (s, sampleRate, line.maxDelay(), hasCoeffs ? coeffs.delay.k : -1), rampSamples);
    }

    const LoopCoefficients& getCoefficients() const noexcept { return coeffs; }
    int currentIntegerDelay() const noexcept { return current.k; }
    bool hasCoefficients() const noexcept { return hasCoeffs; }
    Interpolation getInterpolation() const noexcept { return interpolation; }
    double getSampleRate() const noexcept { return sampleRate; }
    int maxLineDelay() const noexcept { return line.maxDelay(); }

    /** Loop output for this sample (reads the line and runs the in-loop filters). */
    inline float readOutput() noexcept
    {
        if (rampRemaining > 0)
        {
            current.a += rampA;
            current.f += rampF;
            lossB0 += rampB0;
            lossA1 += rampA1;
            dispersion.a += rampDisp;
            --rampRemaining;
        }
        float y = readInterpolated (line, current, interpolation, thiranState);
        lossState = lossB0 * y + lossA1 * lossState;
        return dispersion.process (selectivity.process (lossState));
    }

    /** Loop input for this sample. */
    inline void writeInput (float x) noexcept { line.write (x); }

    /** Standalone use: self-feedback loop with external excitation. */
    inline float process (float excitation) noexcept
    {
        const float y = readOutput();
        writeInput (y + excitation);
        return y;
    }

    /** Clears only the portion of the line the current tuning uses. */
    void clearState() noexcept;

    /** Sum of squares of the samples currently circulating in the line. */
    double lineEnergy() const noexcept
    {
        double e = 0.0;
        for (int i = 1; i <= current.k + 1; ++i)
        {
            const double v = line.tap (i);
            e += v * v;
        }
        return e;
    }

private:
    DelayLine line;
    LoopCoefficients coeffs;
    FractionalDelaySetting current;
    AllpassChain<kMaxDispersionStages> dispersion;
    SelectivityStage selectivity;
    Interpolation interpolation = Interpolation::thiran1;
    float thiranState = 0.0f;
    float lossState = 0.0f;
    float rampA = 0.0f, rampF = 0.0f, rampB0 = 0.0f, rampA1 = 0.0f, rampDisp = 0.0f;
    float lossB0 = 0.99f, lossA1 = 0.0f; // live (ramped) loss coefficients
    int rampRemaining = 0;
    bool hasCoeffs = false;
    double sampleRate = 48000.0;
};

} // namespace arc::dsp

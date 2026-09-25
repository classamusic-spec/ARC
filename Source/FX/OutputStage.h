#pragma once

// Minimal global output stage. ARC is not an effects rack: the resonant network is the
// product and must sound complete with SPACE = 0 and DRIVE = 0.
//
//   width (M/S) -> small ambience (4-line FDN, Householder feedback) -> drive (ADAA
//   tanh) -> master gain -> DC blocker -> safety soft-clip (never > 0 dBFS) + NaN guard

#include <array>
#include <vector>

#include "Synthesis/DspCore.h"
#include "Synthesis/LoopFilters.h"

namespace arc::dsp
{

class OutputStage
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset() noexcept;

    /** masterGainDb: MASTER OUTPUT (-60 dB = mute, at most +6). PATCH LEVEL is applied per
        voice (ArcVoice), so each note keeps the trim of the patch it was played in;
        patchGain (linear) is the current patch's trim, taken out before DRIVE and put back
        after it, so a trim changes a patch's level and never its saturation. */
    void setParameters (float widthAmount, float spaceAmount, float driveAmount, float masterGainDb, float patchGain = 1.0f) noexcept;

    /** In place. Returns false if non-finite input was detected (block zeroed). */
    bool process (float* left, float* right, int n) noexcept;

    /** Peak (linear) and mean-square of the last processed block, per channel. */
    float lastPeak[2] {}, lastMeanSquare[2] {};

private:
    struct Line
    {
        std::vector<float> buf;
        uint32_t mask = 0, pos = 0;
        int delay = 1;
        float lp = 0.0f, damp = 0.3f, gain = 0.8f;
    };
    struct Allpass
    {
        std::vector<float> buf;
        uint32_t mask = 0, pos = 0;
        int delay = 1;
        float g = 0.6f;
        inline float process (float x) noexcept
        {
            const float d = buf[(pos - static_cast<uint32_t> (delay)) & mask];
            const float y = -g * x + d;
            buf[pos] = x + g * y;
            pos = (pos + 1) & mask;
            return y;
        }
    };

    std::array<Line, 4> lines;
    std::array<Allpass, 4> diffusers;
    Smoothed width, space, drive, gain, driveRef;
    DcBlocker dcL, dcR;
    float adaaX1[2] {}, adaaF1[2] {};
    double sr = 48000.0;
};

} // namespace arc::dsp

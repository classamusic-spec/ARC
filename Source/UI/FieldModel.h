#pragma once

// UI-side view of the engine: telemetry read once per frame (lock-free atomics),
// mapped to perceptual display values and smoothed with attack / release so the
// visuals feel inertial and physical rather than flickering with the waveform.
// Nothing here touches audio buffers.

#include <array>

#include "Engine/Telemetry.h"
#include "UI/Theme.h"

namespace arc::ui
{

struct FieldModel
{
    // Display values 0..1
    std::array<float, 5> nodeGlow {};    // CORE, A..D
    std::array<float, 10> edgeGlow {};   // energy flow per edge
    std::array<float, 10> edgeStrength {}; // coupling in use (normalised)
    std::array<float, 10> flowPhase {};  // pulse phase per edge (advances with flux)
    std::array<float, 4> nodeRadius { 0.5f, 0.5f, 0.5f, 0.5f };
    std::array<float, 4> nodeAngle {};   // radians, 0 = up
    std::array<float, 4> nodeRatio {}; // 0 until the engine has published (no audio yet)
    float exciterGlow = 0.0f;
    float activity = 0.0f;
    float freeze = 0.0f;
    float motionPhase = 0.0f;
    int activeVoices = 0;
    float cpu = 0.0f;
    float bpm = 120.0f, beatsPerBar = 4.0f;
    bool hostPlaying = false;

    // Meters (linear), with peak hold
    float meterL = 0.0f, meterR = 0.0f, holdL = 0.0f, holdR = 0.0f;
    float holdTimerL = 0.0f, holdTimerR = 0.0f;

    // Strike pulses: spawned when the engine counts a new transient.
    struct Pulse
    {
        float age = 1.0e9f;
        float strength = 0.0f;
    };
    std::array<Pulse, 6> pulses {};
    uint32_t lastStrikeCounter = 0;
    bool primed = false;

    static float energyToGlow (float e) noexcept
    {
        const float db = 10.0f * std::log10 (e + 1.0e-12f);
        return juce::jlimit (0.0f, 1.0f, (db + 62.0f) / 44.0f);
    }

    void update (Telemetry& t, float dt, bool freezeFlows)
    {
        for (size_t i = 0; i < nodeGlow.size(); ++i)
        {
            const float target = energyToGlow (Telemetry::load (t.nodeEnergy[i]));
            nodeGlow[i] = smoothTowards (nodeGlow[i], target, dt, target > nodeGlow[i] ? 0.03f : 0.22f);
        }
        for (size_t i = 0; i < edgeGlow.size(); ++i)
        {
            const float flux = Telemetry::load (t.edgeFlux[i]);
            const float target = energyToGlow (flux * 4.0f);
            edgeGlow[i] = smoothTowards (edgeGlow[i], target, dt, target > edgeGlow[i] ? 0.04f : 0.3f);
            // sin(phi) of the rotation in use; full COUPLING (0.73 rad, sin 0.67) reads as
            // full weight, the default (~0.2) is left where it was.
            const float sn = std::sqrt (juce::jmax (0.0f, Telemetry::load (t.edgeStrength[i])));
            const float s = sn + 1.18f * sn * sn * sn;
            edgeStrength[i] = smoothTowards (edgeStrength[i], juce::jlimit (0.0f, 1.0f, s), dt, 0.08f);
            if (! freezeFlows)
                flowPhase[i] = std::fmod (flowPhase[i] + dt * (0.08f + 0.9f * edgeGlow[i]), 1.0f);
        }
        for (size_t i = 0; i < 4; ++i)
        {
            nodeRadius[i] = Telemetry::load (t.nodeRadius[i]);
            nodeAngle[i] = Telemetry::load (t.nodeAngle[i]);
            if (const float r = Telemetry::load (t.nodeRatio[i]); r > 0.0f)
                nodeRatio[i] = r;
        }
        exciterGlow = smoothTowards (exciterGlow, energyToGlow (Telemetry::load (t.exciterEnergy) * 10.0f), dt, 0.08f);
        activity = smoothTowards (activity, Telemetry::load (t.networkActivity), dt, 0.15f);
        freeze = Telemetry::load (t.freezeAmount);
        motionPhase = Telemetry::load (t.motionPhase);
        activeVoices = Telemetry::load (t.activeVoices);
        cpu = smoothTowards (cpu, Telemetry::load (t.cpuLoad), dt, 0.5f);
        bpm = Telemetry::load (t.bpm);
        beatsPerBar = Telemetry::load (t.beatsPerBar);
        hostPlaying = Telemetry::load (t.hostPlaying);

        // Meters: exchange so the audio thread's max-hold restarts.
        const float pl = t.peakL.exchange (0.0f, std::memory_order_relaxed);
        const float pr = t.peakR.exchange (0.0f, std::memory_order_relaxed);
        meterL = juce::jmax (pl, meterL * std::exp (-dt / 0.28f));
        meterR = juce::jmax (pr, meterR * std::exp (-dt / 0.28f));
        auto hold = [dt] (float peak, float& h, float& timer)
        {
            if (peak >= h)
            {
                h = peak;
                timer = 1.2f;
            }
            else if ((timer -= dt) < 0.0f)
                h = juce::jmax (peak, h * std::exp (-dt / 0.5f));
        };
        hold (pl, holdL, holdTimerL);
        hold (pr, holdR, holdTimerR);

        // Strike pulses
        const auto counter = Telemetry::load (t.strikeCounter);
        if (! primed)
        {
            lastStrikeCounter = counter;
            primed = true;
        }
        if (counter != lastStrikeCounter)
        {
            lastStrikeCounter = counter;
            auto* oldest = &pulses[0];
            for (auto& p : pulses)
                if (p.age > oldest->age)
                    oldest = &p;
            oldest->age = 0.0f;
            oldest->strength = juce::jlimit (0.15f, 1.0f, Telemetry::load (t.lastStrikeEnergy));
        }
        for (auto& p : pulses)
            p.age += dt;
    }

    bool isQuiet() const noexcept
    {
        float m = activity;
        for (auto g : nodeGlow)
            m = juce::jmax (m, g);
        for (auto& p : pulses)
            if (p.age < 1.5f)
                return false;
        return m < 0.01f && meterL < 1.0e-4f && meterR < 1.0e-4f;
    }
};

} // namespace arc::ui

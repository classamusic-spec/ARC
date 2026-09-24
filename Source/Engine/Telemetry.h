#pragma once

// Audio -> UI telemetry. Written by the audio thread once per block with relaxed
// atomic stores, read by the UI at its own frame rate. Lock-free, allocation-free,
// no audio buffers cross threads — only smoothed energies and state.

#include <array>
#include <atomic>
#include <cstdint>

#include "Synthesis/CouplingMatrix.h"

namespace arc
{

struct Telemetry
{
    // Network activity (summed over voices, mean square of loop outputs).
    std::array<std::atomic<float>, dsp::kNumNodes> nodeEnergy {};
    std::array<std::atomic<float>, dsp::kNumEdges> edgeFlux {};   // energy per sample across each edge
    std::array<std::atomic<float>, dsp::kNumEdges> edgeStrength {}; // rotation sin^2(phi) actually in use
    std::atomic<float> exciterEnergy { 0.0f };
    std::atomic<float> networkActivity { 0.0f }; // 0..1 perceptual activity

    // Effective node geometry after motion (radius 0..1, angle radians).
    std::array<std::atomic<float>, 4> nodeRadius {};
    std::array<std::atomic<float>, 4> nodeAngle {};
    // Effective node frequency ratio (after material, tension, radius) for display.
    std::array<std::atomic<float>, 4> nodeRatio {};

    // Output meters (peak, linear) and state.
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };
    std::atomic<float> rmsL { 0.0f }, rmsR { 0.0f };
    std::atomic<float> freezeAmount { 0.0f };
    std::atomic<int> activeVoices { 0 };
    std::atomic<uint32_t> strikeCounter { 0 }; // increments on each transient excitation
    std::atomic<float> lastStrikeEnergy { 0.0f };
    std::atomic<float> motionPhase { 0.0f };
    std::atomic<float> bpm { 120.0f };        // host tempo (120 when unknown) for gesture tempo snap
    std::atomic<float> beatsPerBar { 4.0f };
    std::atomic<bool> hostPlaying { false };
    std::atomic<float> cpuLoad { 0.0f };       // fraction of real time used by the engine
    std::atomic<uint32_t> nonFiniteEvents { 0 };
    std::atomic<uint32_t> blockCounter { 0 };  // increments every published block (UI: "engine running")

    template <typename T>
    static void store (std::atomic<T>& a, T v) noexcept { a.store (v, std::memory_order_relaxed); }
    template <typename T>
    static T load (const std::atomic<T>& a) noexcept { return a.load (std::memory_order_relaxed); }
};

} // namespace arc

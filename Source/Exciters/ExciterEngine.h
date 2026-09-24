#pragma once

// Owns the four exciters of one voice and maps velocity / EXCITE / MPE pressure to
// each exciter's physical controls. Only the exciter chosen at note-on runs.
//
// EXCITE macro (input energy) per type:
//   STRIKE  amplitude + hardness (harder, shorter contact)
//   PLUCK   amplitude + release sharpness
//   BOW     bow speed + pressure (sustain energy)
//   AIR     breath pressure + jet drive
// MPE pressure adds to the sustained drive of BOW / AIR and to strike energy at onset.

#include "Engine/EngineTypes.h"
#include "Exciters/AirExciter.h"
#include "Exciters/BowExciter.h"
#include "Exciters/PluckExciter.h"
#include "Exciters/StrikeExciter.h"

namespace arc::dsp
{

class ExciterEngine
{
public:
    void prepare (double sampleRate, int maxLoopSamples)
    {
        envAttack = static_cast<float> (1.0 - std::exp (-1.0 / (0.01 * sampleRate)));
        envRelease = static_cast<float> (1.0 - std::exp (-1.0 / (0.12 * sampleRate)));
        strike.prepare (sampleRate);
        pluck.prepare (sampleRate, maxLoopSamples);
        bow.prepare (sampleRate);
        air.prepare (sampleRate, maxLoopSamples);
    }

    void reset() noexcept
    {
        coreEnv = 0.0f;
        driveGain = 1.0f;
        strike.reset();
        pluck.reset();
        bow.reset();
        air.reset();
    }

    /** velocity 0..1, excite 0..1 (macro), pressure 0..1 (MPE, 0 if unused). */
    void noteOn (ExciterType t, const ExciterParams& p, float velocity, float excite, float pressure,
                 float coreLoopSamples, uint32_t seed) noexcept
    {
        type = t;
        const float v = clamp (velocity, 0.0f, 1.0f);
        const float e = clamp (excite, 0.0f, 1.0f);
        coreEnv = 0.0f;
        driveGain = 1.0f;
        targetAmplitude = 0.35f + 0.65f * clamp (0.5f * (v + e) + 0.3f * pressure, 0.0f, 1.2f);
        switch (type)
        {
            case ExciterType::strike:
            {
                const float energy = std::pow (v, 1.25f) * (0.25f + 0.95f * e) * (1.0f + 0.3f * pressure);
                strike.trigger (energy, p.strikeHardness + 0.35f * (e - 0.5f) + 0.25f * (v - 0.6f), p.strikeLength,
                                p.strikeTone, seed);
                break;
            }
            case ExciterType::pluck:
            {
                const float energy = std::pow (v, 1.15f) * (0.25f + 0.95f * e);
                pluck.trigger (energy, p.pluckPosition, p.pluckTone + 0.3f * (e - 0.5f), coreLoopSamples, seed);
                break;
            }
            case ExciterType::bow:
                bow.start (bowEnergy (v, e, pressure), p.bowSpeed, bowPressure (p, v, e, pressure), p.bowFriction, seed);
                break;
            case ExciterType::air:
                air.start (airEnergy (v, e, pressure), p.airFlow, p.airTurbulence, p.airTone, coreLoopSamples, seed);
                break;
            case ExciterType::count:
                break;
        }
    }

    /** Control-rate update for sustained exciters. */
    void update (const ExciterParams& p, float velocity, float excite, float pressure, float coreLoopSamples) noexcept
    {
        const float v = clamp (velocity, 0.0f, 1.0f);
        const float e = clamp (excite, 0.0f, 1.0f);
        if (type == ExciterType::bow && bow.isActive())
            bow.setTargets (bowEnergy (v, e, pressure), p.bowSpeed, bowPressure (p, v, e, pressure), p.bowFriction);
        else if (type == ExciterType::air && air.isActive())
            air.setTargets (airEnergy (v, e, pressure), p.airFlow, p.airTurbulence, p.airTone, coreLoopSamples);
    }

    /** Per-pass CORE loss at the fundamental (control rate), for loss-relative drives. */
    void setLoopLoss (float loss, float frequency) noexcept { air.setLoopLoss (loss, frequency); }

    void noteOff() noexcept
    {
        bow.release();
        air.release();
    }

    /** One sample of excitation force. coreSignal: CORE loop output (feedback types). */
    inline float tick (float coreSignal) noexcept
    {
        switch (type)
        {
            case ExciterType::strike: return strike.tick();
            case ExciterType::pluck:  return pluck.tick();
            case ExciterType::bow:    return regulate (bow.tick (coreSignal), coreSignal, 0.45f);
            case ExciterType::air:    return regulate (air.tick (coreSignal), coreSignal, 0.28f);
            case ExciterType::count:  break;
        }
        return 0.0f;
    }

    bool isActive() const noexcept
    {
        switch (type)
        {
            case ExciterType::strike: return strike.isActive();
            case ExciterType::pluck:  return pluck.isActive();
            case ExciterType::bow:    return bow.isActive();
            case ExciterType::air:    return air.isActive();
            case ExciterType::count:  break;
        }
        return false;
    }

    bool isSustained() const noexcept { return type == ExciterType::bow || type == ExciterType::air; }
    ExciterType getType() const noexcept { return type; }

    /** How widely the exciter spreads energy into the outer nodes (vs CORE only). */
    float nodeSpread() const noexcept
    {
        switch (type)
        {
            case ExciterType::strike: return 0.8f;
            case ExciterType::pluck:  return 0.45f;
            case ExciterType::bow:    return 0.3f;
            case ExciterType::air:    return 0.55f;
            case ExciterType::count:  break;
        }
        return 0.5f;
    }

private:
    /** Drive regulation for sustained exciters: follows the CORE amplitude and eases
        the injected drive above `ceiling * targetAmplitude`, so high-Q (long decay)
        materials settle at a musical level instead of accumulating energy. */
    inline float regulate (float drive, float coreSignal, float ceiling) noexcept
    {
        const float a = std::abs (coreSignal);
        coreEnv += (a > coreEnv ? envAttack : envRelease) * (a - coreEnv);
        const float limit = ceiling * targetAmplitude;
        const float wanted = coreEnv > limit ? limit / coreEnv : 1.0f;
        driveGain += 0.002f * (wanted * wanted - driveGain);
        return drive * driveGain;
    }

    static float bowEnergy (float v, float e, float pressure) noexcept
    {
        return clamp ((0.45f + 0.55f * v) * (0.3f + 0.9f * e) + 0.5f * pressure, 0.0f, 1.5f);
    }
    static float bowPressure (const ExciterParams& p, float v, float e, float pressure) noexcept
    {
        return clamp (p.bowPressure + 0.2f * (v - 0.6f) + 0.2f * (e - 0.5f) + 0.35f * pressure, 0.0f, 1.0f);
    }
    static float airEnergy (float v, float e, float pressure) noexcept
    {
        return clamp ((0.4f + 0.6f * v) * (0.3f + 0.9f * e) + 0.6f * pressure, 0.0f, 1.5f);
    }

    StrikeExciter strike;
    PluckExciter pluck;
    BowExciter bow;
    AirExciter air;
    ExciterType type = ExciterType::strike;
    float coreEnv = 0.0f, driveGain = 1.0f, targetAmplitude = 1.0f;
    float envAttack = 0.01f, envRelease = 0.001f;
};

} // namespace arc::dsp

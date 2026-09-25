#include "Engine/ArcVoice.h"

#include <cmath>

namespace arc
{

using namespace dsp;

namespace
{
/** Exciter-dependent string-likeness of the CORE: a plucked or bowed CORE behaves
    more like a string (keeps its harmonic series) than a struck body does. */
float coreSelectivityFactor (ExciterType t) noexcept
{
    switch (t)
    {
        case ExciterType::strike: return 1.0f;
        case ExciterType::pluck:  return 0.3f;
        case ExciterType::bow:    return 0.55f;
        case ExciterType::air:    return 0.6f;
        case ExciterType::count:  break;
    }
    return 1.0f;
}

/** Loss design point for a loop at frequency f: the material's natural decay at the
    fundamental and at an HF reference (the one-pole loss matches both). Modal
    selectivity is a separate, zero-phase stage (SelectivityStage). */
void lossDesign (const EffectiveMaterial& m, float f, float sampleRate, float& t60Fund, float& hfRef,
                 float& t60High) noexcept
{
    t60Fund = m.t60At (f);
    hfRef = std::min (0.4f * sampleRate, std::max (3000.0f, 6.0f * f));
    t60High = m.t60At (hfRef);
}

constexpr double kMinNodeFrequency = 8.0;
constexpr double kMinCoreFrequency = 16.0;
constexpr float kSilenceLevel = 1.0e-10f; // mean square (-100 dB)
} // namespace

void ArcVoice::prepare (double newSampleRate, int newControlInterval)
{
    sampleRate = newSampleRate;
    controlInterval = newControlInterval;
    network.prepare (sampleRate, kMinNodeFrequency, controlInterval);
    exciter.prepare (sampleRate, static_cast<int> (sampleRate / kMinCoreFrequency) + 8);
    reset();
}

void ArcVoice::reset() noexcept
{
    network.reset();
    exciter.reset();
    state = State::idle;
    gain = 1.0f;
    gainStep = 0.0f;
    patchGain = 1.0f;
    patchGainStep = 0.0f;
    level = levelAcc = 0.0f;
    levelCount = 0;
    nodeEnergy.fill (0.0f);
    smoothEnergy.fill (0.0f);
    edgeFlux.fill (0.0f);
    freqShift.fill (0.0);
    gainFactor.fill (1.0);
    sustainedByPedal = false;
    silentBlocks = 0;
    silentSamples = 0;
}

void ArcVoice::start (int newNote, int newChannel, float newVelocity, uint32_t seed, const VoiceControl& ctl) noexcept
{
    note = newNote;
    channel = newChannel;
    velocity = newVelocity;
    exciter.reset();
    exciter.selectType (ctl.exciterType);
    pressure = 0.0f;
    noteBend = 0.0f;
    timbre = 0.0f;
    pitchNote = static_cast<float> (newNote);
    glideCoeff = 0.0f;
    {
        Random jr;
        jr.seed (seed ^ 0x5bd1e995u);
        for (auto& j : unitJitter)
            j = jr.bipolar();
    }
    governorCaptured = false;
    governorScale = 1.0f;
    voiceFreeze = 0.0f;
    samplesToControl = controlInterval;
    blockLevelAcc = blockExciterAcc = 0.0f;
    blockSamples = 0;
    state = State::active;
    gain = 1.0f;
    gainStep = 0.0f;
    takePatchTrim (ctl);
    releaseT60Mult = 1.0f;
    sustainedByPedal = false;
    nonFiniteDetected = false;
    silentBlocks = 0;
    silentSamples = 0;
    smoothEnergy.fill (0.0f);
    freqShift.fill (0.0);
    gainFactor.fill (1.0);
    levelAcc = 0.0f;
    levelCount = 0;
    controlCounter = 0;
    intonationCents = 0.0f;
    intonationPrev = 0.0f;
    intonationClock = 0;
    intonationLastCrossing = -1.0;
    intonationPeriodAcc = 0.0;
    intonationPeriods = 0;
    intonationFilter.reset();
    intonationFilter.set (SelectivityStage::design (kTwoPi * clamp (midiToHz (note), 16.0, 0.42 * sampleRate) / sampleRate,
                                                    2.0, 0.98));

    updateControl (ctl, true);
    if (ctl.couplingCompensation)
    {
        // Two passes: corrections depend (weakly) on the corrected tuning.
        for (int pass = 0; pass < 2; ++pass)
        {
            const auto corr = network.estimateModeCorrections (intendedFrequency);
            for (int i = 0; i < kNumNodes; ++i)
            {
                freqShift[static_cast<size_t> (i)] = corr[static_cast<size_t> (i)].frequencyShift;
                gainFactor[static_cast<size_t> (i)] = corr[static_cast<size_t> (i)].gainFactor;
            }
            updateControl (ctl, true);
        }
    }
    network.clearState();
    triggerExciter (ctl, seed);
}

void ArcVoice::triggerExciter (const VoiceControl& ctl, uint32_t seed) noexcept
{
    auto params = ctl.exciter;
    params.strikeHardness = clamp (params.strikeHardness + ctl.material.hardnessBias, 0.0f, 1.0f);
    params.strikeTone = clamp (params.strikeTone + ctl.material.toneBias + 0.25f * timbre, 0.0f, 1.0f);
    params.pluckTone = clamp (params.pluckTone + ctl.material.toneBias + 0.25f * timbre, 0.0f, 1.0f);
    params.airTone = clamp (params.airTone + ctl.material.toneBias + 0.25f * timbre, 0.0f, 1.0f);
    exciter.noteOn (ctl.exciterType, params, velocity, ctl.excite, pressure,
                    static_cast<float> (sampleRate / coreFrequency), seed);
}

void ArcVoice::restrike (float newVelocity, uint32_t seed, const VoiceControl& ctl) noexcept
{
    governorCaptured = false; // a frozen voice re-struck freezes its new energy
    velocity = newVelocity;
    state = State::active;
    gain = 1.0f;
    gainStep = 0.0f;
    takePatchTrim (ctl); // a note played now belongs to the current patch
    sustainedByPedal = false;
    silentBlocks = 0;
    silentSamples = 0;
    releaseT60Mult = 1.0f;
    triggerExciter (ctl, seed);
}

void ArcVoice::glideTo (int newNote, float newVelocity, float glideSeconds, bool reexcite, uint32_t seed,
                        const VoiceControl& ctl) noexcept
{
    note = newNote;
    const double updatesPerSecond = sampleRate / controlInterval;
    glideCoeff = glideSeconds > 0.0f ? smoothingCoeff (static_cast<double> (glideSeconds) / 3.0, updatesPerSecond) : 0.0f;
    if (glideCoeff == 0.0f)
        pitchNote = static_cast<float> (newNote);
    state = State::active;
    gain = 1.0f;
    gainStep = 0.0f;
    takePatchTrim (ctl);
    sustainedByPedal = false;
    silentBlocks = 0;
    silentSamples = 0;
    releaseT60Mult = 1.0f;
    if (reexcite)
    {
        velocity = newVelocity;
        triggerExciter (ctl, seed);
    }
}

void ArcVoice::release() noexcept
{
    if (state != State::active)
        return;
    state = State::released;
    exciter.noteOff();
}

void ArcVoice::beginSteal() noexcept
{
    if (state == State::idle)
        return;
    state = State::stealing;
    const float fadeSamples = static_cast<float> (0.005 * sampleRate);
    gainStep = -gain / fadeSamples;
    exciter.noteOff();
}

void ArcVoice::updateControl (const VoiceControl& ctl, bool immediate) noexcept
{
    const auto& m = ctl.material;
    const double sr = sampleRate;
    // The voice's own exciter (chosen at note-on). Switching EXCITER only affects new
    // notes: sounding voices keep their dispersion, selectivity and loudness.
    const auto voiceExciter = exciter.getType();

    if (! immediate)
        pitchNote = static_cast<float> (note) + glideCoeff * (pitchNote - static_cast<float> (note));
    else
        pitchNote = static_cast<float> (note);
    fundamental = midiToHz (static_cast<double> (pitchNote) + static_cast<double> (ctl.bendSemitones) + static_cast<double> (noteBend));
    const double f0 = clamp (fundamental, kMinCoreFrequency, 0.42 * sr);
    if (exciter.isSustained())
        intonationFilter.set (SelectivityStage::design (kTwoPi * f0 / sr, 2.0, 0.98));

    // Release damper / palm mute / FREEZE scale every decay time.
    const float targetRelease = state == State::released
                                    ? std::pow (0.03f, std::pow (clamp (ctl.releaseDamping, 0.0f, 1.0f), 0.7f))
                                    : 1.0f;
    // Per-block smoothing expressed in time (2 ms / 1.2 ms), so every QUALITY behaves alike.
    const float blockSeconds = static_cast<float> (controlInterval / sr);
    const float releaseKeep = std::exp (-blockSeconds / 0.002f);
    releaseT60Mult = immediate ? targetRelease : targetRelease + releaseKeep * (releaseT60Mult - targetRelease);
    pluckDampMult = voiceExciter == ExciterType::pluck ? std::exp2 (-4.0f * ctl.exciter.pluckDamp) : 1.0f;
    // FREEZE: only voices that were sounding when it engaged are captured.
    const float freezeTarget = freezeEpochAtStart < ctl.freezeEpoch ? ctl.freeze : 0.0f;
    voiceFreeze = immediate ? freezeTarget : voiceFreeze + (1.0f - std::exp (-blockSeconds / 0.0012f)) * (freezeTarget - voiceFreeze);
    const float freezeMult = 1.0f + 1.0e4f * voiceFreeze * voiceFreeze;

    // Energy governor: a frozen network is (nearly) lossless, and time-varying delays
    // can pump energy parametrically (measured in the network fuzz). Hold the energy
    // captured at freeze time: extra damping above 1.15x the reference.
    float energyTotal = 0.0f;
    for (float e : smoothEnergy)
        energyTotal += e;
    if (voiceFreeze > 0.5f)
    {
        if (! governorCaptured)
        {
            if (++governorSettle > std::max (4, static_cast<int> (0.005 * sr / controlInterval))) // ~5 ms
            {
                governorReference = std::max (energyTotal, 1.0e-12f);
                governorCaptured = true;
            }
        }
        else
        {
            const float ratio = energyTotal / governorReference;
            const float target = ratio > 1.15f ? std::pow (1.15f / ratio, 4.0f) : 1.0f;
            governorScale += 0.2f * (target - governorScale);
        }
    }
    else
    {
        governorCaptured = false;
        governorSettle = 0;
        governorScale += 0.2f * (1.0f - governorScale);
    }

    // Passive nonlinearities, only at extreme network energy (normal playing sits well
    // below the knees): dynamic damping (loss grows with energy) and coupling saturation.
    const float dynamicDamping = 1.0f / (1.0f + std::max (0.0f, energyTotal - 1.0f));
    couplingSaturation = 1.0f / (1.0f + std::max (0.0f, energyTotal - 0.5f) / 0.5f);

    const float decayCommon = releaseT60Mult * pluckDampMult * freezeMult * governorScale * dynamicDamping;
    // MPE timbre (CC74 / slide) brightens or darkens this note's high-frequency decay.
    const float timbreHf = std::exp2 (2.0f * clamp (timbre, -1.0f, 1.0f));

    auto energyFactor = [&] (int i)
    {
        const float a = std::sqrt (smoothEnergy[static_cast<size_t> (i)]);
        return 1.0 + static_cast<double> (m.energyTuning) * static_cast<double> (std::min (1.0f, a / 0.25f));
    };

    // FREEZE removes every dissipative element: selectivity fades out with it.
    const float selFreeze = 1.0f - clamp (voiceFreeze, 0.0f, 1.0f);
    const double coreDispFactor = voiceExciter == ExciterType::bow ? 0.3 : 1.0;

    // Applies the coupling corrections of node i: pre-shift the loop so the coupled
    // mode lands on the intended frequency, and lengthen the loop's decay so the
    // coupled mode keeps the intended T60 (bounded; extreme coupling still damps).
    auto applyNode = [&] (int i, double intendedFreq, float t60f, float hfRef, float t60h, double disp, double sel,
                          double loopSel, float outGain, float pan)
    {
        auto& n = settings.nodes[static_cast<size_t> (i)];
        intendedFrequency[static_cast<size_t> (i)] = intendedFreq;
        const double shift = ctl.couplingCompensation ? freqShift[static_cast<size_t> (i)] : 0.0;
        const double gf = ctl.couplingCompensation ? gainFactor[static_cast<size_t> (i)] : 1.0;
        n.frequency = std::min (intendedFreq / (1.0 + shift), 0.45 * sr);
        float decayScale = 1.0f;
        if (gf < 0.9999)
        {
            const double period = 1.0 / std::max (intendedFreq, 1.0);
            const double gt = gainForT60 (period, t60f);
            const double gl = std::min (0.99995, gt / gf);
            const double t60Loop = -3.0 * period / std::log10 (gl);
            decayScale = static_cast<float> (clamp (t60Loop / std::max (1.0e-4, static_cast<double> (t60f)), 1.0, 3.0));
        }
        n.t60Fundamental = t60f * decayScale;
        n.t60High = std::min (t60h * timbreHf, t60f) * decayScale;
        n.hfReference = hfRef;
        n.dispersion = disp;
        n.dispersionStages = ctl.dispersionStages;
        n.selectivity = sel;
        n.loopSelectivity = loopSel * static_cast<double> (selFreeze);
        n.outputGain = outGain;
        n.pan = pan;
    };

    // --- CORE --------------------------------------------------------------------------
    {
        const double fCore = f0 * std::exp2 (static_cast<double> (ctl.chaosDetuneCents[0] + intonationCents) / 1200.0) * energyFactor (0);
        const float fc = static_cast<float> (fCore);
        float t60f, hfRef, t60h;
        lossDesign (m, fc, static_cast<float> (sr), t60f, hfRef, t60h);
        const double coreSel = m.coreSelectivity * coreSelectivityFactor (voiceExciter);
        // Sustained drives (bow / air) lock onto whichever CORE mode is most resonant;
        // in-loop selectivity keeps them on the fundamental of high-Q materials.
        const double coreLoopSel = (voiceExciter == ExciterType::bow || voiceExciter == ExciterType::air)
                                       ? 0.85 * static_cast<double> (m.coreSelectivity)
                                       : 0.0;
        applyNode (kCore, fCore, t60f * decayCommon, hfRef, t60h * decayCommon * std::sqrt (pluckDampMult),
                   static_cast<double> (m.coreDispersion) * coreDispFactor, coreSel, coreLoopSel, m.coreLevel, 0.0f);
        coreFrequency = settings.nodes[kCore].frequency;
    }

    // --- A..D --------------------------------------------------------------------------
    for (int i = 0; i < 4; ++i)
    {
        const auto u = static_cast<size_t> (i);
        const double jitterCents = 40.0 * static_cast<double> (unitJitter[u]) * static_cast<double> (clamp (ctl.chaos, 0.0f, 1.0f));
        const double ratio = static_cast<double> (m.nodeRatio[u]) * std::exp2 (static_cast<double> (ctl.nodeOffsetOctaves[u]))
                             * std::exp2 ((static_cast<double> (m.nodeDetuneCents[u] + ctl.chaosDetuneCents[u + 1]) + jitterCents) / 1200.0)
                             * energyFactor (i + 1);
        const double f = clamp (f0 * ratio, kMinNodeFrequency, 0.45 * sr);
        const float ff = static_cast<float> (f);
        float t60f, hfRef, t60h;
        lossDesign (m, ff, static_cast<float> (sr), t60f, hfRef, t60h);
        // Nodes pushed above ~0.4 fs fade out instead of aliasing onto a wrong partial.
        const float nyqFade = clamp ((0.45f * static_cast<float> (sr) - static_cast<float> (f0 * ratio))
                                         / (0.05f * static_cast<float> (sr)),
                                     0.0f, 1.0f);
        applyNode (i + 1, f, t60f * ctl.nodeDecayMult[u] * decayCommon, hfRef,
                   t60h * ctl.nodeDecayMult[u] * ctl.nodeDampMult[u] * decayCommon * std::sqrt (pluckDampMult),
                   m.nodeDispersion, m.nodeSelectivity, 0.0, m.level[u] * ctl.nodeLevel[u] * nyqFade, ctl.nodePan[u]);
        injectWeights[u + 1] = exciter.nodeSpread() * m.inject[u] * nyqFade;
    }
    injectWeights[kCore] = 1.0f;

    // A bowed / blown CORE must stay the dominant resonator. At strong coupling its mode
    // is pulled beyond any retuning (measured: > 12 % at COUPLING 0.65 on a WEB, directly
    // through the spokes and indirectly through strongly coupled nodes) and the drive
    // can no longer sustain it: the "wolf tone" limit of real bowed strings. Sustained
    // exciters soft-limit each edge's rotation (tanh knee: 0.35 rad on CORE<->node
    // spokes, 0.6 rad on node<->node edges); moderate settings are barely touched
    // (-10 % at the default COUPLING). See docs/EXCITERS.md, "Playability maps".
    const bool sustainedDrive = exciter.isSustained();
    for (int e = 0; e < kNumEdges; ++e)
    {
        float theta = ctl.edgeTheta[static_cast<size_t> (e)] * m.couplingScale * couplingSaturation;
        if (sustainedDrive)
        {
            const float knee = e < 4 ? 0.35f : 0.6f; // radians of rotation
            const float phi = 2.0f * std::atan (0.5f * theta);
            theta = 2.0f * std::tan (0.5f * knee * std::tanh (phi / knee));
        }
        settings.edgeTheta[static_cast<size_t> (e)] = theta;
    }

    network.configure (settings, immediate);

    if (! immediate && ctl.couplingCompensation && (controlCounter & 3) == 0)
    {
        // Under-relaxed fixed-point iteration (every 4 control blocks, ~5 ms time
        // constant): the corrections of coupled loops depend on each other.
        const auto corr = network.estimateModeCorrections (intendedFrequency);
        for (int i = 0; i < kNumNodes; ++i)
        {
            freqShift[static_cast<size_t> (i)] += 0.25 * (corr[static_cast<size_t> (i)].frequencyShift - freqShift[static_cast<size_t> (i)]);
            gainFactor[static_cast<size_t> (i)] += 0.25 * (corr[static_cast<size_t> (i)].gainFactor - gainFactor[static_cast<size_t> (i)]);
        }
    }
    else if (! ctl.couplingCompensation)
    {
        freqShift.fill (0.0);
        gainFactor.fill (1.0);
    }
    ++controlCounter;

    // Loudness normalisation (measured, see DEVELOPMENT_LOG): per material x exciter
    // offset, and a per-exciter pitch slope (a strike's momentum spreads over longer
    // periods at low pitch; sustained drives regulate their own level).
    {
        const int ex = static_cast<int> (voiceExciter);
        static constexpr float kPitchSlope[4] = { -0.55f, -0.05f, 0.08f, 0.0f };
        const float slope = kPitchSlope[clamp (ex, 0, 3)];
        outputNorm = m.outputGain * dbToGain (m.exciterGainDb[static_cast<size_t> (clamp (ex, 0, 3))])
                     * static_cast<float> (std::pow (clamp (f0 / 261.63, 0.125, 8.0), static_cast<double> (slope)));
    }

    // Loss-relative drives (AIR) need the CORE's per-pass loss at its fundamental.
    {
        const auto& cc = network.loop (kCore).getCoefficients();
        const double w0 = kTwoPi * coreFrequency / sr;
        const double g = OnePoleLoss::magnitude (cc.lossB0, cc.lossA1, w0) * gainFactor[0];
        exciter.setLoopLoss (static_cast<float> (1.0 - std::min (g, 0.99999)), static_cast<float> (coreFrequency));
    }

    if (! immediate)
        exciter.update (ctl.exciter, velocity, ctl.excite, pressure, static_cast<float> (sampleRate / coreFrequency));

    // PATCH LEVEL: a note keeps the trim of the patch it was played in. After a preset change
    // its tail holds that trim (a quiet patch's +18 dB must not lift the ringing tail of a
    // loud one); notes of the current patch follow the parameter, ramped over the block.
    const float patchTarget = patchEpochAtStart == ctl.patchEpoch ? ctl.patchGain : patchGain;
    if (immediate)
    {
        patchGain = patchTarget;
        patchGainStep = 0.0f;
    }
    else
        patchGainStep = (patchTarget - patchGain) / static_cast<float> (std::max (1, controlInterval));
}

void ArcVoice::trackIntonation (float coreSample) noexcept
{
    // Band-pass the CORE around the intended fundamental, time positive-going zero
    // crossings with sub-sample interpolation, and slowly steer the loop tuning so the
    // self-oscillation of BOW / AIR lands on f0 (+-60 cents authority).
    const float bp = intonationFilter.process (coreSample) - coreSample * (1.0f - intonationFilter.s);
    ++intonationClock;
    if (intonationPrev <= 0.0f && bp > 0.0f)
    {
        const double frac = intonationPrev / (intonationPrev - bp);
        const double t = static_cast<double> (intonationClock) - 1.0 + frac;
        if (intonationLastCrossing > 0.0)
        {
            const double period = t - intonationLastCrossing;
            if (period > 1.0)
            {
                intonationPeriodAcc += period;
                // ~8 periods above 100 Hz, fewer for very low notes (>= ~40 ms per update).
                const int needed = static_cast<int> (clamp (fundamental / 12.5, 3.0, 8.0));
                if (++intonationPeriods >= needed)
                {
                    const double measured = sampleRate * intonationPeriods / intonationPeriodAcc;
                    const double target = clamp (fundamental, 16.0, 0.42 * sampleRate);
                    const double errCents = 1200.0 * std::log2 (target / measured);
                    if (std::abs (errCents) < 150.0)
                        intonationCents = static_cast<float> (clamp (static_cast<double> (intonationCents) + 0.5 * errCents, -60.0, 60.0));
                    intonationPeriodAcc = 0.0;
                    intonationPeriods = 0;
                }
            }
        }
        intonationLastCrossing = t;
    }
    intonationPrev = bp;
}

void ArcVoice::render (float* outL, float* outR, int n, const VoiceControl& ctl) noexcept
{
    int done = 0;
    while (done < n && state != State::idle)
    {
        if (samplesToControl <= 0)
        {
            if (blockSamples > 0)
                finishBlock (blockSamples, ctl);
            if (state == State::idle)
                break;
            if (pendingInterval > 0 && pendingInterval != controlInterval)
            {
                controlInterval = pendingInterval;
                network.setControlInterval (controlInterval);
            }
            pendingInterval = 0;
            updateControl (ctl, false);
            samplesToControl = controlInterval;
        }
        const int len = std::min (n - done, samplesToControl);
        renderSpan (outL + done, outR + done, len);
        samplesToControl -= len;
        blockSamples += len;
        done += len;
    }
}

void ArcVoice::renderSpan (float* outL, float* outR, int n) noexcept
{
    float y[kNumNodes];
    float lAcc = 0.0f, eAcc = 0.0f;
    const float norm = outputNorm;
    const bool rawCore = exciter.isSustained();
    const bool trackPitch = rawCore && state == State::active && voiceFreeze < 0.5f;
    // A frozen voice holds its resonance; sustained drives are faded out ("block new
    // excitation") so a lossless network is not fed forever.
    const float drive = rawCore ? 1.0f - voiceFreeze : 1.0f;
    for (int s = 0; s < n; ++s)
    {
        network.readOutputs (y);
        float nodeForce;
        const float e = exciter.tick (y[kCore], nodeForce) * drive;
        network.writeInputs (y, e, nodeForce * drive, injectWeights.data(), rawCore);
        if (trackPitch)
            trackIntonation (y[kCore]);
        float l, r;
        network.pickup (y, l, r);
        const float g = gain * norm * patchGain;
        l *= g;
        r *= g;
        patchGain += patchGainStep;
        if (gainStep != 0.0f)
            gain = std::max (0.0f, gain + gainStep);
        outL[s] += l;
        outR[s] += r;
        lAcc += l * l + r * r;
        eAcc += e * e;
    }
    blockLevelAcc += lAcc;
    blockExciterAcc += eAcc;
}

void ArcVoice::finishBlock (int n, const VoiceControl&) noexcept
{
    nodeEnergy = network.takeNodeEnergy();
    // Energy-dependent tuning must follow the *envelope* (~40 ms), not the waveform:
    // block energies fluctuate at the vibration rate, and a faster smoother frequency-
    // modulates the loops at f0 (measured: sidebands filling pluck-position notches).
    const float envCoeff = 1.0f - std::exp (-static_cast<float> (n) / (0.04f * static_cast<float> (sampleRate)));
    for (int i = 0; i < kNumNodes; ++i)
        smoothEnergy[static_cast<size_t> (i)] += envCoeff * (nodeEnergy[static_cast<size_t> (i)] - smoothEnergy[static_cast<size_t> (i)]);
    edgeFlux = network.edgeFlux (nodeEnergy);
    const float blockLevel = blockLevelAcc / static_cast<float> (std::max (1, n));
    level += 0.3f * (blockLevel - level);
    exciterEnergy = blockExciterAcc / static_cast<float> (std::max (1, n));
    blockLevelAcc = blockExciterAcc = 0.0f;
    blockSamples = 0;
    ++age;

    if (! std::isfinite (blockLevel))
    {
        // Must never happen (see stability fuzz tests); contain it if it does.
        nonFiniteDetected = true;
        network.reset();
        exciter.reset();
        state = State::idle;
        level = 0.0f;
        return;
    }

    if (state == State::stealing)
    {
        if (gain <= 0.0f)
        {
            state = State::idle;
            gainStep = 0.0f;
        }
        return;
    }

    const bool excitationDone = ! exciter.isActive() && (state == State::released || ! exciter.isSustained());
    if (excitationDone && blockLevel < kSilenceLevel && voiceFreeze < 0.5f)
    {
        silentSamples += n;
        if (++silentBlocks > 4 && silentSamples > static_cast<int> (0.021 * sampleRate)) // ~21 ms of silence
            state = State::idle;
    }
    else
    {
        silentBlocks = 0;
        silentSamples = 0;
    }
}

} // namespace arc

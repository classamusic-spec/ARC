#include "Engine/ArcEngine.h"

#include <chrono>
#include <cmath>

#include "Engine/NetworkGeometry.h"

namespace arc
{

using namespace dsp;

void ArcEngine::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    maxBlock = std::max (32, maxBlockSize);
    // ~0.33 ms control period at every rate.
    controlInterval = sampleRate > 150000.0 ? 64 : sampleRate > 70000.0 ? 32 : 16;
    for (auto& v : voices)
        v.prepare (sampleRate, controlInterval);
    material.setMaterial (params.material, true);
    material.prepare (sampleRate / controlInterval);
    output.prepare (sampleRate, maxBlock);
    bendSmooth.coeff = smoothingCoeff (0.004, sampleRate / controlInterval);
    freezeSmooth.coeff = smoothingCoeff (0.12, sampleRate / controlInterval);
    prepared = true;
    reset();
}

void ArcEngine::reset()
{
    for (auto& v : voices)
        v.reset();
    output.reset();
    numHeld = 0;
    channelBend.fill (0.0f);
    channelPressureValue.fill (0.0f);
    channelTimbre.fill (0.0f);
    sustainDown.fill (false);
    globalBendTarget = 0.0f;
    bendSmooth.reset (0.0f);
    freezeSmooth.reset (params.freeze ? 1.0f : 0.0f);
    freezeWasOn = params.freeze;
    motion.reset (currentSeed);
    chaos.reset (currentSeed);
    controlCountdown = 0;
    updateGlobalControl();
}

bool ArcEngine::postGesture (int node, const Gesture& g) noexcept
{
    GestureMessage m;
    m.node = node;
    m.gesture = g;
    return gestureQueue.push (m);
}

void ArcEngine::postSeed (uint32_t seed) noexcept
{
    pendingSeed.store (seed, std::memory_order_relaxed);
    seedPending.store (true, std::memory_order_release);
}

void ArcEngine::setParameters (const EngineParams& p) noexcept
{
    const bool qualityChanged = p.quality != params.quality;
    params = p;
    params.polyphony = std::clamp (params.polyphony, 1, kMaxPolyphony);
    if (params.material != material.getMaterial())
        material.setMaterial (params.material, false);
    (void) qualityChanged; // control interval changes are applied on the next prepare()
}

uint32_t ArcEngine::nextSeed() noexcept
{
    seedState ^= seedState << 13;
    seedState ^= seedState >> 17;
    seedState ^= seedState << 5;
    return seedState;
}

// -------------------------------------------------------------------------------------
// Global control state (engine clock, every controlInterval samples)
// -------------------------------------------------------------------------------------
void ArcEngine::updateGlobalControl() noexcept
{
    const double dt = controlInterval / sampleRate;

    // Messages from the message thread (lock-free).
    if (seedPending.exchange (false, std::memory_order_acquire))
    {
        currentSeed = pendingSeed.load (std::memory_order_relaxed);
        motion.reset (currentSeed);
        chaos.reset (currentSeed);
    }
    GestureMessage msg;
    while (gestureQueue.pop (msg))
    {
        if (msg.gesture.valid)
            motion.setGesture (msg.node, msg.gesture);
        else
            motion.clearGesture (msg.node);
    }

    material.update (params.materialMods, params.tension);
    control.material = material.effective();
    motion.update (params, transport, dt);
    chaos.update (params.chaos, dt);

    // Effective node geometry = parameters + motion (drift + gesture), bounded.
    std::array<NodeParams, 4> nodes = params.nodes;
    std::array<FieldPoint, 4> pos {};
    for (int i = 0; i < 4; ++i)
    {
        const auto u = static_cast<size_t> (i);
        nodes[u].radius = clamp (nodes[u].radius + motion.radiusOffset (i), 0.0f, 1.0f);
        const float angle = normToAngle (nodes[u].angle) + motion.angleOffset (i);
        effectiveAngle[u] = angle;
        control.nodeOffsetOctaves[u] = radiusToOctaves (nodes[u].radius, params.quantise);
        control.nodePan[u] = angleToPan (angle);
        control.nodeDecayMult[u] = std::exp2 ((nodes[u].decay - 0.5f) * 4.0f);
        control.nodeDampMult[u] = std::exp2 (-(nodes[u].damp - 0.5f) * 4.0f);
        control.nodeLevel[u] = nodes[u].level;
        pos[u] = nodePosition (nodes[u].radius, angle);
    }
    effectiveNodes = nodes;

    // Coupling: topology + LINK + proximity, CHAOS strength variation and extra routing.
    auto weights = edgeWeights (params.topology, nodes, pos);
    const auto mask = topologyMask (params.topology);
    std::array<float, kNumEdges> chaosScale {};
    for (int e = 0; e < kNumEdges; ++e)
    {
        const auto u = static_cast<size_t> (e);
        chaosScale[u] = chaos.edgeScale (e);
        if (mask[u] == 0.0f)
            weights[u] += chaos.extraRouting (e);
    }
    control.edgeTheta = edgeGenerators (params.coupling, weights, chaosScale);

    control.exciterType = params.exciter;
    control.exciter = params.exciterParams;
    control.excite = params.excite;
    control.chaos = params.chaos;
    for (int i = 0; i < kNumNodes; ++i)
        control.chaosDetuneCents[static_cast<size_t> (i)] = chaos.detuneCents (i);
    control.releaseDamping = params.releaseDamping;
    control.dispersionStages = params.quality == Quality::eco ? 2 : params.quality == Quality::high ? 6 : 4;
    control.couplingCompensation = true;

    bendSmooth.setTarget (globalBendTarget);
    control.bendSemitones = bendSmooth.next();
    // FREEZE captures the voices sounding when it engages (new notes play normally).
    if (params.freeze && ! freezeWasOn)
        ++freezeEpoch;
    freezeWasOn = params.freeze;
    control.freezeEpoch = freezeEpoch;
    freezeSmooth.setTarget (params.freeze ? 1.0f : 0.0f);
    control.freeze = freezeSmooth.next();
}

// -------------------------------------------------------------------------------------
// MIDI
// -------------------------------------------------------------------------------------
void ArcEngine::pushHeld (int channel, int note, float velocity) noexcept
{
    removeHeld (channel, note);
    if (numHeld == kMaxHeldNotes)
    {
        for (int i = 1; i < numHeld; ++i)
            held[static_cast<size_t> (i - 1)] = held[static_cast<size_t> (i)];
        --numHeld;
    }
    held[static_cast<size_t> (numHeld++)] = { channel, note, velocity };
}

void ArcEngine::removeHeld (int channel, int note) noexcept
{
    for (int i = 0; i < numHeld; ++i)
        if (held[static_cast<size_t> (i)].note == note && held[static_cast<size_t> (i)].channel == channel)
        {
            for (int j = i + 1; j < numHeld; ++j)
                held[static_cast<size_t> (j - 1)] = held[static_cast<size_t> (j)];
            --numHeld;
            return;
        }
}

ArcVoice* ArcEngine::findVoiceForNewNote() noexcept
{
    int sounding = 0;
    for (auto& v : voices)
        if (v.isActive() && ! v.isStealing())
            ++sounding;

    if (sounding >= params.polyphony)
    {
        // Steal the quietest released voice; if none is released, the quietest held one
        // (ties -> oldest). The victim fades out (5 ms) in its own slot.
        ArcVoice* victim = nullptr;
        for (int pass = 0; pass < 2 && victim == nullptr; ++pass)
            for (auto& v : voices)
            {
                if (! v.isActive() || v.isStealing())
                    continue;
                if (pass == 0 && ! v.isReleased())
                    continue;
                if (victim == nullptr || v.getLevel() < victim->getLevel()
                    || (v.getLevel() == victim->getLevel() && v.getAge() > victim->getAge()))
                    victim = &v;
            }
        if (victim != nullptr)
        {
            victim->beginSteal();
            ++stealCount;
        }
    }

    for (auto& v : voices)
        if (! v.isActive())
            return &v;

    // Every physical slot busy (voices still fading): hard-reuse the quietest fading one.
    ArcVoice* quietest = nullptr;
    for (auto& v : voices)
        if (v.isStealing() && (quietest == nullptr || v.getLevel() < quietest->getLevel()))
            quietest = &v;
    if (quietest == nullptr)
        quietest = &voices[0];
    ++hardStealCount;
    return quietest;
}

void ArcEngine::startVoice (ArcVoice& v, int channel, int note, float velocity) noexcept
{
    v.start (note, channel, velocity, nextSeed(), control);
    v.setFreezeEpoch (params.freeze ? freezeEpoch : freezeEpoch - 1);
    v.setAge (voiceClock++);
    if (isMpeMemberChannel (channel))
    {
        v.setNoteBend (channelBend[static_cast<size_t> (channel)]);
        v.setPressure (channelPressureValue[static_cast<size_t> (channel)]);
        v.setTimbre (channelTimbre[static_cast<size_t> (channel)]);
    }
}

void ArcEngine::noteOn (int channel, int note, float velocity) noexcept
{
    if (! prepared)
        return;
    if (velocity <= 0.0f)
    {
        noteOff (channel, note);
        return;
    }
    channel = std::clamp (channel, 1, 16);
    velocity = std::clamp (velocity, 0.0f, 1.0f);
    const bool transient = params.exciter == ExciterType::strike || params.exciter == ExciterType::pluck;
    if (transient)
    {
        Telemetry::store (telemetry.strikeCounter, Telemetry::load (telemetry.strikeCounter) + 1u);
        Telemetry::store (telemetry.lastStrikeEnergy, velocity * (0.3f + 0.7f * params.excite));
    }

    if (params.voiceMode != VoiceMode::poly)
    {
        const bool anyHeld = numHeld > 0;
        pushHeld (channel, note, velocity);
        ArcVoice* current = nullptr;
        for (auto& v : voices)
            if (v.isActive() && ! v.isStealing())
            {
                current = &v;
                break;
            }
        if (current != nullptr && params.voiceMode == VoiceMode::legato && anyHeld)
        {
            // Legato: glide the same network; transient exciters re-excite, sustained ones
            // keep bowing / blowing.
            current->glideTo (note, velocity, params.glideSeconds, transient, nextSeed(), control);
            return;
        }
        if (current != nullptr)
        {
            current->beginSteal();
            ++stealCount;
        }
        if (auto* v = findVoiceForNewNote())
            startVoice (*v, channel, note, velocity);
        return;
    }

    // Poly: a note that is still ringing on this channel is re-excited in place.
    for (auto& v : voices)
        if (v.isActive() && ! v.isStealing() && v.getNote() == note && v.getChannel() == channel)
        {
            v.restrike (velocity, nextSeed(), control);
            v.setAge (voiceClock++);
            return;
        }

    if (auto* v = findVoiceForNewNote())
        startVoice (*v, channel, note, velocity);
}

void ArcEngine::noteOff (int channel, int note) noexcept
{
    channel = std::clamp (channel, 1, 16);
    const bool pedal = sustainDown[static_cast<size_t> (channel)] || sustainDown[1];

    if (params.voiceMode != VoiceMode::poly)
    {
        removeHeld (channel, note);
        for (auto& v : voices)
        {
            if (! v.isActive() || v.isStealing() || v.getNote() != note)
                continue;
            if (numHeld > 0)
            {
                // Return to the most recent still-held note.
                const auto& h = held[static_cast<size_t> (numHeld - 1)];
                const bool transient = params.exciter == ExciterType::strike || params.exciter == ExciterType::pluck;
                v.glideTo (h.note, h.velocity, params.voiceMode == VoiceMode::legato ? params.glideSeconds : 0.0f,
                           transient && params.voiceMode == VoiceMode::mono, nextSeed(), control);
            }
            else if (pedal)
                v.sustainedByPedal = true;
            else
                v.release();
        }
        return;
    }

    for (auto& v : voices)
        if (v.isActive() && ! v.isStealing() && ! v.isReleased() && v.getNote() == note && v.getChannel() == channel)
        {
            if (pedal)
                v.sustainedByPedal = true;
            else
                v.release();
        }
}

void ArcEngine::sustainPedal (int channel, bool down) noexcept
{
    channel = std::clamp (channel, 1, 16);
    sustainDown[static_cast<size_t> (channel)] = down;
    if (down)
        return;
    const bool anyStillDown = sustainDown[1];
    for (auto& v : voices)
        if (v.sustainedByPedal && (channel == 1 || v.getChannel() == channel) && ! (anyStillDown && channel != 1))
        {
            v.sustainedByPedal = false;
            v.release();
        }
}

void ArcEngine::pitchBend (int channel, float normalised) noexcept
{
    channel = std::clamp (channel, 1, 16);
    normalised = std::clamp (normalised, -1.0f, 1.0f);
    if (isMpeMemberChannel (channel))
    {
        // MPE per-note bend (default MPE member range: 48 semitones).
        const float semis = normalised * 48.0f;
        channelBend[static_cast<size_t> (channel)] = semis;
        for (auto& v : voices)
            if (v.isActive() && v.getChannel() == channel)
                v.setNoteBend (semis);
        return;
    }
    globalBendTarget = normalised * params.bendRange;
}

void ArcEngine::channelPressure (int channel, float value) noexcept
{
    channel = std::clamp (channel, 1, 16);
    value = std::clamp (value, 0.0f, 1.0f);
    channelPressureValue[static_cast<size_t> (channel)] = value;
    for (auto& v : voices)
        if (v.isActive() && (! isMpeMemberChannel (channel) || v.getChannel() == channel))
            v.setPressure (value);
}

void ArcEngine::polyPressure (int channel, int note, float value) noexcept
{
    for (auto& v : voices)
        if (v.isActive() && v.getNote() == note && v.getChannel() == std::clamp (channel, 1, 16))
            v.setPressure (std::clamp (value, 0.0f, 1.0f));
}

void ArcEngine::controller (int channel, int number, float value) noexcept
{
    channel = std::clamp (channel, 1, 16);
    if (number == 64)
        sustainPedal (channel, value >= 0.5f);
    else if (number == 74)
    {
        // MPE timbre / slide: centred at 0.5, -1..1 per note.
        const float t = (std::clamp (value, 0.0f, 1.0f) - 0.5f) * 2.0f;
        channelTimbre[static_cast<size_t> (channel)] = t;
        for (auto& v : voices)
            if (v.isActive() && (! isMpeMemberChannel (channel) || v.getChannel() == channel))
                v.setTimbre (t);
    }
    else if (number == 123 || number == 120)
        allNotesOff (number == 120);
}

void ArcEngine::allNotesOff (bool killSound) noexcept
{
    numHeld = 0;
    sustainDown.fill (false);
    for (auto& v : voices)
    {
        v.sustainedByPedal = false;
        if (killSound)
            v.beginSteal();
        else
            v.release();
    }
}

int ArcEngine::activeVoiceCount() const noexcept
{
    int n = 0;
    for (auto& v : voices)
        if (v.isActive())
            ++n;
    return n;
}

// -------------------------------------------------------------------------------------
// Rendering
// -------------------------------------------------------------------------------------
void ArcEngine::render (float* left, float* right, int n) noexcept
{
    const auto t0 = std::chrono::steady_clock::now();
    int done = 0;
    while (done < n)
    {
        const int len = std::min (n - done, maxBlock);
        renderChunk (left + done, right + done, len);
        done += len;
    }
    const double seconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    publishTelemetry (n, seconds);
}

void ArcEngine::renderChunk (float* left, float* right, int n) noexcept
{
    std::fill (left, left + n, 0.0f);
    std::fill (right, right + n, 0.0f);
    int done = 0;
    while (done < n)
    {
        if (controlCountdown <= 0)
        {
            updateGlobalControl();
            controlCountdown = controlInterval;
        }
        const int len = std::min (n - done, controlCountdown);
        for (auto& v : voices)
            if (v.isActive())
            {
                v.render (left + done, right + done, len, control);
                if (v.nonFiniteDetected)
                {
                    v.nonFiniteDetected = false;
                    ++nonFiniteVoiceResets;
                    Telemetry::store (telemetry.nonFiniteEvents, Telemetry::load (telemetry.nonFiniteEvents) + 1u);
                }
            }
        controlCountdown -= len;
        done += len;
    }
    output.setParameters (params.width, params.space, params.drive, params.masterGainDb);
    if (! output.process (left, right, n))
        Telemetry::store (telemetry.nonFiniteEvents, Telemetry::load (telemetry.nonFiniteEvents) + 1u);
}

void ArcEngine::publishTelemetry (int n, double seconds) noexcept
{
    std::array<float, kNumNodes> energy {};
    std::array<float, kNumEdges> flux {};
    float exc = 0.0f;
    int active = 0;
    for (auto& v : voices)
    {
        if (! v.isActive())
            continue;
        ++active;
        const auto& e = v.getNodeEnergy();
        const auto& f = v.getEdgeFlux();
        for (size_t i = 0; i < energy.size(); ++i)
            energy[i] += e[i];
        for (size_t i = 0; i < flux.size(); ++i)
            flux[i] += f[i];
        exc += v.getExciterEnergy();
    }
    float total = 0.0f;
    for (size_t i = 0; i < energy.size(); ++i)
    {
        Telemetry::store (telemetry.nodeEnergy[i], energy[i]);
        total += energy[i];
    }
    for (size_t i = 0; i < flux.size(); ++i)
    {
        Telemetry::store (telemetry.edgeFlux[i], flux[i]);
        const float phi = 2.0f * std::atan (0.5f * control.edgeTheta[i] * control.material.couplingScale);
        const float sn = std::sin (phi);
        Telemetry::store (telemetry.edgeStrength[i], sn * sn);
    }
    Telemetry::store (telemetry.exciterEnergy, exc);
    // Perceptual activity: ~0 at -80 dB, 1 at -10 dB of summed node energy.
    const float db = 10.0f * std::log10 (total + 1.0e-12f);
    Telemetry::store (telemetry.networkActivity, clamp ((db + 80.0f) / 70.0f, 0.0f, 1.0f));
    Telemetry::store (telemetry.activeVoices, active);
    Telemetry::store (telemetry.freezeAmount, control.freeze);
    Telemetry::store (telemetry.motionPhase, motion.displayPhase());
    Telemetry::store (telemetry.bpm, static_cast<float> (transport.valid && transport.bpm > 1.0 ? transport.bpm : 120.0));
    Telemetry::store (telemetry.beatsPerBar, static_cast<float> (transport.beatsPerBar));
    Telemetry::store (telemetry.hostPlaying, transport.valid && transport.playing);
    for (size_t i = 0; i < 4; ++i)
    {
        Telemetry::store (telemetry.nodeRadius[i], effectiveNodes[i].radius);
        Telemetry::store (telemetry.nodeAngle[i], effectiveAngle[i]);
        Telemetry::store (telemetry.nodeRatio[i],
                          control.material.nodeRatio[i] * std::exp2 (control.nodeOffsetOctaves[i]));
    }
    // Meters: the audio thread accumulates the peak since the UI last read it (the UI
    // exchanges it with 0), so no block's peak is missed between frames.
    Telemetry::store (telemetry.peakL, std::max (output.lastPeak[0], Telemetry::load (telemetry.peakL)));
    Telemetry::store (telemetry.peakR, std::max (output.lastPeak[1], Telemetry::load (telemetry.peakR)));
    Telemetry::store (telemetry.rmsL, std::sqrt (output.lastMeanSquare[0]));
    Telemetry::store (telemetry.rmsR, std::sqrt (output.lastMeanSquare[1]));
    const double budget = n / sampleRate;
    const float load = static_cast<float> (seconds / std::max (budget, 1.0e-9));
    Telemetry::store (telemetry.cpuLoad, 0.9f * Telemetry::load (telemetry.cpuLoad) + 0.1f * load);
    Telemetry::store (telemetry.blockCounter, Telemetry::load (telemetry.blockCounter) + 1u);
}

} // namespace arc

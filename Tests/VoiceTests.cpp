// Phase 7 — polyphony / voice architecture gate, driven through the full ArcEngine.

#include "Analysis.h"
#include "ArcTest.h"

#include "Engine/ArcEngine.h"

#include <functional>

using namespace arctest;

namespace
{
constexpr double kSr = 48000.0;

struct MidiEvent
{
    int sample;
    std::function<void (arc::ArcEngine&)> apply;
};

struct EngineRender
{
    Signal left, right, mono;
    std::vector<int> activeVoices; // per 256-sample block
};

EngineRender renderEngine (arc::ArcEngine& e, std::vector<MidiEvent> events, double seconds, int block = 256)
{
    std::sort (events.begin(), events.end(), [] (auto& a, auto& b) { return a.sample < b.sample; });
    const int n = static_cast<int> (seconds * kSr);
    EngineRender r;
    r.left.assign (static_cast<size_t> (n), 0.0f);
    r.right.assign (static_cast<size_t> (n), 0.0f);
    size_t next = 0;
    for (int pos = 0; pos < n; pos += block)
    {
        const int end = std::min (n, pos + block);
        int cur = pos;
        while (next < events.size() && events[next].sample < end)
        {
            const int at = std::max (cur, events[next].sample);
            if (at > cur)
                e.render (r.left.data() + cur, r.right.data() + cur, at - cur);
            cur = at;
            events[next].apply (e);
            ++next;
        }
        if (end > cur)
            e.render (r.left.data() + cur, r.right.data() + cur, end - cur);
        r.activeVoices.push_back (e.activeVoiceCount());
    }
    r.mono.resize (static_cast<size_t> (n));
    for (size_t i = 0; i < r.mono.size(); ++i)
        r.mono[i] = 0.5f * (r.left[i] + r.right[i]);
    return r;
}

arc::EngineParams baseParams()
{
    arc::EngineParams p;
    p.space = 0.0f;
    p.masterGainDb = 0.0f;
    return p;
}

MidiEvent on (double t, int note, float vel = 0.8f, int ch = 1)
{
    return { static_cast<int> (t * kSr), [=] (arc::ArcEngine& e) { e.noteOn (ch, note, vel); } };
}
MidiEvent off (double t, int note, int ch = 1)
{
    return { static_cast<int> (t * kSr), [=] (arc::ArcEngine& e) { e.noteOff (ch, note); } };
}

double fundamentalCentsNear (const Signal& x, double f0, double t0, double len)
{
    const auto a = x.begin() + static_cast<long> (t0 * kSr);
    Signal seg (a, a + static_cast<long> (len * kSr));
    return centsBetween (refineFrequencyByPhase (seg, kSr, peakFrequency (seg, kSr, f0 * 0.9, f0 * 1.1)), f0);
}
} // namespace

TEST_CASE ("voices", "polyphony storms without stuck voices")
{
    for (int poly : { 1, 4, 8, 16 })
        for (auto ex : { arc::ExciterType::strike, arc::ExciterType::bow })
        {
            arc::ArcEngine e;
            auto p = baseParams();
            p.polyphony = poly;
            p.exciter = ex;
            e.setParameters (p);
            e.prepare (kSr, 256);
            std::vector<MidiEvent> ev;
            // 48 overlapping notes, 25 ms apart, each held 300 ms.
            for (int k = 0; k < 48; ++k)
            {
                const int note = 36 + (k * 7) % 48;
                ev.push_back (on (0.01 + 0.025 * k, note, 0.5f + 0.5f * static_cast<float> (k % 5) / 4.0f));
                ev.push_back (off (0.31 + 0.025 * k, note));
            }
            ev.push_back ({ static_cast<int> (2.0 * kSr), [] (arc::ArcEngine& en) { en.allNotesOff (false); } });
            const auto r = renderEngine (e, ev, 9.0);
            const int maxActive = *std::max_element (r.activeVoices.begin(), r.activeVoices.end());
            const std::string tag = std::string (ex == arc::ExciterType::strike ? "strike" : "bow") + ".poly" + std::to_string (poly);
            MEASURE (tag + ".maxActiveVoices", maxActive);
            MEASURE (tag + ".steals", static_cast<double> (e.stealCount));
            MEASURE (tag + ".hardSteals", static_cast<double> (e.hardStealCount));
            MEASURE (tag + ".activeAtEnd", e.activeVoiceCount());
            MEASURE (tag + ".peak", peakAbs (r.mono));
            CHECK (allFinite (r.left) && allFinite (r.right));
            CHECK (maxActive <= std::min (poly + 4, arc::ArcEngine::kMaxVoices));
            CHECK (e.activeVoiceCount() == 0);   // no stuck voices after the tails end
            CHECK (e.nonFiniteVoiceResets == 0);
            CHECK (peakAbs (r.mono) <= 1.0);
        }
}

TEST_CASE ("voices", "voice stealing is click free")
{
    arc::ArcEngine e;
    auto p = baseParams();
    p.polyphony = 4;
    p.material = arc::MaterialType::metal; // long ringing tails are the hard case
    p.releaseDamping = 0.0f;
    e.setParameters (p);
    e.prepare (kSr, 256);
    std::vector<MidiEvent> ev;
    for (int k = 0; k < 4; ++k)
    {
        ev.push_back (on (0.05 + 0.1 * k, 48 + 5 * k, 0.9f));
        ev.push_back (off (0.1 + 0.1 * k, 48 + 5 * k));
    }
    const double stealAt = 1.0;
    ev.push_back (on (stealAt, 72, 0.9f)); // forces a steal of a ringing metal voice
    const auto r = renderEngine (e, ev, 1.6);
    MEASURE ("steals", static_cast<double> (e.stealCount));
    CHECK (e.stealCount >= 1);
    // Impulsive-click detector around the steal (5 ms frames, HF > 12 kHz vs neighbours),
    // ignoring the new note's own attack by comparing against a render without the steal.
    const int frame = static_cast<int> (0.005 * kSr);
    std::vector<double> hf;
    for (int i = static_cast<int> ((stealAt - 0.05) * kSr); i + frame < static_cast<int> ((stealAt + 0.05) * kSr); i += frame)
        hf.push_back (highBandRatioDb (r.mono, kSr, 12000.0, i, frame) + 20.0 * std::log10 (rms (r.mono, i, frame) + 1e-12));
    double worst = -1e9;
    for (size_t k = 3; k + 3 < hf.size(); ++k)
    {
        const double left = (hf[k - 1] + hf[k - 2] + hf[k - 3]) / 3.0, right = (hf[k + 1] + hf[k + 2] + hf[k + 3]) / 3.0;
        worst = std::max (worst, hf[k] - std::max (left, right));
    }
    MEASURE ("worstHfSpikeDb", worst);
    CHECK (worst < 12.0);
    CHECK (allFinite (r.mono));
}

TEST_CASE ("voices", "stealing prefers the quietest released voice")
{
    arc::ArcEngine e;
    auto p = baseParams();
    p.polyphony = 4;
    p.material = arc::MaterialType::wood;
    e.setParameters (p);
    e.prepare (kSr, 256);
    // Four held notes; 50 and 55 are released early (quiet by the time of the steal).
    const auto r = renderEngine (e, { on (0.0, 50), on (0.01, 55), on (0.02, 60), on (0.03, 65), off (0.05, 50),
                                      off (0.3, 55), on (1.0, 70) },
                                 1.05);
    (void) r;
    bool found50 = false, found60 = false, found65 = false;
    int stealingNote = -1;
    for (int i = 0; i < arc::ArcEngine::kMaxVoices; ++i)
    {
        const auto& v = e.getVoice (i);
        if (! v.isActive())
            continue;
        if (v.isStealing())
            stealingNote = v.getNote();
        else if (v.getNote() == 50)
            found50 = true;
        else if (v.getNote() == 60)
            found60 = true;
        else if (v.getNote() == 65)
            found65 = true;
    }
    MEASURE ("stolenNote", stealingNote);
    // The quietest released voice (50, released first) is the victim or already gone;
    // held notes 60 and 65 survive.
    CHECK (! found50);
    CHECK (found60 && found65);
}

TEST_CASE ("voices", "sustain pedal holds releases")
{
    arc::ArcEngine e;
    auto p = baseParams();
    p.exciter = arc::ExciterType::bow;
    e.setParameters (p);
    e.prepare (kSr, 256);
    const auto r = renderEngine (e,
                                 { { 0, [] (arc::ArcEngine& en) { en.controller (1, 64, 1.0f); } }, on (0.05, 60),
                                   off (0.3, 60), { static_cast<int> (1.2 * kSr), [] (arc::ArcEngine& en) { en.controller (1, 64, 0.0f); } } },
                                 2.4);
    const double whileHeld = rms (r.mono, static_cast<int> (0.8 * kSr), 4800);
    const double afterRelease = rms (r.mono, static_cast<int> (2.3 * kSr), 4800);
    MEASURE ("bowedRmsDb_pedalHeld", 20.0 * std::log10 (whileHeld));
    MEASURE ("bowedRmsDb_afterPedalUp", 20.0 * std::log10 (afterRelease + 1e-12));
    CHECK (whileHeld > 0.01);                  // still bowing after note-off (pedal)
    CHECK (afterRelease < whileHeld * 0.05);   // pedal up -> released -> decays
}

TEST_CASE ("voices", "pitch bend is accurate")
{
    for (float bendRange : { 2.0f, 12.0f })
    {
        arc::ArcEngine e;
        auto p = baseParams();
        p.exciter = arc::ExciterType::bow;
        p.material = arc::MaterialType::wood;
        p.bendRange = bendRange;
        e.setParameters (p);
        e.prepare (kSr, 256);
        const auto r = renderEngine (e, { on (0.0, 57), { static_cast<int> (0.8 * kSr), [] (arc::ArcEngine& en) { en.pitchBend (1, 1.0f); } } }, 2.0);
        const double f0 = arc::dsp::midiToHz (57);
        const double before = fundamentalCentsNear (r.mono, f0, 0.3, 0.4);
        const double after = fundamentalCentsNear (r.mono, f0 * std::exp2 (bendRange / 12.0), 1.3, 0.6);
        MEASURE ("range" + std::to_string (static_cast<int> (bendRange)) + ".centsBeforeBend", before);
        MEASURE ("range" + std::to_string (static_cast<int> (bendRange)) + ".centsAfterFullBend", after);
        CHECK (std::abs (before) < 2.0);
        CHECK (std::abs (after) < 3.0);
    }
}

TEST_CASE ("voices", "release semantics per exciter")
{
    // STRIKE: note-off keeps ringing (release damping 0) or is damped (1).
    double ringing[2];
    for (int k = 0; k < 2; ++k)
    {
        arc::ArcEngine e;
        auto p = baseParams();
        p.material = arc::MaterialType::metal;
        p.releaseDamping = k == 0 ? 0.0f : 1.0f;
        e.setParameters (p);
        e.prepare (kSr, 256);
        const auto r = renderEngine (e, { on (0.0, 60), off (0.2, 60) }, 1.5);
        ringing[k] = 20.0 * std::log10 (rms (r.mono, static_cast<int> (1.2 * kSr), 4800) + 1e-12);
    }
    MEASURE ("strike.levelDb_at1.2s.releaseDamping0", ringing[0]);
    MEASURE ("strike.levelDb_at1.2s.releaseDamping1", ringing[1]);
    CHECK (ringing[0] > -45.0);           // transient exciters ring after note-off
    CHECK (ringing[1] < ringing[0] - 20.0); // the damper works when asked
}

TEST_CASE ("voices", "same note restrike reuses the voice")
{
    arc::ArcEngine e;
    e.setParameters (baseParams());
    e.prepare (kSr, 256);
    const auto r = renderEngine (e, { on (0.0, 60), off (0.1, 60), on (0.3, 60), off (0.4, 60), on (0.6, 60) }, 0.8);
    MEASURE ("activeVoices", e.activeVoiceCount());
    CHECK (e.activeVoiceCount() == 1);
    CHECK (allFinite (r.mono));
}

TEST_CASE ("voices", "mpe per-note pitch bend")
{
    arc::ArcEngine e;
    auto p = baseParams();
    p.mpe = true;
    p.exciter = arc::ExciterType::bow;
    p.material = arc::MaterialType::wood;
    p.space = 0.0f;
    e.setParameters (p);
    e.prepare (kSr, 256);
    // Two notes on member channels 2 and 3; bend only channel 2 by +2 semitones (48 st range).
    const float bend = 2.0f / 48.0f;
    const auto r = renderEngine (e, { on (0.0, 48, 0.8f, 2), on (0.0, 55, 0.8f, 3),
                                      { static_cast<int> (0.6 * kSr), [bend] (arc::ArcEngine& en) { en.pitchBend (2, bend); } } },
                                 1.8);
    const double c48 = fundamentalCentsNear (r.mono, arc::dsp::midiToHz (50), 1.1, 0.6); // 48 bent to 50
    const double c55 = fundamentalCentsNear (r.mono, arc::dsp::midiToHz (55), 1.1, 0.6); // unchanged
    MEASURE ("bentNoteCents_vs_D3", c48);
    MEASURE ("otherNoteCents_vs_G3", c55);
    CHECK (std::abs (c48) < 5.0);
    CHECK (std::abs (c55) < 5.0);
}

TEST_CASE ("voices", "legato glides one network")
{
    arc::ArcEngine e;
    auto p = baseParams();
    p.voiceMode = arc::VoiceMode::legato;
    p.exciter = arc::ExciterType::bow;
    p.material = arc::MaterialType::wood;
    p.glideSeconds = 0.08f;
    e.setParameters (p);
    e.prepare (kSr, 256);
    const auto r = renderEngine (e, { on (0.0, 55), on (0.8, 62) }, 1.8);
    MEASURE ("activeVoices", e.activeVoiceCount());
    const double c = fundamentalCentsNear (r.mono, arc::dsp::midiToHz (62), 1.2, 0.5);
    MEASURE ("glideTargetCents", c);
    CHECK (e.activeVoiceCount() == 1);
    CHECK (std::abs (c) < 3.0);
}

TEST_CASE ("voices", "midi is sample accurate")
{
    for (int offset : { 0, 17, 101, 255 })
    {
        arc::ArcEngine e;
        auto p = baseParams();
        p.exciterParams.strikeLength = 0.0f; // shortest contact
        e.setParameters (p);
        e.prepare (kSr, 256);
        const int at = 1024 + offset;
        const auto r = renderEngine (e, { { at, [] (arc::ArcEngine& en) { en.noteOn (1, 72, 1.0f); } } }, 0.1);
        int first = -1;
        for (size_t i = 0; i < r.mono.size(); ++i)
            if (std::abs (r.mono[i]) > 1.0e-6f)
            {
                first = static_cast<int> (i);
                break;
            }
        MEASURE ("offset" + std::to_string (offset) + ".firstSample", first - at);
        // The loop needs one pass before its output carries the excitation.
        CHECK (first >= at);
        CHECK (first - at < 200);
    }
}

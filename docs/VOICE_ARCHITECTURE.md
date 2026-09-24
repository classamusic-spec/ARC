# ARC — Voice Architecture

Code: `Source/Engine/ArcEngine.*`, `Source/Engine/ArcVoice.*`. Gate: `Tests/VoiceTests.cpp`,
`Tests/PerformanceTests.cpp`.

## Structure

```
MIDI ─► ArcEngine ─┬─► voice allocator (20 physical voices, 1–16 polyphony limit)
                   ├─► global control (every ~0.33 ms): material morph, geometry, coupling
                   │   generators, bend / freeze smoothing  ─► VoiceControl (shared)
                   ├─► ArcVoice × N: exciter ↔ resonant network, per-note pitch / energy /
                   │   expression, own control clock, telemetry
                   └─► OutputStage: width → small space → drive → master → safety clip
```

* **Physical vs logical voices.** 20 physical voices, user polyphony ≤ 16. A stolen voice
  fades out in its own slot (5 ms) while the new note starts immediately in a free slot:
  no delayed onset and no hard reset of ringing state. Measured storms (48 notes, 25 ms
  apart): active voices never exceeded polyphony + 1, zero hard steals.
* **Victim choice.** Quietest *released* voice first; otherwise the quietest held voice
  (ties → oldest).
* **Re-strike.** A note that is still ringing on the same channel is re-excited in place
  (the second strike adds to the resonator's motion, as on a real bell) instead of
  stacking a second network of the same pitch.
* **Release semantics.** STRIKE / PLUCK keep ringing after note-off; RELEASE DAMPING
  (0..1) applies a damper (T60 × 0.03^(d^0.7), smoothed). BOW lifts the bow, AIR stops
  the breath; the body then decays. Voices end after 64 control blocks below −100 dB
  with no active excitation.
* **Sample-accurate MIDI.** The processor splits the block at every event; voices keep
  their own control clock (updates every `controlInterval` samples of *their* time), so
  splitting never forces extra network redesigns.
* **Pitch.** Global bend (range 0–24 st, smoothed 4 ms), MPE member-channel bend
  (±48 st), legato glide (exponential, GLIDE seconds), energy-dependent tuning, coupling
  compensation, intonation tracker for sustained exciters.
* **Expression.** Channel pressure / poly aftertouch → exciter pressure (bow force,
  breath, strike energy); CC74 (MPE timbre) → the note's HF decay (×2^(2·t)) and exciter
  tone. MPE: lower zone, master channel 1, members 2–16.
* **Modes.** POLY, MONO (last-note priority, retrigger), LEGATO (one network glides;
  transient exciters re-excite, sustained ones keep driving).

## Measured (Tests/VoiceTests.cpp)

| Check | Result |
|---|---|
| Storms at polyphony 1/4/8/16, STRIKE and BOW | max active = poly + 1, hard steals 0, stuck voices 0, peak ≤ 0.97, all finite |
| Steal of a ringing METAL voice | worst impulsive HF event 2.9 dB above neighbours (no click) |
| Victim choice | quietest released note stolen, held notes survive |
| Sustain pedal | bowing continues after note-off (−22 dB), ends after pedal-up (−66 dB) |
| Pitch bend ±2 / ±12 st | 0.004 / 0.0006 cents after full bend |
| STRIKE after note-off | rings (−30 dB at 1.2 s) with damping 0; silent with damping 1 |
| Re-strike same note ×3 | one voice |
| MPE per-note bend | bent note 0.005 cents on target, other note unaffected (0.002) |
| Legato | one voice, glide lands within 0.015 cents |
| Sample accuracy | identical onset offset for events at block offsets 0/17/101/255 |

## CPU (Release, one core of the build machine, `docs/measurements/phase7/cpu_profile.csv`)

| voices | 44.1 kHz strike / bow | 48 kHz strike / bow | 96 kHz strike / bow |
|---|---|---|---|
| 1 | 1.4 % / 1.0 % | 1.1 % / 1.4 % | 1.6 % / 1.5 % |
| 4 | 4.8 % / 3.5 % | 4.1 % / 4.7 % | 6.5 % / 6.7 % |
| 8 | 7.7 % / 6.8 % | 7.7 % / 7.6 % | 10.0 % / 10.6 % |
| 16 | 14.1 % / 14.8 % | 16.4 % / 17.6 % | 21.6 % / 25.2 % |

COUPLING 0 vs 1, CHAOS 1, MOTION 1 (8 voices, 48 kHz): 7.2–8.0 %, i.e. cost does not
depend on the sound. Polyphony default 8, maximum 16 — both measured to fit easily.
The quality modes currently differ by < 1 % (see the optimisation phase).

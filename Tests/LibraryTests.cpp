// Phase 16 — the factory library (396 presets): every preset rendered, measured and checked.
//
//  * clean: finite, unclipped, audible, no DC, loudness on the calibrated target;
//  * truthful: the category is what you hear (PLUCKED plucks, METAL is metal, PADS and
//    DRONES sustain, PERCUSSION is short) and pitched presets play the note in tune;
//  * unique: a timbre fingerprint (spectral shape, amplitude envelope, stereo width,
//    modulation) is computed for every preset and no two presets may come close.
//
// ARC_PRESET_FILTER=<text> audits only presets whose name or category contains <text>.
// ARC_CALIBRATE=1 measures every preset at 0 dB patch level and writes the loudness trims to
// Source/Core/Presets/Calibration.inc (rebuild afterwards). ARC_PRESET_WAVS=1 writes renders.

#include "Analysis.h"
#include "ArcTest.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <thread>

#include "Core/FactoryPresets.h"
#include "Core/Parameters.h"
#include "Core/PresetManager.h"
#include "PluginProcessor.h"

using namespace arctest;

namespace arctest
{
std::string outputDir();
}

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 512;
constexpr double kTargetLoudnessDb = -21.0; // momentary loudness of the C-E-G test chord

struct Stereo
{
    Signal left, right, mono;
};

Stereo renderNotes (ArcAudioProcessor& p, double seconds, const std::vector<int>& notes, double hold, float velocity)
{
    Stereo out;
    juce::AudioBuffer<float> buffer (2, kBlock);
    juce::MidiBuffer midi;
    const int blocks = static_cast<int> (seconds * kSr / kBlock);
    for (int b = 0; b < blocks; ++b)
    {
        const double t = b * kBlock / kSr;
        midi.clear();
        if (b == 0)
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOn (1, n, velocity), 0);
        if (t < hold && t + kBlock / kSr >= hold)
            for (int n : notes)
                midi.addEvent (juce::MidiMessage::noteOff (1, n), 0);
        buffer.clear();
        p.processBlock (buffer, midi);
        for (int i = 0; i < kBlock; ++i)
        {
            const float l = buffer.getSample (0, i), r = buffer.getSample (1, i);
            out.left.push_back (l);
            out.right.push_back (r);
            out.mono.push_back (0.5f * (l + r));
        }
    }
    return out;
}

double db (double linear) { return 20.0 * std::log10 (linear + 1.0e-12); }
int at (double seconds) { return static_cast<int> (seconds * kSr); }

double momentaryDb (const Signal& x, double t0, double t1)
{
    double best = 0;
    for (double t = t0; t + 0.4 <= t1; t += 0.1)
        best = std::max (best, rms (x, at (t), at (0.4)));
    return db (best);
}

/** Band energies (dB), 1/6 octave, 60 Hz .. 12 kHz. */
std::vector<double> bandEnergiesDb (const Signal& x, int start, int length)
{
    const auto s = computeSpectrum (x, kSr, 1 << 15, start, length);
    std::vector<double> b;
    for (double f = 60.0; f < 12000.0; f *= std::pow (2.0, 1.0 / 6.0))
    {
        const int lo = static_cast<int> (f / s.binHz), hi = std::max (lo + 1, static_cast<int> (f * std::pow (2.0, 1.0 / 6.0) / s.binHz));
        double e = 0;
        for (int i = lo; i < hi && i < static_cast<int> (s.mag.size()); ++i)
            e += s.mag[static_cast<size_t> (i)] * s.mag[static_cast<size_t> (i)];
        b.push_back (10.0 * std::log10 (e + 1e-20));
    }
    return b;
}

std::vector<double> levelNormalised (std::vector<double> v)
{
    double mean = 0;
    for (auto x : v)
        mean += x;
    mean /= static_cast<double> (v.size());
    for (auto& x : v)
        x -= mean;
    return v;
}

double rmsDistance (const std::vector<double>& a, const std::vector<double>& b)
{
    double d = 0;
    for (size_t i = 0; i < a.size(); ++i)
        d += (a[i] - b[i]) * (a[i] - b[i]);
    return std::sqrt (d / static_cast<double> (a.size()));
}

struct Profile
{
    std::string name, category, tags;
    int exciter = 0, material = 0;
    bool finite = true;
    double loudDb = 0, peakDb = 0, tailDb = 0, centroidHz = 0, dcDb = -200, sustainDb = 0, widthDb = 0, fluxDb = 0;
    double pitchCents = 0, noteVsStrongestDb = 0;
    double hardPeakDb = -200; // single notes C2 / C4 / C6 at full velocity (the worst of them)
    std::vector<double> shape, envelope;
    Stereo chord; // kept only when WAVs are requested
};

float presetValue (const arc::presets::PresetDefinition& p, const char* id, float fallback)
{
    for (const auto& [k, v] : p.values)
        if (k == id)
            return v;
    return fallback;
}

/** Everything measured about one preset (runs on a worker thread). */
void measure (ArcAudioProcessor& proc, Profile& pr, bool keepAudio)
{
    // 1. The C3-E3-G3 chord: held 2 s, then 2 s of release.
    const auto out = renderNotes (proc, 4.0, { 48, 52, 55 }, 2.0, 0.8f);
    pr.finite = allFinite (out.left) && allFinite (out.right);
    pr.loudDb = momentaryDb (out.mono, 0.0, 2.0);
    pr.peakDb = db (std::max (peakAbs (out.left), peakAbs (out.right)));
    pr.tailDb = db (rms (out.mono, at (3.5), at (0.4)));
    pr.centroidHz = spectralCentroid (out.mono, kSr, at (0.1), at (1.8));
    pr.sustainDb = db (rms (out.mono, at (1.5), at (0.4))) - pr.loudDb;
    double mean = 0;
    for (int s = at (0.1); s < at (1.9); ++s)
        mean += out.mono[static_cast<size_t> (s)];
    mean /= 1.8 * kSr;
    pr.dcDb = db (std::abs (mean)) - db (rms (out.mono, at (0.1), at (1.8)));
    double mid = 0, side = 0;
    for (int s = at (0.05); s < at (2.0); ++s)
    {
        const double l = out.left[static_cast<size_t> (s)], r = out.right[static_cast<size_t> (s)];
        mid += 0.25 * (l + r) * (l + r);
        side += 0.25 * (l - r) * (l - r);
    }
    pr.widthDb = 10.0 * std::log10 ((side + 1e-20) / (mid + 1e-20));
    pr.shape = levelNormalised (bandEnergiesDb (out.mono, at (0.05), at (1.5)));

    // Amplitude envelope: 16 x 250 ms, relative to the loudest frame, floor -70 dB.
    double top = -300;
    for (int k = 0; k < 16; ++k)
    {
        const double e = std::max (-200.0, db (rms (out.mono, at (0.25 * k), at (0.25))));
        pr.envelope.push_back (e);
        top = std::max (top, e);
    }
    for (auto& e : pr.envelope)
        e = std::max (-70.0, e - top);

    // Modulation: per-band deviation from each band's own linear trend over 8 frames
    // (decay is a trend; motion, beating and chaos are the residue).
    std::vector<std::vector<double>> frames;
    for (int k = 0; k < 8; ++k)
        frames.push_back (bandEnergiesDb (out.mono, at (0.1 + 0.24 * k), at (0.24)));
    double residue = 0;
    int counted = 0;
    for (size_t b = 0; b < frames[0].size(); ++b)
    {
        double sx = 0, sy = 0, sxx = 0, sxy = 0;
        for (int k = 0; k < 8; ++k)
        {
            const double y = frames[static_cast<size_t> (k)][b];
            sx += k, sy += y, sxx += k * k, sxy += k * y;
        }
        const double slope = (8 * sxy - sx * sy) / (8 * sxx - sx * sx), icpt = (sy - slope * sx) / 8;
        double r = 0;
        for (int k = 0; k < 8; ++k)
        {
            const double d = frames[static_cast<size_t> (k)][b] - (icpt + slope * k);
            r += d * d;
        }
        if (sy / 8 > pr.loudDb - 70.0) // bands that carry energy
        {
            residue += std::sqrt (r / 8);
            ++counted;
        }
    }
    pr.fluxDb = counted > 0 ? residue / counted : 0.0;
    if (keepAudio)
        pr.chord = out;

    // 2. A single held A3: is the note there and in tune?
    proc.releaseResources();
    proc.prepareToPlay (kSr, kBlock);
    const auto note = renderNotes (proc, 2.0, { 57 }, 1.5, 0.8f);
    const double f0 = 220.0;
    const auto spec = computeSpectrum (note.mono, kSr, 1 << 17, at (0.3), at (1.2));
    double atNote = 0, strongest = 0;
    const double coarse = peakFrequency (spec, f0 * 0.96, f0 * 1.04, &atNote);
    peakFrequency (spec, f0 * 0.45, f0 * 8.0, &strongest);
    pr.noteVsStrongestDb = db (atNote) - db (strongest);
    if (coarse > 0)
    {
        const Signal seg (note.mono.begin() + at (0.3), note.mono.begin() + at (1.5));
        pr.pitchCents = centsBetween (refineFrequencyByPhase (seg, kSr, coarse), f0);
    }
    else
        pr.pitchCents = 999.0;

    // 3. Hard single notes across the keyboard: the chord alone does not find the loudest
    //    attack of a short pluck or strike (a hard C6 can peak several dB above it).
    for (int n : { 36, 60, 84 })
    {
        proc.releaseResources();
        proc.prepareToPlay (kSr, kBlock);
        const auto hard = renderNotes (proc, 1.2, { n }, 1.0, 1.0f);
        pr.finite = pr.finite && allFinite (hard.left) && allFinite (hard.right);
        pr.hardPeakDb = std::max (pr.hardPeakDb, db (std::max (peakAbs (hard.left), peakAbs (hard.right))));
    }
}

std::string envOr (const char* name, const char* fallback)
{
    const char* v = std::getenv (name);
    return v != nullptr ? std::string (v) : std::string (fallback);
}

/** Calibration renders this far below MASTER's default, so no peak reaches the output's
    safety clip while it is measured (MASTER is applied after DRIVE: exactly linear). */
constexpr double kCalibrationHeadroomDb = 12.0;

/** Renders and measures the (filtered) library in parallel batches. Calibrating: PATCH LEVEL
    at 0 dB and the headroom above, compensated in the returned levels. */
std::vector<Profile> profileLibrary (bool calibrating, bool keepAudio)
{
    const auto& list = arc::presets::factoryPresets();
    const auto filter = juce::String (envOr ("ARC_PRESET_FILTER", "")).toLowerCase();
    std::vector<size_t> indices;
    for (size_t i = 0; i < list.size(); ++i)
        if (filter.isEmpty() || juce::String (list[i].name).toLowerCase().contains (filter)
            || juce::String (list[i].category).toLowerCase().contains (filter))
            indices.push_back (i);

    std::vector<Profile> profiles (indices.size());
    const size_t threads = std::max (1u, std::min (4u, std::thread::hardware_concurrency()));
    for (size_t start = 0; start < indices.size(); start += threads)
    {
        const size_t count = std::min (threads, indices.size() - start);
        std::vector<std::unique_ptr<ArcAudioProcessor>> procs;
        for (size_t k = 0; k < count; ++k)
        {
            const auto i = indices[start + k];
            auto proc = std::make_unique<ArcAudioProcessor>();
            proc->getPresetManager().loadPreset (static_cast<int> (i));
            if (calibrating)
            {
                if (auto* lvl = proc->getValueTreeState().getParameter (arc::params::patchLevel))
                    lvl->setValueNotifyingHost (lvl->convertTo0to1 (0.0f));
                if (auto* master = proc->getValueTreeState().getParameter (arc::params::masterOutput))
                    master->setValueNotifyingHost (master->convertTo0to1 (master->convertFrom0to1 (master->getDefaultValue())
                                                                           - static_cast<float> (kCalibrationHeadroomDb)));
            }
            proc->prepareToPlay (kSr, kBlock);
            auto& pr = profiles[start + k];
            pr.name = list[i].name;
            pr.category = list[i].category;
            pr.tags = list[i].tags;
            pr.exciter = juce::roundToInt (presetValue (list[i], arc::params::exciterType, 0.0f));
            pr.material = juce::roundToInt (presetValue (list[i], arc::params::materialType, 1.0f));
            procs.push_back (std::move (proc));
        }
        std::vector<std::thread> pool;
        for (size_t k = 0; k < count; ++k)
            pool.emplace_back ([&, k] { measure (*procs[k], profiles[start + k], keepAudio); });
        for (auto& t : pool)
            t.join();
    }
    if (calibrating)
        for (auto& p : profiles)
        {
            p.loudDb += kCalibrationHeadroomDb;
            p.peakDb += kCalibrationHeadroomDb;
            p.hardPeakDb += kCalibrationHeadroomDb;
        }
    return profiles;
}

double uniquenessDistance (const Profile& a, const Profile& b)
{
    const double s = rmsDistance (a.shape, b.shape);
    const double e = rmsDistance (a.envelope, b.envelope);
    const double w = std::abs (std::clamp (a.widthDb, -40.0, 0.0) - std::clamp (b.widthDb, -40.0, 0.0));
    const double f = std::abs (a.fluxDb - b.fluxDb);
    return std::sqrt (s * s + 0.25 * e * e + 0.0625 * w * w + 0.25 * f * f);
}

bool hasTag (const std::string& tags, const char* tag) { return juce::String (tags).toLowerCase().contains (tag); }

/** Presets whose pitch is not meant to follow the keyboard exactly. */
bool pitchExempt (const Profile& p)
{
    return p.category == "PERCUSSION" || p.category == "EXPERIMENTAL" || hasTag (p.tags, "inharmonic") || hasTag (p.tags, "unpitched")
           || hasTag (p.tags, "noise") || hasTag (p.tags, "detuned") || hasTag (p.tags, "chaos");
}
const char* const kExciterNames[] = { "strike", "pluck", "bow", "air" };
const char* const kMaterialNames[] = { "glass", "metal", "wood", "membrane" };

const arc::presets::PresetDefinition* findDefinition (const std::string& name)
{
    for (const auto& d : arc::presets::factoryPresets())
        if (d.name == name)
            return &d;
    static const arc::presets::PresetDefinition none;
    return &none;
}

struct Audit
{
    int presets = 0, unclean = 0, offTarget = 0, wrongCategory = 0, outOfTune = 0, pitchChecked = 0;
    double loudest = 0, quietest = 0, closest = 0;
    std::string closestPair;
};

/** docs/PRESETS.md: the catalogue, generated from the library and this audit. */
void writeCatalogue (const std::vector<Profile>& profiles, const Audit& a)
{
    const std::map<std::string, const char*> blurbs {
        { "PADS", "Slow, sustaining and wide: bowed and blown networks, most of them tuned to chords." },
        { "PLUCKED", "Strings and tines: harps, lutes, basses, keys and zithers, with sympathetic nodes on their harmonics." },
        { "STRUCK", "Mallets and hammers: bells, bars, gongs, bowls and pianos." },
        { "BOWED", "Stick-slip friction on the CORE: strings, saws, glasses, bowed metal and skins." },
        { "AIR", "Breath: flutes, reeds, pipes, vessels, horns, voices and wind." },
        { "GLASS", "The glass material under every exciter: bars, panes, tines, tumblers, fibres and ice." },
        { "METAL", "Steel, iron, copper, tin and titanium: springs, rails, sheets, forks, cones, plates and found objects." },
        { "WOOD", "Bars, boxes, logs, soundboards, bamboo, balsa and oak." },
        { "MEMBRANE", "Skins of every size: kettles, tablas, frame drums, surdos, taiko, balloons and paper." },
        { "DRONES", "Held worlds that sustain for as long as the key is down." },
        { "PERCUSSION", "A kit and a toolbox: every hit falls at least 18 dB within 1.5 s." },
        { "EXPERIMENTAL", "Irrational and golden tunings, square-root lattices, Morse code, polyrhythms, reversed swells and black holes." }
    };
    int folded = 0;
    for (const auto& d : arc::presets::factoryPresets())
        folded += d.foldedTunings > 0 ? 1 : 0;

    std::ostringstream md;
    md << "# ARC Factory Presets\n\n"
          "<!-- Generated by the `library / every preset is clean, truthful and unique` test from the factory\n"
          "     library and its measurements (a full, unfiltered run). Do not edit by hand. -->\n\n"
       << a.presets << " presets in " << arc::presets::categories().size()
       << " categories: the 46 signature sounds and a 350-preset library. Every preset listed here was rendered and "
          "measured by the library audit on this build (C3-E3-G3 chord held for 2 s, then a single A3):\n\n"
          "| Check | Rule | Result |\n|---|---|---|\n";
    char buf[512];
    std::snprintf (buf, sizeof (buf),
                   "| Clean | finite, DC below -30 dB, peak below -1 dBFS on the chord and on hard single notes (C2, C4, C6 at full "
                   "velocity) | %d of %d |\n",
                   a.presets - a.unclean, a.presets);
    md << buf;
    std::snprintf (buf, sizeof (buf),
                   "| Level | chord at -21 dB momentary loudness +-2 dB, or held back by the -3 dBFS peak ceiling (short hits) | %d of %d, "
                   "%.1f to %.1f dB |\n",
                   a.presets - a.offTarget, a.presets, a.quietest, a.loudest);
    md << buf;
    std::snprintf (buf, sizeof (buf),
                   "| In tune | A3 within 12 cents and within 30 dB of the strongest partial (percussion, experimental and presets "
                   "tagged inharmonic, unpitched, noise, detuned or chaos are exempt) | %d of %d checked |\n",
                   a.pitchChecked - a.outOfTune, a.pitchChecked);
    md << buf;
    std::snprintf (buf, sizeof (buf),
                   "| True to category | plucked, struck, bowed and air use their exciter; glass, metal, wood and membrane their "
                   "material; pads and drones hold within 10 dB; percussion falls at least 18 dB | %d of %d |\n",
                   a.presets - a.wrongCategory, a.presets);
    md << buf;
    std::snprintf (buf, sizeof (buf),
                   "| Unique | no two presets closer than 2.0 (spectral shape, envelope, stereo width, modulation) | closest %.2f (%s) |\n\n",
                   a.closest, a.closestPair.c_str());
    md << buf;
    md << "Per-preset measurements: [measurements/presets/library.csv](measurements/presets/library.csv). Rebuild the\n"
          "catalogue with `ARCTests \"clean, truthful\"`; after changing a preset, re-run the loudness calibration first\n"
          "(`ARC_CALIBRATE=1 ARCTests \"calibrate loudness\"`, see TESTING.md).\n\n"
          "## How the presets are made\n\n"
          "Presets are written with a small fluent builder (`Source/Core/PresetBuilder.h`, one file per category in\n"
          "`Source/Core/Presets/`). Values are plain parameter values; anything a preset leaves alone takes its default.\n\n"
          "* **Tuning.** A node's radius reaches one octave either side of its material ratio (glass 2.63 / 4.45 / 6.55 / 9.40,\n"
          "  metal 1.19 / 1.5 / 2.0 / 2.67, wood 2.76 / 5.40 / 8.93 / 13.34, membrane 1.59 / 2.14 / 2.30 / 2.65 x CORE, stretched\n"
          "  by TENSION and INHARMONICITY). `tune()` places a node exactly on a ratio; `chord()` folds an out-of-reach ratio by\n"
          "  octaves, the way a sympathetic string would be tuned ("
       << folded
       << " presets fold at least one node; the audit prints each fold). A radius\n"
          "  that would need clamping fails `factory library is complete and valid`.\n"
          "* **Sustained exciters keep every node at least 5 % from unison.** A bowed or blown CORE locks onto one side of a\n"
          "  near-unison doublet and plays sharp: Obsidian Bloom played 66 to 81 cents sharp until its node A moved from\n"
          "  1.036 to 1.11 x CORE.\n"
          "* **Strongly blown skins keep MASS at 0.6 or more.** A membrane's tension rises with amplitude and a self-sustaining\n"
          "  air drive never lets it settle: Mirliton (FLOW 0.72) read +12 cents at MASS 0.46 and +1.6 at 0.62. The breathy\n"
          "  membrane pads (FLOW 0.62 or less with high TURBULENCE, a weaker drive) stay within 4 cents at MASS 0.40 to 0.58.\n"
          "* **Metal harmonic chords (2, 3, 4, 5) need TENSION near 0.7** to sit inside the nodes' reach.\n"
          "* **Loudness.** Each preset stores a PATCH LEVEL trim, measured on the chord and capped so that neither the chord\n"
          "  nor a hard single note anywhere from C2 to C6 peaks above -3 dBFS (where the output's safety clip begins). Each\n"
          "  note keeps the trim of the patch it was played in, so switching presets never lifts a ringing tail. MASTER stays\n"
          "  yours.\n"
          "* **Recall.** CHAOS and MOTION walks are seeded from the preset name, so a preset sounds the same every time.\n"
          "* **Motion.** Procedural gestures (orbit, sway, figure, breathe, steps, wander) are stored with the preset;\n"
          "  stepped gestures are exact semitones with QUANTISE on.\n\n";

    for (const auto& category : arc::presets::categories())
    {
        std::vector<const Profile*> in;
        for (const auto& p : profiles)
            if (p.category == category)
                in.push_back (&p);
        const std::string& title = category;
        md << "## " << title << " (" << in.size() << ")\n\n" << blurbs.at (title) << "\n\n| Preset | Exciter, material | Sound |\n|---|---|---|\n";
        for (const auto* p : in)
        {
            auto description = findDefinition (p->name)->description;
            for (auto& c : description)
                if (c == '|')
                    c = '/';
            md << "| **" << p->name << "** | " << kExciterNames[p->exciter & 3] << ", " << kMaterialNames[p->material & 3] << " | "
               << description << " |\n";
        }
        md << "\n";
    }
    std::ofstream (std::string (ARC_SOURCE_DIR) + "/docs/PRESETS.md") << md.str();
}
} // namespace

TEST_CASE ("library", "every preset is clean, truthful and unique")
{
    const bool wavs = envOr ("ARC_PRESET_WAVS", "0") == "1";
    const bool wholeLibrary = envOr ("ARC_PRESET_FILTER", "").empty(); // only a full audit rewrites the docs
    const auto profiles = profileLibrary (false, wavs);
    REQUIRE (! profiles.empty());

    std::ostringstream csv;
    csv << "name,category,exciter,material,patch_level_db,loudness_db,peak_db,hard_note_peak_db,tail_db,centroid_hz,sustain_db,width_db,"
           "modulation_db,pitch_cents,note_vs_strongest_db\n";
    if (wavs)
        std::filesystem::create_directories (outputDir() + "/presets");

    int unclean = 0, offTarget = 0, wrongCategory = 0, outOfTune = 0, pitchChecked = 0;
    for (const auto& d : arc::presets::factoryPresets())
        if (d.foldedTunings > 0)
            std::printf ("    note: %-22s folds %d: %s\n", d.name.c_str(), d.foldedTunings, d.tuningNotes.c_str());
    for (const auto& p : profiles)
    {
        csv << p.name << "," << p.category << "," << kExciterNames[p.exciter & 3] << "," << kMaterialNames[p.material & 3] << ","
            << presetValue (*findDefinition (p.name), arc::params::patchLevel, 0.0f) << "," << p.loudDb << "," << p.peakDb << ","
            << p.hardPeakDb << "," << p.tailDb << "," << p.centroidHz << "," << p.sustainDb << "," << p.widthDb << "," << p.fluxDb << "," << p.pitchCents << ","
            << p.noteVsStrongestDb << "\n";
        std::printf ("    %-24s %-12s loud %6.1f  peak %6.1f  hard %6.1f  sus %6.1f  tail %6.1f  cent %5.0f  wid %6.1f  mod %4.1f  pitch %7.1f (%5.1f)\n",
                     p.name.c_str(), p.category.c_str(), p.loudDb, p.peakDb, p.hardPeakDb, p.sustainDb, p.tailDb, p.centroidHz, p.widthDb,
                     p.fluxDb, p.pitchCents, p.noteVsStrongestDb);
        if (wavs)
            writeWav (outputDir() + "/presets/" + juce::File::createLegalFileName (p.name).toStdString() + ".wav", p.chord.left, p.chord.right,
                      kSr);

        // Clean.
        const bool clean = p.finite && p.peakDb < -1.0 && p.hardPeakDb < -1.0 && p.loudDb > -45.0 && p.dcDb < -30.0;
        if (! clean)
        {
            ++unclean;
            std::printf ("    UNCLEAN %s\n", p.name.c_str());
        }
        // On target, unless the trim was held back by the -3 dBFS peak ceiling.
        if (std::abs (p.loudDb - kTargetLoudnessDb) > 2.0 && ! (p.loudDb < kTargetLoudnessDb && std::max (p.peakDb, p.hardPeakDb) > -4.5))
        {
            ++offTarget;
            std::printf ("    OFF-TARGET %s (%.1f dB)\n", p.name.c_str(), p.loudDb);
        }

        // Truthful.
        std::string why;
        if (p.category == "PLUCKED" && p.exciter != 1)
            why = "not plucked";
        if (p.category == "STRUCK" && p.exciter != 0)
            why = "not struck";
        if (p.category == "BOWED" && p.exciter != 2)
            why = "not bowed";
        if (p.category == "AIR" && p.exciter != 3)
            why = "not blown";
        if (p.category == "GLASS" && p.material != 0)
            why = "not glass";
        if (p.category == "METAL" && p.material != 1)
            why = "not metal";
        if (p.category == "WOOD" && p.material != 2)
            why = "not wood";
        if (p.category == "MEMBRANE" && p.material != 3)
            why = "not membrane";
        if ((p.category == "PADS" || p.category == "DRONES") && p.sustainDb < -10.0)
            why = "does not sustain";
        if (p.category == "PERCUSSION" && p.sustainDb > -18.0)
            why = "rings too long for percussion";
        if (! why.empty())
        {
            ++wrongCategory;
            std::printf ("    CATEGORY %s: %s\n", p.name.c_str(), why.c_str());
        }

        pitchChecked += pitchExempt (p) ? 0 : 1;
        if (! pitchExempt (p) && (std::abs (p.pitchCents) > 12.0 || p.noteVsStrongestDb < -30.0))
        {
            ++outOfTune;
            std::printf ("    PITCH %s: %.1f cents, note %.1f dB below the strongest partial\n", p.name.c_str(), p.pitchCents,
                         -p.noteVsStrongestDb);
        }
    }

    // Unique.
    std::vector<std::tuple<double, size_t, size_t>> pairs;
    for (size_t a = 0; a < profiles.size(); ++a)
        for (size_t b = a + 1; b < profiles.size(); ++b)
            pairs.emplace_back (uniquenessDistance (profiles[a], profiles[b]), a, b);
    std::sort (pairs.begin(), pairs.end());
    for (size_t k = 0; k < std::min<size_t> (12, pairs.size()); ++k)
        std::printf ("    close pair %5.2f  %s / %s\n", std::get<0> (pairs[k]), profiles[std::get<1> (pairs[k])].name.c_str(),
                     profiles[std::get<2> (pairs[k])].name.c_str());

    double loudest = -300, quietest = 300;
    for (const auto& p : profiles)
        loudest = std::max (loudest, p.loudDb), quietest = std::min (quietest, p.loudDb);
    MEASURE ("presetsAudited", static_cast<double> (profiles.size()));
    MEASURE ("unclean", unclean);
    MEASURE ("offLoudnessTarget", offTarget);
    MEASURE ("wrongCategory", wrongCategory);
    MEASURE ("outOfTune", outOfTune);
    MEASURE ("loudest_db", loudest);
    MEASURE ("quietest_db", quietest);
    if (! pairs.empty())
        MEASURE ("closestPairDistance", std::get<0> (pairs.front()));

    if (wholeLibrary)
    {
        std::filesystem::create_directories (std::string (ARC_SOURCE_DIR) + "/docs/measurements/presets");
        std::ofstream (std::string (ARC_SOURCE_DIR) + "/docs/measurements/presets/library.csv") << csv.str();
        Audit audit;
        audit.presets = static_cast<int> (profiles.size());
        audit.unclean = unclean, audit.offTarget = offTarget, audit.wrongCategory = wrongCategory, audit.outOfTune = outOfTune;
        audit.pitchChecked = pitchChecked;
        audit.loudest = loudest, audit.quietest = quietest;
        if (! pairs.empty())
        {
            audit.closest = std::get<0> (pairs.front());
            audit.closestPair = profiles[std::get<1> (pairs.front())].name + " / " + profiles[std::get<2> (pairs.front())].name;
        }
        writeCatalogue (profiles, audit);
    }
    CHECK (unclean == 0);
    CHECK (offTarget == 0);
    CHECK (wrongCategory == 0);
    CHECK (outOfTune == 0);
    CHECK (pairs.empty() || std::get<0> (pairs.front()) > 2.0);
}

TEST_CASE ("library", "calibrate loudness")
{
    if (envOr ("ARC_CALIBRATE", "0") != "1")
    {
        MEASURE ("skipped (set ARC_CALIBRATE=1)", 1);
        return;
    }
    const auto profiles = profileLibrary (true, false);
    const auto path = std::string (ARC_SOURCE_DIR) + "/Source/Core/Presets/Calibration.inc";
    // Keep entries of presets outside the filter.
    std::map<std::string, float> trims;
    {
        std::ifstream in (path);
        std::string line;
        while (std::getline (in, line))
        {
            const auto q1 = line.find ('"'), q2 = line.find ('"', q1 + 1);
            if (q1 == std::string::npos || q2 == std::string::npos)
                continue;
            trims[line.substr (q1 + 1, q2 - q1 - 1)] = std::strtof (line.c_str() + q2 + 2, nullptr);
        }
    }
    // Loudness target, but never louder than -3 dBFS peak on the chord or on any hard single
    // note (short plucks and strikes), where the output's safety clip begins.
    for (const auto& p : profiles)
        trims[p.name] = static_cast<float> (
            std::clamp (std::min (kTargetLoudnessDb - p.loudDb, -3.0 - std::max (p.peakDb, p.hardPeakDb)), -24.0, 18.0));
    std::ofstream out (path);
    out << "// Generated by `ARC_CALIBRATE=1 ARCTests calibrate`: per-preset loudness trims (dB) that put the\n"
           "// C-E-G test chord at "
        << kTargetLoudnessDb << " dB momentary loudness. Do not edit by hand; re-run after changing a preset.\n";
    for (const auto& [name, trim] : trims)
    {
        char buf[32];
        std::snprintf (buf, sizeof (buf), "%.2f", static_cast<double> (trim));
        out << "{ \"" << name << "\", " << buf << "f },\n";
    }
    MEASURE ("calibrated", static_cast<double> (profiles.size()));
}

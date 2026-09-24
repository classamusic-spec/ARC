// Phases 9-10 — factory presets, preset manager, RANDOM, complete state save / restore.

#include "Analysis.h"
#include "ArcTest.h"

#include <filesystem>
#include <fstream>
#include <set>

#include "Core/FactoryPresets.h"
#include "Core/Parameters.h"
#include "Core/PresetManager.h"
#include "Core/Randomiser.h"
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

struct Stereo
{
    Signal left, right, mono;
};

/** Renders the processor through processBlock. `events(midi, t)` adds MIDI for a block. */
template <typename Events>
Stereo render (ArcAudioProcessor& p, double seconds, Events&& events)
{
    Stereo out;
    juce::AudioBuffer<float> buffer (2, kBlock);
    juce::MidiBuffer midi;
    const int blocks = static_cast<int> (seconds * kSr / kBlock);
    for (int b = 0; b < blocks; ++b)
    {
        midi.clear();
        events (midi, b * kBlock / kSr);
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

/** C3-E3-G3 chord held for `hold` seconds. */
auto chord (double hold, float velocity = 0.8f)
{
    return [hold, velocity] (juce::MidiBuffer& m, double t)
    {
        if (t == 0.0)
            for (int n : { 48, 52, 55 })
                m.addEvent (juce::MidiMessage::noteOn (1, n, velocity), 0);
        if (t < hold && t + kBlock / kSr >= hold)
            for (int n : { 48, 52, 55 })
                m.addEvent (juce::MidiMessage::noteOff (1, n), 0);
    };
}

double db (double linear) { return 20.0 * std::log10 (linear + 1.0e-12); }

/** Momentary loudness proxy: loudest 400 ms RMS window (100 ms hop) in [t0, t1). */
double momentaryDb (const Signal& x, double t0, double t1)
{
    double best = 0;
    for (double t = t0; t + 0.4 <= t1; t += 0.1)
        best = std::max (best, rms (x, static_cast<int> (t * kSr), static_cast<int> (0.4 * kSr)));
    return db (best);
}

std::vector<double> bandsDb (const Signal& x)
{
    const auto s = computeSpectrum (x, kSr, 1 << 16, static_cast<int> (0.05 * kSr), static_cast<int> (1.5 * kSr));
    std::vector<double> b;
    for (double f = 60.0; f < 12000.0; f *= std::pow (2.0, 1.0 / 6.0))
    {
        const int lo = static_cast<int> (f / s.binHz), hi = static_cast<int> (f * std::pow (2.0, 1.0 / 6.0) / s.binHz);
        double e = 0;
        for (int i = lo; i < hi; ++i)
            e += s.mag[static_cast<size_t> (i)] * s.mag[static_cast<size_t> (i)];
        b.push_back (10.0 * std::log10 (e + 1e-20));
    }
    // Level-normalised: compare spectral *shape*, not loudness.
    double mean = 0;
    for (auto v : b)
        mean += v;
    mean /= static_cast<double> (b.size());
    for (auto& v : b)
        v -= mean;
    return b;
}

double lsd (const std::vector<double>& a, const std::vector<double>& b)
{
    double d = 0;
    for (size_t i = 0; i < a.size(); ++i)
        d += (a[i] - b[i]) * (a[i] - b[i]);
    return std::sqrt (d / static_cast<double> (a.size()));
}

juce::File tempRoot (const char* name)
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("arc-tests").getChildFile (name);
    dir.deleteRecursively();
    dir.createDirectory();
    return dir;
}

void setPlain (ArcAudioProcessor& p, const juce::String& id, float plain)
{
    auto* param = p.getValueTreeState().getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (plain));
}

float getPlain (ArcAudioProcessor& p, const juce::String& id)
{
    auto* param = p.getValueTreeState().getParameter (id);
    return param->convertFrom0to1 (param->getValue());
}
} // namespace

// -------------------------------------------------------------------------------------
TEST_CASE ("presets", "factory library is complete and valid")
{
    ArcAudioProcessor proc;
    const auto& list = arc::presets::factoryPresets();
    MEASURE ("factoryPresets", static_cast<double> (list.size()));
    CHECK (list.size() >= 40);

    std::map<std::string, int> perCategory;
    std::set<std::string> names;
    int badIds = 0, outOfRange = 0, badGestures = 0, clampedTunings = 0, noDescription = 0;
    for (const auto& p : list)
    {
        names.insert (p.name);
        ++perCategory[p.category];
        if (p.description.empty() || p.tags.empty())
            ++noDescription;
        for (const auto& [id, value] : p.values)
        {
            auto* param = proc.getValueTreeState().getParameter (id);
            if (param == nullptr || ! arc::PresetManager::isPresetParameter (id))
            {
                ++badIds;
                std::printf ("    bad id in %s: %s\n", p.name.c_str(), id.c_str());
                continue;
            }
            const auto& range = param->getNormalisableRange();
            if (value < range.start || value > range.end || ! std::isfinite (value))
                ++outOfRange;
            // Tuned nodes must be reachable without clamping.
            if (juce::String (id).contains (".radius.") && (value <= 0.0f || value >= 1.0f))
                ++clampedTunings;
        }
        for (const auto& g : p.gestures)
            if (! g.empty() && ! arc::Gesture::deserialise (g).valid)
                ++badGestures;
    }
    for (const auto& c : arc::presets::categories())
    {
        MEASURE ("category." + c, perCategory[c]);
        CHECK (perCategory[c] >= 3);
    }
    CHECK (perCategory.size() == arc::presets::categories().size());
    CHECK (names.size() == list.size()); // unique names

    // Every example name from the product specification.
    int missing = 0;
    for (const char* required : { "Obsidian Bloom", "Glass Engine", "Frozen Wire", "Ceramic Pulse", "Black Bell",
                                  "Hollow Circuit", "Satellite String", "Magnetic Wood", "Glass Choir", "Membrane Sky",
                                  "Slow Alloy", "Copper Rain", "Deep Frame", "Crystal Thread", "Tension Bloom",
                                  "Broken Halo", "Soft Machine", "Iron Flower", "Zero Gravity Bell", "Infinite Bow",
                                  "Resonant Fog", "Wire Garden", "Cold Plate", "Ghost Membrane", "Mercury String" })
        if (names.count (required) == 0)
        {
            ++missing;
            std::printf ("    missing preset: %s\n", required);
        }
    MEASURE ("missingSpecNames", missing);
    MEASURE ("badIds", badIds);
    MEASURE ("outOfRange", outOfRange);
    MEASURE ("badGestures", badGestures);
    MEASURE ("clampedTunings", clampedTunings);
    CHECK (missing == 0);
    CHECK (badIds == 0);
    CHECK (outOfRange == 0);
    CHECK (badGestures == 0);
    CHECK (clampedTunings == 0);
    CHECK (noDescription == 0);
    CHECK (proc.getNumPrograms() == static_cast<int> (list.size()));
    CHECK (proc.getPresetManager().getCurrentName() == "Obsidian Bloom"); // opens on the signature sound
}

// -------------------------------------------------------------------------------------
TEST_CASE ("presets", "every factory preset renders cleanly")
{
    const auto& list = arc::presets::factoryPresets();
    std::filesystem::create_directories (outputDir() + "/presets");
    std::filesystem::create_directories (std::string (ARC_SOURCE_DIR) + "/docs/measurements/phase9");
    std::ofstream csv (std::string (ARC_SOURCE_DIR) + "/docs/measurements/phase9/factory_presets.csv");
    csv << "name,category,momentary_db,peak_db,tail_db,centroid_hz,dc\n";

    double loudest = -200, quietest = 200;
    std::string loudestName, quietestName;
    int nonFinite = 0, clipped = 0, silent = 0, dcOffsets = 0;
    std::vector<std::vector<double>> shapes;
    for (size_t i = 0; i < list.size(); ++i)
    {
        ArcAudioProcessor proc;
        proc.getPresetManager().loadPreset (static_cast<int> (i));
        proc.prepareToPlay (kSr, kBlock);
        const auto out = render (proc, 4.0, chord (2.0));

        const double level = momentaryDb (out.mono, 0.0, 2.0);
        const double peak = db (std::max (peakAbs (out.left), peakAbs (out.right)));
        const double tail = db (rms (out.mono, static_cast<int> (3.5 * kSr), static_cast<int> (0.4 * kSr)));
        const double centroid = spectralCentroid (out.mono, kSr, static_cast<int> (0.1 * kSr), static_cast<int> (1.8 * kSr));
        double mean = 0;
        for (int s = static_cast<int> (0.1 * kSr); s < static_cast<int> (1.9 * kSr); ++s)
            mean += out.mono[static_cast<size_t> (s)];
        mean /= 1.8 * kSr;
        const double dcDb = db (std::abs (mean)) - db (rms (out.mono, static_cast<int> (0.1 * kSr), static_cast<int> (1.8 * kSr)));

        const auto& name = list[i].name;
        csv << name << "," << list[i].category << "," << level << "," << peak << "," << tail << "," << centroid << ","
            << dcDb << "\n";
        std::printf ("    %-20s %-13s loud %6.1f dB  peak %6.1f dB  tail %6.1f dB  centroid %6.0f Hz\n", name.c_str(),
                     list[i].category.c_str(), level, peak, tail, centroid);
        writeWav (outputDir() + "/presets/" + juce::File::createLegalFileName (name).toStdString() + ".wav", out.left,
                  out.right, kSr);

        if (! allFinite (out.left) || ! allFinite (out.right))
            ++nonFinite;
        if (peak > -0.1)
            ++clipped;
        if (level < -45.0)
            ++silent;
        if (dcDb > -30.0)
            ++dcOffsets;
        if (level > loudest)
            loudest = level, loudestName = name;
        if (level < quietest)
            quietest = level, quietestName = name;
        shapes.push_back (bandsDb (out.mono));
    }
    std::printf ("    loudest: %s (%.1f dB), quietest: %s (%.1f dB)\n", loudestName.c_str(), loudest, quietestName.c_str(),
                 quietest);
    MEASURE ("loudest_db", loudest);
    MEASURE ("quietest_db", quietest);
    MEASURE ("loudnessSpread_db", loudest - quietest);
    MEASURE ("nonFinite", nonFinite);
    MEASURE ("clipped", clipped);
    MEASURE ("silent", silent);
    MEASURE ("dcOffsets", dcOffsets);
    CHECK (nonFinite == 0);
    CHECK (clipped == 0);
    CHECK (silent == 0);
    CHECK (dcOffsets == 0);
    CHECK (loudest - quietest < 12.0);

    // Distinctness: no two presets share a spectral shape.
    double closest = 1e9;
    std::string pair;
    for (size_t a = 0; a < shapes.size(); ++a)
        for (size_t b = a + 1; b < shapes.size(); ++b)
        {
            const double d = lsd (shapes[a], shapes[b]);
            if (d < closest)
                closest = d, pair = list[a].name + " / " + list[b].name;
        }
    std::printf ("    closest pair: %s (%.2f dB)\n", pair.c_str(), closest);
    MEASURE ("closestPairLsd_db", closest);
    CHECK (closest > 1.0);
}

// -------------------------------------------------------------------------------------
TEST_CASE ("state", "round trip reproduces the sound exactly")
{
    ArcAudioProcessor a;
    auto& pm = a.getPresetManager();
    pm.loadPreset (pm.findPreset ("factory/Satellite String"));
    setPlain (a, arc::params::coupling, 0.47f);
    setPlain (a, arc::params::chaos, 0.33f);
    setPlain (a, arc::params::masterOutput, -7.5f);
    setPlain (a, arc::params::freeze, 0.0f);
    setPlain (a, arc::params::topology, 2.0f);
    setPlain (a, arc::params::sync, 1.0f);
    a.setGesture (2, arc::swayGesture (3.0f, 8.0f, 0.7f, 0.03f));
    a.setSeed (0x12345678u);
    CHECK (pm.isModified());

    juce::MemoryBlock state;
    a.getStateInformation (state);
    MEASURE ("stateBytes", static_cast<double> (state.getSize()));

    ArcAudioProcessor b;
    b.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    int paramMismatch = 0;
    auto& pa = a.getParameters();
    auto& pb = b.getParameters();
    for (int i = 0; i < pa.size(); ++i)
        if (std::abs (pa[i]->getValue() - pb[i]->getValue()) > 1.0e-6f)
            ++paramMismatch;
    int gestureMismatch = 0;
    for (int n = 0; n < 4; ++n)
        if (a.getGesture (n).serialise() != b.getGesture (n).serialise())
            ++gestureMismatch;
    MEASURE ("parameterMismatches", paramMismatch);
    MEASURE ("gestureMismatches", gestureMismatch);
    CHECK (paramMismatch == 0);
    CHECK (gestureMismatch == 0);
    CHECK (a.getGesture (0).valid && a.getGesture (2).valid); // preset gesture + recorded one
    CHECK (b.getSeed() == 0x12345678u);
    CHECK (b.getPresetManager().getCurrentName() == "Satellite String");
    CHECK (b.getPresetManager().isModified());

    // "DAW reload must reproduce the sound": both render the same MIDI identically.
    a.prepareToPlay (kSr, kBlock);
    b.prepareToPlay (kSr, kBlock);
    const auto ra = render (a, 3.0, chord (1.5));
    const auto rb = render (b, 3.0, chord (1.5));
    double maxDiff = 0;
    for (size_t i = 0; i < ra.left.size(); ++i)
        maxDiff = std::max ({ maxDiff, static_cast<double> (std::abs (ra.left[i] - rb.left[i])),
                              static_cast<double> (std::abs (ra.right[i] - rb.right[i])) });
    MEASURE ("renderMaxDiff", maxDiff);
    MEASURE ("renderRms_db", db (rms (ra.mono)));
    CHECK (maxDiff < 1.0e-6);
    CHECK (rms (ra.mono) > 1.0e-3);
}

TEST_CASE ("state", "legacy and hostile state is handled")
{
    ArcAudioProcessor p;
    const float before = getPlain (p, arc::params::coupling);

    // Garbage and foreign XML: ignored.
    juce::MemoryBlock junk;
    juce::Random rng (7);
    for (int i = 0; i < 4096; ++i)
        junk.append (&i, 1);
    p.setStateInformation (junk.getData(), static_cast<int> (junk.getSize()));
    juce::MemoryBlock foreign;
    juce::XmlElement other ("SomethingElse");
    other.setAttribute ("x", 1);
    juce::AudioProcessor::copyXmlToBinary (other, foreign);
    p.setStateInformation (foreign.getData(), static_cast<int> (foreign.getSize()));
    p.setStateInformation (nullptr, 0);
    CHECK (std::abs (getPlain (p, arc::params::coupling) - before) < 1.0e-6f);

    // Pre-release state (APVTS tree only) still loads.
    setPlain (p, arc::params::coupling, 0.61f);
    juce::MemoryBlock legacy;
    if (auto xml = p.getValueTreeState().copyState().createXml())
        juce::AudioProcessor::copyXmlToBinary (*xml, legacy);
    ArcAudioProcessor q;
    q.setStateInformation (legacy.getData(), static_cast<int> (legacy.getSize()));
    MEASURE ("legacyCoupling", getPlain (q, arc::params::coupling));
    CHECK (std::abs (getPlain (q, arc::params::coupling) - 0.61f) < 1.0e-4f);

    // A corrupted gesture inside otherwise valid state is dropped, not played.
    juce::ValueTree s ("ARC_STATE");
    s.appendChild (p.getValueTreeState().copyState(), nullptr);
    juce::ValueTree g ("GESTURES"), gt ("GESTURE");
    gt.setProperty ("node", 1, nullptr);
    gt.setProperty ("data", "v1 nan nan 1 2 3", nullptr);
    g.appendChild (gt, nullptr);
    s.appendChild (g, nullptr);
    juce::MemoryBlock bad;
    juce::AudioProcessor::copyXmlToBinary (*s.createXml(), bad);
    q.setStateInformation (bad.getData(), static_cast<int> (bad.getSize()));
    CHECK (! q.getGesture (1).valid);
}

// -------------------------------------------------------------------------------------
TEST_CASE ("presets", "modified flag, favourites and user presets")
{
    ArcAudioProcessor p;
    const auto root = tempRoot ("user-presets");
    {
        arc::PresetManager pm (p, root);
        const int glass = pm.findPreset ("factory/Glass Engine");
        REQUIRE (glass >= 0);
        pm.loadPreset (glass);
        CHECK (! pm.isModified());
        setPlain (p, arc::params::tension, 0.7f);
        CHECK (pm.isModified());
        pm.loadPreset (glass);
        CHECK (! pm.isModified());
        p.setGesture (3, arc::orbitGesture (5.0f, 0.0f, 1.0f, 0.0f));
        CHECK (pm.isModified());
        // Performance parameters are not part of the sound.
        pm.loadPreset (glass);
        setPlain (p, arc::params::masterOutput, -12.0f);
        CHECK (! pm.isModified());

        // Save a user preset, change everything, load it back.
        setPlain (p, arc::params::coupling, 0.57f);
        p.setGesture (1, arc::orbitGesture (7.0f, 0.0f, -1.0f, 0.01f));
        p.setSeed (99u);
        CHECK (pm.saveUserPreset ("My Test Network", "PADS", "a test"));
        CHECK (root.getChildFile ("Presets").getChildFile ("My Test Network.arcpreset").existsAsFile());
        CHECK (pm.getCurrentName() == "My Test Network");
        CHECK (! pm.isModified());
        const int user = pm.findPreset ("user/My Test Network");
        REQUIRE (user >= pm.getNumPresets() - 1);

        pm.loadPreset (0);
        CHECK (std::abs (getPlain (p, arc::params::coupling) - 0.57f) > 0.01f);
        pm.loadPreset (user);
        CHECK (std::abs (getPlain (p, arc::params::coupling) - 0.57f) < 1.0e-3f);
        CHECK (p.getGesture (1).valid);
        CHECK (p.getSeed() == 99u);
        CHECK (std::abs (getPlain (p, arc::params::masterOutput) + 12.0f) < 1.0e-3f); // untouched

        pm.setFavourite (glass, true);
        pm.setFavourite (user, true);
    }
    {
        // A new session sees the saved preset and the favourites.
        arc::PresetManager pm (p, root);
        const int glass = pm.findPreset ("factory/Glass Engine");
        const int user = pm.findPreset ("user/My Test Network");
        CHECK (user >= 0);
        CHECK (pm.isFavourite (glass));
        CHECK (pm.isFavourite (user));
        CHECK (! pm.isFavourite (0));

        // Navigation wraps around the whole library.
        pm.loadPreset (pm.getNumPresets() - 1);
        pm.loadNext();
        CHECK (pm.getCurrentIndex() == 0);
        pm.loadPrevious();
        CHECK (pm.getCurrentIndex() == pm.getNumPresets() - 1);

        CHECK (pm.deleteUserPreset (user));
        CHECK (pm.findPreset ("user/My Test Network") < 0);
        CHECK (! pm.deleteUserPreset (0)); // factory presets are read-only
    }
    // Presets from a newer version with unknown parameters still load.
    {
        arc::PresetManager::Preset future;
        future.name = "Future";
        future.values["arc.future.param.v7"] = 0.3f;
        future.values[arc::params::coupling] = 0.2f;
        const auto text = arc::PresetManager::toXmlString (future);
        arc::PresetManager::Preset parsed;
        auto xml = juce::XmlDocument::parse (text);
        REQUIRE (xml != nullptr);
        CHECK (arc::PresetManager::fromXml (*xml, parsed));
        arc::PresetManager pm (p, root);
        pm.applyPreset (parsed);
        CHECK (std::abs (getPlain (p, arc::params::coupling) - 0.2f) < 1.0e-3f);
        CHECK (std::abs (getPlain (p, arc::params::tension) - 0.5f) < 1.0e-3f); // missing -> default
    }
    root.deleteRecursively();
}

// -------------------------------------------------------------------------------------
TEST_CASE ("random", "gentle mutation stays close to the sound")
{
    ArcAudioProcessor p;
    auto& pm = p.getPresetManager();
    pm.loadPreset (pm.findPreset ("factory/Black Bell"));
    const auto before = pm.captureCurrent();
    juce::Random rng (1234);
    pm.randomise (false, rng);
    const auto after = pm.captureCurrent();

    double maxStep = 0;
    int discreteChanged = 0;
    for (const auto& [id, v] : before.values)
    {
        const float w = after.values.at (id);
        auto* param = p.getValueTreeState().getParameter (id);
        const bool discrete = param->isDiscrete() || param->isBoolean();
        if (discrete)
            discreteChanged += std::abs (w - v) > 1.0e-6f ? 1 : 0;
        else
            maxStep = std::max (maxStep, static_cast<double> (std::abs (param->convertTo0to1 (w) - param->convertTo0to1 (v))));
    }
    MEASURE ("maxNormalisedStep", maxStep);
    MEASURE ("discreteChanged", discreteChanged);
    CHECK (maxStep > 0.005); // it did something
    CHECK (maxStep < 0.2);
    CHECK (discreteChanged == 0);
    CHECK (after.seed != before.seed); // CHAOS reseeded
    CHECK (pm.getCurrentName() == "Black Bell");
    CHECK (pm.isModified());
}

TEST_CASE ("random", "full regeneration is musical and safe")
{
    std::filesystem::create_directories (outputDir() + "/random");
    int nonFinite = 0, clipped = 0, silent = 0, outside = 0;
    double quietest = 0, loudest = -200;
    std::set<int> exciters, materials;
    for (int i = 0; i < 24; ++i)
    {
        ArcAudioProcessor p;
        auto& pm = p.getPresetManager();
        // juce::Random is an LCG: neighbouring seeds give near-identical first draws.
        juce::Random rng (static_cast<juce::int64> ((0x9E3779B97F4A7C15ull * static_cast<uint64_t> (i + 1)) >> 11));
        pm.randomise (true, rng);
        const auto v = pm.captureCurrent().values;
        exciters.insert (juce::roundToInt (v.at (arc::params::exciterType)));
        materials.insert (juce::roundToInt (v.at (arc::params::materialType)));
        // Musical windows. (The whole COUPLING range is in tune since the RC coupling
        // curve; regeneration keeps a little away from both ends.)
        if (v.at (arc::params::tension) < 0.3f || v.at (arc::params::tension) > 0.7f || v.at (arc::params::chaos) > 0.46f
            || v.at (arc::params::coupling) < 0.08f || v.at (arc::params::coupling) > 0.92f)
            ++outside;

        p.prepareToPlay (kSr, kBlock);
        const auto out = render (p, 2.5, chord (1.5));
        const double level = db (rms (out.mono, static_cast<int> (0.1 * kSr), static_cast<int> (1.3 * kSr)));
        const double peak = db (std::max (peakAbs (out.left), peakAbs (out.right)));
        nonFinite += (allFinite (out.left) && allFinite (out.right)) ? 0 : 1;
        clipped += peak > -0.1 ? 1 : 0;
        silent += level < -45.0 ? 1 : 0;
        quietest = std::min (quietest, level);
        loudest = std::max (loudest, level);
        if (i < 6)
            writeWav (outputDir() + "/random/random_" + std::to_string (i) + ".wav", out.left, out.right, kSr);
    }
    MEASURE ("distinctExciters", static_cast<double> (exciters.size()));
    MEASURE ("distinctMaterials", static_cast<double> (materials.size()));
    MEASURE ("quietest_db", quietest);
    MEASURE ("loudest_db", loudest);
    MEASURE ("outsideWindows", outside);
    CHECK (nonFinite == 0);
    CHECK (clipped == 0);
    CHECK (silent == 0);
    CHECK (outside == 0);
    CHECK (exciters.size() == 4);
    CHECK (materials.size() == 4);
}

// -------------------------------------------------------------------------------------
TEST_CASE ("presets", "switching presets while notes sound is safe")
{
    ArcAudioProcessor p;
    auto& pm = p.getPresetManager();
    p.prepareToPlay (kSr, kBlock);
    int next = 0;
    double worstPeak = 0;
    const auto out = render (p, 12.0, [&] (juce::MidiBuffer& m, double t)
                             {
                                 const int block = static_cast<int> (std::lround (t * kSr / kBlock));
                                 if (block % 12 == 0) // ~130 ms
                                 {
                                     pm.loadPreset (next++ % pm.getNumPresets());
                                     m.addEvent (juce::MidiMessage::noteOn (1, 48 + (block / 12) % 24, 0.9f), 0);
                                 }
                                 if (block % 12 == 6)
                                     m.addEvent (juce::MidiMessage::noteOff (1, 48 + (block / 12) % 24), 0);
                             });
    worstPeak = std::max (peakAbs (out.left), peakAbs (out.right));
    MEASURE ("presetsVisited", next);
    MEASURE ("peak_db", db (worstPeak));
    CHECK (allFinite (out.left) && allFinite (out.right));
    CHECK (worstPeak < 1.0);
    CHECK (p.getEngine().nonFiniteVoiceResets == 0);
}

// -------------------------------------------------------------------------------------
TEST_CASE ("state", "automating every parameter is safe and smooth")
{
    ArcAudioProcessor p;
    p.getPresetManager().loadPreset (p.getPresetManager().findPreset ("factory/Temple Lattice"));
    p.prepareToPlay (kSr, kBlock);
    const auto params = p.getParameters();

    std::ofstream csv (std::string (ARC_SOURCE_DIR) + "/docs/measurements/phase9/automation.csv");
    csv << "parameter,peak_db,max_step_ratio\n";
    int nonFinite = 0, worstIndex = -1;
    double worstStepRatio = 0, worstPeak = 0;
    for (int i = 0; i < params.size(); ++i)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (params[i]);
        if (param == nullptr || param->getParameterID() == arc::params::freeze)
            continue;
        const float original = param->getValue();
        // Settle, then sweep 0 -> 1 -> original over ~0.6 s while a strike chord rings.
        const auto out = render (p, 1.0, [&] (juce::MidiBuffer& m, double t)
                                 {
                                     if (t == 0.0)
                                         for (int n : { 48, 55, 64 })
                                             m.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), 0);
                                     if (t > 0.3 && t < 0.9)
                                     {
                                         const double x = (t - 0.3) / 0.6;
                                         const float v = x < 0.5 ? static_cast<float> (2.0 * x)
                                                                 : static_cast<float> (1.0 + (original - 1.0) * (2.0 * x - 1.0));
                                         param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v));
                                     }
                                     if (t >= 0.9 && t < 0.9 + kBlock / kSr)
                                     {
                                         param->setValueNotifyingHost (original);
                                         for (int n : { 48, 55, 64 })
                                             m.addEvent (juce::MidiMessage::noteOff (1, n), 0);
                                     }
                                 });
        if (! allFinite (out.left) || ! allFinite (out.right))
            ++nonFinite;
        // Click metric: largest sample-to-sample step during the sweep relative to the
        // largest step while the chord rings untouched.
        auto maxStep = [&] (double t0, double t1)
        {
            double m = 0;
            for (int s = static_cast<int> (t0 * kSr) + 1; s < static_cast<int> (t1 * kSr); ++s)
                m = std::max (m, static_cast<double> (std::abs (out.mono[static_cast<size_t> (s)] - out.mono[static_cast<size_t> (s - 1)])));
            return m;
        };
        const double ref = std::max (maxStep (0.1, 0.3), 1.0e-4);
        const double ratio = maxStep (0.3, 0.9) / ref;
        const double peak = std::max (peakAbs (out.left), peakAbs (out.right));
        csv << param->getParameterID() << "," << db (peak) << "," << ratio << "\n";
        if (ratio > worstStepRatio)
            worstStepRatio = ratio, worstIndex = i;
        worstPeak = std::max (worstPeak, peak);
        // Silence between parameters so each test starts clean.
        render (p, 0.4, [] (juce::MidiBuffer& m, double t)
                {
                    if (t == 0.0)
                        m.addEvent (juce::MidiMessage::allSoundOff (1), 0);
                });
    }
    MEASURE ("parametersAutomated", static_cast<double> (params.size() - 1));
    MEASURE ("worstStepRatio", worstStepRatio);
    if (worstIndex >= 0)
        std::printf ("    worst: %s\n", params[worstIndex]->getName (64).toRawUTF8());
    MEASURE ("worstPeak_db", db (worstPeak));
    CHECK (nonFinite == 0);
    CHECK (worstPeak < 1.0);
    CHECK (worstStepRatio < 6.0);
}

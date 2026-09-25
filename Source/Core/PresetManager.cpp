#include "Core/PresetManager.h"

#include "Core/FactoryPresets.h"
#include "Core/Parameters.h"
#include "Core/Randomiser.h"
#include "Motion/Gesture.h"
#include "PluginProcessor.h"

namespace arc
{

namespace
{
const juce::Identifier kPresetTag ("ArcPreset");
const juce::Identifier kParamTag ("Param");
const juce::Identifier kGestureTag ("Gesture");
constexpr int kPresetFormat = 1;

const char* exciterName (int i)
{
    static const char* names[] = { "Strike", "Pluck", "Bow", "Air" };
    return names[juce::jlimit (0, 3, i)];
}
const char* materialName (int i)
{
    static const char* names[] = { "Glass", "Metal", "Wood", "Membrane" };
    return names[juce::jlimit (0, 3, i)];
}
} // namespace

PresetManager::PresetManager (ArcAudioProcessor& p, juce::File rootDirectory)
    : processor (p), root (std::move (rootDirectory))
{
    for (auto* param : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (isPresetParameter (ranged->getParameterID()))
                presetParams.push_back (ranged);

    loadFactoryPresets();
    rescanUserPresets();
    loadFavourites();
    takeSnapshot (nullptr);
}

juce::File PresetManager::defaultRootDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("ARC");
}

bool PresetManager::isPresetParameter (const juce::String& id)
{
    return id != params::masterOutput && id != params::polyphony && id != params::bendRange && id != params::mpe
           && id != params::quality && id != params::freeze;
}

// -------------------------------------------------------------------------------------
// Library
// -------------------------------------------------------------------------------------
void PresetManager::loadFactoryPresets()
{
    presets.clear();
    for (const auto& def : presets::factoryPresets())
    {
        Preset p;
        p.name = def.name;
        p.category = def.category;
        p.tags = def.tags;
        p.description = def.description;
        p.author = "ARC Factory";
        p.seed = def.seed;
        for (const auto& kv : def.values)
            p.values[juce::String (kv.first)] = kv.second;
        for (size_t n = 0; n < 4; ++n)
            p.gestures[n] = def.gestures[n];
        p.isFactory = true;
        presets.push_back (std::move (p));
    }
    numFactory = static_cast<int> (presets.size());
}

void PresetManager::rescanUserPresets()
{
    const auto currentKey = currentIndex >= 0 ? presets[static_cast<size_t> (currentIndex)].key() : juce::String();
    presets.resize (static_cast<size_t> (numFactory));

    std::vector<Preset> user;
    const auto dir = getUserPresetDirectory();
    if (dir.isDirectory())
        for (const auto& entry : juce::RangedDirectoryIterator (dir, false, juce::String ("*") + fileExtension))
            if (auto xml = juce::XmlDocument::parse (entry.getFile()))
            {
                Preset p;
                if (fromXml (*xml, p))
                {
                    p.isFactory = false;
                    p.file = entry.getFile();
                    user.push_back (std::move (p));
                }
            }
    std::sort (user.begin(), user.end(), [] (const Preset& a, const Preset& b)
               { return a.category != b.category ? a.category < b.category : a.name.compareIgnoreCase (b.name) < 0; });
    for (auto& p : user)
        presets.push_back (std::move (p));

    if (currentKey.isNotEmpty())
        currentIndex = findPreset (currentKey);
    notify();
}

int PresetManager::findPreset (const juce::String& key) const
{
    for (size_t i = 0; i < presets.size(); ++i)
        if (presets[i].key() == key)
            return static_cast<int> (i);
    return -1;
}

// -------------------------------------------------------------------------------------
// Loading / applying
// -------------------------------------------------------------------------------------
float PresetManager::plainValueFor (const Preset& p, juce::RangedAudioParameter& param) const
{
    const auto& range = param.getNormalisableRange();
    const auto it = p.values.find (param.getParameterID());
    if (it != p.values.end() && std::isfinite (it->second))
        return juce::jlimit (range.start, range.end, it->second);
    return range.convertFrom0to1 (param.getDefaultValue());
}

void PresetManager::applyPreset (const Preset& preset)
{
    processor.beginPatchChange(); // before the values: sounding notes keep their patch's level
    for (auto* param : presetParams)
        param->setValueNotifyingHost (param->convertTo0to1 (plainValueFor (preset, *param)));
    for (int n = 0; n < 4; ++n)
        processor.setGesture (n, Gesture::deserialise (preset.gestures[static_cast<size_t> (n)].toStdString()));
    processor.setSeed (preset.seed);
}

void PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= getNumPresets())
        return;
    const auto& p = presets[static_cast<size_t> (index)];
    applyPreset (p);
    currentIndex = index;
    currentName = p.name;
    currentCategory = p.category;
    takeSnapshot (nullptr);
    notify();
}

void PresetManager::loadNext()
{
    if (presets.empty())
        return;
    loadPreset (currentIndex < 0 ? 0 : (currentIndex + 1) % getNumPresets());
}

void PresetManager::loadPrevious()
{
    if (presets.empty())
        return;
    loadPreset (currentIndex <= 0 ? getNumPresets() - 1 : currentIndex - 1);
}

void PresetManager::loadInit()
{
    Preset init;
    init.name = "Init";
    init.seed = 0xA2C1u;
    applyPreset (init);
    currentIndex = -1;
    currentName = init.name;
    currentCategory = {};
    takeSnapshot (nullptr);
    notify();
}

PresetManager::Preset PresetManager::captureCurrent() const
{
    Preset p;
    p.name = currentName;
    p.category = currentCategory;
    p.seed = processor.getSeed();
    for (auto* param : presetParams)
        p.values[param->getParameterID()] = param->convertFrom0to1 (param->getValue());
    for (int n = 0; n < 4; ++n)
        p.gestures[static_cast<size_t> (n)] = processor.getGesture (n).serialise();
    p.isFactory = false;
    return p;
}

// -------------------------------------------------------------------------------------
// Modified detection
// -------------------------------------------------------------------------------------
void PresetManager::takeSnapshot (const Preset* from)
{
    snapshot.resize (presetParams.size());
    for (size_t i = 0; i < presetParams.size(); ++i)
    {
        auto* param = presetParams[i];
        snapshot[i] = from != nullptr ? param->convertTo0to1 (plainValueFor (*from, *param)) : param->getValue();
    }
    for (int n = 0; n < 4; ++n)
        snapshotGestures[static_cast<size_t> (n)] = from != nullptr
                                                        ? juce::String (Gesture::deserialise (from->gestures[static_cast<size_t> (n)].toStdString()).serialise())
                                                        : juce::String (processor.getGesture (n).serialise());
}

bool PresetManager::isModified() const
{
    for (size_t i = 0; i < presetParams.size(); ++i)
        if (std::abs (presetParams[i]->getValue() - snapshot[i]) > 1.0e-4f)
            return true;
    for (int n = 0; n < 4; ++n)
        if (juce::String (processor.getGesture (n).serialise()) != snapshotGestures[static_cast<size_t> (n)])
            return true;
    return false;
}

// -------------------------------------------------------------------------------------
// RANDOM
// -------------------------------------------------------------------------------------
void PresetManager::randomise (bool full, juce::Random& rng)
{
    auto current = captureCurrent();
    if (full)
    {
        randomiser::regenerate (current.values, rng);
        for (auto& g : current.gestures)
            g.clear();
        // Sometimes a node starts orbiting: MOTION is part of ARC's identity.
        if (rng.nextFloat() < 0.3f)
        {
            const float seconds = 4.0f + 8.0f * rng.nextFloat();
            current.gestures[static_cast<size_t> (rng.nextInt (4))] =
                rng.nextBool() ? orbitGesture (seconds, 0.0f, rng.nextBool() ? 1.0f : -1.0f, 0.004f).serialise()
                               : swayGesture (seconds, 0.0f, 0.4f + 0.8f * rng.nextFloat(), 0.01f).serialise();
        }
    }
    else
        randomiser::mutate (current.values, rng);
    current.seed = static_cast<uint32_t> (rng.nextInt64());

    applyPreset (current);
    if (full)
    {
        currentIndex = -1;
        currentName = juce::String ("Random ") + materialName (juce::roundToInt (current.values[params::materialType])) + " "
                      + exciterName (juce::roundToInt (current.values[params::exciterType]));
        currentCategory = {};
        takeSnapshot (nullptr);
    }
    // A gentle mutation keeps the preset's name and reads as modified.
    notify();
}

void PresetManager::randomise (bool full)
{
    juce::Random rng;
    rng.setSeedRandomly();
    randomise (full, rng);
}

// -------------------------------------------------------------------------------------
// Favourites
// -------------------------------------------------------------------------------------
bool PresetManager::isFavourite (int index) const
{
    return index >= 0 && index < getNumPresets() && favourites.contains (presets[static_cast<size_t> (index)].key());
}

void PresetManager::setFavourite (int index, bool shouldBeFavourite)
{
    if (index < 0 || index >= getNumPresets())
        return;
    const auto key = presets[static_cast<size_t> (index)].key();
    if (shouldBeFavourite)
        favourites.addIfNotAlreadyThere (key);
    else
        favourites.removeString (key);
    saveFavourites();
    notify();
}

void PresetManager::loadFavourites()
{
    favourites.clear();
    if (auto xml = juce::XmlDocument::parse (root.getChildFile ("Favourites.xml")))
        for (auto* e : xml->getChildWithTagNameIterator ("Favourite"))
            favourites.addIfNotAlreadyThere (e->getStringAttribute ("key"));
}

void PresetManager::saveFavourites() const
{
    juce::XmlElement xml ("Favourites");
    for (const auto& key : favourites)
        xml.createNewChildElement ("Favourite")->setAttribute ("key", key);
    if (root.createDirectory())
        xml.writeTo (root.getChildFile ("Favourites.xml"));
}

// -------------------------------------------------------------------------------------
// User presets
// -------------------------------------------------------------------------------------
juce::String PresetManager::toXmlString (const Preset& p)
{
    juce::XmlElement xml (kPresetTag);
    xml.setAttribute ("format", kPresetFormat);
    xml.setAttribute ("pluginVersion", ARC_VERSION_STRING);
    xml.setAttribute ("name", p.name);
    xml.setAttribute ("category", p.category);
    xml.setAttribute ("author", p.author);
    xml.setAttribute ("tags", p.tags);
    xml.setAttribute ("description", p.description);
    xml.setAttribute ("seed", juce::String (static_cast<juce::int64> (p.seed)));
    for (const auto& kv : p.values)
    {
        auto* e = xml.createNewChildElement (kParamTag);
        e->setAttribute ("id", kv.first);
        e->setAttribute ("value", kv.second);
    }
    for (size_t n = 0; n < 4; ++n)
        if (p.gestures[n].isNotEmpty())
        {
            auto* e = xml.createNewChildElement (kGestureTag);
            e->setAttribute ("node", static_cast<int> (n));
            e->setAttribute ("data", p.gestures[n]);
        }
    return xml.toString();
}

bool PresetManager::fromXml (const juce::XmlElement& xml, Preset& p)
{
    if (! xml.hasTagName (kPresetTag) || xml.getIntAttribute ("format", 0) < 1)
        return false;
    p.name = xml.getStringAttribute ("name").trim();
    if (p.name.isEmpty())
        return false;
    p.category = xml.getStringAttribute ("category", "USER");
    p.author = xml.getStringAttribute ("author");
    p.tags = xml.getStringAttribute ("tags");
    p.description = xml.getStringAttribute ("description");
    p.seed = static_cast<uint32_t> (xml.getStringAttribute ("seed", "0").getLargeIntValue());
    for (auto* e : xml.getChildWithTagNameIterator (kParamTag.toString()))
    {
        const auto id = e->getStringAttribute ("id");
        const auto value = static_cast<float> (e->getDoubleAttribute ("value", std::numeric_limits<double>::quiet_NaN()));
        if (id.isNotEmpty() && std::isfinite (value))
            p.values[id] = value;
    }
    for (auto* e : xml.getChildWithTagNameIterator (kGestureTag.toString()))
    {
        const int node = e->getIntAttribute ("node", -1);
        if (node >= 0 && node < 4)
            p.gestures[static_cast<size_t> (node)] = e->getStringAttribute ("data");
    }
    return true;
}

bool PresetManager::saveUserPreset (const juce::String& name, const juce::String& category, const juce::String& description)
{
    const auto cleanName = name.trim();
    if (cleanName.isEmpty())
        return false;
    auto p = captureCurrent();
    p.name = cleanName;
    p.category = category.isNotEmpty() ? category : juce::String ("USER");
    p.description = description;
    p.author = juce::SystemStats::getFullUserName();

    const auto dir = getUserPresetDirectory();
    if (! dir.createDirectory())
        return false;
    const auto file = dir.getChildFile (juce::File::createLegalFileName (cleanName) + fileExtension);
    if (! file.replaceWithText (toXmlString (p)))
        return false;

    rescanUserPresets();
    currentIndex = findPreset ("user/" + cleanName);
    currentName = cleanName;
    currentCategory = p.category;
    takeSnapshot (nullptr);
    notify();
    return true;
}

bool PresetManager::deleteUserPreset (int index)
{
    if (index < numFactory || index >= getNumPresets())
        return false;
    const auto key = presets[static_cast<size_t> (index)].key();
    if (! presets[static_cast<size_t> (index)].file.deleteFile())
        return false;
    favourites.removeString (key);
    saveFavourites();
    if (index == currentIndex)
        currentIndex = -1;
    rescanUserPresets();
    notify();
    return true;
}

// -------------------------------------------------------------------------------------
// DAW state
// -------------------------------------------------------------------------------------
juce::ValueTree PresetManager::getState() const
{
    juce::ValueTree t ("PRESET");
    {
        const juce::SpinLock::ScopedLockType lock (publishedLock);
        t.setProperty ("name", publishedName, nullptr);
        t.setProperty ("category", publishedCategory, nullptr);
        if (publishedKey.isNotEmpty())
            t.setProperty ("key", publishedKey, nullptr);
    }
    t.setProperty ("modified", isModified(), nullptr);
    return t;
}

void PresetManager::restoreState (const juce::ValueTree& t)
{
    if (! t.isValid())
        return;
    currentName = t.getProperty ("name", "Init").toString();
    currentCategory = t.getProperty ("category").toString();
    currentIndex = findPreset (t.getProperty ("key").toString());
    // Compare against the stored preset when it still exists; otherwise the restored
    // state is the reference.
    takeSnapshot (currentIndex >= 0 ? &presets[static_cast<size_t> (currentIndex)] : nullptr);
    notify();
}

void PresetManager::notify()
{
    {
        const juce::SpinLock::ScopedLockType lock (publishedLock);
        publishedName = currentName;
        publishedCategory = currentCategory;
        publishedKey = currentIndex >= 0 ? presets[static_cast<size_t> (currentIndex)].key() : juce::String();
    }
    if (onChange)
        onChange();
}

} // namespace arc

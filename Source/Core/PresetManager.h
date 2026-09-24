#pragma once

// Preset library, navigation, favourites and user presets. Message thread only.
//
// A preset is: plain values for every *sound* parameter (performance / setup parameters
// such as MASTER OUTPUT, POLYPHONY, BEND RANGE, MPE, QUALITY and FREEZE are never
// changed by a preset), four optional node gestures and the CHAOS / MOTION seed.
// Parameters missing from a preset take their defaults, so presets written by older
// versions load unchanged when parameters are added. See docs/PARAMETERS.md.

#include <array>
#include <functional>
#include <map>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

class ArcAudioProcessor;

namespace arc
{

class PresetManager
{
public:
    struct Preset
    {
        juce::String name, category, tags, description, author;
        uint32_t seed = 0;
        std::map<juce::String, float> values; // parameter id -> plain value
        std::array<juce::String, 4> gestures; // serialised Gesture ("" = none)
        bool isFactory = true;
        juce::File file; // user presets

        juce::String key() const { return (isFactory ? "factory/" : "user/") + name; }
    };

    static constexpr const char* fileExtension = ".arcpreset";

    explicit PresetManager (ArcAudioProcessor& processor, juce::File rootDirectory = defaultRootDirectory());

    /** ~/Documents/ARC (presets in Presets/, favourites in Favourites.xml). */
    static juce::File defaultRootDirectory();
    /** True for parameters stored in presets (sound), false for performance / setup ones. */
    static bool isPresetParameter (const juce::String& parameterId);

    // --- library ------------------------------------------------------------------------
    int getNumPresets() const noexcept { return static_cast<int> (presets.size()); }
    const Preset& getPreset (int index) const { return presets[static_cast<size_t> (index)]; }
    int findPreset (const juce::String& key) const;
    void rescanUserPresets();
    juce::File getUserPresetDirectory() const { return root.getChildFile ("Presets"); }

    // --- current preset -----------------------------------------------------------------
    int getCurrentIndex() const noexcept { return currentIndex; }
    juce::String getCurrentName() const { return currentName; }
    juce::String getCurrentCategory() const { return currentCategory; }
    /** Any sound parameter or gesture differs from the loaded preset. */
    bool isModified() const;

    void loadPreset (int index);
    void loadNext();
    void loadPrevious();
    /** Default sound ("Init"). */
    void loadInit();

    /** Snapshot of the processor's current sound. */
    Preset captureCurrent() const;
    /** Applies values, gestures and seed to the processor. */
    void applyPreset (const Preset& preset);

    // --- RANDOM ------------------------------------------------------------------------
    /** Gentle mutation of the current sound (full = regenerate within musical bounds). */
    void randomise (bool full, juce::Random& rng);
    void randomise (bool full);

    // --- favourites ---------------------------------------------------------------------
    bool isFavourite (int index) const;
    void setFavourite (int index, bool shouldBeFavourite);

    // --- user presets -------------------------------------------------------------------
    /** Saves the current sound; returns false on I/O failure. Overwrites a user preset of
        the same name. The saved preset becomes current. */
    bool saveUserPreset (const juce::String& name, const juce::String& category, const juce::String& description = {});
    bool deleteUserPreset (int index);

    static juce::String toXmlString (const Preset& p);
    static bool fromXml (const juce::XmlElement& xml, Preset& p);

    // --- DAW state ----------------------------------------------------------------------
    /** Current preset metadata. Safe from any non-audio thread (hosts save state from
        worker threads). */
    juce::ValueTree getState() const;
    void restoreState (const juce::ValueTree& state);

    /** Called (message thread) whenever the current preset or its name changes. */
    std::function<void()> onChange;

private:
    void loadFactoryPresets();
    void loadFavourites();
    void saveFavourites() const;
    void takeSnapshot (const Preset* from);
    float plainValueFor (const Preset& p, juce::RangedAudioParameter& param) const;
    void notify();

    ArcAudioProcessor& processor;
    juce::File root;
    std::vector<Preset> presets;
    int numFactory = 0;
    int currentIndex = -1;
    juce::String currentName { "Init" }, currentCategory;
    juce::StringArray favourites;

    // Published copy of the current preset's identity for getState() off the message thread.
    mutable juce::SpinLock publishedLock;
    juce::String publishedName { "Init" }, publishedCategory, publishedKey;

    std::vector<juce::RangedAudioParameter*> presetParams;
    std::vector<float> snapshot; // normalised values of presetParams when loaded
    std::array<juce::String, 4> snapshotGestures;
};

} // namespace arc

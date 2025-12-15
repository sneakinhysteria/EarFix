/*
  ==============================================================================

    HeadphoneEQ.h
    Headphone frequency response correction using AutoEq data

    Loads headphone EQ profiles from external JSON files, allowing users to
    update the database without rebuilding the plugin.

    Data location: ~/Library/Application Support/EarFix/headphones/

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
struct HeadphoneFilter
{
    juce::String type;  // "PK" (peak), "LSC" (low shelf), "HSC" (high shelf)
    float frequency = 1000.0f;
    float gain = 0.0f;
    float q = 1.0f;
};

//==============================================================================
struct HeadphoneProfile
{
    juce::String name;
    juce::String source;
    juce::String type;  // "over-ear", "in-ear", "earbud"
    float preamp = 0.0f;
    std::vector<HeadphoneFilter> filters;

    bool isValid() const { return name.isNotEmpty() && !filters.empty(); }
};

//==============================================================================
struct HeadphoneIndexEntry
{
    juce::String name;
    juce::String filename;
    juce::String type;
    juce::String source;
};

//==============================================================================
class HeadphoneEQ
{
public:
    HeadphoneEQ();
    ~HeadphoneEQ() = default;

    //==========================================================================
    // Database management

    /** Scans the headphones directory and loads the index. */
    void loadDatabase();

    /** Returns the path to the headphones data directory. */
    static juce::File getHeadphonesDirectory();

    /** Returns list of available headphone profiles. */
    const std::vector<HeadphoneIndexEntry>& getAvailableHeadphones() const { return availableHeadphones; }

    /** Returns the database version string. */
    juce::String getDatabaseVersion() const { return databaseVersion; }

    /** Returns the number of available headphones. */
    int getNumHeadphones() const { return static_cast<int> (availableHeadphones.size()); }

    //==========================================================================
    // Profile selection

    /** Loads a headphone profile by name. Returns true if successful. */
    bool loadProfile (const juce::String& headphoneName);

    /** Clears the current profile (no headphone correction). */
    void clearProfile();

    /** Returns the currently loaded profile name, or empty if none. */
    juce::String getCurrentProfileName() const { return currentProfile.name; }

    /** Returns true if a profile is currently loaded. */
    bool hasProfile() const { return currentProfile.isValid(); }

    //==========================================================================
    // Audio processing

    /** Prepares the EQ for playback. */
    void prepare (double sampleRate, int samplesPerBlock);

    /** Resets the filter states. */
    void reset();

    /** Processes a stereo audio buffer. */
    void process (juce::AudioBuffer<float>& buffer);

    /** Sets whether headphone EQ is enabled. */
    void setEnabled (bool shouldBeEnabled) { enabled = shouldBeEnabled; }

    /** Returns true if headphone EQ is enabled. */
    bool isEnabled() const { return enabled; }

private:
    //==========================================================================
    // JSON parsing

    HeadphoneProfile parseProfileJSON (const juce::File& jsonFile);
    void parseIndexJSON (const juce::File& indexFile);

    //==========================================================================
    // Filter management

    void updateFilterCoefficients();
    juce::dsp::IIR::Coefficients<float>::Ptr createFilterCoefficients (const HeadphoneFilter& filter);

    //==========================================================================
    // Data

    std::vector<HeadphoneIndexEntry> availableHeadphones;
    juce::String databaseVersion;
    HeadphoneProfile currentProfile;

    // Processing state
    bool enabled = false;
    double currentSampleRate = 44100.0;

    // Up to 10 filter bands (typical AutoEq output)
    static constexpr int maxFilters = 10;
    std::array<juce::dsp::IIR::Filter<float>, maxFilters> leftFilters;
    std::array<juce::dsp::IIR::Filter<float>, maxFilters> rightFilters;
    int activeFilterCount = 0;
    float preampGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeadphoneEQ)
};

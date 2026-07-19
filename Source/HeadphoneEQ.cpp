/*
  ==============================================================================

    HeadphoneEQ.cpp
    Headphone frequency response correction using AutoEq data

  ==============================================================================
*/

#include "HeadphoneEQ.h"

//==============================================================================
HeadphoneEQ::HeadphoneEQ()
{
    loadDatabase();
}

//==============================================================================
juce::File HeadphoneEQ::getHeadphonesDirectory()
{
#if JUCE_MAC
    // userApplicationDataDirectory on macOS is ~/Library, so we need to add Application Support
    auto library = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    return library.getChildFile ("Application Support").getChildFile ("EarFix").getChildFile ("headphones");
#elif JUCE_WINDOWS
    auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    return appData.getChildFile ("EarFix").getChildFile ("headphones");
#else
    auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    return home.getChildFile (".config").getChildFile ("EarFix").getChildFile ("headphones");
#endif
}

//==============================================================================
void HeadphoneEQ::loadDatabase()
{
    availableHeadphones.clear();
    databaseVersion = "local";

    auto dir = getHeadphonesDirectory();
    if (! dir.exists())
    {
        dir.createDirectory();
        return;
    }

    // Directory is the source of truth: scan every profile JSON and read its
    // metadata. Profiles added by paste-import (or dropped in manually) therefore
    // appear automatically with no index to maintain.
    for (const auto& file : dir.findChildFiles (juce::File::findFiles, false, "*.json"))
    {
        if (file.getFileName() == "index.json")
            continue;

        auto json = juce::JSON::parse (file.loadFileAsString());
        auto* obj = json.getDynamicObject();
        if (obj == nullptr)
            continue;

        HeadphoneIndexEntry entry;
        entry.name     = obj->getProperty ("name").toString();
        if (entry.name.isEmpty())
            entry.name = file.getFileNameWithoutExtension();
        entry.filename = file.getFileName();
        entry.type     = obj->getProperty ("type").toString();
        entry.source   = obj->getProperty ("source").toString();
        availableHeadphones.push_back (entry);
    }

    std::sort (availableHeadphones.begin(), availableHeadphones.end(),
               [] (const HeadphoneIndexEntry& a, const HeadphoneIndexEntry& b)
               { return a.name.compareIgnoreCase (b.name) < 0; });

    DBG ("HeadphoneEQ: Loaded " + juce::String (availableHeadphones.size()) + " headphone profiles");
}

//==============================================================================
void HeadphoneEQ::parseIndexJSON (const juce::File& indexFile)
{
    auto jsonText = indexFile.loadFileAsString();
    auto json = juce::JSON::parse (jsonText);

    if (json.isVoid())
    {
        DBG ("HeadphoneEQ: Failed to parse index.json");
        return;
    }

    if (auto* obj = json.getDynamicObject())
    {
        databaseVersion = obj->getProperty ("version").toString();

        if (auto* headphonesArray = obj->getProperty ("headphones").getArray())
        {
            for (const auto& item : *headphonesArray)
            {
                if (auto* hpObj = item.getDynamicObject())
                {
                    HeadphoneIndexEntry entry;
                    entry.name = hpObj->getProperty ("name").toString();
                    entry.filename = hpObj->getProperty ("file").toString();
                    entry.type = hpObj->getProperty ("type").toString();
                    entry.source = hpObj->getProperty ("source").toString();

                    if (entry.name.isNotEmpty() && entry.filename.isNotEmpty())
                        availableHeadphones.push_back (entry);
                }
            }
        }
    }
}

//==============================================================================
bool HeadphoneEQ::loadProfile (const juce::String& headphoneName)
{
    if (headphoneName.isEmpty())
    {
        clearProfile();
        return true;
    }

    // Find the headphone in the index
    juce::String filename;
    for (const auto& entry : availableHeadphones)
    {
        if (entry.name == headphoneName)
        {
            filename = entry.filename;
            break;
        }
    }

    if (filename.isEmpty())
    {
        DBG ("HeadphoneEQ: Headphone not found in database: " + headphoneName);
        return false;
    }

    auto dir = getHeadphonesDirectory();
    auto profileFile = dir.getChildFile (filename);

    if (!profileFile.exists())
    {
        DBG ("HeadphoneEQ: Profile file not found: " + profileFile.getFullPathName());
        return false;
    }

    currentProfile = parseProfileJSON (profileFile);

    if (!currentProfile.isValid())
    {
        DBG ("HeadphoneEQ: Failed to parse profile: " + headphoneName);
        return false;
    }

    updateFilterCoefficients();
    DBG ("HeadphoneEQ: Loaded profile: " + currentProfile.name + " with " +
         juce::String (currentProfile.filters.size()) + " filters");

    return true;
}

//==============================================================================
void HeadphoneEQ::clearProfile()
{
    currentProfile = HeadphoneProfile();
    activeFilterCount = 0;
    preampGain = 1.0f;
}

//==============================================================================
HeadphoneProfile HeadphoneEQ::parseParametricEQText (const juce::String& name,
                                                     const juce::String& text)
{
    HeadphoneProfile profile;
    profile.name   = name.trim();
    profile.source = "custom (imported)";
    profile.type   = "custom";

    auto lines = juce::StringArray::fromLines (text);
    for (auto line : lines)
    {
        line = line.trim();
        if (line.isEmpty())
            continue;

        // Preamp:  "Preamp: -6.0 dB"
        if (line.startsWithIgnoreCase ("Preamp"))
        {
            auto after = line.fromFirstOccurrenceOf (":", false, true).upToFirstOccurrenceOf ("dB", false, true);
            profile.preamp = after.trim().getFloatValue();
            continue;
        }

        // Filter line: "Filter 1: ON PK Fc 105 Hz Gain -2.7 dB Q 0.70"
        // (the leading "Filter N:" and the "ON" token are optional across sources)
        if (! line.containsIgnoreCase ("Fc"))
            continue;

        auto tokens = juce::StringArray::fromTokens (line, " \t", "");
        tokens.removeEmptyStrings();

        auto valueAfter = [&tokens] (const juce::String& key) -> juce::String
        {
            for (int i = 0; i < tokens.size() - 1; ++i)
                if (tokens[i].equalsIgnoreCase (key))
                    return tokens[i + 1];
            return {};
        };

        // Filter type = the token immediately before "Fc"
        juce::String type;
        int fcIndex = -1;
        for (int i = 0; i < tokens.size(); ++i)
            if (tokens[i].equalsIgnoreCase ("Fc")) { fcIndex = i; break; }
        if (fcIndex > 0)
            type = tokens[fcIndex - 1].toUpperCase();

        static const juce::StringArray known { "PK", "LSC", "LS", "HSC", "HS", "LP", "HP" };
        if (! known.contains (type))
            continue;

        HeadphoneFilter f;
        f.type      = type;
        f.frequency = valueAfter ("Fc").getFloatValue();
        f.gain      = valueAfter ("Gain").getFloatValue();          // 0 for LP/HP (no Gain token)
        f.q         = valueAfter ("Q").getFloatValue();
        if (f.q <= 0.0f)
            f.q = 0.707f;

        if (f.frequency > 0.0f)
            profile.filters.push_back (f);
    }

    return profile;
}

//==============================================================================
juce::String HeadphoneEQ::importParametricEQText (const juce::String& name, const juce::String& text)
{
    auto profile = parseParametricEQText (name, text);
    if (! profile.isValid())     // needs a name and at least one filter
        return {};

    auto dir = getHeadphonesDirectory();
    dir.createDirectory();

    // Sanitise the filename
    juce::String safe = profile.name;
    for (auto c : juce::String ("<>:\"/\\|?*"))
        safe = safe.replaceCharacter (c, '_');

    auto* root = new juce::DynamicObject();
    root->setProperty ("name",   profile.name);
    root->setProperty ("source", profile.source);
    root->setProperty ("type",   profile.type);
    root->setProperty ("custom", true);
    root->setProperty ("preamp", profile.preamp);

    juce::Array<juce::var> filterArray;
    for (const auto& f : profile.filters)
    {
        auto* fo = new juce::DynamicObject();
        fo->setProperty ("type", f.type);
        fo->setProperty ("freq", f.frequency);
        fo->setProperty ("gain", f.gain);
        fo->setProperty ("q",    f.q);
        filterArray.add (juce::var (fo));
    }
    root->setProperty ("filters", filterArray);

    auto file = dir.getChildFile (safe + ".json");
    if (! file.replaceWithText (juce::JSON::toString (juce::var (root))))
        return {};

    return profile.name;
}

//==============================================================================
HeadphoneProfile HeadphoneEQ::parseProfileJSON (const juce::File& jsonFile)
{
    HeadphoneProfile profile;

    auto jsonText = jsonFile.loadFileAsString();
    auto json = juce::JSON::parse (jsonText);

    if (json.isVoid())
        return profile;

    if (auto* obj = json.getDynamicObject())
    {
        profile.name = obj->getProperty ("name").toString();
        profile.source = obj->getProperty ("source").toString();
        profile.type = obj->getProperty ("type").toString();
        profile.preamp = static_cast<float> (obj->getProperty ("preamp"));

        if (auto* filtersArray = obj->getProperty ("filters").getArray())
        {
            for (const auto& item : *filtersArray)
            {
                if (auto* filterObj = item.getDynamicObject())
                {
                    HeadphoneFilter filter;
                    filter.type = filterObj->getProperty ("type").toString();
                    filter.frequency = static_cast<float> (filterObj->getProperty ("freq"));
                    filter.gain = static_cast<float> (filterObj->getProperty ("gain"));
                    filter.q = static_cast<float> (filterObj->getProperty ("q"));

                    if (filter.frequency > 0.0f && filter.q > 0.0f)
                        profile.filters.push_back (filter);
                }
            }
        }
    }

    return profile;
}

//==============================================================================
void HeadphoneEQ::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;

    for (int i = 0; i < maxFilters; ++i)
    {
        leftFilters[i].prepare (spec);
        rightFilters[i].prepare (spec);
    }

    if (currentProfile.isValid())
        updateFilterCoefficients();
}

//==============================================================================
void HeadphoneEQ::reset()
{
    for (int i = 0; i < maxFilters; ++i)
    {
        leftFilters[i].reset();
        rightFilters[i].reset();
    }
}

//==============================================================================
void HeadphoneEQ::updateFilterCoefficients()
{
    activeFilterCount = 0;
    preampGain = juce::Decibels::decibelsToGain (currentProfile.preamp);

    for (size_t i = 0; i < currentProfile.filters.size() && activeFilterCount < maxFilters; ++i)
    {
        const auto& filter = currentProfile.filters[i];

        // Skip filters above Nyquist
        if (filter.frequency >= currentSampleRate * 0.45f)
            continue;

        auto coeffs = createFilterCoefficients (filter);
        if (coeffs != nullptr)
        {
            leftFilters[activeFilterCount].coefficients = coeffs;
            rightFilters[activeFilterCount].coefficients = coeffs;
            ++activeFilterCount;
        }
    }

    DBG ("HeadphoneEQ: Updated " + juce::String (activeFilterCount) + " filters, preamp: " +
         juce::String (currentProfile.preamp, 1) + " dB");
}

//==============================================================================
juce::dsp::IIR::Coefficients<float>::Ptr HeadphoneEQ::createFilterCoefficients (const HeadphoneFilter& filter)
{
    float gain = juce::Decibels::decibelsToGain (filter.gain);

    if (filter.type == "PK")
    {
        // Peak/parametric filter
        return juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, filter.frequency, filter.q, gain);
    }
    else if (filter.type == "LSC" || filter.type == "LS")
    {
        // Low shelf filter
        return juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            currentSampleRate, filter.frequency, filter.q, gain);
    }
    else if (filter.type == "HSC" || filter.type == "HS")
    {
        // High shelf filter
        return juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            currentSampleRate, filter.frequency, filter.q, gain);
    }
    else if (filter.type == "LP")
    {
        // Low pass filter (gain ignored)
        return juce::dsp::IIR::Coefficients<float>::makeLowPass (
            currentSampleRate, filter.frequency, filter.q);
    }
    else if (filter.type == "HP")
    {
        // High pass filter (gain ignored)
        return juce::dsp::IIR::Coefficients<float>::makeHighPass (
            currentSampleRate, filter.frequency, filter.q);
    }

    DBG ("HeadphoneEQ: Unknown filter type: " + filter.type);
    return nullptr;
}

//==============================================================================
void HeadphoneEQ::process (juce::AudioBuffer<float>& buffer)
{
    if (!enabled || !currentProfile.isValid() || activeFilterCount == 0)
        return;

    const int numSamples = buffer.getNumSamples();

    // Apply preamp
    if (std::abs (preampGain - 1.0f) > 0.001f)
        buffer.applyGain (preampGain);

    if (buffer.getNumChannels() >= 2)
    {
        auto* leftChannel = buffer.getWritePointer (0);
        auto* rightChannel = buffer.getWritePointer (1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float leftSample = leftChannel[sample];
            float rightSample = rightChannel[sample];

            for (int i = 0; i < activeFilterCount; ++i)
            {
                leftSample = leftFilters[i].processSample (leftSample);
                rightSample = rightFilters[i].processSample (rightSample);
            }

            leftChannel[sample] = leftSample;
            rightChannel[sample] = rightSample;
        }
    }
    else if (buffer.getNumChannels() >= 1)
    {
        auto* channel = buffer.getWritePointer (0);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float sampleVal = channel[sample];

            for (int i = 0; i < activeFilterCount; ++i)
                sampleVal = leftFilters[i].processSample (sampleVal);

            channel[sample] = sampleVal;
        }
    }
}

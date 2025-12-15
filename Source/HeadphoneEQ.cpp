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
    databaseVersion = "No database";

    auto dir = getHeadphonesDirectory();
    if (!dir.exists())
    {
        DBG ("HeadphoneEQ: Database directory does not exist: " + dir.getFullPathName());
        return;
    }

    auto indexFile = dir.getChildFile ("index.json");
    if (indexFile.exists())
    {
        parseIndexJSON (indexFile);
    }
    else
    {
        // Fallback: scan directory for JSON files
        DBG ("HeadphoneEQ: No index.json found, scanning directory...");
        for (const auto& file : dir.findChildFiles (juce::File::findFiles, false, "*.json"))
        {
            if (file.getFileName() != "index.json")
            {
                HeadphoneIndexEntry entry;
                entry.name = file.getFileNameWithoutExtension();
                entry.filename = file.getFileName();
                entry.type = "unknown";
                entry.source = "unknown";
                availableHeadphones.push_back (entry);
            }
        }
        databaseVersion = "Scanned";
    }

    DBG ("HeadphoneEQ: Loaded database with " + juce::String (availableHeadphones.size()) + " headphones");
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

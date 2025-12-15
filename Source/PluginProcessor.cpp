/*
  ==============================================================================

    Hearing Correction AU v2 - Alpha
    Per-ear audiogram-driven EQ correction plugin

    JUCE 8 native implementation with pluggable correction models

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Sortable parameter ID suffixes (fully numeric, zero-padded for correct sort)
static const std::array<juce::String, 6> rightParamSuffixes = {
    "01", "02", "03", "04", "05", "06"
};
static const std::array<juce::String, 6> leftParamSuffixes = {
    "07", "08", "09", "10", "11", "12"
};

// Display names (numeric prefix forces sort order)
static const std::array<juce::String, 6> rightFreqNames = {
    "01 R 250", "02 R 500", "03 R 1k", "04 R 2k", "05 R 4k", "06 R 8k"
};
static const std::array<juce::String, 6> leftFreqNames = {
    "07 L 250", "08 L 500", "09 L 1k", "10 L 2k", "11 L 4k", "12 L 8k"
};

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
HearingCorrectionAUv2AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Bypass
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false));

    // Model selection: 0 = Half-Gain, 1 = NAL, 2 = MOSL (Music)
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "modelSelect", 1 },
        "Model",
        juce::StringArray { "Half-Gain", "NAL (Speech)", "MOSL (Music)" },
        2));  // Default to MOSL for music-focused use

    // Output gain: -24 to +24 dB
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // Correction strength: 0% to 100%
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "correctionStrength", 1 },
        "Correction",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // Max boost: limits per-band gain to prevent distortion with severe losses
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "maxBoost", 1 },
        "Max Boost",
        juce::NormalisableRange<float> (10.0f, 40.0f, 1.0f),
        25.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // Compression speed: 0 = Fast, 1 = Slow (only used by NAL model)
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "compressionSpeed", 1 },
        "Compression",
        juce::StringArray { "Fast", "Slow" },
        0));

    // Experience level: NAL-NL2 reduces gain for new users (0 = New, 1 = Experienced)
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "experienceLevel", 1 },
        "Experience",
        juce::StringArray { "New User", "Some Experience", "Experienced" },
        2));  // Default to Experienced

    // Right ear enable (R before L - audiological convention)
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "rightEnable", 1 },
        "Right Enable",
        true));

    // Left ear enable
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "leftEnable", 1 },
        "Left Enable",
        true));

    // Headphone EQ enable
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "headphoneEQEnable", 1 },
        "Headphone EQ",
        false));

    // Audiogram values per ear (-20 to 120 dB HL, standard audiometric range)
    // Right ear first (audiological convention)
    // Version 4: simplified numeric IDs for correct host Controls view ordering
    for (int i = 0; i < 6; ++i)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "audiogram_" + rightParamSuffixes[i], 4 },
            rightFreqNames[i],
            juce::NormalisableRange<float> (-20.0f, 120.0f, 5.0f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB HL")));
    }

    // Left ear
    for (int i = 0; i < 6; ++i)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "audiogram_" + leftParamSuffixes[i], 4 },
            leftFreqNames[i],
            juce::NormalisableRange<float> (-20.0f, 120.0f, 5.0f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB HL")));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
HearingCorrectionAUv2AudioProcessor::HearingCorrectionAUv2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    bypassParam             = parameters.getRawParameterValue ("bypass");
    modelSelectParam        = parameters.getRawParameterValue ("modelSelect");
    outputGainParam         = parameters.getRawParameterValue ("outputGain");
    correctionStrengthParam = parameters.getRawParameterValue ("correctionStrength");
    maxBoostParam           = parameters.getRawParameterValue ("maxBoost");
    compressionSpeedParam   = parameters.getRawParameterValue ("compressionSpeed");
    experienceLevelParam    = parameters.getRawParameterValue ("experienceLevel");
    leftEnableParam         = parameters.getRawParameterValue ("leftEnable");
    rightEnableParam        = parameters.getRawParameterValue ("rightEnable");
    headphoneEQEnableParam  = parameters.getRawParameterValue ("headphoneEQEnable");

    for (int i = 0; i < numAudiogramBands; ++i)
    {
        rightAudiogramParams[i] = parameters.getRawParameterValue ("audiogram_" + rightParamSuffixes[i]);
        leftAudiogramParams[i]  = parameters.getRawParameterValue ("audiogram_" + leftParamSuffixes[i]);
    }
}

HearingCorrectionAUv2AudioProcessor::~HearingCorrectionAUv2AudioProcessor() = default;

//==============================================================================
void HearingCorrectionAUv2AudioProcessor::updateCurrentModel()
{
    int modelIndex = static_cast<int> (modelSelectParam->load());

    switch (modelIndex)
    {
        case 0:  currentModel = &halfGainModel; break;
        case 1:  currentModel = &nalModel; break;
        case 2:  currentModel = &moslModel; break;
        default: currentModel = &moslModel; break;
    }

    // Update model-specific settings
    float strength = correctionStrengthParam->load() / 100.0f;
    currentModel->setOverallGainOffset ((strength - 0.5f) * 10.0f);  // -5 to +5 dB based on strength

    // NAL-specific settings
    if (modelIndex == 1)
    {
        bool fastCompression = compressionSpeedParam->load() < 0.5f;
        nalModel.setCompressionSpeed (fastCompression);

        int experienceLevel = static_cast<int> (experienceLevelParam->load());
        nalModel.setExperienceLevel (experienceLevel);
    }

    // MOSL-specific settings
    if (modelIndex == 2)
    {
        bool fastCompression = compressionSpeedParam->load() < 0.5f;
        moslModel.setCompressionSpeed (fastCompression);

        // Use experience level to control brightness boost for MOSL
        // New users might prefer less brightness, experienced users more
        int experienceLevel = static_cast<int> (experienceLevelParam->load());
        moslModel.setBrightnessBoost (experienceLevel >= 1);  // Enable for experienced users
        moslModel.setBassEmphasis (experienceLevel);          // More bass for experienced
    }
}

//==============================================================================
const juce::String HearingCorrectionAUv2AudioProcessor::getName() const { return JucePlugin_Name; }
bool HearingCorrectionAUv2AudioProcessor::acceptsMidi() const { return false; }
bool HearingCorrectionAUv2AudioProcessor::producesMidi() const { return false; }
bool HearingCorrectionAUv2AudioProcessor::isMidiEffect() const { return false; }
double HearingCorrectionAUv2AudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HearingCorrectionAUv2AudioProcessor::getNumPrograms()    { return 1; }
int HearingCorrectionAUv2AudioProcessor::getCurrentProgram() { return 0; }
void HearingCorrectionAUv2AudioProcessor::setCurrentProgram (int) {}
const juce::String HearingCorrectionAUv2AudioProcessor::getProgramName (int) { return {}; }
void HearingCorrectionAUv2AudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void HearingCorrectionAUv2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare headphone EQ
    headphoneEQ.prepare (sampleRate, samplesPerBlock);

    previousGain = juce::Decibels::decibelsToGain (outputGainParam->load());

    // Prepare filter spec for mono processing
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    // Prepare Linkwitz-Riley crossover filters (5 crossovers for 6 bands)
    for (int i = 0; i < numCrossovers; ++i)
    {
        leftLowpass[i].prepare (spec);
        leftHighpass[i].prepare (spec);
        rightLowpass[i].prepare (spec);
        rightHighpass[i].prepare (spec);

        leftLowpass[i].reset();
        leftHighpass[i].reset();
        rightLowpass[i].reset();
        rightHighpass[i].reset();
    }

    // Reset WDRC state for all bands
    for (int i = 0; i < numAudiogramBands; ++i)
    {
        leftWDRC[i].envelope = 0.0f;
        leftWDRC[i].smoothedGain = 1.0f;
        rightWDRC[i].envelope = 0.0f;
        rightWDRC[i].smoothedGain = 1.0f;
    }

    updateWDRCCoefficients();
    updateCrossoverCoefficients();
    updateCurrentModel();
}

void HearingCorrectionAUv2AudioProcessor::releaseResources() {}

void HearingCorrectionAUv2AudioProcessor::updateCrossoverCoefficients()
{
    // Set up Linkwitz-Riley crossover filters at each crossover frequency
    for (int i = 0; i < numCrossovers; ++i)
    {
        float freq = crossoverFrequencies[i];

        // Skip if frequency is too high for current sample rate
        if (freq >= currentSampleRate * 0.45f)
            freq = static_cast<float> (currentSampleRate * 0.44f);

        leftLowpass[i].setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        leftLowpass[i].setCutoffFrequency (freq);

        leftHighpass[i].setType (juce::dsp::LinkwitzRileyFilterType::highpass);
        leftHighpass[i].setCutoffFrequency (freq);

        rightLowpass[i].setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        rightLowpass[i].setCutoffFrequency (freq);

        rightHighpass[i].setType (juce::dsp::LinkwitzRileyFilterType::highpass);
        rightHighpass[i].setCutoffFrequency (freq);
    }
}

void HearingCorrectionAUv2AudioProcessor::updateWDRCCoefficients()
{
    bool fastCompression = compressionSpeedParam->load() < 0.5f;

    // Attack/release times for envelope follower
    float attackMs  = fastCompression ? 5.0f : 10.0f;
    float releaseMs = fastCompression ? 50.0f : 150.0f;

    attackCoeff  = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * attackMs / 1000.0f));
    releaseCoeff = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * releaseMs / 1000.0f));

    // Gain smoothing (10ms time constant)
    gainSmoothCoeff = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * 0.01f));

    // Update target gains for each band based on hearing loss
    const float strength = correctionStrengthParam->load() / 100.0f;
    const float maxBoost = maxBoostParam->load();

    for (int i = 0; i < numAudiogramBands; ++i)
    {
        const float freq = audiogramFrequencies[i];
        const float leftLoss  = std::max (0.0f, leftAudiogramParams[i]->load());
        const float rightLoss = std::max (0.0f, rightAudiogramParams[i]->load());

        // Calculate target gain for soft sounds (full correction)
        float leftGain  = currentModel->calculateGain (freq, leftLoss) * strength;
        float rightGain = currentModel->calculateGain (freq, rightLoss) * strength;

        // Cap to maxBoost
        leftWDRC[i].targetGainForSoftSounds = std::min (leftGain, maxBoost);
        rightWDRC[i].targetGainForSoftSounds = std::min (rightGain, maxBoost);
    }
}

float HearingCorrectionAUv2AudioProcessor::calculateWDRCGain (float inputLevelDb,
                                                               float targetGainDb,
                                                               float maxBoostDb) const
{
    // WDRC: Wide Dynamic Range Compression
    // Soft sounds get full gain, loud sounds get reduced gain

    // Kneepoint: below this input level, apply full target gain
    const float kneepoint = -40.0f;  // dB (relative to 0dBFS)

    // Above kneepoint, compression kicks in
    // Compression ratio increases with target gain (more correction = more compression)
    float compressionRatio = 1.0f + (targetGainDb / 30.0f);
    compressionRatio = juce::jlimit (1.5f, 4.0f, compressionRatio);

    if (inputLevelDb <= kneepoint)
    {
        // Below kneepoint: full target gain
        return targetGainDb;
    }
    else
    {
        // Above kneepoint: compress
        float overKnee = inputLevelDb - kneepoint;
        float compressedOver = overKnee / compressionRatio;
        float gainReduction = overKnee - compressedOver;

        // Reduce target gain based on how much above kneepoint
        float gain = targetGainDb - gainReduction;

        // Never go below 0 dB gain (no attenuation in correction bands)
        return std::max (0.0f, gain);
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HearingCorrectionAUv2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void HearingCorrectionAUv2AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                         juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto numSamples = buffer.getNumSamples();

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    // Measure input levels
    if (buffer.getNumChannels() >= 2)
    {
        inputLevelLeft.store (buffer.getMagnitude (0, 0, numSamples), std::memory_order_relaxed);
        inputLevelRight.store (buffer.getMagnitude (1, 0, numSamples), std::memory_order_relaxed);
    }

    if (bypassParam->load() > 0.5f)
    {
        outputLevelLeft.store (inputLevelLeft.load (std::memory_order_relaxed), std::memory_order_relaxed);
        outputLevelRight.store (inputLevelRight.load (std::memory_order_relaxed), std::memory_order_relaxed);
        return;
    }

    // Apply headphone EQ correction (flattens headphone response before hearing correction)
    bool headphoneEQEnabled = headphoneEQEnableParam->load() > 0.5f;
    headphoneEQ.setEnabled (headphoneEQEnabled);
    headphoneEQ.process (buffer);

    // Update model and WDRC parameters
    updateCurrentModel();
    updateWDRCCoefficients();

    const bool leftEnabled  = leftEnableParam->load() > 0.5f;
    const bool rightEnabled = rightEnableParam->load() > 0.5f;
    const float maxBoost = maxBoostParam->load();

    if (buffer.getNumChannels() >= 2)
    {
        auto* leftChannel  = buffer.getWritePointer (0);
        auto* rightChannel = buffer.getWritePointer (1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float leftIn  = leftChannel[sample];
            float rightIn = rightChannel[sample];
            float leftOut = 0.0f;
            float rightOut = 0.0f;

            // Process through multiband crossover with WDRC
            // Signal flow: Input -> Split into bands -> WDRC each band -> Sum

            float leftRemaining = leftIn;
            float rightRemaining = rightIn;

            for (int band = 0; band < numAudiogramBands; ++band)
            {
                float leftBand, rightBand;

                if (band < numCrossovers)
                {
                    // Extract this band using lowpass, pass remainder through highpass
                    leftBand = leftLowpass[band].processSample (0, leftRemaining);
                    leftRemaining = leftHighpass[band].processSample (0, leftRemaining);

                    rightBand = rightLowpass[band].processSample (0, rightRemaining);
                    rightRemaining = rightHighpass[band].processSample (0, rightRemaining);
                }
                else
                {
                    // Last band gets the remainder (highpass only)
                    leftBand = leftRemaining;
                    rightBand = rightRemaining;
                }

                // Apply WDRC to this band if enabled
                if (leftEnabled && leftWDRC[band].targetGainForSoftSounds > 0.0f)
                {
                    // Envelope follower for this band
                    float inputLevel = std::abs (leftBand);
                    float& env = leftWDRC[band].envelope;
                    float coeff = (inputLevel > env) ? attackCoeff : releaseCoeff;
                    env = env * coeff + inputLevel * (1.0f - coeff);

                    // Calculate input level in dB
                    float inputDb = juce::Decibels::gainToDecibels (env + 1e-6f);

                    // Calculate WDRC gain based on input level
                    float targetGainDb = calculateWDRCGain (inputDb,
                                                            leftWDRC[band].targetGainForSoftSounds,
                                                            maxBoost);

                    // Smooth gain changes
                    float targetGainLinear = juce::Decibels::decibelsToGain (targetGainDb);
                    leftWDRC[band].smoothedGain = leftWDRC[band].smoothedGain * gainSmoothCoeff
                                                  + targetGainLinear * (1.0f - gainSmoothCoeff);

                    leftBand *= leftWDRC[band].smoothedGain;
                }

                if (rightEnabled && rightWDRC[band].targetGainForSoftSounds > 0.0f)
                {
                    // Envelope follower for this band
                    float inputLevel = std::abs (rightBand);
                    float& env = rightWDRC[band].envelope;
                    float coeff = (inputLevel > env) ? attackCoeff : releaseCoeff;
                    env = env * coeff + inputLevel * (1.0f - coeff);

                    // Calculate input level in dB
                    float inputDb = juce::Decibels::gainToDecibels (env + 1e-6f);

                    // Calculate WDRC gain based on input level
                    float targetGainDb = calculateWDRCGain (inputDb,
                                                            rightWDRC[band].targetGainForSoftSounds,
                                                            maxBoost);

                    // Smooth gain changes
                    float targetGainLinear = juce::Decibels::decibelsToGain (targetGainDb);
                    rightWDRC[band].smoothedGain = rightWDRC[band].smoothedGain * gainSmoothCoeff
                                                   + targetGainLinear * (1.0f - gainSmoothCoeff);

                    rightBand *= rightWDRC[band].smoothedGain;
                }

                // Sum this band to output
                leftOut += leftBand;
                rightOut += rightBand;
            }

            // If ear is disabled, pass through original signal
            leftChannel[sample]  = leftEnabled ? leftOut : leftIn;
            rightChannel[sample] = rightEnabled ? rightOut : rightIn;
        }
    }

    // Output gain with smoothing
    const float targetGain = juce::Decibels::decibelsToGain (outputGainParam->load());

    if (std::abs (targetGain - previousGain) > 0.0001f)
    {
        buffer.applyGainRamp (0, numSamples, previousGain, targetGain);
        previousGain = targetGain;
    }
    else
    {
        buffer.applyGain (targetGain);
    }

    // Measure output levels
    if (buffer.getNumChannels() >= 2)
    {
        outputLevelLeft.store (buffer.getMagnitude (0, 0, numSamples), std::memory_order_relaxed);
        outputLevelRight.store (buffer.getMagnitude (1, 0, numSamples), std::memory_order_relaxed);
    }
}

//==============================================================================
bool HearingCorrectionAUv2AudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* HearingCorrectionAUv2AudioProcessor::createEditor()
{
    return new HearingCorrectionAUv2AudioProcessorEditor (*this);
}

//==============================================================================
void HearingCorrectionAUv2AudioProcessor::loadHeadphoneProfile (const juce::String& name)
{
    if (name.isEmpty())
    {
        headphoneEQ.clearProfile();
        selectedHeadphoneName.clear();
    }
    else
    {
        if (headphoneEQ.loadProfile (name))
            selectedHeadphoneName = name;
        else
            selectedHeadphoneName.clear();
    }
}

//==============================================================================
void HearingCorrectionAUv2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Add headphone name to state
    state.setProperty ("headphoneName", selectedHeadphoneName, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HearingCorrectionAUv2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xml));

            // Restore headphone profile
            auto headphoneName = parameters.state.getProperty ("headphoneName").toString();
            if (headphoneName.isNotEmpty())
                loadHeadphoneProfile (headphoneName);
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HearingCorrectionAUv2AudioProcessor();
}

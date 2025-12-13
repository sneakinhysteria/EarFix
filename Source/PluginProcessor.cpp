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
    compressionSpeedParam   = parameters.getRawParameterValue ("compressionSpeed");
    experienceLevelParam    = parameters.getRawParameterValue ("experienceLevel");
    leftEnableParam         = parameters.getRawParameterValue ("leftEnable");
    rightEnableParam        = parameters.getRawParameterValue ("rightEnable");

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

void HearingCorrectionAUv2AudioProcessor::updateCompressorCoefficients()
{
    bool fastCompression = compressionSpeedParam->load() < 0.5f;

    float attackMs  = fastCompression ? 5.0f : 10.0f;
    float releaseMs = fastCompression ? 50.0f : 150.0f;

    attackCoeff  = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * attackMs / 1000.0f));
    releaseCoeff = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * releaseMs / 1000.0f));
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

    previousGain = juce::Decibels::decibelsToGain (outputGainParam->load());

    // Prepare filter spec for mono processing (each filter handles one channel)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = 1;

    // Prepare and reset all filter bands (11 with interpolation)
    for (int i = 0; i < numFilterBands; ++i)
    {
        leftFilters[i].prepare (spec);
        rightFilters[i].prepare (spec);
        leftFilters[i].reset();
        rightFilters[i].reset();
        leftFilterGains[i] = 0.0f;
        rightFilterGains[i] = 0.0f;
    }

    // Reset compressors (6 at audiogram frequencies only)
    for (int i = 0; i < numAudiogramBands; ++i)
    {
        leftCompressors[i].envelope = 0.0f;
        rightCompressors[i].envelope = 0.0f;
    }

    updateCompressorCoefficients();
    updateCurrentModel();
    updateFilterCoefficients();
}

void HearingCorrectionAUv2AudioProcessor::releaseResources() {}

void HearingCorrectionAUv2AudioProcessor::updateFilterCoefficients()
{
    const float strength = correctionStrengthParam->load() / 100.0f;

    // First, calculate gains at the 6 audiogram frequencies
    std::array<float, numAudiogramBands> leftAudiogramGains;
    std::array<float, numAudiogramBands> rightAudiogramGains;

    for (int i = 0; i < numAudiogramBands; ++i)
    {
        const float freq = audiogramFrequencies[i];
        const float leftLoss  = std::max (0.0f, leftAudiogramParams[i]->load());
        const float rightLoss = std::max (0.0f, rightAudiogramParams[i]->load());

        leftAudiogramGains[i]  = currentModel->calculateGain (freq, leftLoss) * strength;
        rightAudiogramGains[i] = currentModel->calculateGain (freq, rightLoss) * strength;
    }

    // Now calculate gains for all 11 filter bands using linear interpolation
    // Filter bands: 250, 354, 500, 707, 1000, 1414, 2000, 2828, 4000, 5657, 8000
    // Audiogram indices: 0,   -,   1,   -,    2,     -,    3,     -,    4,     -,    5
    for (int i = 0; i < numFilterBands; ++i)
    {
        const float freq = filterFrequencies[i];

        // Determine if this is an audiogram band or interpolated band
        // Even indices (0,2,4,6,8,10) are audiogram bands, odd indices are interpolated
        if (i % 2 == 0)
        {
            // Direct audiogram band
            int audiogramIdx = i / 2;
            leftFilterGains[i] = leftAudiogramGains[audiogramIdx];
            rightFilterGains[i] = rightAudiogramGains[audiogramIdx];
        }
        else
        {
            // Interpolated band - linear interpolation between adjacent audiogram bands
            int lowerIdx = i / 2;
            int upperIdx = lowerIdx + 1;

            // Linear interpolation (0.5 blend since we're at geometric midpoint)
            leftFilterGains[i] = (leftAudiogramGains[lowerIdx] + leftAudiogramGains[upperIdx]) * 0.5f;
            rightFilterGains[i] = (rightAudiogramGains[lowerIdx] + rightAudiogramGains[upperIdx]) * 0.5f;
        }

        // Apply filter coefficients
        if (freq < currentSampleRate * 0.45f)
        {
            auto leftCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                currentSampleRate, freq, filterQ,
                juce::Decibels::decibelsToGain (leftFilterGains[i]));

            auto rightCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                currentSampleRate, freq, filterQ,
                juce::Decibels::decibelsToGain (rightFilterGains[i]));

            // Assign new coefficient objects to ensure left and right filters
            // have completely independent coefficients
            leftFilters[i].coefficients  = leftCoeffs;
            rightFilters[i].coefficients = rightCoeffs;
        }
        else
        {
            // High frequencies above Nyquist - use bypass (separate objects for each filter)
            leftFilters[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderAllPass (
                currentSampleRate, 1000.0f);
            rightFilters[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderAllPass (
                currentSampleRate, 1000.0f);
        }
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

    // Update model and filters
    updateCurrentModel();
    updateCompressorCoefficients();
    updateFilterCoefficients();

    const bool leftEnabled  = leftEnableParam->load() > 0.5f;
    const bool rightEnabled = rightEnableParam->load() > 0.5f;
    const bool useCompression = currentModel->hasCompression();

    if (buffer.getNumChannels() >= 2)
    {
        auto* leftChannel  = buffer.getWritePointer (0);
        auto* rightChannel = buffer.getWritePointer (1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float leftSample  = leftChannel[sample];
            float rightSample = rightChannel[sample];

            // Process left channel
            if (leftEnabled)
            {
                // Apply all 11 EQ filter bands (including interpolated)
                for (int band = 0; band < numFilterBands; ++band)
                    leftSample = leftFilters[band].processSample (leftSample);

                // Apply compression at 6 audiogram frequencies only
                if (useCompression)
                {
                    for (int band = 0; band < numAudiogramBands; ++band)
                    {
                        float leftLoss = std::max (0.0f, leftAudiogramParams[band]->load());
                        auto compParams = currentModel->getCompressionParams (
                            audiogramFrequencies[band], leftLoss);

                        if (compParams.ratio > 1.0f)
                        {
                            float inputLevel = std::abs (leftSample);
                            float& env = leftCompressors[band].envelope;

                            float coeff = (inputLevel > env) ? attackCoeff : releaseCoeff;
                            env = env * coeff + inputLevel * (1.0f - coeff);

                            float envDb = juce::Decibels::gainToDecibels (env + 1e-6f);

                            if (envDb > compParams.threshold)
                            {
                                float overDb = envDb - compParams.threshold;
                                float compressedOverDb = overDb / compParams.ratio;
                                float gainReduction = overDb - compressedOverDb;
                                leftSample *= juce::Decibels::decibelsToGain (-gainReduction);
                            }
                        }
                    }
                }
            }

            // Process right channel
            if (rightEnabled)
            {
                // Apply all 11 EQ filter bands (including interpolated)
                for (int band = 0; band < numFilterBands; ++band)
                    rightSample = rightFilters[band].processSample (rightSample);

                // Apply compression at 6 audiogram frequencies only
                if (useCompression)
                {
                    for (int band = 0; band < numAudiogramBands; ++band)
                    {
                        float rightLoss = std::max (0.0f, rightAudiogramParams[band]->load());
                        auto compParams = currentModel->getCompressionParams (
                            audiogramFrequencies[band], rightLoss);

                        if (compParams.ratio > 1.0f)
                        {
                            float inputLevel = std::abs (rightSample);
                            float& env = rightCompressors[band].envelope;

                            float coeff = (inputLevel > env) ? attackCoeff : releaseCoeff;
                            env = env * coeff + inputLevel * (1.0f - coeff);

                            float envDb = juce::Decibels::gainToDecibels (env + 1e-6f);

                            if (envDb > compParams.threshold)
                            {
                                float overDb = envDb - compParams.threshold;
                                float compressedOverDb = overDb / compParams.ratio;
                                float gainReduction = overDb - compressedOverDb;
                                rightSample *= juce::Decibels::decibelsToGain (-gainReduction);
                            }
                        }
                    }
                }
            }

            leftChannel[sample]  = leftSample;
            rightChannel[sample] = rightSample;
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
void HearingCorrectionAUv2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HearingCorrectionAUv2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HearingCorrectionAUv2AudioProcessor();
}

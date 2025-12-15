/*
  ==============================================================================

    Hearing Correction AU v2 - Alpha
    Per-ear audiogram-driven EQ correction plugin

    JUCE 8 native implementation with pluggable correction models

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Models/CorrectionModel.h"
#include "Models/HalfGainModel.h"
#include "Models/NALModel.h"
#include "Models/MOSLModel.h"
#include "HeadphoneEQ.h"

//==============================================================================
class HearingCorrectionAUv2AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    HearingCorrectionAUv2AudioProcessor();
    ~HearingCorrectionAUv2AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters { *this, nullptr, "PARAMETERS", createParameterLayout() };

    //==============================================================================
    // Audiogram input frequencies (user-adjustable)
    static constexpr int numAudiogramBands = 6;
    static constexpr std::array<float, numAudiogramBands> audiogramFrequencies = {
        250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f
    };

    // Processing bands (audiogram + interpolated intermediate bands)
    static constexpr int numFilterBands = 11;

    // Level metering (read by UI)
    std::atomic<float> inputLevelLeft { 0.0f };
    std::atomic<float> inputLevelRight { 0.0f };
    std::atomic<float> outputLevelLeft { 0.0f };
    std::atomic<float> outputLevelRight { 0.0f };
    static constexpr std::array<float, numFilterBands> filterFrequencies = {
        250.0f,    // Audiogram band 0
        354.0f,    // Interpolated (geometric mean of 250 & 500)
        500.0f,    // Audiogram band 1
        707.0f,    // Interpolated (geometric mean of 500 & 1000)
        1000.0f,   // Audiogram band 2
        1414.0f,   // Interpolated (geometric mean of 1000 & 2000)
        2000.0f,   // Audiogram band 3
        2828.0f,   // Interpolated (geometric mean of 2000 & 4000)
        4000.0f,   // Audiogram band 4
        5657.0f,   // Interpolated (geometric mean of 4000 & 8000)
        8000.0f    // Audiogram band 5
    };

    //==============================================================================
    // Headphone EQ correction
    HeadphoneEQ headphoneEQ;

    /** Loads a headphone profile by name. Called when parameter changes. */
    void loadHeadphoneProfile (const juce::String& name);

    /** Returns list of available headphone names for the UI. */
    const std::vector<HeadphoneIndexEntry>& getAvailableHeadphones() const { return headphoneEQ.getAvailableHeadphones(); }

    /** Returns currently selected headphone name. */
    juce::String getCurrentHeadphoneName() const { return headphoneEQ.getCurrentProfileName(); }

    /** Reloads the headphone database (for UI refresh button). */
    void reloadHeadphoneDatabase() { headphoneEQ.loadDatabase(); }

private:
    //==============================================================================
    // Correction models
    HalfGainModel halfGainModel;
    NALModel nalModel;
    MOSLModel moslModel;
    CorrectionModel* currentModel = &halfGainModel;

    void updateCurrentModel();

    //==============================================================================
    // Cached parameter pointers
    std::atomic<float>* bypassParam           = nullptr;
    std::atomic<float>* outputGainParam       = nullptr;
    std::atomic<float>* modelSelectParam      = nullptr;
    std::atomic<float>* correctionStrengthParam = nullptr;
    std::atomic<float>* maxBoostParam         = nullptr;
    std::atomic<float>* compressionSpeedParam = nullptr;
    std::atomic<float>* experienceLevelParam  = nullptr;
    std::atomic<float>* leftEnableParam       = nullptr;
    std::atomic<float>* rightEnableParam      = nullptr;
    std::atomic<float>* headphoneEQEnableParam = nullptr;

    // Headphone profile name (stored separately as strings aren't supported in APVTS)
    juce::String selectedHeadphoneName;

    std::array<std::atomic<float>*, numAudiogramBands> leftAudiogramParams;
    std::array<std::atomic<float>*, numAudiogramBands> rightAudiogramParams;

    // Gain smoothing
    float previousGain = 1.0f;

    //==============================================================================
    // Linkwitz-Riley Multiband Crossover (5 crossovers for 6 bands)
    // Crossover frequencies at geometric means between audiogram bands
    static constexpr int numCrossovers = 5;
    static constexpr std::array<float, numCrossovers> crossoverFrequencies = {
        354.0f, 707.0f, 1414.0f, 2828.0f, 5657.0f
    };

    // Per-channel crossover filters (LP and HP pairs)
    std::array<juce::dsp::LinkwitzRileyFilter<float>, numCrossovers> leftLowpass;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, numCrossovers> leftHighpass;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, numCrossovers> rightLowpass;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, numCrossovers> rightHighpass;

    //==============================================================================
    // True WDRC state per band per ear
    struct WDRCBandState
    {
        float envelope = 0.0f;           // Envelope follower state
        float smoothedGain = 0.0f;       // Smoothed gain value
        float targetGainForSoftSounds = 0.0f;  // Max gain (for quiet inputs)
    };

    std::array<WDRCBandState, numAudiogramBands> leftWDRC;
    std::array<WDRCBandState, numAudiogramBands> rightWDRC;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float gainSmoothCoeff = 0.0f;  // For smooth gain transitions

    void updateWDRCCoefficients();
    void updateCrossoverCoefficients();

    // Calculate WDRC gain based on input level and hearing loss
    float calculateWDRCGain (float inputLevelDb, float hearingLossDb, float maxBoostDb) const;

    //==============================================================================
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HearingCorrectionAUv2AudioProcessor)
};

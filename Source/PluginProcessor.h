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
    std::atomic<float>* compressionSpeedParam = nullptr;
    std::atomic<float>* experienceLevelParam  = nullptr;
    std::atomic<float>* leftEnableParam       = nullptr;
    std::atomic<float>* rightEnableParam      = nullptr;

    std::array<std::atomic<float>*, numAudiogramBands> leftAudiogramParams;
    std::array<std::atomic<float>*, numAudiogramBands> rightAudiogramParams;

    // Gain smoothing
    float previousGain = 1.0f;

    //==============================================================================
    // Multi-band EQ filters (11 bands with interpolation)
    static constexpr float filterQ = 1.4f;

    std::array<juce::dsp::IIR::Filter<float>, numFilterBands> leftFilters;
    std::array<juce::dsp::IIR::Filter<float>, numFilterBands> rightFilters;

    // Interpolated gains for each filter band
    std::array<float, numFilterBands> leftFilterGains;
    std::array<float, numFilterBands> rightFilterGains;

    //==============================================================================
    // WDRC Compressor state per band per ear (only at audiogram frequencies)
    struct CompressorState
    {
        float envelope = 0.0f;
    };

    std::array<CompressorState, numAudiogramBands> leftCompressors;
    std::array<CompressorState, numAudiogramBands> rightCompressors;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    void updateCompressorCoefficients();

    //==============================================================================
    double currentSampleRate = 44100.0;

    void updateFilterCoefficients();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HearingCorrectionAUv2AudioProcessor)
};

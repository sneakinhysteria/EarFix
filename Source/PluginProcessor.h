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

    // Live per-band applied correction gain in dB (read by the audiogram chart overlay).
    // Written each block in updateWDRCCoefficients() from the current model/strength/
    // maxBoost/loudnessMode -- reflects exactly what's being applied right now.
    std::array<std::atomic<float>, numAudiogramBands> leftAppliedGainDb;
    std::array<std::atomic<float>, numAudiogramBands> rightAppliedGainDb;

    // True when Max Boost is actually constraining the curve right now (either ear),
    // for either loudness mode -- vs. sitting above the curve's natural peak and doing
    // nothing. Lets the UI dim the control when raising it wouldn't change anything for
    // the current audiogram/model/strength.
    std::atomic<bool> maxBoostActive { false };

    // Highest value either ear's curve would need with no ceiling at all -- the point
    // past which raising Max Boost further stops changing the output. Drives the UI's
    // active-range marker on the Max Boost fader; updates live with Strength/audiogram.
    std::atomic<float> maxBoostThresholdDb { 0.0f };
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

    /** Imports a pasted ParametricEQ profile, then reloads the database.
        Returns the saved profile name, or empty on failure. */
    juce::String importHeadphoneProfile (const juce::String& name, const juce::String& text)
    {
        auto saved = headphoneEQ.importParametricEQText (name, text);
        if (saved.isNotEmpty())
            headphoneEQ.loadDatabase();
        return saved;
    }

    //==============================================================================
    // Presets (in-plugin, host-independent). State = APVTS tree + headphone name.

    /** Standard AU preset directory (~/Library/Audio/Presets/<Manufacturer>/<Name>),
        shared with hosts like Logic so presets are interchangeable. */
    static juce::File getPresetsDirectory();

    /** Saves the current state as an Apple .aupreset (loadable by Logic and other AU
        hosts, and by this plugin). macOS only. */
    void saveAUPreset (const juce::File& file, const juce::String& presetName);

    /** Loads an Apple .aupreset by extracting the embedded plugin state.
        Returns true on success. macOS only. */
    bool loadAUPresetFile (const juce::File& file);

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
    std::atomic<float>* loudnessModeParam     = nullptr;
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

    // Phase-compensation all-pass filters. Each already-extracted lower band is
    // passed through an all-pass at every later crossover so all bands share the
    // same phase and sum to a flat magnitude response at unity gain.
    // Indexed [band][crossover]; only entries with crossover > band are used.
    std::array<std::array<juce::dsp::LinkwitzRileyFilter<float>, numCrossovers>, numAudiogramBands> leftAllpass;
    std::array<std::array<juce::dsp::LinkwitzRileyFilter<float>, numCrossovers>, numAudiogramBands> rightAllpass;

    //==============================================================================
    // True WDRC state per band per ear
    struct WDRCBandState
    {
        float envelope = 0.0f;                  // Envelope follower state
        float smoothedGain = 1.0f;              // Smoothed linear gain (starts at unity)
        float targetGainForSoftSounds = 0.0f;   // Signed dB: full-reshape target for soft input
        float compressionRatio = 1.0f;          // Per-band WDRC ratio (from the model)
    };

    std::array<WDRCBandState, numAudiogramBands> leftWDRC;
    std::array<WDRCBandState, numAudiogramBands> rightWDRC;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float gainSmoothCoeff = 0.0f;  // For smooth gain transitions

    void updateWDRCCoefficients();
    void updateCrossoverCoefficients();

    // Effective per-band gain for the current input level. Below the knee the full
    // reshaped (soft) gain is applied; above it the deviation from flat is reduced
    // per the band ratio so loud passages approach the input spectrum.
    float calculateWDRCGain (float inputLevelDb, float softGainDb, float ratio) const;

    // Input level (dBFS) used to sample the model's uncompressed prescriptive gain.
    // Kept below every model's compression threshold so the soft target is the full
    // insertion gain, with level-dependence handled solely by the processor WDRC.
    static constexpr float kSoftReferenceLevelDb = 25.0f;

    // WDRC knee (dBFS): below this, quiet passages receive the full reshaped gain.
    static constexpr float kWDRCKneeDb = -40.0f;

    //==============================================================================
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HearingCorrectionAUv2AudioProcessor)
};

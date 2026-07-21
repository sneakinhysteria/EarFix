/*
  ==============================================================================

    Hearing Correction AU v2 - Alpha
    Per-ear audiogram-driven EQ correction plugin

    JUCE 8 native implementation with pluggable correction models

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

#if JUCE_MAC
 #include <CoreFoundation/CoreFoundation.h>

 // Minimal RAII wrapper for CF types (CoreFoundation is a plain C API, so this file
 // does not need to be compiled as Objective-C++ to use it).
 template <typename CFType>
 struct EarFixCFPtr
 {
     EarFixCFPtr() = default;
     explicit EarFixCFPtr (CFType obj) : object (obj) {}
     ~EarFixCFPtr() { if (object != nullptr) CFRelease (object); }
     EarFixCFPtr (const EarFixCFPtr&) = delete;
     EarFixCFPtr& operator= (const EarFixCFPtr&) = delete;

     CFType get() const { return object; }
     explicit operator bool() const { return object != nullptr; }
     bool operator== (std::nullptr_t) const { return object == nullptr; }
     bool operator!= (std::nullptr_t) const { return object != nullptr; }

     CFType object = nullptr;
 };
#endif

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

    // Output gain: -48 to +24 dB. The wide negative range is deliberate: hearing
    // correction in Boost Only mode never cuts and can add tens of dB of boost, so the
    // corrected signal runs far hotter than the input. Trimming here (a lossless float
    // scale, no clipping anywhere in the path) is the correct place to bring it back --
    // never by reducing source/system volume, which throws away resolution upstream.
    // The "Auto" button sets this to cancel the correction's measured loudness excess.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-48.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // Correction strength: 0% to 100%. The single "how much correction" control --
    // a uniform multiplier on the model's per-band prescribed gain. (The old separate
    // "Max Boost" fader was removed: in Boost Only mode it re-normalized the curve to a
    // dB ceiling, which mathematically cancelled Strength whenever it was active, so the
    // two were one degree of freedom expressed two ways. The per-band gain is still hard-
    // limited by each model's own internal clamp -- kCorrectionCeilingDb below.)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "correctionStrength", 1 },
        "Correction",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        85.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // Loudness mode: how the per-band curve is kept from running louder than the input.
    //   Centered   - subtract the K-weighted mean from every band. Exact loudness match,
    //                but bands with little/no loss can get cut to subsidize bands that
    //                need a large boost elsewhere.
    //   Boost Only - never cut a band below its own absolute model-prescribed gain. If
    //                the raw curve would run louder than the input, the whole curve is
    //                scaled down by one uniform factor (shape preserved) until it isn't.
    //                A barely-affected band still gets its own (small) prescribed boost --
    //                matching published prescriptive-formula behavior (e.g. NAL-NL2 still
    //                applies ~5-10 dB of gain at near-normal thresholds rather than zero).
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "loudnessMode", 1 },
        "Loudness",
        juce::StringArray { "Centered", "Boost Only" },
        1));  // Default: Boost Only

    // Compression speed: 0 = Fast, 1 = Slow (used by NAL and MOSL)
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "compressionSpeed", 1 },
        "Compression",
        juce::StringArray { "Fast", "Slow" },
        0));

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
    compressionSpeedParam   = parameters.getRawParameterValue ("compressionSpeed");
    loudnessModeParam       = parameters.getRawParameterValue ("loudnessMode");
    leftEnableParam         = parameters.getRawParameterValue ("leftEnable");
    rightEnableParam        = parameters.getRawParameterValue ("rightEnable");
    headphoneEQEnableParam  = parameters.getRawParameterValue ("headphoneEQEnable");

    for (int i = 0; i < numAudiogramBands; ++i)
    {
        rightAudiogramParams[i] = parameters.getRawParameterValue ("audiogram_" + rightParamSuffixes[i]);
        leftAudiogramParams[i]  = parameters.getRawParameterValue ("audiogram_" + leftParamSuffixes[i]);
        leftAppliedGainDb[i].store (0.0f, std::memory_order_relaxed);
        rightAppliedGainDb[i].store (0.0f, std::memory_order_relaxed);
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

    // Correction strength is applied once as a multiplier on the centered curve in
    // updateWDRCCoefficients(). Keep the model's prescriptive gain pure (no offset).
    currentModel->setOverallGainOffset (0.0f);

    // NAL-specific settings. NAL has no use for Level -- it would only have scaled
    // gain by a fixed 0.7/0.85/1.0 factor, which Strength already does continuously
    // (the two multiply into the same value, so it was a redundant discrete preset).
    if (modelIndex == 1)
    {
        bool fastCompression = compressionSpeedParam->load() < 0.5f;
        nalModel.setCompressionSpeed (fastCompression);
    }

    // MOSL-specific settings. Brightness/bass emphasis stay at MOSL's own defaults
    // (the former "Experience" control that drove them was removed -- it shifted gain
    // by only ~1 dB, below clear audibility next to Model/Strength).
    if (modelIndex == 2)
    {
        bool fastCompression = compressionSpeedParam->load() < 0.5f;
        moslModel.setCompressionSpeed (fastCompression);
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
    inputLoudnessMS = 0.0f;
    processedLoudnessMS = 0.0f;

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

    // Prepare phase-compensation all-pass filters
    for (int band = 0; band < numAudiogramBands; ++band)
    {
        for (int k = 0; k < numCrossovers; ++k)
        {
            leftAllpass[band][k].prepare (spec);
            rightAllpass[band][k].prepare (spec);
            leftAllpass[band][k].reset();
            rightAllpass[band][k].reset();
        }
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

    // Configure phase-compensation all-pass filters. Band j must be all-pass
    // filtered at every crossover k > j so it stays phase-aligned with the higher
    // bands split off at those later crossovers.
    for (int band = 0; band < numAudiogramBands; ++band)
    {
        for (int k = band + 1; k < numCrossovers; ++k)
        {
            float freq = crossoverFrequencies[k];

            if (freq >= currentSampleRate * 0.45f)
                freq = static_cast<float> (currentSampleRate * 0.44f);

            leftAllpass[band][k].setType (juce::dsp::LinkwitzRileyFilterType::allpass);
            leftAllpass[band][k].setCutoffFrequency (freq);
            rightAllpass[band][k].setType (juce::dsp::LinkwitzRileyFilterType::allpass);
            rightAllpass[band][k].setCutoffFrequency (freq);
        }
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

    // Update target gains for each band based on hearing loss.
    const float strength = correctionStrengthParam->load() / 100.0f;
    const bool  modelComp = currentModel->hasCompression();
    const int   loudnessMode = static_cast<int> (loudnessModeParam->load());  // 0=Centered, 1=Boost Only

    // Fixed per-band ceiling (dB), the silent safety limit that replaced the former
    // user "Max Boost" fader. Matches each model's own internal calculateGain clamp,
    // so in Boost Only it effectively never binds -- the curve is just strength*model.
    constexpr float kCorrectionCeilingDb = 40.0f;

    // Perceptual (K-weighted, BS.1770) loudness weight per audiogram band:
    //   weight = octave bandwidth of the crossover band x K-weighting power gain at
    //   its centre. Band edges 20/354/707/1414/2828/5657/20000 Hz; K-weight (dB)
    //   0/0.5/1.5/3/4/4. Used to hold perceived loudness constant across the reshape.
    static constexpr std::array<float, numAudiogramBands> loudnessWeights =
        { 4.146f, 1.120f, 1.413f, 1.995f, 2.512f, 4.577f };

    // K-weighted power sum of a candidate curve, for comparing against flat (0 dB).
    auto kWeightedPowerSum = [] (const std::array<float, numAudiogramBands>& gains)
    {
        double sum = 0.0;
        for (int i = 0; i < numAudiogramBands; ++i)
            sum += loudnessWeights[i] * std::pow (10.0, static_cast<double> (gains[i]) / 10.0);
        return sum;
    };

    static constexpr double wsum = 4.146 + 1.120 + 1.413 + 1.995 + 2.512 + 4.577;

    // Builds the loudness-safe target curve for one ear from the model's pure
    // (uncompressed) prescriptive gain, scaled by strength, then shaped per the
    // selected loudness mode (see the parameter comment above).
    auto computeEar = [this, strength, modelComp, loudnessMode, &kWeightedPowerSum]
        (const std::array<std::atomic<float>*, numAudiogramBands>& audioParams,
         std::array<WDRCBandState, numAudiogramBands>& wdrc,
         std::array<std::atomic<float>, numAudiogramBands>& appliedGainDb)
    {
        std::array<float, numAudiogramBands> g {};

        for (int i = 0; i < numAudiogramBands; ++i)
        {
            const float loss = std::max (0.0f, audioParams[i]->load());
            g[i] = currentModel->calculateGain (audiogramFrequencies[i], loss, kSoftReferenceLevelDb) * strength;
        }

        std::array<float, numAudiogramBands> shaped {};

        if (loudnessMode == 1)
        {
            // Boost Only: every band is >= 0 by construction (the model's own gain is
            // never negative), so no band is ever cut to subsidize another. Strength is
            // the sole scale; the fixed ceiling only guards against a runaway band and
            // in practice never binds (the model already clamps calculateGain to 40 dB).
            for (int i = 0; i < numAudiogramBands; ++i)
                shaped[i] = juce::jlimit (0.0f, kCorrectionCeilingDb, g[i]);
        }
        else
        {
            const float offset = 10.0f * std::log10 (static_cast<float> (kWeightedPowerSum (g) / wsum));
            for (int i = 0; i < numAudiogramBands; ++i)
                shaped[i] = juce::jlimit (-kCorrectionCeilingDb, kCorrectionCeilingDb, g[i] - offset);
        }

        for (int i = 0; i < numAudiogramBands; ++i)
        {
            wdrc[i].targetGainForSoftSounds = shaped[i];
            appliedGainDb[i].store (shaped[i], std::memory_order_relaxed);

            const float loss = std::max (0.0f, audioParams[i]->load());
            wdrc[i].compressionRatio = modelComp
                ? currentModel->getCompressionParams (audiogramFrequencies[i], loss).ratio
                : 1.0f;
        }
    };

    computeEar (leftAudiogramParams, leftWDRC, leftAppliedGainDb);
    computeEar (rightAudiogramParams, rightWDRC, rightAppliedGainDb);
}

float HearingCorrectionAUv2AudioProcessor::calculateWDRCGain (float inputLevelDb,
                                                               float softGainDb,
                                                               float ratio) const
{
    // WDRC: quiet passages get the full reshaped (soft) gain; as the band level
    // rises the deviation from flat is reduced, so loud passages approach the
    // original input spectrum (no net loudness increase). Works for boosts and
    // cuts alike, since it scales the signed soft gain toward zero.

    if (inputLevelDb <= kWDRCKneeDb || ratio <= 1.0f)
        return softGainDb;

    // Fraction of the way from the knee (full gain) to 0 dBFS (max compression).
    const float t = juce::jlimit (0.0f, 1.0f,
                                  (inputLevelDb - kWDRCKneeDb) / (0.0f - kWDRCKneeDb));

    // At the knee: remaining = 1 (full gain). At 0 dBFS: remaining = 1/ratio.
    const float remaining = 1.0f - t * (1.0f - 1.0f / ratio);

    return softGainDb * remaining;
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

    // Measure input levels (peak, for meters) and block loudness (mean-square, for the
    // signal-based Auto trim below).
    float inputBlockMS = 0.0f;
    if (buffer.getNumChannels() >= 2)
    {
        inputLevelLeft.store (buffer.getMagnitude (0, 0, numSamples), std::memory_order_relaxed);
        inputLevelRight.store (buffer.getMagnitude (1, 0, numSamples), std::memory_order_relaxed);
        const float rL = buffer.getRMSLevel (0, 0, numSamples);
        const float rR = buffer.getRMSLevel (1, 0, numSamples);
        inputBlockMS = 0.5f * (rL * rL + rR * rR);
    }

    if (bypassParam->load() > 0.5f)
    {
        outputLevelLeft.store (inputLevelLeft.load (std::memory_order_relaxed), std::memory_order_relaxed);
        outputLevelRight.store (inputLevelRight.load (std::memory_order_relaxed), std::memory_order_relaxed);
        return;
    }

    const bool headphoneEQEnabled = headphoneEQEnableParam->load() > 0.5f;

    // Update model and WDRC parameters (kept live so the overlay stays current even
    // while correction is momentarily disabled).
    updateCurrentModel();
    updateWDRCCoefficients();

    const bool leftEnabled  = leftEnableParam->load() > 0.5f;
    const bool rightEnabled = rightEnableParam->load() > 0.5f;
    const bool modelComp    = currentModel->hasCompression();

    // Whether anything is actually correcting: either ear, or headphone EQ. When
    // nothing is, the Output Gain trim is skipped below so the plugin is transparent
    // (matches host bypass). We keep running the crossover/WDRC per-sample loop
    // regardless, so those filters stay warm and re-enabling doesn't click from stale
    // filter state.
    const bool anyCorrectionActive = leftEnabled || rightEnabled || headphoneEQEnabled;

    // Applies the envelope-following WDRC gain for one band and returns the new
    // smoothed linear gain. When the model has no compression the static soft
    // target is used directly (true linear EQ, e.g. Half-Gain).
    auto bandGain = [this, modelComp] (WDRCBandState& st, float bandSample) -> float
    {
        const float level = std::abs (bandSample);
        const float coeff = (level > st.envelope) ? attackCoeff : releaseCoeff;
        st.envelope = st.envelope * coeff + level * (1.0f - coeff);

        const float inputDb = juce::Decibels::gainToDecibels (st.envelope + 1e-6f);
        const float gainDb  = modelComp
            ? calculateWDRCGain (inputDb, st.targetGainForSoftSounds, st.compressionRatio)
            : st.targetGainForSoftSounds;

        const float gainLin = juce::Decibels::decibelsToGain (gainDb);
        st.smoothedGain = st.smoothedGain * gainSmoothCoeff + gainLin * (1.0f - gainSmoothCoeff);
        return st.smoothedGain;
    };

    if (buffer.getNumChannels() >= 2)
    {
        auto* leftChannel  = buffer.getWritePointer (0);
        auto* rightChannel = buffer.getWritePointer (1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float leftIn  = leftChannel[sample];
            const float rightIn = rightChannel[sample];

            // Phase-compensated multiband split.
            // Signal flow: Input -> split into bands (phase-aligned) -> WDRC -> sum.
            float leftBands[numAudiogramBands];
            float rightBands[numAudiogramBands];

            leftBands[0]  = leftLowpass[0].processSample  (0, leftIn);
            rightBands[0] = rightLowpass[0].processSample (0, rightIn);
            float leftHigh  = leftHighpass[0].processSample  (0, leftIn);
            float rightHigh = rightHighpass[0].processSample (0, rightIn);

            for (int k = 1; k < numCrossovers; ++k)
            {
                const float leftLow   = leftLowpass[k].processSample   (0, leftHigh);
                const float rightLow  = rightLowpass[k].processSample  (0, rightHigh);
                const float leftNext  = leftHighpass[k].processSample  (0, leftHigh);
                const float rightNext = rightHighpass[k].processSample (0, rightHigh);

                // Keep the already-extracted lower bands phase-aligned.
                for (int j = 0; j < k; ++j)
                {
                    leftBands[j]  = leftAllpass[j][k].processSample  (0, leftBands[j]);
                    rightBands[j] = rightAllpass[j][k].processSample (0, rightBands[j]);
                }

                leftBands[k]  = leftLow;
                rightBands[k] = rightLow;
                leftHigh  = leftNext;
                rightHigh = rightNext;
            }

            leftBands[numCrossovers]  = leftHigh;   // final (highest) band
            rightBands[numCrossovers] = rightHigh;

            float leftOut = 0.0f;
            float rightOut = 0.0f;

            for (int band = 0; band < numAudiogramBands; ++band)
            {
                leftOut  += leftBands[band]  * bandGain (leftWDRC[band],  leftBands[band]);
                rightOut += rightBands[band] * bandGain (rightWDRC[band], rightBands[band]);
            }

            // If an ear is disabled, pass through the original signal.
            leftChannel[sample]  = leftEnabled  ? leftOut  : leftIn;
            rightChannel[sample] = rightEnabled ? rightOut : rightIn;
        }
    }

    // Headphone EQ AFTER hearing correction. The transducer is the last physical stage,
    // so its compensation is a final linear touch-up on the corrected signal. Running it
    // here (not before) means the WDRC compression above responded to the clean source
    // dynamics rather than the signal pre-distorted by the headphone-inverse EQ. The net
    // magnitude correction is identical either way (linear filters commute); only the
    // level-dependent compression benefits. Loudness-neutral, so it changes tone not level.
    headphoneEQ.setEnabled (headphoneEQEnabled);
    headphoneEQ.process (buffer);   // no-op when disabled

    // Signal-based loudness measurement for the Auto trim: compare the actual processed
    // loudness (post correction + headphone EQ, pre output gain) against the input,
    // integrated over ~400 ms. Unlike a gain-curve estimate this reflects the real energy
    // the processing added for whatever is playing -- a large HF boost adds little when the
    // material has little HF energy -- so Auto matches loudness instead of over-trimming.
    // Gated on a real input signal so silence doesn't skew the running average.
    if (buffer.getNumChannels() >= 2 && inputBlockMS > 1.0e-7f)   // ~-70 dBFS gate
    {
        const float rL = buffer.getRMSLevel (0, 0, numSamples);
        const float rR = buffer.getRMSLevel (1, 0, numSamples);
        const float processedBlockMS = 0.5f * (rL * rL + rR * rR);

        const float a = std::exp (-static_cast<float> (numSamples)
                                   / static_cast<float> (currentSampleRate * 0.4));
        inputLoudnessMS     = inputLoudnessMS     * a + inputBlockMS     * (1.0f - a);
        processedLoudnessMS = processedLoudnessMS * a + processedBlockMS * (1.0f - a);

        if (inputLoudnessMS > 1.0e-9f)
            correctionExcessDb.store (10.0f * std::log10 (processedLoudnessMS / inputLoudnessMS),
                                      std::memory_order_relaxed);
    }

    // Output gain with smoothing. Applied only while something is being corrected --
    // the trim compensates the correction's added loudness, so with nothing active the
    // signal is already the untouched input (both ears passed through, headphone EQ a
    // no-op) and must stay untrimmed to match host bypass. While inactive we hold
    // previousGain AT the target so that re-enabling snaps the trim in from sample 0
    // alongside the correction (they cancel to ~unity loudness) instead of ramping up
    // from unity, which would let a full un-trimmed correction burst through -> click.
    const float targetGain = juce::Decibels::decibelsToGain (outputGainParam->load());

    if (anyCorrectionActive)
    {
        if (std::abs (targetGain - previousGain) > 0.0001f)
        {
            buffer.applyGainRamp (0, numSamples, previousGain, targetGain);
            previousGain = targetGain;
        }
        else
        {
            buffer.applyGain (targetGain);
        }
    }
    else
    {
        previousGain = targetGain;
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
// Curated Basic-mode presets. The only research-supported differentiators between
// speech and music listening are the Model and the compression Speed -- Strength and
// the loudness mode are shared:
//   Speech -- NAL model, Fast compression (near-universal clinical practice for speech;
//     faster release preserves consonant transients).
//   Music  -- MOSL model, Slow compression (published music-fitting guidance: slow
//     time constants + gentle compression avoid pumping and preserve dynamics).
// Both use Strength 85% and Boost Only. 85% was calibrated against the developer's own
// real professional Phonak fitting (APD Contrast 3.0, confirmed comfortable): at
// average input levels EarFix's per-band gain best-fits that clinical target at
// 79-99% strength across both ears (~85% centre). No fixed number is per-person optimal
// -- the models already shape per-audiogram, so this is a sensible starting point to
// refine in Advanced, not a Phonak-Target-style individual prescription.
namespace
{
    struct CuratedParamValue { const char* paramId; float rawValue; };

    const CuratedParamValue kSpeechPreset[] = {
        { "modelSelect",        1.0f },   // NAL (Speech)
        { "correctionStrength", 85.0f },
        { "loudnessMode",        1.0f },  // Boost Only
        { "compressionSpeed",    0.0f },  // Fast
    };

    const CuratedParamValue kMusicPreset[] = {
        { "modelSelect",        2.0f },   // MOSL (Music)
        { "correctionStrength", 85.0f },
        { "loudnessMode",        1.0f },  // Boost Only
        { "compressionSpeed",    1.0f },  // Slow
    };
}

void HearingCorrectionAUv2AudioProcessor::applyCuratedPreset (CuratedPreset preset)
{
    const auto* table = (preset == CuratedPreset::Speech) ? kSpeechPreset : kMusicPreset;
    const int   count = (preset == CuratedPreset::Speech)
                             ? (int) (sizeof (kSpeechPreset) / sizeof (kSpeechPreset[0]))
                             : (int) (sizeof (kMusicPreset) / sizeof (kMusicPreset[0]));

    for (int i = 0; i < count; ++i)
    {
        auto* param = parameters.getParameter (table[i].paramId);
        jassert (param != nullptr);
        if (param == nullptr)
            continue;

        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (table[i].rawValue));
        param->endChangeGesture();
    }
}

//==============================================================================
void HearingCorrectionAUv2AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Add headphone name to state
    state.setProperty ("headphoneName", selectedHeadphoneName, nullptr);

    // Add UI mode (Basic/Advanced) to state -- editor display preference, not a parameter
    state.setProperty ("uiMode", uiMode, nullptr);

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

            // Restore UI mode -- "Advanced" fallback covers both brand-new instances and
            // any session/project saved before this property existed.
            auto storedMode = parameters.state.getProperty ("uiMode", "Advanced").toString();
            if (storedMode == "Basic" || storedMode == "Advanced")
                uiMode = storedMode;
        }
    }
}

//==============================================================================
// AU presets (.aupreset). Same file format Logic and other AU hosts use, saved to
// the standard per-user AU presets location, so presets round-trip with hosts and
// are interchangeable with anything else that reads/writes EarFix .aupreset files.
//
// A .aupreset is a plist with plugin-identity keys (name/type/subtype/manufacturer/
// version) plus a "jucePluginState" CFData entry holding exactly the bytes JUCE's
// getStateInformation()/setStateInformation() produce/consume (see
// juce_audio_plugin_client_AU_1.mm SaveState/RestoreState, JUCE_STATE_DICTIONARY_KEY).

juce::File HearingCorrectionAUv2AudioProcessor::getPresetsDirectory()
{
   #if JUCE_MAC
    auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                   .getChildFile ("Library").getChildFile ("Audio").getChildFile ("Presets")
                   .getChildFile (JucePlugin_Manufacturer).getChildFile (JucePlugin_Name);
    dir.createDirectory();
    return dir;
   #else
    // .aupreset is an Apple Audio Unit format; only meaningful on macOS.
    return {};
   #endif
}

#if JUCE_MAC
void HearingCorrectionAUv2AudioProcessor::saveAUPreset (const juce::File& file, const juce::String& presetName)
{
    juce::MemoryBlock state;
    getStateInformation (state);

    EarFixCFPtr<CFMutableDictionaryRef> dict (
        CFDictionaryCreateMutable (kCFAllocatorDefault, 0,
                                   &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    auto setNumber = [&dict] (CFStringRef key, SInt32 value)
    {
        EarFixCFPtr<CFNumberRef> num (CFNumberCreate (kCFAllocatorDefault, kCFNumberSInt32Type, &value));
        CFDictionarySetValue (dict.get(), key, num.get());
    };
    auto setString = [&dict] (CFStringRef key, const juce::String& value)
    {
        auto cfStr = value.toCFString();
        CFDictionarySetValue (dict.get(), key, cfStr);
        CFRelease (cfStr);
    };

    setNumber (CFSTR ("version"), 0);
    setString (CFSTR ("name"), presetName);
    setNumber (CFSTR ("type"), (SInt32) JucePlugin_AUMainType);
    setNumber (CFSTR ("subtype"), (SInt32) JucePlugin_AUSubType);
    setNumber (CFSTR ("manufacturer"), (SInt32) JucePlugin_AUManufacturerCode);

    EarFixCFPtr<CFDataRef> stateData (
        CFDataCreate (kCFAllocatorDefault, (const UInt8*) state.getData(), (CFIndex) state.getSize()));
    EarFixCFPtr<CFStringRef> stateKey (
        CFStringCreateWithCString (kCFAllocatorDefault, "jucePluginState", kCFStringEncodingUTF8));
    CFDictionarySetValue (dict.get(), stateKey.get(), stateData.get());

    EarFixCFPtr<CFDataRef> xmlData (
        CFPropertyListCreateData (kCFAllocatorDefault, dict.get(),
                                  kCFPropertyListXMLFormat_v1_0, 0, nullptr));
    if (xmlData != nullptr)
    {
        juce::MemoryBlock xmlBlock (CFDataGetBytePtr (xmlData.get()),
                                    (size_t) CFDataGetLength (xmlData.get()));
        file.replaceWithData (xmlBlock.getData(), xmlBlock.getSize());
    }
}

bool HearingCorrectionAUv2AudioProcessor::loadAUPresetFile (const juce::File& file)
{
    juce::MemoryBlock fileData;
    if (! file.loadFileAsData (fileData))
        return false;

    EarFixCFPtr<CFDataRef> cfData (
        CFDataCreate (kCFAllocatorDefault, (const UInt8*) fileData.getData(), (CFIndex) fileData.getSize()));

    CFErrorRef error = nullptr;
    EarFixCFPtr<CFPropertyListRef> plist (
        CFPropertyListCreateWithData (kCFAllocatorDefault, cfData.get(),
                                      kCFPropertyListImmutable, nullptr, &error));
    if (error != nullptr)
        CFRelease (error);

    if (plist == nullptr || CFGetTypeID (plist.get()) != CFDictionaryGetTypeID())
        return false;

    auto* dict = (CFDictionaryRef) plist.get();
    EarFixCFPtr<CFStringRef> stateKey (
        CFStringCreateWithCString (kCFAllocatorDefault, "jucePluginState", kCFStringEncodingUTF8));

    CFDataRef stateData = nullptr;
    if (! CFDictionaryGetValueIfPresent (dict, stateKey.get(), (const void**) &stateData) || stateData == nullptr)
        return false;

    setStateInformation (CFDataGetBytePtr (stateData), (int) CFDataGetLength (stateData));

    // setStateInformation already restores the headphone profile via the
    // "headphoneName" property embedded in our state XML.
    return true;
}
#else
void HearingCorrectionAUv2AudioProcessor::saveAUPreset (const juce::File&, const juce::String&) {}
bool HearingCorrectionAUv2AudioProcessor::loadAUPresetFile (const juce::File&) { return false; }
#endif

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HearingCorrectionAUv2AudioProcessor();
}

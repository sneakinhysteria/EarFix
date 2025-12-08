/*
  ==============================================================================

    CorrectionModel.h
    Base interface for hearing correction models

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

//==============================================================================
// Audiogram data structure
struct AudiogramData
{
    static constexpr int numBands = 6;
    static constexpr std::array<float, numBands> frequencies = {
        250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f
    };

    std::array<float, numBands> hearingLoss = {};  // dB HL per frequency
};

//==============================================================================
// Compression parameters per band
struct CompressionParams
{
    float threshold = 50.0f;    // dB SPL where compression kicks in
    float ratio = 1.0f;         // Compression ratio (1.0 = no compression)
    float attackMs = 5.0f;      // Attack time in ms
    float releaseMs = 100.0f;   // Release time in ms
    float makeupGain = 0.0f;    // Post-compression gain in dB
};

//==============================================================================
// Base class for all correction models
class CorrectionModel
{
public:
    virtual ~CorrectionModel() = default;

    // Model identification
    virtual juce::String getName() const = 0;
    virtual juce::String getDescription() const = 0;

    // Core calculation: returns gain in dB for a given frequency and hearing loss
    virtual float calculateGain (float frequency, float hearingLossDb,
                                  float inputLevelDb = 65.0f) const = 0;

    // Compression parameters for a given frequency/hearing loss
    virtual CompressionParams getCompressionParams (float frequency,
                                                     float hearingLossDb) const = 0;

    // Whether this model uses compression
    virtual bool hasCompression() const = 0;

    // User-configurable parameters specific to this model
    virtual void setOverallGainOffset (float dB) { overallGainOffset = dB; }
    virtual float getOverallGainOffset() const { return overallGainOffset; }

protected:
    float overallGainOffset = 0.0f;  // User adjustment (-10 to +10 dB)

    // Helper: linear interpolation between audiogram frequencies
    static float interpolateHearingLoss (const AudiogramData& audiogram, float frequency)
    {
        const auto& freqs = AudiogramData::frequencies;
        const auto& loss = audiogram.hearingLoss;

        // Below lowest frequency
        if (frequency <= freqs[0])
            return loss[0];

        // Above highest frequency
        if (frequency >= freqs[AudiogramData::numBands - 1])
            return loss[AudiogramData::numBands - 1];

        // Find surrounding frequencies and interpolate
        for (int i = 0; i < AudiogramData::numBands - 1; ++i)
        {
            if (frequency >= freqs[i] && frequency <= freqs[i + 1])
            {
                float t = (frequency - freqs[i]) / (freqs[i + 1] - freqs[i]);
                return loss[i] + t * (loss[i + 1] - loss[i]);
            }
        }

        return 0.0f;
    }
};

//==============================================================================
// Model type enumeration for parameter selection
enum class CorrectionModelType
{
    HalfGain = 0,
    NAL = 1
};

inline juce::String getModelName (CorrectionModelType type)
{
    switch (type)
    {
        case CorrectionModelType::HalfGain: return "Half-Gain (Simple)";
        case CorrectionModelType::NAL:      return "NAL (with Compression)";
        default: return "Unknown";
    }
}

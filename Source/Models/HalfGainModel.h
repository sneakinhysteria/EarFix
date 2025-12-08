/*
  ==============================================================================

    HalfGainModel.h
    Simple half-gain correction model (no compression)

    Formula: Gain = 0.5 × HearingLoss
    This is the simplest prescription approach.

  ==============================================================================
*/

#pragma once

#include "CorrectionModel.h"

//==============================================================================
class HalfGainModel : public CorrectionModel
{
public:
    HalfGainModel() = default;

    juce::String getName() const override
    {
        return "Half-Gain";
    }

    juce::String getDescription() const override
    {
        return "Simple 0.5x hearing loss as gain. No compression. "
               "Predictable and transparent.";
    }

    float calculateGain (float /*frequency*/, float hearingLossDb,
                         float /*inputLevelDb*/) const override
    {
        // Simple half-gain rule: apply 50% of hearing loss as boost
        float gain = hearingLossDb * 0.5f;

        // Apply user's overall adjustment
        gain += overallGainOffset;

        // Clamp to reasonable range
        return juce::jlimit (0.0f, 40.0f, gain);
    }

    CompressionParams getCompressionParams (float /*frequency*/,
                                             float /*hearingLossDb*/) const override
    {
        // No compression in this model
        CompressionParams params;
        params.ratio = 1.0f;  // 1:1 = no compression
        return params;
    }

    bool hasCompression() const override
    {
        return false;
    }
};

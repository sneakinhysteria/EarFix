/*
  ==============================================================================

    NALModel.h
    NAL-inspired correction model with WDRC compression

    Based on National Acoustic Laboratories research:
    - Half-gain base with frequency-specific adjustments
    - Low frequencies: reduced gain (-3 to -5 dB) to avoid muddiness
    - High frequencies: reduced gain for steep losses (>60 dB)
    - Wide Dynamic Range Compression (WDRC) per band

  ==============================================================================
*/

#pragma once

#include "CorrectionModel.h"

//==============================================================================
class NALModel : public CorrectionModel
{
public:
    NALModel() = default;

    juce::String getName() const override
    {
        return "NAL";
    }

    juce::String getDescription() const override
    {
        return "NAL-inspired model with frequency shaping and WDRC compression. "
               "Optimized for speech intelligibility.";
    }

    float calculateGain (float frequency, float hearingLossDb,
                         float inputLevelDb) const override
    {
        // Start with half-gain rule
        float gain = hearingLossDb * 0.5f;

        // Apply frequency-specific adjustments
        gain += getFrequencyAdjustment (frequency, hearingLossDb);

        // Apply compression gain reduction for louder inputs
        if (inputLevelDb > compressionThreshold)
        {
            float compressionRatio = calculateCompressionRatio (hearingLossDb);
            float aboveThreshold = inputLevelDb - compressionThreshold;
            float compressedAbove = aboveThreshold / compressionRatio;
            gain -= (aboveThreshold - compressedAbove);
        }

        // Apply experience level factor (new users get reduced gain)
        gain *= experienceGainFactor;

        // Apply user's overall adjustment
        gain += overallGainOffset;

        // Clamp to reasonable range
        return juce::jlimit (0.0f, 40.0f, gain);
    }

    CompressionParams getCompressionParams (float /*frequency*/,
                                             float hearingLossDb) const override
    {
        CompressionParams params;

        params.threshold = compressionThreshold;
        params.ratio = calculateCompressionRatio (hearingLossDb);
        params.attackMs = attackMs;
        params.releaseMs = releaseMs;
        params.makeupGain = 0.0f;

        return params;
    }

    bool hasCompression() const override
    {
        return true;
    }

    // NAL-specific configurable parameters
    void setCompressionSpeed (bool fast)
    {
        if (fast)
        {
            attackMs = 5.0f;
            releaseMs = 50.0f;
        }
        else
        {
            attackMs = 10.0f;
            releaseMs = 150.0f;
        }
    }

    void setCompressionThreshold (float thresholdDb)
    {
        compressionThreshold = juce::jlimit (30.0f, 60.0f, thresholdDb);
    }

    // Experience level: 0 = New User (reduce gain), 1 = Some Experience, 2 = Experienced (full gain)
    // NAL-NL2 recommends reducing gain for new users to improve acceptance
    void setExperienceLevel (int level)
    {
        switch (level)
        {
            case 0:  experienceGainFactor = 0.7f;  break;  // New user: 70% of prescribed gain
            case 1:  experienceGainFactor = 0.85f; break;  // Some experience: 85%
            default: experienceGainFactor = 1.0f;  break;  // Experienced: full gain
        }
    }

    float getExperienceGainFactor() const { return experienceGainFactor; }

private:
    float compressionThreshold = 50.0f;  // dB SPL
    float attackMs = 5.0f;
    float releaseMs = 100.0f;
    float experienceGainFactor = 1.0f;   // 0.7 for new, 0.85 for some, 1.0 for experienced

    // Frequency-specific gain adjustments based on NAL research
    float getFrequencyAdjustment (float frequency, float hearingLossDb) const
    {
        // Low frequencies (250-500 Hz): reduce gain to avoid muddiness
        if (frequency <= 500.0f)
        {
            // -3 to -5 dB reduction, scaled by how low the frequency is
            float reduction = juce::jmap (frequency, 250.0f, 500.0f, -5.0f, -3.0f);
            return reduction;
        }

        // Mid frequencies (1-2 kHz): no adjustment, critical for speech
        if (frequency >= 1000.0f && frequency <= 2000.0f)
        {
            return 0.0f;
        }

        // High frequencies (4-8 kHz): reduce gain for steep/severe losses
        if (frequency >= 4000.0f)
        {
            if (hearingLossDb > 60.0f)
            {
                // Severe loss: significant reduction (diminishing returns)
                float reduction = juce::jmap (hearingLossDb, 60.0f, 80.0f, -5.0f, -10.0f);
                return reduction;
            }
            else if (hearingLossDb > 40.0f)
            {
                // Moderate-severe: slight reduction
                return -2.0f;
            }
        }

        return 0.0f;
    }

    // Calculate compression ratio based on hearing loss severity
    // Formula: CR = 1 + (hearingLoss / 40), clamped to 1.5-3.0
    float calculateCompressionRatio (float hearingLossDb) const
    {
        float ratio = 1.0f + (hearingLossDb / 40.0f);
        return juce::jlimit (1.5f, 3.0f, ratio);
    }
};

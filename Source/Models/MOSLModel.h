/*
  ==============================================================================

    MOSLModel.h
    Music-Optimized Specific Loudness (MOSL) correction model

    Designed specifically for music listening based on research from:
    - Fitz & McKinney (Starkey, 2010): Specific loudness restoration
    - Moore & Glasberg (Cambridge): Loudness perception models
    - Chasin et al.: Music program optimization guidelines

    Key principles:
    - Preserve spectral balance rather than reshape for speech
    - Restore specific loudness across frequency bands
    - Gentle compression (max 1.7:1) to preserve dynamics
    - Slow time constants for better sound quality
    - Enhanced high-frequency response for brightness/air
    - Preserved bass foundation for musical enjoyment

  ==============================================================================
*/

#pragma once

#include "CorrectionModel.h"

//==============================================================================
class MOSLModel : public CorrectionModel
{
public:
    MOSLModel() = default;

    juce::String getName() const override
    {
        return "MOSL";
    }

    juce::String getDescription() const override
    {
        return "Music-Optimized Specific Loudness model. Preserves spectral "
               "balance, gentle compression, enhanced highs. Best for music.";
    }

    float calculateGain (float frequency, float hearingLossDb,
                         float /*inputLevelDb*/) const override
    {
        // Get frequency-specific gain factor based on music perception research
        float gainFactor = getGainFactor (frequency);

        // Base gain calculation
        float gain = hearingLossDb * gainFactor;

        // Apply optional brightness boost for mild-moderate losses
        if (brightnessBoost && hearingLossDb < 60.0f)
        {
            gain += getBrightnessBoost (frequency, hearingLossDb);
        }

        // Apply bass preservation adjustment
        // Unlike NAL, we don't reduce low frequencies - music needs bass!
        // But we do apply a gentle rolloff for very severe low-freq losses
        // to avoid boominess from excessive amplification
        if (frequency <= 500.0f && hearingLossDb > 70.0f)
        {
            float excessLoss = hearingLossDb - 70.0f;
            float reduction = excessLoss * 0.1f;  // Very gentle: 1 dB per 10 dB excess
            gain -= reduction;
        }

        // Apply user's overall adjustment
        gain += overallGainOffset;

        // Clamp to reasonable range (same as NAL)
        return juce::jlimit (0.0f, 40.0f, gain);
    }

    CompressionParams getCompressionParams (float /*frequency*/,
                                             float hearingLossDb) const override
    {
        CompressionParams params;

        // Higher threshold than speech formulas - music is often played louder
        // and we want to preserve dynamics at normal listening levels
        params.threshold = compressionThreshold;

        // Gentle compression ratio: 1.0 to 1.7 max
        // Formula: 1.0 + (hearingLoss / 120), capped at 1.7
        // This gives nearly linear for mild losses, gentle compression for moderate
        params.ratio = calculateCompressionRatio (hearingLossDb);

        // Slow time constants for music - prevents "pumping" and artifacts
        params.attackMs = attackMs;
        params.releaseMs = releaseMs;
        params.makeupGain = 0.0f;

        return params;
    }

    bool hasCompression() const override
    {
        return true;
    }

    //==========================================================================
    // MOSL-specific configuration

    // Brightness boost adds subtle high-frequency enhancement
    void setBrightnessBoost (bool enabled)
    {
        brightnessBoost = enabled;
    }

    bool getBrightnessBoost() const { return brightnessBoost; }

    // Compression speed: even "fast" is slower than NAL for music
    void setCompressionSpeed (bool fast)
    {
        if (fast)
        {
            attackMs = 5.0f;
            releaseMs = 150.0f;   // Still slower than NAL's 50ms
        }
        else
        {
            attackMs = 10.0f;
            releaseMs = 300.0f;   // Very slow for best sound quality
        }
    }

    void setCompressionThreshold (float thresholdDb)
    {
        compressionThreshold = juce::jlimit (50.0f, 75.0f, thresholdDb);
    }

    float getCompressionThreshold() const { return compressionThreshold; }

    // Bass emphasis: 0 = neutral, 1 = enhanced, 2 = strong
    void setBassEmphasis (int level)
    {
        bassEmphasis = juce::jlimit (0, 2, level);
    }

    int getBassEmphasis() const { return bassEmphasis; }

private:
    float compressionThreshold = 65.0f;  // Higher than NAL's 50 dB
    float attackMs = 8.0f;               // Slightly slower attack
    float releaseMs = 200.0f;            // Much slower release
    bool brightnessBoost = false;        // Disabled by default to avoid excess gain
    int bassEmphasis = 1;                // Neutral bass by default

    //==========================================================================
    // Frequency-dependent gain factors based on:
    // - Equal-loudness contours (ISO 226)
    // - Music perception research (Fitz & McKinney)
    // - CAM2 approach (more HF gain than NAL)
    //
    // These factors determine what proportion of hearing loss becomes gain.
    // Unlike simple half-gain (0.5 everywhere), we vary by frequency to
    // better restore the perceived spectral balance for music.

    float getGainFactor (float frequency) const
    {
        // Frequency-specific insertion gain factors
        // Designed to restore specific loudness pattern for music
        // Reduced from original values to better align with NAL output levels

        if (frequency <= 250.0f)
        {
            // Low bass: 0.32 factor (was 0.40)
            // Matches NAL's approach of reduced low-freq gain
            float factor = 0.32f;
            if (bassEmphasis == 1) factor = 0.34f;
            else if (bassEmphasis == 2) factor = 0.36f;
            return factor;
        }

        if (frequency <= 500.0f)
        {
            // Upper bass / low mids: interpolate 0.32 -> 0.38
            float t = (frequency - 250.0f) / 250.0f;
            float baseFactor = 0.32f + t * 0.06f;
            if (bassEmphasis >= 1) baseFactor += 0.02f;
            return baseFactor;
        }

        if (frequency <= 1000.0f)
        {
            // Low-mids: interpolate 0.38 -> 0.42
            float t = (frequency - 500.0f) / 500.0f;
            return 0.38f + t * 0.04f;
        }

        if (frequency <= 2000.0f)
        {
            // Presence region: interpolate 0.42 -> 0.45
            // Core speech/music clarity range
            float t = (frequency - 1000.0f) / 1000.0f;
            return 0.42f + t * 0.03f;
        }

        if (frequency <= 4000.0f)
        {
            // Brilliance region: interpolate 0.45 -> 0.48
            // Slightly more than NAL for music brightness
            float t = (frequency - 2000.0f) / 2000.0f;
            return 0.45f + t * 0.03f;
        }

        if (frequency <= 8000.0f)
        {
            // Air/sparkle region: interpolate 0.48 -> 0.45
            // Tapering to avoid harshness
            float t = (frequency - 4000.0f) / 4000.0f;
            return 0.48f - t * 0.03f;
        }

        // Above 8 kHz: 0.40 (if extended response)
        return 0.40f;
    }

    //==========================================================================
    // Brightness boost: subtle high-frequency shelf
    // Based on research showing CAM2's HF advantage for music perception

    float getBrightnessBoost (float frequency, float hearingLossDb) const
    {
        if (frequency < 3000.0f)
            return 0.0f;

        // Maximum boost of 1.5 dB at 6-8 kHz for mild losses (reduced from 3 dB)
        // Tapers down for more severe losses to avoid harshness
        float maxBoost = juce::jmap (hearingLossDb, 0.0f, 60.0f, 1.5f, 0.0f);
        maxBoost = std::max (0.0f, maxBoost);

        // Shelf shape: ramps up from 3 kHz, plateaus at 6 kHz
        if (frequency < 6000.0f)
        {
            float t = (frequency - 3000.0f) / 3000.0f;
            return maxBoost * t;
        }

        return maxBoost;
    }

    //==========================================================================
    // Gentle compression ratio calculation
    // Much more conservative than NAL's formula

    float calculateCompressionRatio (float hearingLossDb) const
    {
        // Formula: 1.0 + (hearingLoss / 120)
        // This gives:
        //   0 dB loss  -> 1.0:1 (linear)
        //  30 dB loss  -> 1.25:1
        //  60 dB loss  -> 1.5:1
        //  84 dB loss  -> 1.7:1 (max)
        //
        // Compare to NAL which can go up to 3:1!

        float ratio = 1.0f + (hearingLossDb / 120.0f);
        return juce::jlimit (1.0f, 1.7f, ratio);
    }
};

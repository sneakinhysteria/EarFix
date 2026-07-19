/*
    Minimal JuceHeader.h shim for the offline measurement harness.

    The real model headers (HalfGain/NAL/MOSL) #include <JuceHeader.h> but only use
    a handful of juce utilities for their math. This shim provides just those so the
    models compile with a plain clang++ command, with no JUCE library link.

    IMPORTANT: this is only on the include path for tools/harness. The plugin build
    still uses the real JuceHeader.h from JuceLibraryCode.
*/

#pragma once

#include <string>
#include <cmath>
#include <array>
#include <algorithm>

namespace juce
{
    // --- jlimit / jmap (subset used by the models) ---
    template <typename T>
    inline T jlimit (T lowerBound, T upperBound, T valueToConstrain) noexcept
    {
        return valueToConstrain < lowerBound ? lowerBound
             : (valueToConstrain > upperBound ? upperBound : valueToConstrain);
    }

    template <typename T>
    inline T jmap (T sourceValue, T sourceRangeMin, T sourceRangeMax,
                   T targetRangeMin, T targetRangeMax) noexcept
    {
        return targetRangeMin
             + ((targetRangeMax - targetRangeMin) * (sourceValue - sourceRangeMin))
                 / (sourceRangeMax - sourceRangeMin);
    }

    // --- Minimal String (models only use it for name/description text) ---
    struct String
    {
        std::string s;
        String() = default;
        String (const char* c) : s (c) {}
        String (const std::string& v) : s (v) {}
        String operator+ (const String& o) const { return String (s + o.s); }
        const char* toRawUTF8() const { return s.c_str(); }
    };

    // --- Decibels (used by the harness, mirroring the plugin) ---
    struct Decibels
    {
        static float gainToDecibels (float gain) noexcept
        {
            return gain > 1.0e-9f ? 20.0f * std::log10 (gain) : -180.0f;
        }
        static float decibelsToGain (float dB) noexcept
        {
            return std::pow (10.0f, dB / 20.0f);
        }
    };
}

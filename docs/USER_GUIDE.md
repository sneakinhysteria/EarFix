# EarFix User Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Understanding Your Audiogram](#understanding-your-audiogram)
3. [Interface Overview](#interface-overview)
4. [Getting Started](#getting-started)
5. [Correction Models](#correction-models)
6. [Tips for Best Results](#tips-for-best-results)
7. [FAQ](#faq)

---

## Introduction

EarFix is a hearing correction plugin that applies personalized equalization based on your audiogram. Unlike generic "hearing enhancement" tools, EarFix uses your actual hearing test results to provide accurate, frequency-specific correction.

**Who is EarFix for?**
- Musicians and audio engineers with mild to moderate hearing loss
- Anyone who wants to enjoy music with personalized correction
- Those who want to preview how hearing aids might affect their listening experience

**What EarFix is NOT:**
- A replacement for professional hearing aids
- A medical device
- Suitable for severe or profound hearing loss

---

## Understanding Your Audiogram

An audiogram is a graph showing your hearing thresholds at different frequencies. You'll need audiogram data from a professional hearing test to use EarFix effectively.

### Reading Your Audiogram

Your audiogram shows:
- **Frequencies** (Hz): Typically 250, 500, 1000, 2000, 4000, and 8000 Hz
- **Hearing Level** (dB HL): The softest sound you can hear at each frequency

**Hearing Level Scale:**
| dB HL | Classification |
|-------|----------------|
| -10 to 15 | Normal hearing |
| 16 to 25 | Slight loss |
| 26 to 40 | Mild loss |
| 41 to 55 | Moderate loss |
| 56 to 70 | Moderately severe |
| 71 to 90 | Severe loss |
| 90+ | Profound loss |

### Example Audiogram Values

A typical age-related hearing loss might look like:

| Frequency | Right Ear | Left Ear |
|-----------|-----------|----------|
| 250 Hz | 10 dB | 15 dB |
| 500 Hz | 15 dB | 20 dB |
| 1000 Hz | 20 dB | 25 dB |
| 2000 Hz | 30 dB | 35 dB |
| 4000 Hz | 45 dB | 50 dB |
| 8000 Hz | 55 dB | 60 dB |

This pattern (worse hearing at high frequencies) is common and well-suited for EarFix correction.

---

## Interface Overview

### Controls Panel (Top Section)

**Left Column (Model & Options):**
- **Model**: Select correction algorithm (Half-Gain, NAL Speech, or MOSL Music)
- **Speed**: Compression speed for NAL/MOSL models (Fast/Slow)
- **Level**: Experience level for NAL/MOSL models (New/Some/Experienced)

**Right Column (Signal Flow):**
- **Input Meters**: Stereo level meters showing input signal
- **Strength**: Correction intensity fader (0-100%)
- **Output**: Master output gain fader (-24 to +24 dB)
- **Output Meters**: Stereo level meters showing output signal
- **Auto Gain**: Hold to automatically match output level to input level

### Audiogram Section (Bottom)

**Right Ear (Left Chart):**
- Red curve and markers
- Toggle switch to enable/disable
- Click markers to adjust values

**Left Ear (Right Chart):**
- Blue curve and markers
- Toggle switch to enable/disable
- Click markers to adjust values

**Chart Axes:**
- X-axis: Frequency (250 Hz to 8 kHz)
- Y-axis: Hearing Level (-20 to 120 dB HL)

---

## Getting Started

### Step 1: Enter Your Audiogram

1. Locate your audiogram results (from your audiologist or hearing test)
2. For each frequency (250, 500, 1k, 2k, 4k, 8k Hz):
   - Click and drag the marker on the chart, OR
   - Click the marker and type a value directly
3. Repeat for both ears

**Tip:** If you only have values for some frequencies, estimate the missing ones based on nearby values.

### Step 2: Choose a Correction Model

**Start with Half-Gain** if:
- You have mild hearing loss (under 40 dB)
- You want transparent, natural sound
- You're new to hearing correction

**Try NAL (Speech)** if:
- You have moderate hearing loss (40-70 dB)
- You're primarily listening to speech/podcasts/audiobooks
- You want clinically-validated correction

**Try MOSL (Music)** if:
- You're primarily listening to music
- You want to preserve musical dynamics
- NAL sounds too "compressed" or "pumpy" on music

### Step 3: Adjust Strength

Start with the Strength slider at 50% and adjust to taste:
- **Lower (25-50%)**: Subtle correction, more natural
- **Medium (50-75%)**: Balanced correction
- **Higher (75-100%)**: Maximum correction, may sound processed

### Step 4: Set Output Level

Adjust the Output slider to match your original listening level. Hearing correction adds gain, so you may need to reduce output to avoid clipping.

---

## Correction Models

### Half-Gain Model

**How it works:**
Applies gain equal to half your hearing threshold at each frequency.

**Formula:** `Gain = Hearing_Loss × 0.5`

**Example:** If your hearing loss at 4kHz is 40 dB, Half-Gain applies +20 dB at 4kHz.

**Best for:**
- Mild to moderate hearing loss
- Music listening
- When you want minimal processing

**Characteristics:**
- Clean, transparent sound
- Linear processing (no compression)
- Simple and predictable

### NAL (Speech) Model

**How it works:**
Based on the National Acoustic Laboratories' Non-Linear 2 prescription formula, which is used clinically for fitting hearing aids.

**Features:**
- Frequency-dependent compression
- Loudness normalization
- Accounts for "recruitment" (abnormal loudness growth)
- Optimized for speech intelligibility

**NAL Options:**

**Compression Speed:**
- **Fast**: Quick response to level changes (5ms attack, 50ms release)
- **Slow**: Smoother response (10ms attack, 150ms release)

**Experience Level:**
- **New**: More conservative correction (70% of prescribed gain)
- **Some**: Moderate correction (85% of prescribed gain)
- **Experienced**: Full prescription correction (100%)

**Best for:**
- Moderate to moderately-severe hearing loss
- Speech, podcasts, audiobooks
- Those accustomed to hearing aids

### MOSL (Music) Model

**How it works:**
Music-Optimized Specific Loudness model based on research from Fitz & McKinney (Starkey), Moore & Glasberg (Cambridge), and Chasin's music program optimization guidelines.

**Features:**
- Preserves spectral balance rather than reshaping for speech
- Gentle compression (1.0:1 to 1.7:1 maximum)
- Slower time constants to prevent "pumping" artifacts
- Optional brightness boost for enhanced high-frequency air
- Preserved bass foundation for musical enjoyment

**MOSL Options:**

**Compression Speed:**
- **Fast**: 5ms attack, 150ms release (still slower than NAL)
- **Slow**: 10ms attack, 300ms release (best for critical listening)

**Experience Level:**
- **New/Some**: Brightness boost disabled
- **Experienced**: Brightness boost enabled (subtle HF enhancement)

**Best for:**
- Music listening and production
- Users who find NAL too "compressed" sounding
- Preserving musical dynamics and punch

---

## Tips for Best Results

### General Tips

1. **Start Conservative**: Begin with lower strength and increase gradually
2. **Give Time to Adjust**: Your brain needs time to adapt to corrected audio
3. **Compare A/B**: Toggle ears on/off to compare corrected vs. original
4. **Protect Your Hearing**: Don't use correction as an excuse to listen louder

### For Music Listening

- Use the **MOSL (Music)** model for best results
- Alternatively, **Half-Gain** for maximum transparency
- Set **Strength** to 50-75%
- Use **Auto Gain** to quickly match levels
- Consider using on the master bus or headphone output

### For Mixing/Mastering

- **Caution**: EarFix changes your perception of the frequency balance
- Use for reference only, not for critical mixing decisions
- Consider A/B testing with correction on/off
- Make mix decisions with correction OFF

### Headphones vs. Speakers

- EarFix works with both, but results may differ
- Headphones provide more consistent correction
- Speaker response and room acoustics add variables

---

## FAQ

### Can I use EarFix instead of hearing aids?

No. EarFix is designed for music and audio listening, not for daily communication. Hearing aids provide features like directional microphones, feedback cancellation, and all-day wearability that software cannot replicate.

### Why does the corrected audio sound "bright" or "harsh"?

You may be perceiving frequencies you haven't heard clearly in a while. Try:
- Reducing the Strength slider
- Giving your ears time to adjust
- Using the NAL model with "New User" experience level

### Can I save my audiogram settings?

Yes! Your settings are saved with your DAW project. You can also save the plugin state as a preset in most DAWs.

### How accurate is the NAL-NL2 implementation?

EarFix implements a simplified version of the NAL-NL2 formula suitable for audio processing. For clinical accuracy, consult an audiologist.

### What's the difference between NAL and MOSL?

**NAL (Speech)** is optimized for speech intelligibility with faster compression designed to handle the dynamic range of conversation. **MOSL (Music)** uses gentler compression with slower time constants to preserve musical dynamics and avoid the "pumping" effect that can occur with speech-optimized algorithms on music.

### How does Auto Gain work?

Hold the Auto Gain button while audio is playing. EarFix compares the input and output levels and automatically adjusts the output gain to match them. This is useful for A/B comparing corrected vs. uncorrected audio at the same perceived volume.

### Does EarFix add latency?

EarFix adds minimal latency (a few samples for the EQ processing). It should not cause noticeable delay in most applications.

### Can I use EarFix on mobile devices?

The AUv3 version works with iOS/iPadOS apps that support Audio Unit extensions. Performance may vary by device.

### My hearing loss is asymmetrical. Can EarFix handle that?

Yes! Enter different values for each ear. You can also disable correction for one ear entirely using the toggle switches.

### Is my audiogram data private?

Yes. EarFix stores settings locally in your DAW project. No data is sent to any server.

---

## Getting Help

- **GitHub Issues**: [Report bugs or request features](https://github.com/BrighterRealities/EarFix/issues)
- **Discussions**: [Ask questions and share tips](https://github.com/BrighterRealities/EarFix/discussions)

---

*EarFix is not a medical device. Always consult a qualified audiologist for hearing health concerns.*

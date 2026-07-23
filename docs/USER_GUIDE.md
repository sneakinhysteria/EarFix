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

**Intended use:** EarFix is meant for hearing loss significant enough that a hearing aid would normally be recommended — the same bar you'd use to decide you need prescription glasses in the first place. It is not a general listening "enhancer" for normal hearing, and it is not meant to address ordinary, mild age-related hearing decline unless that decline has reached a level where a hearing aid would actually be indicated. If you're unsure where your hearing falls, get a professional hearing test. See [Intended Use](../README.md#intended-use) in the main README.

**Who is EarFix for?**
- People with hearing loss significant enough that a hearing aid would normally be recommended
- Musicians and audio engineers who need that level of correction while working

**What EarFix is NOT:**
- A replacement for professional hearing aids
- A medical device
- A casual listening enhancement, or a way to compensate for normal age-related hearing changes that don't rise to the level of needing a hearing aid
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

Cards read top-to-bottom in signal order: your **audiogram** (the input data) feeds **hearing loss correction**, which runs through **headphone correction** last, as a final tone-shaping step.

### Top Bar

- **Basic / Advanced**: Switch between a simplified view (two preset buttons) and the full control set. Both drive the exact same underlying settings — switching modes never changes or loses your configuration, it only changes what's visible.
- **Save / Load**: Save or load the plugin's entire state (audiogram, model, all settings) as a standard `.aupreset` file, interchangeable with Logic and other AU hosts.

### Audiogram Section (Top)

**Right Ear (Left Chart):**
- Red curve and markers
- Toggle switch to enable/disable
- Click markers to adjust values

**Left Ear (Right Chart):**
- Blue curve and markers
- Toggle switch to enable/disable
- Click markers to adjust values

**Link icon** (between the two charts): links both ears' enable toggles together, so disabling one also disables the other. Click again to unlink.

**Chart Axes:**
- X-axis: Frequency (250 Hz to 8 kHz)
- Y-axis: Hearing Level (-20 to 120 dB HL)

The green line and shaded fill show your *aided* threshold — your hearing loss curve after the currently-applied correction — so you always have a visual baseline for what the correction is doing to each band, live as you adjust settings.

### Hearing Loss Correction (Middle Section)

**Basic mode:**
- **Speech** / **Music**: one-click, research-grounded starting points (see [Correction Models](#correction-models) for what each sets)
- A summary line under the buttons shows exactly what the active preset set (model, strength, compression speed)

**Advanced mode — left column:**
- **Model**: Select correction algorithm (Half-Gain, NAL Speech, or MOSL Music)
- **Speed**: Compression speed for NAL/MOSL models (Fast/Slow)
- **Loudness**: How the curve is kept from running louder than input (Centered / Boost Only)

**Right column (both modes):**
- **Input Meters**: Stereo level meters showing input signal
- **Strength** (Advanced only): Correction intensity fader (0-100%)
- **Output**: Master output gain fader (-48 to +24 dB). The wide negative range covers the large boosts hearing correction can add — always trim here rather than reducing your source/system volume, which loses quality before the signal reaches EarFix.
- **Auto**: Click to set Output automatically so the corrected signal matches the input's loudness, measured from the actual audio while it plays — the fader moves to the computed value and stays adjustable afterward.
- **Output Meters**: Stereo level meters showing output signal

### Headphone Correction (Bottom Section)

Runs last in the signal chain, as a final linear touch-up on the corrected signal for your specific headphones, based on measurements from the [AutoEQ](https://github.com/jaakkopasanen/AutoEq) project, [squig.link](https://squig.link), and [oratory1990](https://www.reddit.com/r/oratory1990/). It's loudness-neutral — enabling it changes tone, not overall volume.

- **Headphone Selector**: Choose your headphone model from the dropdown
- **Enable Toggle**: Turn headphone EQ correction on/off
- **Import**: Paste a Parametric EQ profile (AutoEQ, REW, or EqualizerAPO format) to add a headphone not already in the database
- **Refresh**: Reload the headphone profiles from disk

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

In **Basic** mode, click **Speech** or **Music** for a one-click, research-grounded starting point, then skip to [Step 4](#step-4-set-output-level). To pick the model yourself, switch to **Advanced**:

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

### Step 3: Adjust Strength (Advanced mode)

Strength defaults to 85% — calibrated against real hearing-aid fitting data as a reasonable starting point — and can be adjusted to taste:
- **Lower (25-50%)**: Subtle correction, more natural
- **Medium (50-75%)**: Balanced correction
- **Higher (75-100%)**: Maximum correction, may sound processed

### Step 4: Set Output Level

Click **Auto** to set the Output level automatically — it matches the corrected signal's loudness back to the input so the overall volume feels unchanged. Then fine-tune the Output slider to taste. Hearing correction adds a lot of gain (especially in Boost Only mode, which never cuts), so the Output fader reaches down to -48 dB; always trim here rather than turning down your source or system volume, which degrades quality before the signal even reaches EarFix.

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
- Frequency-dependent gain shaping (reduced bass to avoid muddiness, reduced treble for severe loss)
- WDRC compression (up to 3:1) that accounts for "recruitment" (abnormal loudness growth)
- Fast compression speed by default, preserving speech consonant transients
- Optimized for speech intelligibility

(The **Loudness** setting — Centered or Boost Only — is a separate, shared control that applies to every model, not something specific to NAL; see [Interface Overview](#interface-overview).)

**NAL Options:**

**Compression Speed:**
- **Fast**: Quick response to level changes (5ms attack, 50ms release)
- **Slow**: Smoother response (10ms attack, 150ms release)

Overall correction amount is set continuously with the **Strength** fader (there is no separate experience-level preset).

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

Overall correction amount is set continuously with the **Strength** fader.

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

- Use the **MOSL (Music)** model for best results (or click **Music** in Basic mode)
- Alternatively, **Half-Gain** for maximum transparency
- The default 85% Strength is a good starting point; lower it if things feel too processed
- Play some audio and click **Auto** to quickly match levels
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
- Switching to the MOSL (Music) model, which shapes more gently than NAL

### Can I save my audiogram settings?

Yes! Your settings are saved with your DAW project. You can also save the plugin state as a preset in most DAWs.

### How accurate is the NAL-NL2 implementation?

EarFix implements a simplified version of the NAL-NL2 formula suitable for audio processing. For clinical accuracy, consult an audiologist.

### What's the difference between NAL and MOSL?

**NAL (Speech)** is optimized for speech intelligibility with faster compression designed to handle the dynamic range of conversation. **MOSL (Music)** uses gentler compression with slower time constants to preserve musical dynamics and avoid the "pumping" effect that can occur with speech-optimized algorithms on music.

### How does Auto work?

Play some audio, then click the **Auto** button next to the Output fader. EarFix measures the actual loudness of the processed signal against the input (a short, real-time comparison, not an estimate) and sets the Output gain to cancel the difference, so corrected and uncorrected audio sit at the same perceived loudness. The fader visibly jumps to the computed value and stays adjustable, so you can fine-tune from there. Because it measures the real signal, it needs audio playing to give a meaningful result — clicking it on silence won't change anything useful.

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

- **GitHub Issues**: [Report bugs or request features](https://github.com/sneakinhysteria/EarFix/issues)
- **Discussions**: [Ask questions and share tips](https://github.com/sneakinhysteria/EarFix/discussions)

---

*EarFix is not a medical device. Always consult a qualified audiologist for hearing health concerns.*

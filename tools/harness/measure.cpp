/*
    Offline frequency-response harness for EarFix correction models.

    Reuses the REAL model headers (HalfGain/NAL/MOSL) for the prescriptive gain, and
    mirrors the exact centering + phase-compensated crossover + WDRC from
    PluginProcessor.cpp so the measured curves reflect the shipping DSP.

    The Linkwitz-Riley crossover here is a standard LR4 (two cascaded Butterworth
    biquads); its all-pass output is defined as LP4 + HP4, the same identity JUCE's
    LinkwitzRileyFilter uses, so the reconstruction test is valid.

    Build/run:  see tools/harness/run.sh
*/

#include <cstdio>
#include <cmath>
#include <vector>
#include <random>

#include "../../Source/Models/HalfGainModel.h"
#include "../../Source/Models/NALModel.h"
#include "../../Source/Models/MOSLModel.h"

static constexpr float kPi = 3.14159265358979323846f;

//==============================================================================
// Standard RBJ biquad (transposed direct form II)
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    float process (float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void setLowpass (float fc, float fs)
    {
        const float w0 = 2.0f * kPi * fc / fs;
        const float c = std::cos (w0), s = std::sin (w0);
        const float alpha = s / (2.0f * 0.70710678f);      // Butterworth Q
        const float a0 = 1.0f + alpha;
        b0 = ((1.0f - c) * 0.5f) / a0;
        b1 = (1.0f - c) / a0;
        b2 = ((1.0f - c) * 0.5f) / a0;
        a1 = (-2.0f * c) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    void setHighpass (float fc, float fs)
    {
        const float w0 = 2.0f * kPi * fc / fs;
        const float c = std::cos (w0), s = std::sin (w0);
        const float alpha = s / (2.0f * 0.70710678f);
        const float a0 = 1.0f + alpha;
        b0 = ((1.0f + c) * 0.5f) / a0;
        b1 = (-(1.0f + c)) / a0;
        b2 = ((1.0f + c) * 0.5f) / a0;
        a1 = (-2.0f * c) / a0;
        a2 = (1.0f - alpha) / a0;
    }
};

//==============================================================================
// LR4 filter (mirrors juce::dsp::LinkwitzRileyFilter: allpass == lowpass + highpass)
struct LRFilter
{
    enum Type { lowpass, highpass, allpass };
    Type type = lowpass;
    Biquad lp1, lp2, hp1, hp2;

    void configure (Type t, float fc, float fs)
    {
        type = t;
        lp1.setLowpass (fc, fs);  lp2.setLowpass (fc, fs);
        hp1.setHighpass (fc, fs); hp2.setHighpass (fc, fs);
        reset();
    }

    void reset() { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); }

    float process (float x)
    {
        if (type == lowpass)  return lp2.process (lp1.process (x));
        if (type == highpass) return hp2.process (hp1.process (x));
        return lp2.process (lp1.process (x)) + hp2.process (hp1.process (x)); // allpass
    }
};

//==============================================================================
// Correction engine — mirrors PluginProcessor.cpp for one channel
struct Engine
{
    static constexpr int NB = 6;   // audiogram bands
    static constexpr int NC = 5;   // crossovers
    const float freqs[NB] = { 250, 500, 1000, 2000, 4000, 8000 };
    const float cross[NC] = { 354, 707, 1414, 2828, 5657 };

    static constexpr float kSoftReferenceLevelDb = 25.0f;
    static constexpr float kWDRCKneeDb = -40.0f;

    LRFilter lp[NC], hp[NC], ap[NB][NC];

    struct WD { float env = 0, sg = 1, target = 0, ratio = 1; };
    WD wd[NB];

    float attackC = 0, releaseC = 0, smoothC = 0, fs = 48000;
    bool modelComp = false;

    void prepare (float sampleRate)
    {
        fs = sampleRate;
        for (int i = 0; i < NC; ++i)
        {
            lp[i].configure (LRFilter::lowpass,  cross[i], fs);
            hp[i].configure (LRFilter::highpass, cross[i], fs);
        }
        for (int band = 0; band < NB; ++band)
            for (int k = band + 1; k < NC; ++k)
                ap[band][k].configure (LRFilter::allpass, cross[k], fs);

        // Fast compression time constants (plugin default)
        attackC  = std::exp (-1.0f / (fs * 5.0f  / 1000.0f));
        releaseC = std::exp (-1.0f / (fs * 50.0f / 1000.0f));
        smoothC  = std::exp (-1.0f / (fs * 0.01f));
    }

    void resetState()
    {
        for (int i = 0; i < NC; ++i) { lp[i].reset(); hp[i].reset(); }
        for (int band = 0; band < NB; ++band)
            for (int k = band + 1; k < NC; ++k) ap[band][k].reset();
        for (int b = 0; b < NB; ++b) { wd[b].env = 0.0f; wd[b].sg = 1.0f; }
    }

    // normMode: 0 = arithmetic dB mean, 1 = energy (pink-RMS) mean, 2 = perceptual
    // (K-weighted) centered [shipped as "Centered"], 3 = Boost Only (global scale-down,
    // never cuts a band -- shipped default), 4 = Boost Only (Anchored, relative to the
    // curve's own least-affected band)
    void computeTargets (const CorrectionModel& model, const float audiogram[NB],
                         float strength, float maxBoost, int normMode)
    {
        modelComp = model.hasCompression();

        // Bandwidth (octave) weights for each crossover band over 20 Hz .. 20 kHz
        const float edges[NB + 1] = { 20, 354, 707, 1414, 2828, 5657, 20000 };
        float bw[NB];
        for (int i = 0; i < NB; ++i) bw[i] = std::log2 (edges[i+1] / edges[i]);

        // K-weighting (BS.1770) approx power gain at each band center, relative
        const float kDb[NB] = { 0.0f, 0.5f, 1.5f, 3.0f, 4.0f, 4.0f };

        // Per-band loudness weights
        float w[NB];
        for (int i = 0; i < NB; ++i)
        {
            if (normMode == 2 || normMode == 3 || normMode == 4) w[i] = bw[i] * std::pow (10.0f, kDb[i] / 10.0f);
            else                                                  w[i] = bw[i];
        }
        double wsumAll = 0.0; for (int i = 0; i < NB; ++i) wsumAll += w[i];

        // Prescriptive gain, pre-scaled by strength
        float g[NB];
        for (int i = 0; i < NB; ++i)
        {
            const float loss = std::max (0.0f, audiogram[i]);
            g[i] = model.calculateGain (freqs[i], loss, kSoftReferenceLevelDb) * strength;
        }

        if (normMode == 3 || normMode == 4)
        {
            // Boost Only: every band is >= 0 by construction, so no band is ever cut.
            // A curve that only adds gain can't be scaled to exact loudness parity
            // (any positive scale strictly increases weighted loudness above flat), so
            // instead scale the whole curve down (shape preserved) only if needed so
            // its loudest band never exceeds Max Boost.
            //
            // Anchored (mode 4) additionally subtracts the curve's own minimum first,
            // so the best-hearing band lands at 0 (untouched) instead of getting its
            // own absolute prescribed gain -- still >= 0 after subtraction by
            // construction, so the never-cut guarantee holds either way.
            float base[NB];
            for (int i = 0; i < NB; ++i) base[i] = g[i];
            if (normMode == 4)
            {
                float anchor = g[0];
                for (int i = 1; i < NB; ++i) anchor = std::min (anchor, g[i]);
                for (int i = 0; i < NB; ++i) base[i] = g[i] - anchor;
            }
            float peak = base[0];
            for (int i = 1; i < NB; ++i) peak = std::max (peak, base[i]);
            const float k = (peak > maxBoost && peak > 0.0f) ? (maxBoost / peak) : 1.0f;
            for (int i = 0; i < NB; ++i)
            {
                wd[i].target = juce::jlimit (0.0f, maxBoost, base[i] * k);
                const float loss = std::max (0.0f, audiogram[i]);
                wd[i].ratio = modelComp ? model.getCompressionParams (freqs[i], loss).ratio : 1.0f;
            }
            return;
        }

        // Normalisation offset
        float offset;
        if (normMode == 0)
        {
            float sum = 0.0f; for (int i = 0; i < NB; ++i) sum += g[i];
            offset = sum / (float) NB;                       // dB mean
        }
        else
        {
            double p = 0.0, wsum = 0.0;                       // weighted power mean
            for (int i = 0; i < NB; ++i) { p += w[i] * std::pow (10.0, g[i] / 10.0); wsum += w[i]; }
            offset = 10.0f * std::log10 ((float) (p / wsum));
        }

        for (int i = 0; i < NB; ++i)
        {
            wd[i].target = juce::jlimit (-maxBoost, maxBoost, g[i] - offset);
            const float loss = std::max (0.0f, audiogram[i]);
            wd[i].ratio = modelComp ? model.getCompressionParams (freqs[i], loss).ratio : 1.0f;
        }
    }

    float calcWDRC (float inDb, float soft, float ratio) const
    {
        if (inDb <= kWDRCKneeDb || ratio <= 1.0f) return soft;
        const float t = juce::jlimit (0.0f, 1.0f, (inDb - kWDRCKneeDb) / (0.0f - kWDRCKneeDb));
        const float rem = 1.0f - t * (1.0f - 1.0f / ratio);
        return soft * rem;
    }

    float bandGain (WD& st, float bandSample)
    {
        const float level = std::fabs (bandSample);
        const float coeff = (level > st.env) ? attackC : releaseC;
        st.env = st.env * coeff + level * (1.0f - coeff);
        const float inDb = juce::Decibels::gainToDecibels (st.env + 1e-6f);
        const float gDb  = modelComp ? calcWDRC (inDb, st.target, st.ratio) : st.target;
        const float gLin = juce::Decibels::decibelsToGain (gDb);
        st.sg = st.sg * smoothC + gLin * (1.0f - smoothC);
        return st.sg;
    }

    float process (float x)
    {
        float bands[NB];
        bands[0] = lp[0].process (x);
        float high = hp[0].process (x);

        for (int k = 1; k < NC; ++k)
        {
            const float lo = lp[k].process (high);
            const float nh = hp[k].process (high);
            for (int j = 0; j < k; ++j) bands[j] = ap[j][k].process (bands[j]);
            bands[k] = lo;
            high = nh;
        }
        bands[NC] = high;

        float out = 0.0f;
        for (int b = 0; b < NB; ++b) out += bands[b] * bandGain (wd[b], bands[b]);
        return out;
    }
};

//==============================================================================
// Steady-state sine magnitude at a given frequency and level
static float measureGainDb (Engine& e, float freq, float ampDb)
{
    e.resetState();
    const float amp = juce::Decibels::decibelsToGain (ampDb);
    const int N = (int) (e.fs * 0.8f);
    const int start = (int) (N * 0.6f);   // measure after filters + WDRC settle

    double inSq = 0, outSq = 0; int cnt = 0;
    for (int n = 0; n < N; ++n)
    {
        const float x = amp * std::sin (2.0f * kPi * freq * (float) n / e.fs);
        const float y = e.process (x);
        if (n >= start) { inSq += (double) x * x; outSq += (double) y * y; ++cnt; }
    }
    const double inRms = std::sqrt (inSq / cnt), outRms = std::sqrt (outSq / cnt);
    return 20.0f * std::log10 ((float) (outRms / (inRms + 1e-20)));
}

// Broadband pink-noise loudness change (dB). ~0 => loudness preserved.
static float measurePinkDeltaDb (Engine& e, float rmsDb)
{
    e.resetState();
    std::mt19937 rng (12345);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);

    const int N = (int) (e.fs * 2.0f);
    const int start = (int) (N * 0.5f);
    std::vector<float> pink (N);

    float b0=0,b1=0,b2=0,b3=0,b4=0,b5=0,b6=0;
    for (int n = 0; n < N; ++n)
    {
        const float w = dist (rng);
        b0 = 0.99886f*b0 + w*0.0555179f;
        b1 = 0.99332f*b1 + w*0.0750759f;
        b2 = 0.96900f*b2 + w*0.1538520f;
        b3 = 0.86650f*b3 + w*0.3104856f;
        b4 = 0.55000f*b4 + w*0.5329522f;
        b5 = -0.7616f*b5 - w*0.0168980f;
        pink[n] = (b0+b1+b2+b3+b4+b5+b6 + w*0.5362f) * 0.11f;
        b6 = w*0.115926f;
    }

    // normalise input RMS (over measurement region) to target
    double sq = 0; for (int n = start; n < N; ++n) sq += (double) pink[n]*pink[n];
    const float curRms = (float) std::sqrt (sq / (N - start));
    const float scale = juce::Decibels::decibelsToGain (rmsDb) / (curRms + 1e-20f);

    double inSq = 0, outSq = 0;
    for (int n = 0; n < N; ++n)
    {
        const float x = pink[n] * scale;
        const float y = e.process (x);
        if (n >= start) { inSq += (double) x*x; outSq += (double) y*y; }
    }
    const double inRms = std::sqrt (inSq/(N-start)), outRms = std::sqrt (outSq/(N-start));
    return 20.0f * std::log10 ((float) (outRms / (inRms + 1e-20)));
}

// BS.1770 K-weighting (two biquads, coefficients for 48 kHz)
struct KWeighting
{
    Biquad shelf, hp;
    KWeighting()
    {
        shelf.b0 = 1.53512485958697f;  shelf.b1 = -2.69169618940638f; shelf.b2 = 1.19839281085285f;
        shelf.a1 = -1.69065929318241f; shelf.a2 = 0.73248077421585f;
        hp.b0 = 1.0f; hp.b1 = -2.0f; hp.b2 = 1.0f;
        hp.a1 = -1.99004745483398f; hp.a2 = 0.99007225036621f;
    }
    void reset() { shelf.reset(); hp.reset(); }
    float process (float x) { return hp.process (shelf.process (x)); }
};

// K-weighted (perceived) loudness change through the engine, pink noise input.
static float measurePinkDeltaKWeighted (Engine& e, float rmsDb)
{
    e.resetState();
    std::mt19937 rng (12345);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);

    const int N = (int) (e.fs * 2.0f);
    const int start = (int) (N * 0.5f);

    KWeighting kin, kout;
    float b0=0,b1=0,b2=0,b3=0,b4=0,b5=0,b6=0;
    double inSq = 0, outSq = 0;
    for (int n = 0; n < N; ++n)
    {
        const float w = dist (rng);
        b0 = 0.99886f*b0 + w*0.0555179f; b1 = 0.99332f*b1 + w*0.0750759f;
        b2 = 0.96900f*b2 + w*0.1538520f; b3 = 0.86650f*b3 + w*0.3104856f;
        b4 = 0.55000f*b4 + w*0.5329522f; b5 = -0.7616f*b5 - w*0.0168980f;
        const float x = (b0+b1+b2+b3+b4+b5+b6 + w*0.5362f) * 0.11f;
        b6 = w*0.115926f;

        const float y  = e.process (x);
        const float xk = kin.process (x);
        const float yk = kout.process (y);
        if (n >= start) { inSq += (double) xk*xk; outSq += (double) yk*yk; }
    }
    return 10.0f * std::log10 ((float) (outSq / (inSq + 1e-20)));
}

//==============================================================================
static const float kSweep[] = {
    20, 31.5f, 63, 125, 250, 354, 500, 707, 1000, 1414,
    2000, 2828, 4000, 5657, 8000, 11000, 16000
};
static constexpr int kNumSweep = sizeof (kSweep) / sizeof (kSweep[0]);

static void runModel (const char* label, CorrectionModel& model,
                      const float audiogram[6], float fs, float strength, float maxBoost)
{
    const float bf[6] = { 250,500,1000,2000,4000,8000 };

    printf ("\n================================================================\n");
    printf (" MODEL: %s   (strength %.0f%%, maxBoost %.0f dB, comp=%s)\n",
            label, strength * 100.0f, maxBoost, model.hasCompression() ? "yes" : "no");
    printf ("================================================================\n");

    // Compare the five normalisation/loudness modes
    Engine eDb;  eDb.prepare  (fs); eDb.computeTargets  (model, audiogram, strength, maxBoost, 0);
    Engine eEn;  eEn.prepare  (fs); eEn.computeTargets  (model, audiogram, strength, maxBoost, 1);
    Engine eK;   eK.prepare   (fs); eK.computeTargets   (model, audiogram, strength, maxBoost, 2);
    Engine eBO;  eBO.prepare  (fs); eBO.computeTargets  (model, audiogram, strength, maxBoost, 3);
    Engine eAnc; eAnc.prepare (fs); eAnc.computeTargets (model, audiogram, strength, maxBoost, 4);

    printf ("                  ");
    for (int i = 0; i < 6; ++i) printf ("%6.0fHz", bf[i]);
    printf ("   RMS d    K-wtd d\n");
    printf ("   dB-mean center ");
    for (int i = 0; i < 6; ++i) printf ("%+7.1f", eDb.wd[i].target);
    printf ("  %+6.2f   %+6.2f dB\n", measurePinkDeltaDb (eDb, -20.0f), measurePinkDeltaKWeighted (eDb, -20.0f));
    printf ("   energy center  ");
    for (int i = 0; i < 6; ++i) printf ("%+7.1f", eEn.wd[i].target);
    printf ("  %+6.2f   %+6.2f dB\n", measurePinkDeltaDb (eEn, -20.0f), measurePinkDeltaKWeighted (eEn, -20.0f));
    printf ("   Centered (K)   ");
    for (int i = 0; i < 6; ++i) printf ("%+7.1f", eK.wd[i].target);
    printf ("  %+6.2f   %+6.2f dB\n", measurePinkDeltaDb (eK, -20.0f), measurePinkDeltaKWeighted (eK, -20.0f));
    printf ("   Boost Only     ");
    for (int i = 0; i < 6; ++i) printf ("%+7.1f", eBO.wd[i].target);
    printf ("  %+6.2f   %+6.2f dB\n", measurePinkDeltaDb (eBO, -20.0f), measurePinkDeltaKWeighted (eBO, -20.0f));
    printf ("   Boost Only (A) ");
    for (int i = 0; i < 6; ++i) printf ("%+7.1f", eAnc.wd[i].target);
    printf ("  %+6.2f   %+6.2f dB\n\n", measurePinkDeltaDb (eAnc, -20.0f), measurePinkDeltaKWeighted (eAnc, -20.0f));

    printf (" Measured response (Boost Only, shipped default):  soft(-50dBFS)   loud(-6dBFS)\n");
    for (int i = 0; i < kNumSweep; ++i)
    {
        const float gSoft = measureGainDb (eBO, kSweep[i], -50.0f);
        const float gLoud = measureGainDb (eBO, kSweep[i], -6.0f);
        printf ("   %8.0f Hz   %+8.2f dB    %+8.2f dB\n", kSweep[i], gSoft, gLoud);
    }
}

static float maxAbsSweep (Engine& e, float ampDb)
{
    float m = 0;
    for (int i = 0; i < kNumSweep; ++i)
        m = std::max (m, std::fabs (measureGainDb (e, kSweep[i], ampDb)));
    return m;
}

int main()
{
    const float fs = 48000.0f;
    const float strength = 1.0f;    // full correction, to show the model curves clearly
    const float maxBoost = 25.0f;

    // Typical sloping high-frequency (presbycusis) loss, dB HL
    const float audiogram[6] = { 10, 15, 25, 40, 55, 70 };

    printf ("EarFix correction measurement  (fs=%.0f Hz)\n", fs);
    printf ("Audiogram (dB HL): 250=%.0f 500=%.0f 1k=%.0f 2k=%.0f 4k=%.0f 8k=%.0f\n",
            audiogram[0],audiogram[1],audiogram[2],audiogram[3],audiogram[4],audiogram[5]);

    HalfGainModel half;

    NALModel nal;
    nal.setCompressionSpeed (true);   // Fast (default)

    MOSLModel mosl;
    mosl.setCompressionSpeed (true);
    mosl.setBrightnessBoost (true);   // experienced
    mosl.setBassEmphasis (2);

    runModel ("Half-Gain", half, audiogram, fs, strength, maxBoost);
    runModel ("NAL (Speech)", nal, audiogram, fs, strength, maxBoost);
    runModel ("MOSL (Music)", mosl, audiogram, fs, strength, maxBoost);

    // ---- Sanity checks: loudness neutrality ----
    printf ("\n================================================================\n");
    printf (" SANITY CHECKS (max |gain| across the sweep should be ~0)\n");
    printf ("================================================================\n");
    {
        Engine e; e.prepare (fs);
        const float zero[6] = { 0,0,0,0,0,0 };
        e.computeTargets (half, zero, 1.0f, maxBoost, 1);
        printf ("   Zero audiogram   -> crossover flatness:  %.3f dB (soft), %.3f dB (loud)\n",
                maxAbsSweep (e, -50.0f), maxAbsSweep (e, -6.0f));

        const float flat[6] = { 40,40,40,40,40,40 };
        e.computeTargets (half, flat, 1.0f, maxBoost, 1);
        printf ("   Flat 40 dB loss  -> reshape (should ~0):  %.3f dB (soft)\n",
                maxAbsSweep (e, -50.0f));
    }
    printf ("\nDone.\n");
    return 0;
}

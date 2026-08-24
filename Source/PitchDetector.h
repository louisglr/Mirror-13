#pragma once

#include <vector>
#include <cmath>
#include <juce_core/juce_core.h>

// Real-time monofonisk tonehøjde-detektor baseret på YIN-algoritmen
// (de Cheveigné & Kawahara, 2002) - samme familie af algoritme som
// klassiske pitch-correction-værktøjer bruger. Analyserer i "hops",
// ikke hvert sample, for at holde CPU-forbruget nede.
class PitchDetector
{
public:
    void prepare(double sampleRateIn)
    {
        sampleRate = sampleRateIn;
        windowSize = 1024;
        maxLag = windowSize / 2;
        hopSize = 256;

        history.assign((size_t) windowSize, 0.0f);
        temp.assign((size_t) windowSize, 0.0f);
        diffFn.assign((size_t) maxLag, 0.0f);
        cmnd.assign((size_t) maxLag, 1.0f);

        writePos = 0;
        hopCounter = 0;
        lastFrequency = 0.0f;
        lastConfidence = 0.0f;
    }

    void pushSample(float x)
    {
        history[(size_t) writePos] = x;
        writePos = (writePos + 1) % windowSize;

        if (++hopCounter >= hopSize)
        {
            hopCounter = 0;
            detect();
        }
    }

    float getFrequency() const { return lastFrequency; }
    float getConfidence() const { return lastConfidence; }

    // Constraining the expected vocal register prevents common octave jumps.
    // 0 is a permissive automatic range; the remaining values correspond to
    // the Vocal Range choices exposed by the plug-in.
    void setVocalRange(int range)
    {
        static constexpr float minHz[] = { 55.0f, 55.0f, 75.0f, 95.0f, 145.0f, 210.0f };
        static constexpr float maxHz[] = { 1100.0f, 250.0f, 350.0f, 550.0f, 750.0f, 1100.0f };
        const int index = juce::jlimit(0, 5, range);
        expectedMinHz = minHz[index];
        expectedMaxHz = maxHz[index];
    }

private:
    void detect()
    {
        // Lineariser den cirkulære historik ind i temp
        float energy = 0.0f;
        for (int i = 0; i < windowSize; ++i)
        {
            const float sample = history[(size_t) ((writePos + i) % windowSize)];
            temp[(size_t) i] = sample;
            energy += sample * sample;
        }

        // No voiced signal means no useful pitch estimate.  Skipping YIN in
        // silence removes needless audio-thread work and prevents noise from
        // being mistaken for a low vocal note.
        if (energy / (float) windowSize < 1.0e-7f)
        {
            lastFrequency = 0.0f;
            lastConfidence = 0.0f;
            return;
        }

        // --- YIN differensfunktion ---
        int usable = windowSize - maxLag;
        for (int tau = 1; tau < maxLag; ++tau)
        {
            float sum = 0.0f;
            for (int j = 0; j < usable; ++j)
            {
                float d = temp[(size_t) j] - temp[(size_t) (j + tau)];
                sum += d * d;
            }
            diffFn[(size_t) tau] = sum;
        }

        // --- Kumulativ middel-normaliseret differens ---
        diffFn[0] = 1.0f;
        float runningSum = 0.0f;
        // This runs on the audio thread every hop.  Keep the scratch buffer
        // allocated in prepare() so pitch detection never allocates memory
        // while audio is playing.
        std::fill(cmnd.begin(), cmnd.end(), 1.0f);
        for (int tau = 1; tau < maxLag; ++tau)
        {
            runningSum += diffFn[(size_t) tau];
            cmnd[(size_t) tau] = runningSum > 0.0f
                ? diffFn[(size_t) tau] * (float) tau / runningSum
                : 1.0f;
        }

        // --- Absolut threshold: find første lokale minimum under threshold ---
        const float threshold = 0.15f;
        int tauEstimate = -1;
        for (int tau = 2; tau < maxLag - 1; ++tau)
        {
            if (cmnd[(size_t) tau] < threshold
                && cmnd[(size_t) tau] < cmnd[(size_t) (tau + 1)]
                && cmnd[(size_t) tau] <= cmnd[(size_t) (tau - 1)])
            {
                tauEstimate = tau;
                break;
            }
        }

        if (tauEstimate == -1)
        {
            // Intet klart minimum - lav confidence, ignorer denne analyse
            lastConfidence = 0.0f;
            return;
        }

        // --- Parabolsk interpolation for præcision ---
        float betterTau = (float) tauEstimate;
        if (tauEstimate > 0 && tauEstimate < maxLag - 1)
        {
            float s0 = cmnd[(size_t) (tauEstimate - 1)];
            float s1 = cmnd[(size_t) tauEstimate];
            float s2 = cmnd[(size_t) (tauEstimate + 1)];
            float denom = (2.0f * s1 - s2 - s0);
            if (std::abs(denom) > 1.0e-9f)
                betterTau = (float) tauEstimate + 0.5f * (s0 - s2) / denom;
        }

        if (betterTau > 1.0f)
        {
            lastFrequency = (float) sampleRate / betterTau;
            lastConfidence = juce::jlimit(0.0f, 1.0f, 1.0f - cmnd[(size_t) tauEstimate]);
            if (lastFrequency < expectedMinHz || lastFrequency > expectedMaxHz)
            {
                lastFrequency = 0.0f;
                lastConfidence = 0.0f;
            }
        }
        else
        {
            lastConfidence = 0.0f;
        }
    }

    std::vector<float> history, temp, diffFn, cmnd;
    int windowSize = 1024, maxLag = 512, hopSize = 256;
    int writePos = 0, hopCounter = 0;
    double sampleRate = 44100.0;

    float lastFrequency = 0.0f;
    float lastConfidence = 0.0f;
    float expectedMinHz = 55.0f, expectedMaxHz = 1100.0f;
};

#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

// Kvantiserer en detekteret frekvens til nærmeste node i en valgt skala,
// og glider derhen med en hastighed styret af TRACKING-parameteren
// (FAST = hurtig reaktion, SMOOTH = stabil/roligt glid, reducerer jitter).
class PitchCorrector
{
public:
    enum ScaleType { Chromatic = 0, Major = 1, Minor = 2 };

    static constexpr int majorSteps[7] = { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr int minorSteps[7] = { 0, 2, 3, 5, 7, 8, 10 };

    void prepare(double sampleRateIn)
    {
        sampleRate = sampleRateIn;
        smoothedRatio = 1.0f;
    }

    // trackingSmooth01: 0 = FAST (hurtig, kan give lidt jitter),
    //                    1 = SMOOTH (stabil, roligt glid, reducerer jitter)
    float process(float detectedFreq, float confidence, int rootNote, int scaleType, float trackingSmooth01)
    {
        if (detectedFreq <= 0.0f || confidence < 0.35f)
        {
            smoothedRatio += (1.0f - smoothedRatio) * 0.001f;
            return smoothedRatio;
        }

        float targetFreq = nearestScaleFreq(detectedFreq, rootNote, scaleType);
        float targetRatio = targetFreq / detectedFreq;

        // FAST -> hurtig glid (høj smoothing-koefficient), SMOOTH -> langsom/stabil
        float smoothing = juce::jmap(trackingSmooth01, 0.0f, 1.0f, 0.5f, 0.01f);
        smoothedRatio += (targetRatio - smoothedRatio) * smoothing;
        return smoothedRatio;
    }

    // Beregner MIDI-tal for den korrigerede grundtone (bruges til diatonisk
    // trin-forskydning af harmonistemmerne i SCALE-tilstand).
    static int nearestScaleMidi(float freq, int rootNote, int scaleType)
    {
        float midiFloat = 69.0f + 12.0f * std::log2(freq / 440.0f);
        return quantizeToScale(midiFloat, rootNote, scaleType);
    }

    // Flytter et MIDI-tal N diatoniske trin op/ned inden for den valgte skala
    // (bruges til SCALE-tilstandens harmoni-intervaller, fx "3rd+" = 2 trin op).
    static int shiftByScaleSteps(int midiNote, int rootNote, int scaleType, int steps)
    {
        if (scaleType == Chromatic)
            return midiNote + steps * 2; // uden skala: tilnærm et "trin" til en heltone

        const int* stepsArr = (scaleType == Major) ? majorSteps : minorSteps;

        // Find hvilket af de 7 skala-trin midiNote svarer til, og hvilken oktav
        int relative = midiNote - rootNote;
        int octave = (int) std::floor((float) relative / 12.0f);
        int withinOctave = relative - octave * 12;

        int nearestStepIndex = 0;
        int bestDist = 1000;
        for (int i = 0; i < 7; ++i)
        {
            int dist = std::abs(stepsArr[i] - withinOctave);
            if (dist < bestDist) { bestDist = dist; nearestStepIndex = i; }
        }

        int totalStepIndex = octave * 7 + nearestStepIndex + steps;
        int newOctave = (int) std::floor((float) totalStepIndex / 7.0f);
        int newStepIndex = totalStepIndex - newOctave * 7;

        return rootNote + newOctave * 12 + stepsArr[newStepIndex];
    }

    static float midiToFreq(int midiNote)
    {
        return 440.0f * std::pow(2.0f, (float) (midiNote - 69) / 12.0f);
    }

private:
    static float nearestScaleFreq(float freq, int rootNote, int scaleType)
    {
        int nearestMidi = quantizeToScale(69.0f + 12.0f * std::log2(freq / 440.0f), rootNote, scaleType);
        return midiToFreq(nearestMidi);
    }

    static int quantizeToScale(float midiFloat, int root, int scaleType)
    {
        if (scaleType == Chromatic)
            return (int) std::round(midiFloat);

        const int* steps = (scaleType == Major) ? majorSteps : minorSteps;

        int nearestChromatic = (int) std::round(midiFloat);
        int best = nearestChromatic;
        float bestDist = 1.0e9f;

        for (int octaveOffset = -12; octaveOffset <= 12; octaveOffset += 12)
        {
            for (int i = 0; i < 7; ++i)
            {
                int candidate = root + steps[i] + octaveOffset
                                 + 12 * (int) std::floor((float) (nearestChromatic - root) / 12.0f);
                float dist = std::abs((float) candidate - midiFloat);
                if (dist < bestDist) { bestDist = dist; best = candidate; }
            }
        }
        return best;
    }

    float smoothedRatio = 1.0f;
    double sampleRate = 44100.0;
};

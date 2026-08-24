#pragma once

#include <cmath>
#include <vector>
#include <juce_core/juce_core.h>
#include "VoiceBuffer.h"

// Pitch-synchronous granulær pitch-shifter med FIRE overlappende, krydsfadede
// "korn". Hvert korns længde følger den detekterede grundtone, så overlap-add
// arbejder med stemmens perioder i stedet for en fast tidsramme. Det giver
// markant mere naturlige vokaler end en fast-grain shifter.
// Læser fra en delt VoiceBuffer, så flere GrainVoice-instanser (lead +
// harmonier) kan afspille samme kilde ved forskellige pitch-forhold
// samtidig, uden at duplikere lyddata.
class GrainVoice
{
public:
    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        grainSize = juce::jmax(64, (int) (sampleRate * 0.014)); // fallback: 14 ms
        targetGrainSize = grainSize;
        desiredGrainSize = grainSize;
        smoothedGrainSize = (float) grainSize;
        minGrainSize = juce::jmax(64, (int) (sampleRate * 0.005)); // 5 ms
        maxGrainSize = juce::jmax(minGrainSize, (int) (sampleRate * 0.018)); // 18 ms
        hannTable.resize((size_t) maxGrainSize + 1);
        for (int i = 0; i <= maxGrainSize; ++i)
        {
            const float phase = (float) i / (float) maxGrainSize;
            hannTable[(size_t) i] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
        }
        for (int i = 0; i < kNumGrains; ++i)
        {
            age[i] = (grainSize * i) / kNumGrains;
            grainLength[i] = grainSize;
        }
        for (auto& p : pos) p = 0.0;
        initialised = false;
    }

    float process(const VoiceBuffer& vb, float pitchRatio, float sourceFrequency)
    {
        // Corrupt or extremely implausible detector output must never send a
        // grain reader many buffer lengths away from the live signal.
        if (!std::isfinite(pitchRatio))
            pitchRatio = 1.0f;
        pitchRatio = juce::jlimit(0.25f, 4.0f, pitchRatio);
        updateTargetGrainSize(sourceFrequency);

        // Start every overlapping grain at a valid, phase-correct read
        // position.  Previously only the grain at age zero was initialised;
        // the remaining grains briefly read buffer position zero after a
        // transport start or sample-rate change.
        if (!initialised)
        {
            for (int i = 0; i < kNumGrains; ++i)
            {
                const double candidate = (double) vb.getWriteHead()
                    - (double) (grainLength[i] - age[i]) * (double) pitchRatio - 3.0;
                pos[i] = vb.findNearestRisingZeroCrossing(candidate, grainLength[i] / 5);
            }
            initialised = true;
        }

        float sum = 0.0f;
        float windowSum = 0.0f;
        for (int i = 0; i < kNumGrains; ++i)
        {
            if (age[i] == 0)
            {
                // KRITISK: lookback skal skaleres med pitchRatio. Ved opadgående
                // pitch-shift (ratio > 1) læser kornet hurtigere fremad end
                // real-time, så uden denne skalering løber læsepositionen forbi
                // det faktisk skrevne data og begynder at læse "fremtid"/gammelt
                // ombrudt data - det giver præcis den krakelering/artefakt der
                // bliver værre jo højere (mere ekstremt) intervallet er.
                // Leave a few interpolation samples behind the write head as
                // well.  Cubic interpolation needs samples on both sides of
                // the read position; without this margin a grain may touch
                // unwritten data at its end.
                grainLength[i] = targetGrainSize;
                double lookback = (double) grainLength[i] * (double) pitchRatio + 3.0;
                const double candidate = (double) vb.getWriteHead() - lookback;
                pos[i] = vb.findNearestRisingZeroCrossing(candidate, grainLength[i] / 5);
            }

            float s = vb.readInterpolated(pos[i]);
            pos[i] += pitchRatio;

            float env = window(age[i], grainLength[i]);
            sum += s * env;
            windowSum += env;

            if (++age[i] >= grainLength[i])
                age[i] = 0;
        }
        // Dynamic pitch-synchronous lengths do not have a fixed envelope sum;
        // normalising every sample prevents audible gain pumping at note changes.
        // A rapid succession of note changes can temporarily line up grain
        // nulls.  Never normalise a nearly silent envelope by a tiny number:
        // that turns harmless interpolation error into crackle.
        return sum / juce::jmax(0.15f, windowSum);
    }

private:
    void updateTargetGrainSize(float sourceFrequency)
    {
        if (sourceFrequency > 55.0f && sourceFrequency < 1500.0f)
        {
            // Two cycles give enough waveform context for a vowel while
            // retaining a bounded, practical latency on low notes.
            const int desired = (int) std::round(sampleRate / sourceFrequency * 2.0);
            desiredGrainSize = juce::jlimit(minGrainSize, maxGrainSize, desired);
        }
        else
        {
            desiredGrainSize = grainSize;
        }

        // Grain length changes must be slow.  Abruptly changing four
        // independent window lengths is a common source of long-running
        // granular crackle as their phases gradually converge.
        smoothedGrainSize += ((float) desiredGrainSize - smoothedGrainSize) * 0.002f;
        targetGrainSize = juce::jlimit(minGrainSize, maxGrainSize,
            (int) std::round(smoothedGrainSize));
    }

    float window(int grainAge, int length) const
    {
        // Avoid several expensive cosine calls per audio sample.  The table
        // is prepared off the audio thread and linearly interpolated here.
        const float position = (float) grainAge / (float) length * (float) maxGrainSize;
        const int i0 = juce::jlimit(0, maxGrainSize - 1, (int) position);
        const float fraction = position - (float) i0;
        return hannTable[(size_t) i0] + fraction
            * (hannTable[(size_t) (i0 + 1)] - hannTable[(size_t) i0]);
    }

    static constexpr int kNumGrains = 4;
    int grainSize = 2205;
    int targetGrainSize = 2205;
    int desiredGrainSize = 2205;
    float smoothedGrainSize = 2205.0f;
    int minGrainSize = 64;
    int maxGrainSize = 2205;
    int grainLength[kNumGrains] = { 2205, 2205, 2205, 2205 };
    int age[kNumGrains] = { 0, 0, 0, 0 };
    double pos[kNumGrains] = { 0.0, 0.0, 0.0, 0.0 };
    double sampleRate = 44100.0;
    std::vector<float> hannTable;
    bool initialised = false;
};

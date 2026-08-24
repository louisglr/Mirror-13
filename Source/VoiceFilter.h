#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

// Lightweight per-voice high-pass / low-pass shaping.  It deliberately keeps
// the dry vocal open while moving generated voices behind it in the mix.
class VoiceFilter
{
public:
    void prepare(double sampleRateIn)
    {
        sampleRate = sampleRateIn;
        reset();
        setCutoffs(100.0f, 12000.0f);
    }

    void reset()
    {
        hpInput = hpOutput = lpState = 0.0f;
    }

    void setCutoffs(float highPassHz, float lowPassHz)
    {
        const float sr = (float) sampleRate;
        const float hp = juce::jlimit(30.0f, sr * 0.25f, highPassHz);
        const float lp = juce::jlimit(hp * 1.5f, sr * 0.45f, lowPassHz);
        hpCoeff = std::exp(-juce::MathConstants<float>::twoPi * hp / sr);
        lpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * lp / sr);
    }

    float process(float x)
    {
        // One-pole high-pass followed by a one-pole low-pass: enough to
        // create depth without phase-heavy, ringing filter behaviour.
        hpOutput = hpCoeff * (hpOutput + x - hpInput);
        hpInput = x;
        lpState += lpCoeff * (hpOutput - lpState);
        return lpState;
    }

private:
    double sampleRate = 44100.0;
    float hpCoeff = 0.98f, lpCoeff = 0.3f;
    float hpInput = 0.0f, hpOutput = 0.0f, lpState = 0.0f;
};

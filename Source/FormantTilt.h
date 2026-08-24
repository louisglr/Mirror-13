#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

// Simpel spektral tilt-kontrol der bruges som en praktisk "FORMANT"-knap.
// Dette er IKKE ægte formant-forskydning (det ville kræve LPC/cepstral
// analyse af taleorganets resonanser) - det er en tilt-EQ der gør stemmen
// lysere/tyndere (positivt) eller mørkere/fyldigere (negativt), hvilket
// perceptuelt minder om hævede/sænkede formanter, men er en tilnærmelse.
class FormantTilt
{
public:
    float process(float x, float amount)
    {
        lp += 0.06f * (x - lp);
        float hp = x - lp;
        float a = juce::jlimit(-1.0f, 1.0f, amount);
        return lp * (1.0f - 0.5f * a) + hp * (1.0f + 0.5f * a);
    }

private:
    float lp = 0.0f;
};

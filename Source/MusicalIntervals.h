#pragma once

#include <juce_core/juce_core.h>

// Definerer de musikalske intervaller brugeren kan vælge pr. harmonistemme.
// "+"/"-" betyder over/under originalen. Bruges i to tilstande:
//  - INTERVAL-tilstand: faste halvtoner (kromatisk transponering)
//  - SCALE-tilstand: faste antal skala-trin (diatonisk transponering,
//    som automatisk følger den valgte toneart/skala)
struct MusicalInterval
{
    const char* name;
    int semitones;     // brugt i INTERVAL-tilstand
    int scaleSteps;     // brugt i SCALE-tilstand (antal trin i 7-tone-skalaen)
};

// scaleSteps er signeret ligesom semitones - positiv = op, negativ = ned.
static const MusicalInterval kMusicalIntervals[] = {
    { "Unison",  0,   0 },
    { "2nd+",    2,   1 },
    { "2nd-",   -2,  -1 },
    { "3rd+",    4,   2 },
    { "3rd-",   -4,  -2 },
    { "4th+",    5,   3 },
    { "4th-",   -5,  -3 },
    { "5th+",    7,   4 },
    { "5th-",   -7,  -4 },
    { "6th+",    9,   5 },
    { "6th-",   -9,  -5 },
    { "7th+",   11,   6 },
    { "7th-",  -11,  -6 },
    { "Octave+", 12,   7 },
    { "Octave-", -12,  -7 },
    { "9th+",   14,   8 },
    { "9th-",  -14,  -8 },
    { "10th+",  16,   9 },
    { "10th-", -16,  -9 },
    { "11th+",  17,  10 },
    { "11th-", -17, -10 },
    { "12th+",  19,  11 },
    { "12th-", -19, -11 },
};

static constexpr int kNumMusicalIntervals = (int) (sizeof(kMusicalIntervals) / sizeof(MusicalInterval));

inline juce::StringArray getIntervalNames()
{
    juce::StringArray names;
    for (auto& iv : kMusicalIntervals)
        names.add(iv.name);
    return names;
}

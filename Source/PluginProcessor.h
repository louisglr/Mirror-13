#pragma once

#include <array>
#include <algorithm>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PitchDetector.h"
#include "PitchCorrector.h"
#include "GrainVoice.h"
#include "FormantTilt.h"
#include "VoiceFilter.h"
#include "HumanizeWalker.h"
#include "MusicalIntervals.h"

static constexpr int kNumHarmonyVoices = 4;
static constexpr int kMaxHeldNotes = 8;

// Lille fast-størrelse delay-linje pr. stemme til MICRO DELAY - giver hver
// stemme sin egen, faste, lette timing-forskydning (naturlig ensemble-
// fornemmelse, ikke en tydelig ekko-effekt).
class MicroDelayLine
{
public:
    void prepare(double sampleRateIn)
    {
        sampleRate = sampleRateIn;
        int size = (int) (sampleRate * 0.06) + 8;
        buf.assign((size_t) size, 0.0f);
        writePos = 0;
    }

    float process(float x, float delayMs)
    {
        int size = (int) buf.size();
        buf[(size_t) writePos] = x;

        float delaySamples = delayMs * 0.001f * (float) sampleRate;
        float readPos = (float) writePos - delaySamples;
        while (readPos < 0.0f) readPos += (float) size;

        int i0 = (int) readPos;
        float frac = readPos - (float) i0;
        int idx0 = i0 % size;
        int idx1 = (idx0 + 1) % size;
        float out = buf[(size_t) idx0] + frac * (buf[(size_t) idx1] - buf[(size_t) idx0]);

        writePos = (writePos + 1) % size;
        return out;
    }

private:
    std::vector<float> buf;
    int writePos = 0;
    double sampleRate = 44100.0;
};

// Simpel allpass-diffuser til AMBIENCE - udvisker transienter subtilt uden
// at tilføje en tydelig, hørbar reverb-hale.
class SimpleAllpass
{
public:
    void prepare(int delaySamples, float coeffIn)
    {
        buf.assign((size_t) juce::jmax(1, delaySamples), 0.0f);
        pos = 0;
        coeff = coeffIn;
    }

    float process(float in)
    {
        float bufOut = buf[(size_t) pos];
        float out = -in * coeff + bufOut;
        buf[(size_t) pos] = in + bufOut * coeff;
        pos = (pos + 1) % (int) buf.size();
        return out;
    }

private:
    std::vector<float> buf;
    int pos = 0;
    float coeff = 0.5f;
};

class MirrorAudioProcessor : public juce::AudioProcessor
{
public:
    MirrorAudioProcessor();
    ~MirrorAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MIRROR"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> currentDetectedFreq { 0.0f };
    std::atomic<float> currentConfidence { 0.0f };
    std::atomic<int> currentHeldNoteCount { 0 };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    VoiceBuffer voiceBuffer;
    PitchDetector pitchDetector;
    PitchCorrector pitchCorrector;

    GrainVoice dryVoice;
    // Formant filtering needs independent state per channel; sharing one
    // filter state made an active dry formant control collapse stereo to mono.
    FormantTilt dryFormantProcL, dryFormantProcR;

    std::array<GrainVoice, kNumHarmonyVoices> harmonyVoices;
    std::array<FormantTilt, kNumHarmonyVoices> harmonyFormant;
    std::array<VoiceFilter, kNumHarmonyVoices> harmonyFilters;
    std::array<HumanizeWalker, kNumHarmonyVoices> harmonyHumanize;
    std::array<MicroDelayLine, kNumHarmonyVoices> harmonyMicroDelay;
    std::array<float, kNumHarmonyVoices> voiceRatioSmoothed;
    std::array<float, kNumHarmonyVoices> voiceVibratoPhase;
    std::array<float, kNumHarmonyVoices> voiceLastMidi;
    int lastStableBaseMidi = 69;

    SimpleAllpass ambienceApL1, ambienceApL2, ambienceApR1, ambienceApR2;

    std::array<int, kMaxHeldNotes> heldNotes {};
    std::array<float, kMaxHeldNotes> heldNoteVelocities {};
    int numHeldNotes = 0;
    void handleMidiMessage(const juce::MidiMessage& m);

    float frozenLeadRatio = 1.0f;
    // A short, smooth voicing gate keeps pitch-shift artefacts from noise and
    // unvoiced consonants out of the harmony bus without chopping phrases.
    float harmonyVoicing = 0.0f;

    juce::SmoothedValue<float> dryLevelSmoothed, harmonyLevelSmoothed, dryWidthSmoothed;
    std::array<juce::SmoothedValue<float>, kNumHarmonyVoices> voiceLevelSmoothed, voicePanSmoothed;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MirrorAudioProcessor)
};

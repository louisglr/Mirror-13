#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    float softSafetyLimit(float x)
    {
        const float sign = x < 0.0f ? -1.0f : 1.0f;
        const float magnitude = std::abs(x);
        if (magnitude <= 0.95f)
            return x;
        const float excess = magnitude - 0.95f;
        return sign * (0.95f + excess / (1.0f + 4.0f * excess));
    }
}

MirrorAudioProcessor::MirrorAudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

MirrorAudioProcessor::~MirrorAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout MirrorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    using Range = juce::NormalisableRange<float>;

    auto add01 = [&](const char* id, const char* name, float def)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ id, 1 }, name, Range(0.0f, 1.0f, 0.001f), def));
    };
    auto addBipolar = [&](const char* id, const char* name, float def)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ id, 1 }, name, Range(-1.0f, 1.0f, 0.001f), def));
    };

    // --- INPUT ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "rootNote", 1 }, "Key",
        juce::StringArray{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "scaleType", 1 }, "Scale",
        juce::StringArray{ "Chromatic", "Major", "Minor" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "vocalRange", 1 }, "Vocal Range",
        juce::StringArray{ "Auto", "Bass", "Baritone", "Tenor", "Alto", "Soprano" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "harmonyStyle", 1 }, "Harmony Style",
        juce::StringArray{ "Tight", "Natural", "Wide", "Choir" }, 1));

    add01("tracking", "Tracking", 0.5f);
    add01("glide", "Glide", 0.3f);
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "freeze", 1 }, "Freeze", false));

    // --- MODE (kun Manual/MIDI) ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "mode", 1 }, "Mode",
        juce::StringArray{ "Manual", "MIDI" }, 0));
    add01("midiVelocity", "MIDI Velocity", 0.0f);

    // --- DRY VOICE ---
    add01("dry", "Dry Level", 0.7f);
    addBipolar("dryPan", "Dry Pan", 0.0f);
    addBipolar("dryFormant", "Dry Formant", 0.0f);
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "dryPitch", 1 }, "Dry Pitch", Range(-12.0f, 12.0f, 0.01f), 0.0f, "st"));
    add01("dryWidth", "Dry Width", 0.5f);

    // --- HARMONY (4 stemmer) ---
    const int defaultIntervalIdx[kNumHarmonyVoices] = { 3, 7, 13, 14 };
    const float defaultPans[kNumHarmonyVoices] = { -0.25f, 0.3f, -0.45f, 0.5f };
    const bool defaultEnabled[kNumHarmonyVoices] = { true, true, false, false };
    const float defaultLevels[kNumHarmonyVoices] = { 0.75f, 0.65f, 0.55f, 0.45f };
    const float defaultTone[kNumHarmonyVoices] = { 0.05f, 0.12f, 0.18f, 0.25f };
    const float defaultSaturation[kNumHarmonyVoices] = { 0.05f, 0.07f, 0.08f, 0.1f };
    // Keep the ensemble effect available, but do not make it part of the
    // default timing cost.  Users who want a pronounced "separate singers"
    // effect can still raise these controls.
    const float defaultMicroDelayMs[kNumHarmonyVoices] = { 0.0f, 3.0f, 6.0f, 9.0f };

    auto intervalNames = getIntervalNames();

    for (int i = 0; i < kNumHarmonyVoices; ++i)
    {
        juce::String idx(i + 1);

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ "voiceEnable" + idx, 1 }, "Voice " + idx + " Enable", defaultEnabled[i]));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ "voiceSolo" + idx, 1 }, "Voice " + idx + " Solo", false));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ "voiceInterval" + idx, 1 }, "Voice " + idx + " Interval",
            intervalNames, defaultIntervalIdx[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voiceLevel" + idx, 1 }, "Voice " + idx + " Level",
            Range(0.0f, 1.0f, 0.001f), defaultLevels[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voicePan" + idx, 1 }, "Voice " + idx + " Pan",
            Range(-1.0f, 1.0f, 0.001f), defaultPans[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voiceFormant" + idx, 1 }, "Voice " + idx + " Formant",
            Range(-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voiceFineTune" + idx, 1 }, "Voice " + idx + " Fine Tune",
            Range(-50.0f, 50.0f, 0.1f), 0.0f, "cents"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voiceTone" + idx, 1 }, "Voice " + idx + " Tone",
            Range(0.0f, 1.0f, 0.001f), defaultTone[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voiceSaturation" + idx, 1 }, "Voice " + idx + " Saturation",
            Range(0.0f, 1.0f, 0.001f), defaultSaturation[i]));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voiceMicroDelay" + idx, 1 }, "Voice " + idx + " Micro Delay",
            Range(0.0f, 45.0f, 0.1f), defaultMicroDelayMs[i], "ms"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voiceVibrato" + idx, 1 }, "Voice " + idx + " Vibrato",
            Range(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ "voiceVibratoRate" + idx, 1 }, "Voice " + idx + " Vibrato Rate",
            Range(0.0f, 1.0f, 0.001f), 0.45f));
    }

    // --- CHARACTER / HUMANIZE ---
    add01("humanize", "Humanize", 0.3f);
    add01("character", "Character", 0.0f);
    add01("spread", "Spread", 0.5f);

    // --- AMBIENCE ---
    add01("ambience", "Ambience", 0.2f);

    // --- MIX ---
    add01("harmony", "Harmony", 0.5f);
    add01("globalSaturation", "Global Saturation", 0.0f);

    return { params.begin(), params.end() };
}

void MirrorAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;

    voiceBuffer.prepare(sampleRate, 1.0f);
    pitchDetector.prepare(sampleRate);
    pitchCorrector.prepare(sampleRate);

    dryVoice.prepare(sampleRate);
    dryFormantProcL = FormantTilt();
    dryFormantProcR = FormantTilt();

    for (int i = 0; i < kNumHarmonyVoices; ++i)
    {
        harmonyVoices[(size_t) i].prepare(sampleRate);
        harmonyFormant[(size_t) i] = FormantTilt();
        harmonyFilters[(size_t) i].prepare(sampleRate);
        harmonyHumanize[(size_t) i].prepare(sampleRate, 1000 + i * 137);
        harmonyMicroDelay[(size_t) i].prepare(sampleRate);
        voiceRatioSmoothed[(size_t) i] = 1.0f;
        voiceVibratoPhase[(size_t) i] = (float) i * 0.91f;
        voiceLastMidi[(size_t) i] = 69.0f + (float) i * 3.0f;
    }

    ambienceApL1.prepare((int) (sampleRate * 0.009), 0.55f);
    ambienceApL2.prepare((int) (sampleRate * 0.015), 0.45f);
    ambienceApR1.prepare((int) (sampleRate * 0.011), 0.55f);
    ambienceApR2.prepare((int) (sampleRate * 0.017), 0.45f);

    numHeldNotes = 0;
    harmonyVoicing = 0.0f;

    dryLevelSmoothed.reset(sampleRate, 0.03);
    harmonyLevelSmoothed.reset(sampleRate, 0.03);
    dryWidthSmoothed.reset(sampleRate, 0.05);
    for (auto& s : voiceLevelSmoothed) s.reset(sampleRate, 0.03);
    for (auto& s : voicePanSmoothed) s.reset(sampleRate, 0.05);

    dryLevelSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("dry"));
    harmonyLevelSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("harmony"));
    dryWidthSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("dryWidth"));
}

void MirrorAudioProcessor::releaseResources() {}

bool MirrorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void MirrorAudioProcessor::handleMidiMessage(const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        int note = m.getNoteNumber();
        bool alreadyHeld = false;
        for (int i = 0; i < numHeldNotes; ++i)
            if (heldNotes[(size_t) i] == note) { alreadyHeld = true; break; }

        if (!alreadyHeld && numHeldNotes < kMaxHeldNotes)
        {
            int insertAt = numHeldNotes;
            while (insertAt > 0 && heldNotes[(size_t) (insertAt - 1)] > note)
            {
                heldNotes[(size_t) insertAt] = heldNotes[(size_t) (insertAt - 1)];
                heldNoteVelocities[(size_t) insertAt] = heldNoteVelocities[(size_t) (insertAt - 1)];
                --insertAt;
            }
            heldNotes[(size_t) insertAt] = note;
            heldNoteVelocities[(size_t) insertAt] = m.getFloatVelocity();
            ++numHeldNotes;
        }
    }
    else if (m.isNoteOff() || (m.isNoteOn() && m.getVelocity() == 0))
    {
        int note = m.getNoteNumber();
        for (int i = 0; i < numHeldNotes; ++i)
        {
            if (heldNotes[(size_t) i] == note)
            {
                for (int j = i; j < numHeldNotes - 1; ++j)
                {
                    heldNotes[(size_t) j] = heldNotes[(size_t) (j + 1)];
                    heldNoteVelocities[(size_t) j] = heldNoteVelocities[(size_t) (j + 1)];
                }
                --numHeldNotes;
                break;
            }
        }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        numHeldNotes = 0;
    }
}

void MirrorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (const auto metadata : midiMessages)
        handleMidiMessage(metadata.getMessage());

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2)
        return;

    int rootNote = (int) *apvts.getRawParameterValue("rootNote");
    int scaleType = (int) *apvts.getRawParameterValue("scaleType");
    int vocalRange = (int) *apvts.getRawParameterValue("vocalRange");
    int harmonyStyle = (int) *apvts.getRawParameterValue("harmonyStyle");
    pitchDetector.setVocalRange(vocalRange);
    float tracking = *apvts.getRawParameterValue("tracking");
    float glide = *apvts.getRawParameterValue("glide");
    bool freeze = *apvts.getRawParameterValue("freeze") > 0.5f;
    int mode = (int) *apvts.getRawParameterValue("mode");
    float midiVelocitySensitivity = *apvts.getRawParameterValue("midiVelocity");

    float humanizeAmt = *apvts.getRawParameterValue("humanize");
    float character = *apvts.getRawParameterValue("character");
    float spread = *apvts.getRawParameterValue("spread") * 2.0f;
    float ambienceAmt = *apvts.getRawParameterValue("ambience");
    float globalSaturation = *apvts.getRawParameterValue("globalSaturation");

    // One musical macro controls the amount of separation while retaining
    // direct access to individual voices in Advanced.
    static constexpr float styleHumanize[] = { 0.35f, 1.0f, 1.15f, 0.85f };
    static constexpr float styleSpread[] = { 0.55f, 1.0f, 1.25f, 1.45f };
    static constexpr float styleAmbience[] = { 0.35f, 1.0f, 1.15f, 1.55f };
    const int styleIndex = juce::jlimit(0, 3, harmonyStyle);
    humanizeAmt = juce::jlimit(0.0f, 1.0f, humanizeAmt * styleHumanize[styleIndex]);
    spread *= styleSpread[styleIndex];
    ambienceAmt = juce::jlimit(0.0f, 1.0f, ambienceAmt * styleAmbience[styleIndex]);

    float dryPanP = *apvts.getRawParameterValue("dryPan");
    float dryFormantP = *apvts.getRawParameterValue("dryFormant");
    float dryPitchSemis = *apvts.getRawParameterValue("dryPitch");

    dryLevelSmoothed.setTargetValue(*apvts.getRawParameterValue("dry"));
    harmonyLevelSmoothed.setTargetValue(*apvts.getRawParameterValue("harmony"));
    dryWidthSmoothed.setTargetValue(*apvts.getRawParameterValue("dryWidth"));

    struct VoiceParams
    {
        bool enable, solo; int intervalIdx; float formant, fineTune, tone, saturation, microDelayMs, vibrato, vibratoRate;
    };
    std::array<VoiceParams, kNumHarmonyVoices> vp;
    bool anySolo = false;
    for (int i = 0; i < kNumHarmonyVoices; ++i)
    {
        juce::String idx(i + 1);
        vp[(size_t) i].enable = *apvts.getRawParameterValue("voiceEnable" + idx) > 0.5f;
        vp[(size_t) i].solo = *apvts.getRawParameterValue("voiceSolo" + idx) > 0.5f;
        vp[(size_t) i].intervalIdx = (int) *apvts.getRawParameterValue("voiceInterval" + idx);
        vp[(size_t) i].formant = *apvts.getRawParameterValue("voiceFormant" + idx)
                                  + character * 0.08f * ((i % 2 == 0) ? 1.0f : -1.0f);
        vp[(size_t) i].fineTune = *apvts.getRawParameterValue("voiceFineTune" + idx);
        vp[(size_t) i].tone = juce::jlimit(0.0f, 1.0f, *apvts.getRawParameterValue("voiceTone" + idx) + character * 0.1f);
        vp[(size_t) i].saturation = juce::jlimit(0.0f, 1.0f, *apvts.getRawParameterValue("voiceSaturation" + idx) + character * 0.12f);
        vp[(size_t) i].microDelayMs = *apvts.getRawParameterValue("voiceMicroDelay" + idx);
        vp[(size_t) i].vibrato = *apvts.getRawParameterValue("voiceVibrato" + idx);
        vp[(size_t) i].vibratoRate = *apvts.getRawParameterValue("voiceVibratoRate" + idx);

        // The four voices occupy deliberately different spectral spaces:
        // clear lead support, darker width, low-mid body and airy texture.
        static constexpr float baseHighPassHz[kNumHarmonyVoices] = { 70.0f, 100.0f, 140.0f, 220.0f };
        static constexpr float baseLowPassHz[kNumHarmonyVoices] = { 15000.0f, 11800.0f, 9000.0f, 10500.0f };
        const float highPassHz = baseHighPassHz[i] * (1.0f + vp[(size_t) i].tone * 0.45f);
        const float lowPassHz = baseLowPassHz[i] * juce::jmap(vp[(size_t) i].tone, 0.0f, 1.0f, 1.0f, 0.58f);
        harmonyFilters[(size_t) i].setCutoffs(highPassHz, lowPassHz);

        voiceLevelSmoothed[(size_t) i].setTargetValue(*apvts.getRawParameterValue("voiceLevel" + idx));
        float panSpreadAmt = juce::jlimit(0.0f, 2.0f, spread + character * 0.1f);
        voicePanSmoothed[(size_t) i].setTargetValue(*apvts.getRawParameterValue("voicePan" + idx) * panSpreadAmt);
        if (vp[(size_t) i].solo) anySolo = true;
    }

    float glideCoeff = juce::jmap(juce::jlimit(0.0f, 1.0f, glide), 0.0f, 1.0f, 0.4f, 0.0015f);

    auto* channelL = buffer.getWritePointer(0);
    auto* channelR = buffer.getWritePointer(1);

    for (int n = 0; n < numSamples; ++n)
    {
        float dryL = channelL[n];
        float dryR = channelR[n];
        float monoIn = 0.5f * (dryL + dryR);

        voiceBuffer.write(monoIn);
        pitchDetector.pushSample(monoIn);

        float detectedFreq = pitchDetector.getFrequency();
        float confidence = pitchDetector.getConfidence();

        float leadRatio;
        if (freeze)
        {
            leadRatio = frozenLeadRatio;
        }
        else
        {
            leadRatio = pitchCorrector.process(detectedFreq, confidence, rootNote, scaleType, tracking);
            frozenLeadRatio = leadRatio;
        }

        float dryProcL, dryProcR;
        if (std::abs(dryPitchSemis) < 0.001f)
        {
            dryProcL = dryL;
            dryProcR = dryR;
        }
        else
        {
            float dryRatio = std::pow(2.0f, dryPitchSemis / 12.0f);
            float shifted = dryVoice.process(voiceBuffer, dryRatio, detectedFreq);
            dryProcL = shifted;
            dryProcR = shifted;
        }
        // A pitch shift moves formants with the source.  Counteracting part
        // of that movement makes upward voices less chipmunk-like and
        // downward voices less muffled; the user control remains additive.
        const float dryFormantAmount = juce::jlimit(-1.0f, 1.0f,
            dryFormantP - 0.45f * dryPitchSemis / 12.0f);
        if (std::abs(dryFormantAmount) > 0.001f)
        {
            dryProcL = dryFormantProcL.process(dryProcL, dryFormantAmount);
            dryProcR = dryFormantProcR.process(dryProcR, dryFormantAmount);
        }

        // YIN confidence is a useful voiced/unvoiced detector.  Fading the
        // harmony bus around weakly pitched material avoids the metallic,
        // bubbling quality granular shifters otherwise create on breaths,
        // sibilants and room noise.  The slower release preserves legato.
        const float voicingTarget = confidence > 0.40f ? 1.0f : 0.0f;
        const float voicingCoeff = voicingTarget > harmonyVoicing ? 0.008f : 0.001f;
        harmonyVoicing += (voicingTarget - harmonyVoicing) * voicingCoeff;

        float dryWidthNow = dryWidthSmoothed.getNextValue() * 2.0f;
        float mid = 0.5f * (dryProcL + dryProcR);
        float side = 0.5f * (dryProcL - dryProcR) * dryWidthNow;
        float widenedL = mid + side;
        float widenedR = mid - side;
        float dryPanPos = (juce::jlimit(-1.0f, 1.0f, dryPanP) * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;
        float dryOutL = widenedL * std::cos(dryPanPos) * 1.4142f;
        float dryOutR = widenedR * std::sin(dryPanPos) * 1.4142f;

        float harmonySumL = 0.0f, harmonySumR = 0.0f;

        int rawBaseMidi = (detectedFreq > 20.0f)
            ? PitchCorrector::nearestScaleMidi(detectedFreq, rootNote, scaleType)
            : lastStableBaseMidi;
        if (confidence > 0.45f)
            lastStableBaseMidi = rawBaseMidi;
        int baseMidi = lastStableBaseMidi;

        int localHeld = numHeldNotes;

        std::array<int, kNumHarmonyVoices> assignedMidiNote {};
        std::array<float, kNumHarmonyVoices> assignedMidiVelocity {};
        std::array<bool, kMaxHeldNotes> noteClaimed {};
        if (mode == 1 && localHeld > 0)
        {
            for (int v = 0; v < kNumHarmonyVoices; ++v)
            {
                int bestNoteIdx = -1;
                float bestDist = 1.0e9f;
                for (int nIdx = 0; nIdx < localHeld; ++nIdx)
                {
                    if (noteClaimed[(size_t) nIdx]) continue;
                    float dist = std::abs((float) heldNotes[(size_t) nIdx] - voiceLastMidi[(size_t) v]);
                    if (dist < bestDist) { bestDist = dist; bestNoteIdx = nIdx; }
                }
                if (bestNoteIdx == -1)
                {
                    for (int nIdx = 0; nIdx < localHeld; ++nIdx)
                    {
                        float dist = std::abs((float) heldNotes[(size_t) nIdx] - voiceLastMidi[(size_t) v]);
                        if (dist < bestDist) { bestDist = dist; bestNoteIdx = nIdx; }
                    }
                }
                else
                {
                    noteClaimed[(size_t) bestNoteIdx] = true;
                }
                assignedMidiNote[(size_t) v] = heldNotes[(size_t) bestNoteIdx];
                assignedMidiVelocity[(size_t) v] = heldNoteVelocities[(size_t) bestNoteIdx];
            }
        }

        for (int i = 0; i < kNumHarmonyVoices; ++i)
        {
            bool active = vp[(size_t) i].enable && (!anySolo || vp[(size_t) i].solo);

            if (mode == 1 && localHeld == 0)
                active = false;

            if (!active)
            {
                voiceRatioSmoothed[(size_t) i] += (1.0f - voiceRatioSmoothed[(size_t) i]) * glideCoeff;
                continue;
            }

            const auto& interval = kMusicalIntervals[(size_t) vp[(size_t) i].intervalIdx];
            auto hz = harmonyHumanize[(size_t) i].tick(humanizeAmt);
            float fineTuneCents = vp[(size_t) i].fineTune;

            float targetRatio;

            if (mode == 0)
            {
                if (scaleType == PitchCorrector::Chromatic)
                {
                    float semis = (float) interval.semitones + hz.pitchCents / 100.0f + fineTuneCents / 100.0f;
                    targetRatio = leadRatio * std::pow(2.0f, semis / 12.0f);
                }
                else
                {
                    int targetMidi = PitchCorrector::shiftByScaleSteps(baseMidi, rootNote, scaleType, interval.scaleSteps);
                    float targetFreq = PitchCorrector::midiToFreq(targetMidi)
                                        * std::pow(2.0f, (hz.pitchCents + fineTuneCents) / 1200.0f);
                    targetRatio = (detectedFreq > 20.0f) ? (targetFreq / detectedFreq) : 1.0f;
                }
            }
            else
            {
                int midiNote = assignedMidiNote[(size_t) i];
                voiceLastMidi[(size_t) i] = (float) midiNote;
                float targetFreq = PitchCorrector::midiToFreq(midiNote)
                                    * std::pow(2.0f, (hz.pitchCents + fineTuneCents) / 1200.0f);
                targetRatio = (detectedFreq > 20.0f) ? (targetFreq / detectedFreq) : 1.0f;
            }

            voiceRatioSmoothed[(size_t) i] += (targetRatio - voiceRatioSmoothed[(size_t) i]) * glideCoeff;

            // Vibrato is a continuous sine modulation, deliberately separate
            // from Humanize's random drift to avoid zipper/saw artefacts.
            const float vibratoRateHz = juce::jmap(vp[(size_t) i].vibratoRate, 3.0f, 7.2f)
                + (float) i * 0.12f;
            voiceVibratoPhase[(size_t) i] += juce::MathConstants<float>::twoPi
                * vibratoRateHz / (float) currentSampleRate;
            if (voiceVibratoPhase[(size_t) i] >= juce::MathConstants<float>::twoPi)
                voiceVibratoPhase[(size_t) i] -= juce::MathConstants<float>::twoPi;
            const float vibratoCents = std::sin(voiceVibratoPhase[(size_t) i])
                * vp[(size_t) i].vibrato * 16.0f;
            const float readRatio = voiceRatioSmoothed[(size_t) i]
                * std::pow(2.0f, vibratoCents / 1200.0f);

            float raw = harmonyVoices[(size_t) i].process(voiceBuffer, readRatio, detectedFreq);
            const float shiftSemitones = 12.0f * std::log2(
                juce::jmax(0.01f, readRatio));
            const float formantAmount = juce::jlimit(-1.0f, 1.0f,
                vp[(size_t) i].formant - 0.45f * shiftSemitones / 12.0f);
            raw = harmonyFormant[(size_t) i].process(raw, formantAmount);

            raw = harmonyFilters[(size_t) i].process(raw);

            // Zero saturation must be transparent.  The previous code ran
            // tanh even at zero, which coloured every voice unnecessarily.
            const float sat = vp[(size_t) i].saturation;
            const float drive = 1.0f + sat * 0.8f;
            const float saturated = std::tanh(raw * drive) / std::tanh(drive);
            raw += (saturated - raw) * sat;

            raw *= (1.0f + hz.ampMod);
            if (mode == 1)
            {
                const float velocityGain = 0.35f + 0.65f * assignedMidiVelocity[(size_t) i];
                raw *= 1.0f + midiVelocitySensitivity * (velocityGain - 1.0f);
            }

            // Modulating a short fractional delay is effectively a second
            // pitch shifter.  It was the remaining source of the sharp,
            // saw-like artefact when Humanize was raised.  Keep Micro Delay
            // stable; Humanize still supplies gentle pitch and level drift.
            raw = harmonyMicroDelay[(size_t) i].process(raw, vp[(size_t) i].microDelayMs);
            raw *= harmonyVoicing;

            float level = voiceLevelSmoothed[(size_t) i].getNextValue();
            float pan = voicePanSmoothed[(size_t) i].getNextValue();
            float panPos = (juce::jlimit(-1.0f, 1.0f, pan) * 0.5f + 0.5f) * juce::MathConstants<float>::halfPi;

            harmonySumL += raw * level * std::cos(panPos);
            harmonySumR += raw * level * std::sin(panPos);
        }

        if (ambienceAmt > 0.001f)
        {
            float diffL = ambienceApL2.process(ambienceApL1.process(harmonySumL));
            float diffR = ambienceApR2.process(ambienceApR1.process(harmonySumR));
            harmonySumL = harmonySumL * (1.0f - ambienceAmt * 0.5f) + diffL * ambienceAmt * 0.5f;
            harmonySumR = harmonySumR * (1.0f - ambienceAmt * 0.5f) + diffR * ambienceAmt * 0.5f;
        }

        float harmonyLevelNow = harmonyLevelSmoothed.getNextValue();
        float wetL = harmonySumL * harmonyLevelNow;
        float wetR = harmonySumR * harmonyLevelNow;

        float dryNow = dryLevelSmoothed.getNextValue();
        float outL = dryOutL * dryNow + wetL;
        float outR = dryOutR * dryNow + wetR;

        const float saturationDrive = 1.0f + globalSaturation * 0.6f;
        const float saturatedL = std::tanh(outL * saturationDrive) / std::tanh(saturationDrive);
        const float saturatedR = std::tanh(outR * saturationDrive) / std::tanh(saturationDrive);
        outL += (saturatedL - outL) * globalSaturation;
        outR += (saturatedR - outR) * globalSaturation;
        // A single invalid sample can otherwise persist in recursive filters
        // and present itself as crackle after a long session.
        if (!std::isfinite(outL)) outL = 0.0f;
        if (!std::isfinite(outR)) outR = 0.0f;
        channelL[n] = softSafetyLimit(outL);
        channelR[n] = softSafetyLimit(outR);
    }

    currentDetectedFreq.store(pitchDetector.getFrequency());
    currentConfidence.store(pitchDetector.getConfidence());
    currentHeldNoteCount.store(numHeldNotes);
}

juce::AudioProcessorEditor* MirrorAudioProcessor::createEditor()
{
    return new MirrorAudioProcessorEditor(*this);
}

void MirrorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MirrorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MirrorAudioProcessor();
}

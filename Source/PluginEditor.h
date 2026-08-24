#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class MirrorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MirrorAudioProcessorEditor(MirrorAudioProcessor&);
    ~MirrorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct KnobWithLabel
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct VoiceColumn
    {
        juce::Label title;
        juce::ToggleButton enableButton, soloButton;
        juce::ComboBox intervalBox;
        juce::Slider levelSlider, panSlider, formantSlider;
        juce::Slider fineTuneSlider, toneSlider, saturationSlider, microDelaySlider, vibratoSlider, vibratoRateSlider;
        juce::Label levelLabel, panLabel, formantLabel;
        juce::Label fineTuneLabel, toneLabel, saturationLabel, microDelayLabel, vibratoLabel, vibratoRateLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment, soloAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> intervalAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment, panAttachment, formantAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fineTuneAttachment, toneAttachment, saturationAttachment, microDelayAttachment, vibratoAttachment, vibratoRateAttachment;
    };

    void setupKnob(KnobWithLabel& k, const juce::String& paramId, const juce::String& labelText);
    void setupVoiceColumn(VoiceColumn& c, int voiceIndex);
    void applyPreset(int presetIndex);
    void showPage(int pageIndex);

    MirrorAudioProcessor& audioProcessor;

    juce::Label titleLabel, creditLabel;

    juce::TextButton mainPageButton, harmonyPageButton;
    int currentPage = 0;
    juce::TextButton advancedButton;
    bool showAdvanced = false;

    juce::ComboBox modeBox;
    juce::ComboBox vocalRangeBox, harmonyStyleBox;
    juce::Label modeLabel, vocalRangeLabel, harmonyStyleLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment, vocalRangeAttachment, harmonyStyleAttachment;

    // INPUT
    juce::ComboBox rootBox, scaleBox;
    juce::Label rootLabel, scaleLabel, inputSectionLabel;
    KnobWithLabel trackingKnob, glideKnob;
    juce::ToggleButton freezeButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rootAttachment, scaleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;

    juce::ComboBox presetBox;
    juce::Label presetLabel;

    // DRY VOICE
    juce::Label drySectionLabel;
    KnobWithLabel dryLevelKnob, dryPanKnob, dryFormantKnob, dryPitchKnob, dryWidthKnob;

    // HARMONY
    juce::Label harmonySectionLabel;
    std::array<VoiceColumn, kNumHarmonyVoices> voiceColumns;

    // CHARACTER
    juce::Label characterSectionLabel;
    KnobWithLabel humanizeKnob, characterKnob, spreadKnob;

    // AMBIENCE + MIX
    juce::Label ambienceSectionLabel, mixSectionLabel;
    KnobWithLabel ambienceKnob, harmonyMixKnob, globalSaturationKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MirrorAudioProcessorEditor)
};

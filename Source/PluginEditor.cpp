#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::Colour kBg1 { 0xff211a17 };
    juce::Colour kBg2 { 0xff0b0908 };
    juce::Colour kAccent { 0xffd4bd8a };
    juce::Colour kAccentDim { 0xff8c7555 };
    juce::Colour kText { 0xfff2eadb };
    juce::Colour kTextDim { 0xffb3a58e };

}

MirrorAudioProcessorEditor::MirrorAudioProcessorEditor(MirrorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    titleLabel.setText("MIRROR", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, kText);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::plain).withExtraKerningFactor(0.15f));
    addAndMakeVisible(titleLabel);

    creditLabel.setText("by Louis Gabriel", juce::dontSendNotification);
    creditLabel.setJustificationType(juce::Justification::centredLeft);
    creditLabel.setColour(juce::Label::textColourId, kTextDim);
    creditLabel.setFont(juce::Font(10.0f, juce::Font::italic));
    addAndMakeVisible(creditLabel);

    mainPageButton.setButtonText("MAIN");
    mainPageButton.setClickingTogglesState(true);
    mainPageButton.setRadioGroupId(1001);
    mainPageButton.setColour(juce::TextButton::buttonOnColourId, kAccent);
    mainPageButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff382d25));
    mainPageButton.onClick = [this] { showPage(0); };
    addAndMakeVisible(mainPageButton);

    harmonyPageButton.setButtonText("HARMONY");
    harmonyPageButton.setClickingTogglesState(true);
    harmonyPageButton.setRadioGroupId(1001);
    harmonyPageButton.setColour(juce::TextButton::buttonOnColourId, kAccent);
    harmonyPageButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff382d25));
    harmonyPageButton.onClick = [this] { showPage(1); };
    addAndMakeVisible(harmonyPageButton);

    modeBox.addItemList({ "Manual", "MIDI" }, 1);
    addAndMakeVisible(modeBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "mode", modeBox);
    modeLabel.setText("MODE", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, kTextDim);
    addAndMakeVisible(modeLabel);

    vocalRangeBox.addItemList({ "Auto", "Bass", "Baritone", "Tenor", "Alto", "Soprano" }, 1);
    addAndMakeVisible(vocalRangeBox);
    vocalRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "vocalRange", vocalRangeBox);
    vocalRangeLabel.setText("VOCAL RANGE", juce::dontSendNotification);
    vocalRangeLabel.setColour(juce::Label::textColourId, kTextDim);
    addAndMakeVisible(vocalRangeLabel);

    harmonyStyleBox.addItemList({ "Tight", "Natural", "Wide", "Choir" }, 1);
    addAndMakeVisible(harmonyStyleBox);
    harmonyStyleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "harmonyStyle", harmonyStyleBox);
    harmonyStyleLabel.setText("STYLE", juce::dontSendNotification);
    harmonyStyleLabel.setColour(juce::Label::textColourId, kTextDim);
    addAndMakeVisible(harmonyStyleLabel);

    presetBox.addItem("Vocoder Glass", 1);
    presetBox.addItem("Fractured Stack", 2);
    presetBox.addItem("Stadium Choir", 3);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this] { applyPreset(presetBox.getSelectedId()); };
    addAndMakeVisible(presetBox);
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    presetLabel.setColour(juce::Label::textColourId, kTextDim);
    addAndMakeVisible(presetLabel);

    // --- INPUT ---
    inputSectionLabel.setText("INPUT", juce::dontSendNotification);
    inputSectionLabel.setColour(juce::Label::textColourId, kAccentDim);
    inputSectionLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    addAndMakeVisible(inputSectionLabel);

    rootBox.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1);
    addAndMakeVisible(rootBox);
    rootAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "rootNote", rootBox);
    rootLabel.setText("KEY", juce::dontSendNotification);
    rootLabel.setColour(juce::Label::textColourId, kTextDim);
    addAndMakeVisible(rootLabel);

    scaleBox.addItemList({ "Chromatic", "Major", "Minor" }, 1);
    addAndMakeVisible(scaleBox);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "scaleType", scaleBox);
    scaleLabel.setText("SCALE", juce::dontSendNotification);
    scaleLabel.setColour(juce::Label::textColourId, kTextDim);
    addAndMakeVisible(scaleLabel);

    setupKnob(trackingKnob, "tracking", "TRACKING");
    setupKnob(glideKnob, "glide", "TRANSITION");

    freezeButton.setButtonText("FREEZE");
    freezeButton.setColour(juce::ToggleButton::tickColourId, kAccent);
    freezeButton.setColour(juce::ToggleButton::textColourId, kTextDim);
    addAndMakeVisible(freezeButton);
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "freeze", freezeButton);

    // --- DRY VOICE ---
    drySectionLabel.setText("DRY VOICE", juce::dontSendNotification);
    drySectionLabel.setColour(juce::Label::textColourId, kAccentDim);
    drySectionLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    addAndMakeVisible(drySectionLabel);

    setupKnob(dryLevelKnob, "dry", "LEVEL");
    setupKnob(dryPanKnob, "dryPan", "PAN");
    setupKnob(dryFormantKnob, "dryFormant", "FORMANT");
    setupKnob(dryPitchKnob, "dryPitch", "PITCH");
    setupKnob(dryWidthKnob, "dryWidth", "WIDTH");

    // --- HARMONY (fane 2) ---
    harmonySectionLabel.setText("HARMONY", juce::dontSendNotification);
    harmonySectionLabel.setColour(juce::Label::textColourId, kAccentDim);
    harmonySectionLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    addAndMakeVisible(harmonySectionLabel);

    advancedButton.setButtonText("ADVANCED");
    advancedButton.setClickingTogglesState(true);
    advancedButton.setColour(juce::TextButton::buttonOnColourId, kAccent);
    advancedButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff382d25));
    advancedButton.onClick = [this]
    {
        showAdvanced = advancedButton.getToggleState();
        showPage(currentPage);
    };
    addAndMakeVisible(advancedButton);

    for (int i = 0; i < kNumHarmonyVoices; ++i)
        setupVoiceColumn(voiceColumns[(size_t) i], i);

    // --- CHARACTER ---
    characterSectionLabel.setText("CHARACTER", juce::dontSendNotification);
    characterSectionLabel.setColour(juce::Label::textColourId, kAccentDim);
    characterSectionLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    addAndMakeVisible(characterSectionLabel);

    setupKnob(humanizeKnob, "humanize", "HUMANIZE");
    setupKnob(characterKnob, "character", "CHARACTER");
    setupKnob(spreadKnob, "spread", "SPREAD");

    // --- AMBIENCE + MIX ---
    ambienceSectionLabel.setText("AMBIENCE", juce::dontSendNotification);
    ambienceSectionLabel.setColour(juce::Label::textColourId, kAccentDim);
    ambienceSectionLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    addAndMakeVisible(ambienceSectionLabel);
    setupKnob(ambienceKnob, "ambience", "AMBIENCE");

    mixSectionLabel.setText("MIX", juce::dontSendNotification);
    mixSectionLabel.setColour(juce::Label::textColourId, kAccentDim);
    mixSectionLabel.setFont(juce::Font(10.5f, juce::Font::bold));
    addAndMakeVisible(mixSectionLabel);
    setupKnob(harmonyMixKnob, "harmony", "HARMONY");
    setupKnob(globalSaturationKnob, "globalSaturation", "GLUE");

    setSize(700, 520);

    mainPageButton.setToggleState(true, juce::dontSendNotification);
    showPage(0);
}

MirrorAudioProcessorEditor::~MirrorAudioProcessorEditor() {}

void MirrorAudioProcessorEditor::setupKnob(KnobWithLabel& k, const juce::String& paramId, const juce::String& labelText)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
    k.slider.setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    k.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff262b36));
    k.slider.setColour(juce::Slider::thumbColourId, kText);
    addAndMakeVisible(k.slider);

    k.label.setText(labelText, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setColour(juce::Label::textColourId, kTextDim);
    k.label.setFont(juce::Font(10.0f, juce::Font::plain));
    addAndMakeVisible(k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, paramId, k.slider);
}

void MirrorAudioProcessorEditor::setupVoiceColumn(VoiceColumn& c, int voiceIndex)
{
    juce::String idx(voiceIndex + 1);

    c.title.setText("VOICE " + idx, juce::dontSendNotification);
    c.title.setJustificationType(juce::Justification::centred);
    c.title.setColour(juce::Label::textColourId, kText);
    c.title.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(c.title);

    c.enableButton.setButtonText("ON");
    c.enableButton.setColour(juce::ToggleButton::tickColourId, kAccent);
    c.enableButton.setColour(juce::ToggleButton::textColourId, kTextDim);
    addAndMakeVisible(c.enableButton);
    c.enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "voiceEnable" + idx, c.enableButton);

    c.soloButton.setButtonText("SOLO");
    c.soloButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffffd479));
    c.soloButton.setColour(juce::ToggleButton::textColourId, kTextDim);
    addAndMakeVisible(c.soloButton);
    c.soloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "voiceSolo" + idx, c.soloButton);

    c.intervalBox.addItemList(getIntervalNames(), 1);
    addAndMakeVisible(c.intervalBox);
    c.intervalAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, "voiceInterval" + idx, c.intervalBox);

    auto setupSmallKnob = [this](juce::Slider& s, juce::Label& lbl, const juce::String& text, juce::Colour colour)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 12);
        s.setColour(juce::Slider::rotarySliderFillColourId, colour);
        s.setColour(juce::Slider::thumbColourId, kText);
        addAndMakeVisible(s);

        lbl.setText(text, juce::dontSendNotification);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setColour(juce::Label::textColourId, kTextDim);
        lbl.setFont(juce::Font(8.5f, juce::Font::plain));
        addAndMakeVisible(lbl);

    };

    setupSmallKnob(c.levelSlider, c.levelLabel, "LEVEL", kAccent);
    setupSmallKnob(c.panSlider, c.panLabel, "PAN", kAccent);
    setupSmallKnob(c.formantSlider, c.formantLabel, "FORMANT", kAccent);
    setupSmallKnob(c.fineTuneSlider, c.fineTuneLabel, "FINE", kAccentDim);
    setupSmallKnob(c.toneSlider, c.toneLabel, "TONE", kAccentDim);
    setupSmallKnob(c.saturationSlider, c.saturationLabel, "SAT", kAccentDim);
    setupSmallKnob(c.microDelaySlider, c.microDelayLabel, "DELAY", kAccentDim);
    setupSmallKnob(c.vibratoSlider, c.vibratoLabel, "VIBRATO", kAccentDim);
    setupSmallKnob(c.vibratoRateSlider, c.vibratoRateLabel, "VIB RATE", kAccentDim);

    c.levelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voiceLevel" + idx, c.levelSlider);
    c.panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voicePan" + idx, c.panSlider);
    c.formantAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voiceFormant" + idx, c.formantSlider);
    c.fineTuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voiceFineTune" + idx, c.fineTuneSlider);
    c.toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voiceTone" + idx, c.toneSlider);
    c.saturationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voiceSaturation" + idx, c.saturationSlider);
    c.microDelayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voiceMicroDelay" + idx, c.microDelaySlider);
    c.vibratoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voiceVibrato" + idx, c.vibratoSlider);
    c.vibratoRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "voiceVibratoRate" + idx, c.vibratoRateSlider);
}

void MirrorAudioProcessorEditor::showPage(int pageIndex)
{
    currentPage = pageIndex;
    bool showMain = (pageIndex == 0);
    bool showHarmony = (pageIndex == 1);

    for (juce::Component* c : { (juce::Component*) &modeBox, (juce::Component*) &vocalRangeBox,
                     (juce::Component*) &harmonyStyleBox, (juce::Component*) &rootBox,
                     (juce::Component*) &scaleBox, (juce::Component*) &trackingKnob.slider,
                     (juce::Component*) &glideKnob.slider, (juce::Component*) &dryLevelKnob.slider,
                     (juce::Component*) &dryPanKnob.slider, (juce::Component*) &dryFormantKnob.slider,
                     (juce::Component*) &dryPitchKnob.slider, (juce::Component*) &dryWidthKnob.slider,
                     (juce::Component*) &humanizeKnob.slider, (juce::Component*) &characterKnob.slider,
                     (juce::Component*) &spreadKnob.slider, (juce::Component*) &ambienceKnob.slider,
                     (juce::Component*) &harmonyMixKnob.slider, (juce::Component*) &globalSaturationKnob.slider })
        c->setVisible(showMain);

    for (auto* l : { &modeLabel, &vocalRangeLabel, &harmonyStyleLabel, &inputSectionLabel, &rootLabel, &scaleLabel,
                     &trackingKnob.label, &glideKnob.label, &drySectionLabel, &dryLevelKnob.label,
                     &dryPanKnob.label, &dryFormantKnob.label, &dryPitchKnob.label, &dryWidthKnob.label,
                     &characterSectionLabel, &humanizeKnob.label, &characterKnob.label, &spreadKnob.label,
                     &ambienceSectionLabel, &ambienceKnob.label,
                     &mixSectionLabel, &harmonyMixKnob.label, &globalSaturationKnob.label })
        l->setVisible(showMain);
    freezeButton.setVisible(showMain);

    harmonySectionLabel.setVisible(showHarmony);
    advancedButton.setVisible(showHarmony);
    for (auto& c : voiceColumns)
    {
        c.title.setVisible(showHarmony);
        c.enableButton.setVisible(showHarmony);
        c.soloButton.setVisible(showHarmony);
        c.intervalBox.setVisible(showHarmony);
        c.levelSlider.setVisible(showHarmony); c.levelLabel.setVisible(showHarmony);
        c.panSlider.setVisible(showHarmony); c.panLabel.setVisible(showHarmony);
        c.formantSlider.setVisible(showHarmony); c.formantLabel.setVisible(showHarmony);
        c.fineTuneSlider.setVisible(showHarmony && showAdvanced); c.fineTuneLabel.setVisible(showHarmony && showAdvanced);
        c.toneSlider.setVisible(showHarmony && showAdvanced); c.toneLabel.setVisible(showHarmony && showAdvanced);
        c.saturationSlider.setVisible(showHarmony && showAdvanced); c.saturationLabel.setVisible(showHarmony && showAdvanced);
        c.microDelaySlider.setVisible(showHarmony && showAdvanced); c.microDelayLabel.setVisible(showHarmony && showAdvanced);
        c.vibratoSlider.setVisible(showHarmony && showAdvanced); c.vibratoLabel.setVisible(showHarmony && showAdvanced);
        c.vibratoRateSlider.setVisible(showHarmony && showAdvanced); c.vibratoRateLabel.setVisible(showHarmony && showAdvanced);
    }

    resized();
}

void MirrorAudioProcessorEditor::applyPreset(int presetIndex)
{
    auto& apvts = audioProcessor.apvts;

    auto set = [&](const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(value));
    };
    auto setChoice = [&](const juce::String& id, int choiceIndex, int numChoices)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost((float) choiceIndex / (float) juce::jmax(1, numChoices - 1));
    };
    auto voice = [&](int i, bool enable, int intervalIdx, float level, float pan)
    {
        juce::String idx(i + 1);
        static constexpr float tone[kNumHarmonyVoices] = { 0.06f, 0.10f, 0.14f, 0.18f };
        static constexpr float saturation[kNumHarmonyVoices] = { 0.02f, 0.03f, 0.04f, 0.05f };
        static constexpr float microDelayMs[kNumHarmonyVoices] = { 0.0f, 2.0f, 4.0f, 6.0f };
        set("voiceEnable" + idx, enable ? 1.0f : 0.0f);
        set("voiceSolo" + idx, 0.0f);
        setChoice("voiceInterval" + idx, intervalIdx, kNumMusicalIntervals);
        set("voiceLevel" + idx, level);
        set("voicePan" + idx, pan);
        set("voiceFormant" + idx, 0.0f);
        set("voiceFineTune" + idx, 0.0f);
        set("voiceTone" + idx, tone[i]);
        set("voiceSaturation" + idx, saturation[i]);
        set("voiceMicroDelay" + idx, microDelayMs[i]);
        set("voiceVibrato" + idx, 0.0f);
        set("voiceVibratoRate" + idx, 0.45f);
    };
    auto advanced = [&](int i, float formant, float fineTune, float tone, float saturation,
                        float microDelayMs, float vibrato, float vibratoRate)
    {
        juce::String idx(i + 1);
        set("voiceFormant" + idx, formant);
        set("voiceFineTune" + idx, fineTune);
        set("voiceTone" + idx, tone);
        set("voiceSaturation" + idx, saturation);
        set("voiceMicroDelay" + idx, microDelayMs);
        set("voiceVibrato" + idx, vibrato);
        set("voiceVibratoRate" + idx, vibratoRate);
    };

    // A preset should be deterministic: reset the controls outside a voice
    // column as well as the visible voice parameters.
    set("dryPan", 0.0f); set("dryFormant", 0.0f); set("dryPitch", 0.0f); set("dryWidth", 0.5f);
    set("midiVelocity", 0.0f); set("globalSaturation", 0.04f);
    setChoice("vocalRange", 0, 6);
    setChoice("harmonyStyle", 1, 4);

    switch (presetIndex)
    {
        case 1:
            setChoice("harmonyStyle", 0, 4);
            set("humanize", 0.10f); set("character", 0.0f); set("spread", 0.5f); set("ambience", 0.16f);
            set("dry", 0.18f); set("harmony", 0.78f); set("tracking", 0.72f); set("glide", 0.48f);
            voice(0, true, 0, 0.70f, -0.22f); voice(1, true, 3, 0.56f, 0.22f); voice(2, true, 7, 0.42f, -0.45f); voice(3, true, 13, 0.30f, 0.45f);
            advanced(0, -0.08f, -4.0f, 0.14f, 0.02f, 0.0f, 0.00f, 0.42f);
            advanced(1, -0.14f, 5.0f, 0.20f, 0.03f, 1.5f, 0.02f, 0.47f);
            advanced(2, -0.20f, -7.0f, 0.30f, 0.03f, 3.0f, 0.01f, 0.39f);
            advanced(3, 0.10f, 8.0f, 0.38f, 0.02f, 5.0f, 0.01f, 0.53f);
            break;
        case 2:
            setChoice("harmonyStyle", 2, 4);
            set("humanize", 0.32f); set("character", 0.08f); set("spread", 0.86f); set("ambience", 0.25f);
            set("dry", 0.32f); set("harmony", 0.72f); set("tracking", 0.64f); set("glide", 0.44f);
            voice(0, true, 3, 0.64f, -0.62f); voice(1, true, 7, 0.58f, 0.58f); voice(2, true, 14, 0.38f, -0.18f); voice(3, true, 4, 0.28f, 0.30f);
            advanced(0, -0.12f, -5.0f, 0.16f, 0.03f, 0.0f, 0.03f, 0.38f);
            advanced(1, -0.18f, 6.0f, 0.24f, 0.04f, 2.0f, 0.02f, 0.46f);
            advanced(2, -0.28f, -9.0f, 0.36f, 0.04f, 4.0f, 0.03f, 0.34f);
            advanced(3, 0.05f, 9.0f, 0.42f, 0.02f, 6.0f, 0.02f, 0.51f);
            break;
        case 3:
            setChoice("harmonyStyle", 3, 4);
            set("humanize", 0.24f); set("character", 0.10f); set("spread", 0.95f); set("ambience", 0.30f);
            set("dry", 0.42f); set("harmony", 0.76f); set("tracking", 0.58f); set("glide", 0.38f);
            voice(0, true, 3, 0.68f, -0.72f); voice(1, true, 7, 0.64f, 0.72f); voice(2, true, 13, 0.42f, -0.25f); voice(3, true, 14, 0.34f, 0.28f);
            advanced(0, -0.04f, -3.0f, 0.10f, 0.04f, 0.0f, 0.04f, 0.43f);
            advanced(1, -0.14f, 4.0f, 0.18f, 0.05f, 2.5f, 0.04f, 0.49f);
            advanced(2, -0.22f, -8.0f, 0.30f, 0.05f, 5.0f, 0.03f, 0.37f);
            advanced(3, 0.12f, 8.0f, 0.40f, 0.03f, 7.0f, 0.03f, 0.56f);
            break;
        default:
            // Unknown preset ids fall back to the first, deterministic preset.
            applyPreset(1);
            break;
    }
}

void MirrorAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient grad(kBg1, 0, 0, kBg2, 0, (float) getHeight(), false);
    g.setGradientFill(grad);
    g.fillAll();
}

void MirrorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);

    auto headerRow = area.removeFromTop(38);
    auto titleArea = headerRow.removeFromLeft(180);
    titleLabel.setBounds(titleArea.removeFromTop(24));
    creditLabel.setBounds(titleArea);

    mainPageButton.setBounds(headerRow.removeFromLeft(70).reduced(2, 4));
    harmonyPageButton.setBounds(headerRow.removeFromLeft(90).reduced(2, 4));
    headerRow.removeFromLeft(10);
    presetLabel.setBounds(headerRow.removeFromLeft(48));
    presetBox.setBounds(headerRow.removeFromLeft(140).reduced(2, 6));

    area.removeFromTop(8);

    if (currentPage == 0)
    {
        auto modeRow = area.removeFromTop(48);
        auto modeCol = modeRow.removeFromLeft(modeRow.getWidth() / 3).reduced(4);
        modeLabel.setBounds(modeCol.removeFromTop(13));
        modeBox.setBounds(modeCol.removeFromTop(24));
        auto vocalRangeCol = modeRow.removeFromLeft(modeRow.getWidth() / 2).reduced(4);
        vocalRangeLabel.setBounds(vocalRangeCol.removeFromTop(13));
        vocalRangeBox.setBounds(vocalRangeCol.removeFromTop(24));
        auto styleCol = modeRow.reduced(4);
        harmonyStyleLabel.setBounds(styleCol.removeFromTop(13));
        harmonyStyleBox.setBounds(styleCol.removeFromTop(24));

        area.removeFromTop(6);
        inputSectionLabel.setBounds(area.removeFromTop(14));
        auto inputRow = area.removeFromTop(58);
        int inputColW = inputRow.getWidth() / 5;

        auto keyCol = inputRow.removeFromLeft(inputColW).reduced(4);
        rootLabel.setBounds(keyCol.removeFromTop(13));
        rootBox.setBounds(keyCol.removeFromTop(24));
        auto scaleCol = inputRow.removeFromLeft(inputColW).reduced(4);
        scaleLabel.setBounds(scaleCol.removeFromTop(13));
        scaleBox.setBounds(scaleCol.removeFromTop(24));
        auto trackCol = inputRow.removeFromLeft(inputColW).reduced(2);
        trackingKnob.label.setBounds(trackCol.removeFromTop(12));
        trackingKnob.slider.setBounds(trackCol);
        auto glideCol = inputRow.removeFromLeft(inputColW).reduced(2);
        glideKnob.label.setBounds(glideCol.removeFromTop(12));
        glideKnob.slider.setBounds(glideCol);
        auto freezeCol = inputRow.reduced(4);
        freezeButton.setBounds(freezeCol.withY(freezeCol.getY() + 16).withHeight(24));

        area.removeFromTop(6);
        drySectionLabel.setBounds(area.removeFromTop(14));
        auto dryRow = area.removeFromTop(74);
        int dryColW = dryRow.getWidth() / 5;
        for (auto* k : { &dryLevelKnob, &dryPanKnob, &dryFormantKnob, &dryPitchKnob, &dryWidthKnob })
        {
            auto cell = dryRow.removeFromLeft(dryColW);
            k->label.setBounds(cell.removeFromTop(12));
            k->slider.setBounds(cell.reduced(6, 0));
        }

        area.removeFromTop(6);
        characterSectionLabel.setBounds(area.removeFromTop(14));
        auto charRow = area.removeFromTop(74);
        int charColW = charRow.getWidth() / 3;
        for (auto* k : { &humanizeKnob, &characterKnob, &spreadKnob })
        {
            auto cell = charRow.removeFromLeft(charColW);
            k->label.setBounds(cell.removeFromTop(12));
            k->slider.setBounds(cell.reduced(6, 0));
        }

        area.removeFromTop(6);
        auto bottomRow = area.removeFromTop(74);
        int thirdW = bottomRow.getWidth() / 3;
        auto ambCol = bottomRow.removeFromLeft(thirdW);
        ambienceSectionLabel.setBounds(ambCol.removeFromTop(14));
        ambienceKnob.label.setBounds(ambCol.removeFromTop(12));
        ambienceKnob.slider.setBounds(ambCol.reduced(6, 0));

        auto mixCol = bottomRow.removeFromLeft(thirdW);
        mixSectionLabel.setBounds(mixCol.removeFromTop(14));
        harmonyMixKnob.label.setBounds(mixCol.removeFromTop(12));
        harmonyMixKnob.slider.setBounds(mixCol.reduced(6, 0));

        auto glueCol = bottomRow;
        globalSaturationKnob.label.setBounds(glueCol.removeFromTop(12));
        globalSaturationKnob.slider.setBounds(glueCol.reduced(6, 0));
    }
    else
    {
        auto harmonyHeader = area.removeFromTop(22);
        harmonySectionLabel.setBounds(harmonyHeader.removeFromLeft(120));
        advancedButton.setBounds(harmonyHeader.removeFromRight(105).reduced(1, 1));
        area.removeFromTop(6);

        int voiceColW = area.getWidth() / kNumHarmonyVoices;

        for (int i = 0; i < kNumHarmonyVoices; ++i)
        {
            auto col = area.withWidth(voiceColW).withX(area.getX() + i * voiceColW).reduced(8, 2);
            auto& c = voiceColumns[(size_t) i];

            c.title.setBounds(col.removeFromTop(16));

            auto toggleRow = col.removeFromTop(18);
            int half = toggleRow.getWidth() / 2;
            c.enableButton.setBounds(toggleRow.removeFromLeft(half));
            c.soloButton.setBounds(toggleRow);

            col.removeFromTop(4);
            c.intervalBox.setBounds(col.removeFromTop(24));
            col.removeFromTop(6);

            // Keep the remaining column area intact for the Advanced rows.
            // The old code consumed the complete column with removeFromLeft,
            // leaving the Advanced controls a zero-width rectangle.
            auto basicRow = col.removeFromTop(76);
            int third = basicRow.getWidth() / 3;
            auto levelCol = basicRow.removeFromLeft(third).reduced(2);
            auto panCol = basicRow.removeFromLeft(third).reduced(2);
            auto formantCol = basicRow.reduced(2);

            c.levelLabel.setBounds(levelCol.removeFromTop(11));
            c.levelSlider.setBounds(levelCol.removeFromTop(64));
            c.panLabel.setBounds(panCol.removeFromTop(11));
            c.panSlider.setBounds(panCol.removeFromTop(64));
            c.formantLabel.setBounds(formantCol.removeFromTop(11));
            c.formantSlider.setBounds(formantCol.removeFromTop(64));

            if (showAdvanced)
            {
                col.removeFromTop(10);
                auto advancedRow1 = col.removeFromTop(54);
                auto fineCol = advancedRow1.removeFromLeft(advancedRow1.getWidth() / 2).reduced(1);
                auto toneCol = advancedRow1.reduced(1);
                c.fineTuneLabel.setBounds(fineCol.removeFromTop(11));
                c.fineTuneSlider.setBounds(fineCol);
                c.toneLabel.setBounds(toneCol.removeFromTop(11));
                c.toneSlider.setBounds(toneCol);

                col.removeFromTop(4);
                auto advancedRow2 = col.removeFromTop(54);
                auto saturationCol = advancedRow2.removeFromLeft(advancedRow2.getWidth() / 2).reduced(1);
                auto delayCol = advancedRow2.reduced(1);
                c.saturationLabel.setBounds(saturationCol.removeFromTop(11));
                c.saturationSlider.setBounds(saturationCol);
                c.microDelayLabel.setBounds(delayCol.removeFromTop(11));
                c.microDelaySlider.setBounds(delayCol);

                col.removeFromTop(4);
                auto vibratoRow = col.removeFromTop(54).reduced(1);
                auto vibratoCol = vibratoRow.removeFromLeft(vibratoRow.getWidth() / 2).reduced(1);
                auto vibratoRateCol = vibratoRow.reduced(1);
                c.vibratoLabel.setBounds(vibratoCol.removeFromTop(11));
                c.vibratoSlider.setBounds(vibratoCol);
                c.vibratoRateLabel.setBounds(vibratoRateCol.removeFromTop(11));
                c.vibratoRateSlider.setBounds(vibratoRateCol);
            }
        }
    }
}

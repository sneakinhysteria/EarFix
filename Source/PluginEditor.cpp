/*
  ==============================================================================

    EarFix Hearing Correction - Alpha
    Plugin Editor (UI) - Premium machined aluminum styling

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Sortable parameter ID suffixes (must match processor)
static const std::array<juce::String, 6> rightParamSuffixes = {
    "01", "02", "03", "04", "05", "06"
};
static const std::array<juce::String, 6> leftParamSuffixes = {
    "07", "08", "09", "10", "11", "12"
};

//==============================================================================
HearingCorrectionAUv2AudioProcessorEditor::HearingCorrectionAUv2AudioProcessorEditor (
    HearingCorrectionAUv2AudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    // Vertical sliders for Strength and Output
    correctionStrengthSlider.setSliderStyle (juce::Slider::LinearVertical);
    correctionStrengthSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 18);
    correctionStrengthSlider.setTextValueSuffix ("%");
    correctionStrengthSlider.setColour (juce::Slider::textBoxTextColourId, CustomLookAndFeel::textDark);
    correctionStrengthSlider.setColour (juce::Slider::textBoxBackgroundColourId, CustomLookAndFeel::panelWhite);
    correctionStrengthSlider.setColour (juce::Slider::textBoxOutlineColourId, CustomLookAndFeel::borderNeutral);
    addAndMakeVisible (correctionStrengthSlider);
    correctionLabel.setText ("STRENGTH", juce::dontSendNotification);
    correctionLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    correctionLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    correctionLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (correctionLabel);

    outputGainSlider.setSliderStyle (juce::Slider::LinearVertical);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 18);
    outputGainSlider.setTextValueSuffix (" dB");
    outputGainSlider.setColour (juce::Slider::textBoxTextColourId, CustomLookAndFeel::textDark);
    outputGainSlider.setColour (juce::Slider::textBoxBackgroundColourId, CustomLookAndFeel::panelWhite);
    outputGainSlider.setColour (juce::Slider::textBoxOutlineColourId, CustomLookAndFeel::borderNeutral);
    addAndMakeVisible (outputGainSlider);
    outputGainLabel.setText ("OUTPUT", juce::dontSendNotification);
    outputGainLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    outputGainLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    outputGainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputGainLabel);

    // Model selector
    modelSelector.addItem ("Half-Gain", 1);
    modelSelector.addItem ("NAL (Speech)", 2);
    modelSelector.addItem ("MOSL (Music)", 3);
    addAndMakeVisible (modelSelector);
    modelLabel.setText ("MODEL", juce::dontSendNotification);
    modelLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    modelLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    modelLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (modelLabel);

    // Compression speed selector
    compressionSpeedSelector.addItem ("Fast", 1);
    compressionSpeedSelector.addItem ("Slow", 2);
    addAndMakeVisible (compressionSpeedSelector);
    compressionSpeedLabel.setText ("SPEED", juce::dontSendNotification);
    compressionSpeedLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    compressionSpeedLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    compressionSpeedLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (compressionSpeedLabel);

    // Experience level selector
    experienceLevelSelector.addItem ("New", 1);
    experienceLevelSelector.addItem ("Some", 2);
    experienceLevelSelector.addItem ("Experienced", 3);
    addAndMakeVisible (experienceLevelSelector);
    experienceLevelLabel.setText ("LEVEL", juce::dontSendNotification);
    experienceLevelLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    experienceLevelLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    experienceLevelLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (experienceLevelLabel);

    // Loudness mode selector
    loudnessModeSelector.addItem ("Centered", 1);
    loudnessModeSelector.addItem ("Boost Only", 2);
    addAndMakeVisible (loudnessModeSelector);
    loudnessModeLabel.setText ("LOUDNESS", juce::dontSendNotification);
    loudnessModeLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    loudnessModeLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    loudnessModeLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (loudnessModeLabel);

    // Max boost slider (vertical fader in control section)
    maxBoostSlider.setSliderStyle (juce::Slider::LinearVertical);
    maxBoostSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 18);
    maxBoostSlider.setTextValueSuffix (" dB");
    maxBoostSlider.setColour (juce::Slider::textBoxTextColourId, CustomLookAndFeel::textDark);
    maxBoostSlider.setColour (juce::Slider::textBoxBackgroundColourId, CustomLookAndFeel::panelWhite);
    maxBoostSlider.setColour (juce::Slider::textBoxOutlineColourId, CustomLookAndFeel::borderNeutral);
    addAndMakeVisible (maxBoostSlider);
    maxBoostLabel.setText ("MAX BOOST", juce::dontSendNotification);
    maxBoostLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    maxBoostLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    maxBoostLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (maxBoostLabel);

    // Meter labels (same style as fader labels for consistency)
    inputMeterLabel.setText ("IN", juce::dontSendNotification);
    inputMeterLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    inputMeterLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    inputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (inputMeterLabel);

    outputMeterLabel.setText ("OUT", juce::dontSendNotification);
    outputMeterLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    outputMeterLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    outputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputMeterLabel);

    // Enable buttons
    rightEnableButton.setName ("right");
    leftEnableButton.setName ("left");
    addAndMakeVisible (rightEnableButton);
    addAndMakeVisible (leftEnableButton);
    rightEnableButton.onClick = [this]() { onEarEnableClicked (true); };
    leftEnableButton.onClick  = [this]() { onEarEnableClicked (false); };

    // Link toggle: click either to link/unlink both ears' enable state
    for (auto* b : { &rightLinkButton, &leftLinkButton })
    {
        b->setClickingTogglesState (false);  // we manage the visual state ourselves
        b->setColour (juce::TextButton::buttonColourId, CustomLookAndFeel::panelWhite);
        b->setColour (juce::TextButton::buttonOnColourId, CustomLookAndFeel::accentBlue);
        addAndMakeVisible (*b);
    }
    rightLinkButton.onClick = [this]() { setEarsLinked (! earsLinked); };
    leftLinkButton.onClick  = [this]() { setEarsLinked (! earsLinked); };
    setEarsLinked (false);

    // Headphone EQ components
    headphoneSelector.onChange = [this]() {
        auto selectedName = headphoneSelector.getText();
        if (headphoneSelector.getSelectedId() == 1)
            selectedName = ""; // "-- None --" option
        audioProcessor.loadHeadphoneProfile (selectedName);
        updateHeadphoneInfo();
    };
    addAndMakeVisible (headphoneSelector);
    populateHeadphoneList();

    headphoneEnableButton.setName ("headphoneEQ");
    addAndMakeVisible (headphoneEnableButton);

    headphoneRefreshButton.setColour (juce::TextButton::buttonColourId, CustomLookAndFeel::panelWhite);
    headphoneRefreshButton.setColour (juce::TextButton::textColourOffId, CustomLookAndFeel::textDark);
    headphoneRefreshButton.onClick = [this]() {
        audioProcessor.reloadHeadphoneDatabase();
        populateHeadphoneList();
    };
    addAndMakeVisible (headphoneRefreshButton);

    headphoneImportButton.setColour (juce::TextButton::buttonColourId, CustomLookAndFeel::panelWhite);
    headphoneImportButton.setColour (juce::TextButton::textColourOffId, CustomLookAndFeel::textDark);
    headphoneImportButton.onClick = [this]() { openHeadphoneImportDialog(); };
    addAndMakeVisible (headphoneImportButton);

    // Preset buttons (top-right)
    for (auto* b : { &savePresetButton, &loadPresetButton })
    {
        b->setColour (juce::TextButton::buttonColourId, CustomLookAndFeel::panelWhite);
        b->setColour (juce::TextButton::textColourOffId, CustomLookAndFeel::textDark);
        addAndMakeVisible (*b);
    }
    savePresetButton.onClick = [this]() { savePreset(); };
    loadPresetButton.onClick = [this]() { loadPreset(); };

    headphoneInfoLabel.setFont (juce::FontOptions (10.0f));
    headphoneInfoLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    headphoneInfoLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (headphoneInfoLabel);
    updateHeadphoneInfo();

    // Ear labels
    rightEarLabel.setText ("Right ear", juce::dontSendNotification);
    rightEarLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (rightEarLabel);

    leftEarLabel.setText ("Left ear", juce::dontSendNotification);
    leftEarLabel.setJustificationType (juce::Justification::centredLeft);  // Same as right ear
    addAndMakeVisible (leftEarLabel);

    // Audiogram components
    addAndMakeVisible (rightAudiogram);
    addAndMakeVisible (leftAudiogram);

    // Set up audiogram parameter attachments
    juce::StringArray rightParamIds, leftParamIds;
    for (int i = 0; i < 6; ++i)
    {
        rightParamIds.add ("audiogram_" + rightParamSuffixes[i]);
        leftParamIds.add ("audiogram_" + leftParamSuffixes[i]);
    }
    rightAudiogram.setParameterAttachments (audioProcessor.parameters, rightParamIds);
    leftAudiogram.setParameterAttachments (audioProcessor.parameters, leftParamIds);

    // Create APVTS attachments
    outputGainAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "outputGain", outputGainSlider);
    correctionStrengthAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "correctionStrength", correctionStrengthSlider);
    maxBoostAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.parameters, "maxBoost", maxBoostSlider);
    modelSelectAttachment = std::make_unique<ComboBoxAttachment> (
        audioProcessor.parameters, "modelSelect", modelSelector);
    compressionSpeedAttachment = std::make_unique<ComboBoxAttachment> (
        audioProcessor.parameters, "compressionSpeed", compressionSpeedSelector);
    experienceLevelAttachment = std::make_unique<ComboBoxAttachment> (
        audioProcessor.parameters, "experienceLevel", experienceLevelSelector);
    loudnessModeAttachment = std::make_unique<ComboBoxAttachment> (
        audioProcessor.parameters, "loudnessMode", loudnessModeSelector);
    rightEnableAttachment = std::make_unique<ButtonAttachment> (
        audioProcessor.parameters, "rightEnable", rightEnableButton);
    leftEnableAttachment = std::make_unique<ButtonAttachment> (
        audioProcessor.parameters, "leftEnable", leftEnableButton);
    headphoneEnableAttachment = std::make_unique<ButtonAttachment> (
        audioProcessor.parameters, "headphoneEQEnable", headphoneEnableButton);

    // Listen for model changes
    audioProcessor.parameters.addParameterListener ("modelSelect", this);
    updateNALOptionsVisibility();

    // Start timer for meter updates
    startTimerHz (30);

    setSize (560, 580);  // Compact height - audiograms fill available space
}

HearingCorrectionAUv2AudioProcessorEditor::~HearingCorrectionAUv2AudioProcessorEditor()
{
    stopTimer();
    audioProcessor.parameters.removeParameterListener ("modelSelect", this);
    setLookAndFeel (nullptr);
}

//==============================================================================
void HearingCorrectionAUv2AudioProcessorEditor::timerCallback()
{
    // Smooth meter decay
    const float decay = 0.8f;
    const float attack = 0.5f;

    auto updateLevel = [] (float& display, float target, float att, float dec) {
        display = (target > display) ? (display + att * (target - display))
                                     : (display * dec);
    };

    updateLevel (displayInputL, audioProcessor.inputLevelLeft.load (std::memory_order_relaxed), attack, decay);
    updateLevel (displayInputR, audioProcessor.inputLevelRight.load (std::memory_order_relaxed), attack, decay);
    updateLevel (displayOutputL, audioProcessor.outputLevelLeft.load (std::memory_order_relaxed), attack, decay);
    updateLevel (displayOutputR, audioProcessor.outputLevelRight.load (std::memory_order_relaxed), attack, decay);

    // Push the live applied-correction curve to the audiogram overlay
    std::array<float, 6> leftGains {}, rightGains {};
    for (int i = 0; i < 6; ++i)
    {
        leftGains[i]  = audioProcessor.leftAppliedGainDb[i].load (std::memory_order_relaxed);
        rightGains[i] = audioProcessor.rightAppliedGainDb[i].load (std::memory_order_relaxed);
    }
    leftAudiogram.setAppliedCorrection (leftGains);
    rightAudiogram.setAppliedCorrection (rightGains);

    // Dim Max Boost when it isn't currently constraining the curve for the active
    // audiogram/model/strength/loudness-mode combination -- raising it further
    // wouldn't change anything right now (though it may start to as those change).
    const bool maxBoostActive = audioProcessor.maxBoostActive.load (std::memory_order_relaxed);
    const float maxBoostAlpha = maxBoostActive ? 1.0f : 0.45f;
    maxBoostSlider.setAlpha (maxBoostAlpha);
    maxBoostLabel.setAlpha (maxBoostAlpha);

    repaint();
}

void HearingCorrectionAUv2AudioProcessorEditor::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID == "modelSelect")
        juce::MessageManager::callAsync ([this]() { updateNALOptionsVisibility(); });
}

void HearingCorrectionAUv2AudioProcessorEditor::updateNALOptionsVisibility()
{
    auto* modelParam = audioProcessor.parameters.getRawParameterValue ("modelSelect");
    int modelIndex = modelParam != nullptr ? static_cast<int> (modelParam->load()) : 0;
    bool showCompressionOptions = (modelIndex >= 1);

    compressionSpeedLabel.setVisible (showCompressionOptions);
    compressionSpeedSelector.setVisible (showCompressionOptions);
    experienceLevelLabel.setVisible (showCompressionOptions);
    experienceLevelSelector.setVisible (showCompressionOptions);
    repaint();
}

void HearingCorrectionAUv2AudioProcessorEditor::setEarsLinked (bool linked)
{
    earsLinked = linked;
    rightLinkButton.setToggleState (linked, juce::dontSendNotification);
    leftLinkButton.setToggleState (linked, juce::dontSendNotification);
    repaint();
}

void HearingCorrectionAUv2AudioProcessorEditor::onEarEnableClicked (bool isRight)
{
    if (! earsLinked)
        return;

    // Mirror the just-clicked ear's new state onto the other ear's parameter.
    // ButtonAttachment has already applied the click to its own parameter by the
    // time onClick fires, so read the clicked button's current state and copy it.
    bool newState = isRight ? rightEnableButton.getToggleState() : leftEnableButton.getToggleState();
    const char* otherParamId = isRight ? "leftEnable" : "rightEnable";

    if (auto* param = audioProcessor.parameters.getParameter (otherParamId))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (newState ? 1.0f : 0.0f);
        param->endChangeGesture();
    }
}

void HearingCorrectionAUv2AudioProcessorEditor::populateHeadphoneList()
{
    headphoneSelector.clear();
    headphoneSelector.addItem ("-- None --", 1);

    const auto& headphones = audioProcessor.getAvailableHeadphones();
    int itemId = 2;
    for (const auto& hp : headphones)
    {
        headphoneSelector.addItem (hp.name, itemId++);
    }

    // Select current profile if any
    auto currentName = audioProcessor.getCurrentHeadphoneName();
    if (currentName.isEmpty())
    {
        headphoneSelector.setSelectedId (1, juce::dontSendNotification);
    }
    else
    {
        for (int i = 0; i < headphoneSelector.getNumItems(); ++i)
        {
            if (headphoneSelector.getItemText (i) == currentName)
            {
                headphoneSelector.setSelectedItemIndex (i, juce::dontSendNotification);
                break;
            }
        }
    }
}

void HearingCorrectionAUv2AudioProcessorEditor::updateHeadphoneInfo()
{
    auto currentName = audioProcessor.getCurrentHeadphoneName();
    if (currentName.isEmpty())
    {
        headphoneInfoLabel.setText ("Select headphone model for EQ correction", juce::dontSendNotification);
        return;
    }

    // Find the headphone info
    const auto& headphones = audioProcessor.getAvailableHeadphones();
    for (const auto& hp : headphones)
    {
        if (hp.name == currentName)
        {
            // Only show source (type is often unknown)
            juce::String info = "Source: " + hp.source;
            headphoneInfoLabel.setText (info, juce::dontSendNotification);
            return;
        }
    }

    headphoneInfoLabel.setText ("", juce::dontSendNotification);
}

//==============================================================================
void HearingCorrectionAUv2AudioProcessorEditor::paint (juce::Graphics& g)
{
    CustomLookAndFeel::drawAluminumBackground (g, getLocalBounds());

    // Universal spacing (must match resized())
    const int MARGIN = 16, HEADER_H = 16, GAP = 6;
    auto bounds = getLocalBounds().toFloat().reduced (MARGIN);

    // === HEADPHONE CORRECTION header ===
    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    g.drawText ("HEADPHONE CORRECTION", bounds.removeFromTop (HEADER_H), juce::Justification::centred);

    // Draw headphone panel
    if (!headphonePanelBounds.isEmpty())
    {
        const int PAD = 10;  // Must match PANEL_PAD
        CustomLookAndFeel::drawMachinedPanel (g, headphonePanelBounds, 8.0f);

        // Headphone emoji icon (at top-left with padding)
        g.setFont (juce::FontOptions (18.0f));
        g.setColour (CustomLookAndFeel::textDark);
        g.drawText (juce::String::fromUTF8 ("\xF0\x9F\x8E\xA7"),
                   headphonePanelBounds.getX() + PAD, headphonePanelBounds.getY() + PAD,
                   28, 26, juce::Justification::centred);
    }

    // === AUDIOGRAM header ===
    float audiogramHeaderY = headphonePanelBounds.getBottom() + GAP;
    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    g.drawText ("AUDIOGRAM", MARGIN, audiogramHeaderY, getWidth() - 2 * MARGIN, HEADER_H, juce::Justification::centred);

    // Draw audiogram panels with R/L indicators
    if (!audiogramPanelBounds.isEmpty())
    {
        const int chartGap = 12;
        const int PAD = 10;  // Must match PANEL_PAD
        auto agArea = audiogramPanelBounds;
        auto rPanel = agArea.removeFromLeft ((agArea.getWidth() - chartGap) / 2);
        agArea.removeFromLeft (chartGap);
        auto lPanel = agArea;

        CustomLookAndFeel::drawMachinedPanel (g, rPanel, 8.0f);
        CustomLookAndFeel::drawMachinedPanel (g, lPanel, 8.0f);

        // R/L circles: toggle at (X+PAD, Y+PAD), circle after toggle
        float circleSize = 20.0f;
        float circleY = rPanel.getY() + PAD;  // Aligned with toggle

        // R circle (after toggle: X + PAD + 36 + 4)
        float rCircleX = rPanel.getX() + PAD + 36 + 4;
        g.setColour (CustomLookAndFeel::accentRed);
        g.fillEllipse (rCircleX, circleY, circleSize, circleSize);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
        g.drawText ("R", rCircleX, circleY, circleSize, circleSize, juce::Justification::centred);

        // L circle
        float lCircleX = lPanel.getX() + PAD + 36 + 4;
        g.setColour (CustomLookAndFeel::accentBlue);
        g.fillEllipse (lCircleX, circleY, circleSize, circleSize);
        g.setColour (juce::Colours::white);
        g.drawText ("L", lCircleX, circleY, circleSize, circleSize, juce::Justification::centred);
    }

    // === HEARING LOSS CORRECTION header ===
    float hlHeaderY = audiogramPanelBounds.getBottom() + GAP;
    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    g.drawText ("HEARING LOSS CORRECTION MODEL & PARAMETERS", MARGIN, hlHeaderY, getWidth() - 2 * MARGIN, HEADER_H, juce::Justification::centred);

    // Draw control panel
    if (!controlPanelBounds.isEmpty())
    {
        const int PAD = 10;
        CustomLookAndFeel::drawMachinedPanel (g, controlPanelBounds, 8.0f);

        // Divider (after 28% dropdown section + padding)
        auto dividerX = controlPanelBounds.getX() + PAD + controlPanelBounds.getWidth() * 0.28f;
        g.setColour (CustomLookAndFeel::borderNeutral);
        g.drawVerticalLine (static_cast<int> (dividerX),
                           controlPanelBounds.getY() + PAD,
                           controlPanelBounds.getBottom() - PAD);

        // Input meters
        if (!inputMeterBounds.isEmpty())
        {
            drawMeter (g, inputMeterBounds.getX(), inputMeterBounds.getY(),
                      10, inputMeterBounds.getHeight(), displayInputL);
            drawMeter (g, inputMeterBounds.getX() + 12, inputMeterBounds.getY(),
                      10, inputMeterBounds.getHeight(), displayInputR);
        }

        // Output meters
        if (!outputMeterBounds.isEmpty())
        {
            drawMeter (g, outputMeterBounds.getX(), outputMeterBounds.getY(),
                      10, outputMeterBounds.getHeight(), displayOutputL);
            drawMeter (g, outputMeterBounds.getX() + 12, outputMeterBounds.getY(),
                      10, outputMeterBounds.getHeight(), displayOutputR);
        }

    }

    // Version footer
    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("v" JucePlugin_VersionString, 0, getHeight() - 24, getWidth(), 20, juce::Justification::centred);
}

void HearingCorrectionAUv2AudioProcessorEditor::drawMeter (juce::Graphics& g, float x, float y,
                                                            float w, float h, float level)
{
    // Background
    g.setColour (juce::Colour (0xff333333));
    g.fillRoundedRectangle (x, y, w, h, 2.0f);

    // Level fill with gradient
    float fillHeight = h * juce::jlimit (0.0f, 1.0f, level);
    if (fillHeight > 0)
    {
        juce::ColourGradient gradient (
            CustomLookAndFeel::meterGreen, x, y + h,
            CustomLookAndFeel::meterRed, x, y,
            false);
        gradient.addColour (0.6, CustomLookAndFeel::meterGreen);
        gradient.addColour (0.8, CustomLookAndFeel::meterYellow);

        g.setGradientFill (gradient);
        g.fillRoundedRectangle (x, y + h - fillHeight, w, fillHeight, 2.0f);
    }
}

void HearingCorrectionAUv2AudioProcessorEditor::resized()
{
    // ============ UNIVERSAL SPACING RULES ============
    const int MARGIN = 16;           // Window edge margin
    const int PANEL_PAD = 10;        // Panel internal padding
    const int HEADER_H = 16;         // Section header height
    const int GAP = 6;               // Gap between sections
    const int VERSION_H = 20;        // Space for version at bottom

    // Preset buttons: top-right corner, centred in the band between the card's top
    // edge (y = 0) and the headphone panel's top (y = MARGIN + HEADER_H), so the
    // padding above the buttons equals the padding below them.
    const int presetBtnW = 46, presetBtnH = 20, presetGap = 6;
    const int presetBandH = MARGIN + HEADER_H;
    const int presetBtnY = (presetBandH - presetBtnH) / 2;
    loadPresetButton.setBounds (getWidth() - MARGIN - presetBtnW, presetBtnY, presetBtnW, presetBtnH);
    savePresetButton.setBounds (loadPresetButton.getX() - presetGap - presetBtnW, presetBtnY, presetBtnW, presetBtnH);

    // ============ LAYOUT CALCULATION ============
    auto bounds = getLocalBounds().reduced (MARGIN);
    bounds.removeFromBottom (VERSION_H);  // Reserve for version label

    // Fixed heights
    const int HP_PANEL_H = 60;       // Headphone panel (room for dropdown + info)
    const int CTRL_PANEL_H = 200;    // Control panel (4 dropdown rows: Model/Speed/Level/Loudness)

    // Calculate audiogram height to fill remaining space
    int usedHeight = HEADER_H + HP_PANEL_H + GAP + HEADER_H + GAP + HEADER_H + CTRL_PANEL_H;
    int audiogramHeight = bounds.getHeight() - usedHeight;

    // ============ 1. HEADPHONE SECTION ============
    bounds.removeFromTop (HEADER_H);
    headphonePanelBounds = bounds.removeFromTop (HP_PANEL_H).toFloat();

    // Content area with PANEL_PAD from all edges
    int hpX = static_cast<int>(headphonePanelBounds.getX()) + PANEL_PAD;
    int hpY = static_cast<int>(headphonePanelBounds.getY()) + PANEL_PAD;
    int hpW = static_cast<int>(headphonePanelBounds.getWidth()) - 2 * PANEL_PAD;
    int hpH = static_cast<int>(headphonePanelBounds.getHeight()) - 2 * PANEL_PAD;

    // Row 1: icon, dropdown, toggle, import, refresh
    int iconW = 28, toggleW = 40, refreshW = 50, importW = 50, elemH = 26;
    int refreshX = hpX + hpW - refreshW;
    int importX  = refreshX - 6 - importW;
    int toggleX  = importX - 8 - toggleW;
    int dropX = hpX + iconW + 8;
    int dropW = toggleX - 8 - dropX;

    headphoneSelector.setBounds (dropX, hpY, dropW, elemH);
    headphoneEnableButton.setBounds (toggleX, hpY + 3, toggleW, 20);
    headphoneImportButton.setBounds (importX, hpY, importW, elemH);
    headphoneRefreshButton.setBounds (refreshX, hpY, refreshW, elemH);

    // Row 2: info label (with padding from bottom)
    headphoneInfoLabel.setBounds (dropX, hpY + hpH - 12, dropW, 12);

    bounds.removeFromTop (GAP);

    // ============ 2. AUDIOGRAM SECTION ============
    bounds.removeFromTop (HEADER_H);
    audiogramPanelBounds = bounds.removeFromTop (audiogramHeight).toFloat();

    auto agArea = audiogramPanelBounds.toNearestInt();
    const int chartGap = 12;
    const int chartW = (agArea.getWidth() - chartGap) / 2;
    const int toggleRowH = 24;  // Toggle + circle + label row height

    const int linkW = 22;

    // Right ear panel (left side)
    auto rPanel = agArea.removeFromLeft (chartW);
    int agContentY = rPanel.getY() + PANEL_PAD;
    rightEnableButton.setBounds (rPanel.getX() + PANEL_PAD, agContentY, 36, 20);
    rightEarLabel.setBounds (rPanel.getX() + PANEL_PAD + 36 + 24 + 4, agContentY, 80, 20);
    rightLinkButton.setBounds (rPanel.getRight() - PANEL_PAD - linkW, agContentY, linkW, 20);
    // Chart starts after toggle row + 10px gap (PANEL_PAD)
    int chartTop = agContentY + toggleRowH + PANEL_PAD;
    rightAudiogram.setBounds (rPanel.getX(), chartTop,
                              rPanel.getWidth(), rPanel.getBottom() - chartTop);

    agArea.removeFromLeft (chartGap);

    // Left ear panel (right side)
    auto lPanel = agArea;
    leftEnableButton.setBounds (lPanel.getX() + PANEL_PAD, agContentY, 36, 20);
    leftEarLabel.setBounds (lPanel.getX() + PANEL_PAD + 36 + 24 + 4, agContentY, 80, 20);
    leftLinkButton.setBounds (lPanel.getRight() - PANEL_PAD - linkW, agContentY, linkW, 20);
    leftAudiogram.setBounds (lPanel.getX(), chartTop,
                             lPanel.getWidth(), lPanel.getBottom() - chartTop);

    bounds.removeFromTop (GAP);

    // ============ 3. CONTROL SECTION ============
    bounds.removeFromTop (HEADER_H);
    controlPanelBounds = bounds.toFloat();
    auto ctrlArea = controlPanelBounds.reduced (PANEL_PAD).toNearestInt();

    // --- Left side: dropdowns (28% width) ---
    int dropdownW = static_cast<int> (ctrlArea.getWidth() * 0.28f);
    auto ddArea = ctrlArea.removeFromLeft (dropdownW);

    const int ddH = 26, lblH = 14, ddGap = 4;
    int totalDDH = 4 * (lblH + ddH) + 3 * ddGap;
    int ddStartY = ddArea.getY() + (ddArea.getHeight() - totalDDH) / 2;

    modelLabel.setBounds (ddArea.getX(), ddStartY, ddArea.getWidth(), lblH);
    modelSelector.setBounds (ddArea.getX(), ddStartY + lblH, ddArea.getWidth(), ddH);

    int y2 = ddStartY + lblH + ddH + ddGap;
    compressionSpeedLabel.setBounds (ddArea.getX(), y2, ddArea.getWidth(), lblH);
    compressionSpeedSelector.setBounds (ddArea.getX(), y2 + lblH, ddArea.getWidth(), ddH);

    int y3 = y2 + lblH + ddH + ddGap;
    experienceLevelLabel.setBounds (ddArea.getX(), y3, ddArea.getWidth(), lblH);
    experienceLevelSelector.setBounds (ddArea.getX(), y3 + lblH, ddArea.getWidth(), ddH);

    int y4 = y3 + lblH + ddH + ddGap;
    loudnessModeLabel.setBounds (ddArea.getX(), y4, ddArea.getWidth(), lblH);
    loudnessModeSelector.setBounds (ddArea.getX(), y4 + lblH, ddArea.getWidth(), ddH);

    // --- Right side: 5 equal columns (IN meter, STRENGTH, MAX BOOST, OUTPUT, OUT meter) ---
    // Columns fill the space between the divider and the right edge with equal padding.
    // Each element is horizontally centred on its column; the title/control/value block
    // is vertically centred with equal top/bottom margin.
    ctrlArea.removeFromLeft (PANEL_PAD);  // gap after the divider
    auto mfArea = ctrlArea;

    const int LBL_H     = 14;
    const int meterW    = 22;
    const int faderW    = 40;
    const int valueBoxH = 18;   // matches setTextBoxStyle height

    const int mfY = mfArea.getY();

    const int titleGap = 6;
    const int valueGap = CustomLookAndFeel::faderValueGap;
    const int titleY   = mfY;                            // top padding == value-box bottom padding (PANEL_PAD)
    const int barTop   = titleY + LBL_H + titleGap;      // meter top; also the knob's top at max
    const int faderBottom = mfArea.getBottom() + valueGap;          // extends into the card pad so the value box drops
    const int meterBottom = faderBottom - valueBoxH - valueGap;     // meter bottom; also the knob's bottom at min
    const int faderTop = barTop;                         // travel is inset inside the look-and-feel

    const int N    = 5;
    const int colW = mfArea.getWidth() / N;

    auto colCentre  = [&] (int i) { return mfArea.getX() + colW * i + colW / 2; };
    auto placeTitle = [&] (juce::Label& l, int i)
    {
        l.setJustificationType (juce::Justification::centred);
        l.setBounds (mfArea.getX() + colW * i, titleY, colW, LBL_H);
    };
    auto placeFader = [&] (juce::Slider& s, int i)
    {
        s.setBounds (colCentre (i) - faderW / 2, faderTop, faderW, faderBottom - faderTop);
    };
    auto meterRect  = [&] (int i)
    {
        return juce::Rectangle<float> ((float) (colCentre (i) - meterW / 2), (float) barTop,
                                       (float) meterW, (float) (meterBottom - barTop));
    };

    placeTitle (inputMeterLabel,  0);  inputMeterBounds  = meterRect (0);
    placeTitle (correctionLabel,  1);  placeFader (correctionStrengthSlider, 1);
    placeTitle (maxBoostLabel,    2);  placeFader (maxBoostSlider,           2);
    placeTitle (outputGainLabel,  3);  placeFader (outputGainSlider,         3);
    placeTitle (outputMeterLabel, 4);  outputMeterBounds = meterRect (4);
}

//==============================================================================
// Headphone EQ paste-import dialog
namespace {

class HeadphoneImportComponent : public juce::Component
{
public:
    std::function<juce::String (const juce::String&, const juce::String&)> onImport;
    std::function<void (const juce::String&)> onDone;

    HeadphoneImportComponent()
    {
        auto styleLabel = [this] (juce::Label& l, const juce::String& text, float size)
        {
            l.setText (text, juce::dontSendNotification);
            l.setFont (juce::FontOptions (size));
            l.setColour (juce::Label::textColourId, CustomLookAndFeel::textDark);
            addAndMakeVisible (l);
        };

        styleLabel (nameLabel, "Headphone name", 13.0f);
        nameEditor.setTextToShowWhenEmpty ("e.g. Sennheiser HD 660S2", juce::Colours::grey);
        addAndMakeVisible (nameEditor);

        styleLabel (pasteLabel, "Paste the Parametric EQ (Equalizer APO format):", 13.0f);

        styleLabel (hintLabel,
                    "squig.link: Equalizer tab → autoEQ → Export → Parametric.   "
                    "AutoEq: the …ParametricEQ.txt file.", 11.0f);
        hintLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);

        pasteEditor.setMultiLine (true, true);
        pasteEditor.setReturnKeyStartsNewLine (true);
        pasteEditor.setTextToShowWhenEmpty (
            "Preamp: -6.9 dB\n"
            "Filter 1: ON PK Fc 21 Hz Gain 6.7 dB Q 0.70\n"
            "Filter 2: ON PK Fc 120 Hz Gain -2.4 dB Q 1.10\n"
            "Filter 3: ON PK Fc 3000 Hz Gain 4.2 dB Q 2.00\n…",
            juce::Colours::grey);
        addAndMakeVisible (pasteEditor);

        styleLabel (sourcesLabel, "Find your headphone at:", 12.0f);
        addLink (autoEqButton,  "AutoEq",      "https://github.com/jaakkopasanen/AutoEq/tree/master/results");
        addLink (squigButton,   "squig.link",  "https://squig.link");
        addLink (oratoryButton, "oratory1990", "https://www.reddit.com/r/oratory1990/wiki/index/list_of_presets");

        statusLabel.setFont (juce::FontOptions (12.0f));
        statusLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::accentRed);
        addAndMakeVisible (statusLabel);

        saveButton.setButtonText ("Save");
        saveButton.onClick = [this] { doSave(); };
        addAndMakeVisible (saveButton);

        cancelButton.setButtonText ("Cancel");
        cancelButton.onClick = [this] { close(); };
        addAndMakeVisible (cancelButton);

        setSize (480, 420);
    }

    void paint (juce::Graphics& g) override { g.fillAll (CustomLookAndFeel::panelWhite); }

    void resized() override
    {
        auto r = getLocalBounds().reduced (14);

        nameLabel.setBounds (r.removeFromTop (18));
        nameEditor.setBounds (r.removeFromTop (26));
        r.removeFromTop (10);
        pasteLabel.setBounds (r.removeFromTop (18));
        hintLabel.setBounds (r.removeFromTop (16));
        r.removeFromTop (4);

        auto buttonRow = r.removeFromBottom (30);
        cancelButton.setBounds (buttonRow.removeFromRight (90));
        buttonRow.removeFromRight (8);
        saveButton.setBounds (buttonRow.removeFromRight (90));

        r.removeFromBottom (8);
        statusLabel.setBounds (r.removeFromBottom (20));

        r.removeFromBottom (8);
        auto sourcesRow = r.removeFromBottom (22);
        sourcesLabel.setBounds  (sourcesRow.removeFromLeft (130));
        autoEqButton.setBounds  (sourcesRow.removeFromLeft (64));  sourcesRow.removeFromLeft (6);
        squigButton.setBounds   (sourcesRow.removeFromLeft (78));  sourcesRow.removeFromLeft (6);
        oratoryButton.setBounds (sourcesRow.removeFromLeft (86));

        r.removeFromBottom (8);
        pasteEditor.setBounds (r);
    }

private:
    juce::Label nameLabel, pasteLabel, hintLabel, sourcesLabel, statusLabel;
    juce::TextEditor nameEditor, pasteEditor;
    juce::TextButton saveButton, cancelButton, autoEqButton, squigButton, oratoryButton;

    void addLink (juce::TextButton& b, const juce::String& text, const juce::String& url)
    {
        b.setButtonText (text);
        b.onClick = [url] { juce::URL (url).launchInDefaultBrowser(); };
        addAndMakeVisible (b);
    }

    void doSave()
    {
        auto name = nameEditor.getText().trim();
        if (name.isEmpty())
        {
            statusLabel.setText ("Enter a name.", juce::dontSendNotification);
            return;
        }

        auto raw = pasteEditor.getText();
        auto saved = onImport ? onImport (name, raw) : juce::String();
        if (saved.isEmpty())
        {
            if (raw.containsIgnoreCase ("GraphicEQ") || raw.contains ("; "))
                statusLabel.setText ("Graphic EQ detected — use the Parametric export instead.",
                                     juce::dontSendNotification);
            else
                statusLabel.setText ("No filters found — paste the Parametric EQ text.",
                                     juce::dontSendNotification);
            return;
        }

        if (onDone) onDone (saved);
        close();
    }

    void close()
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (0);
    }
};

} // namespace

void HearingCorrectionAUv2AudioProcessorEditor::openHeadphoneImportDialog()
{
    auto* comp = new HeadphoneImportComponent();

    comp->onImport = [this] (const juce::String& name, const juce::String& text)
    {
        return audioProcessor.importHeadphoneProfile (name, text);
    };

    comp->onDone = [this] (const juce::String& savedName)
    {
        populateHeadphoneList();
        for (int i = 0; i < headphoneSelector.getNumItems(); ++i)
            if (headphoneSelector.getItemText (i) == savedName)
            {
                headphoneSelector.setSelectedItemIndex (i, juce::sendNotification);
                break;
            }
    };

    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (comp);
    o.dialogTitle = "Import Headphone EQ";
    o.dialogBackgroundColour = CustomLookAndFeel::panelWhite;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = true;
    o.resizable = false;
    o.launchAsync();
}

//==============================================================================
// Presets are saved/loaded as Apple .aupreset files — the same format Logic and
// other AU hosts use — so EarFix presets are interchangeable with the host's own
// preset menu, and with presets from earlier EarFix versions (e.g. saved by Logic).
void HearingCorrectionAUv2AudioProcessorEditor::savePreset()
{
    auto dir = HearingCorrectionAUv2AudioProcessor::getPresetsDirectory();
    presetChooser = std::make_unique<juce::FileChooser> (
        "Save EarFix preset", dir.getChildFile ("EarFix Preset.aupreset"), "*.aupreset");

    auto flags = juce::FileBrowserComponent::saveMode
               | juce::FileBrowserComponent::canSelectFiles
               | juce::FileBrowserComponent::warnAboutOverwriting;

    presetChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{})
            return;
        if (! file.hasFileExtension ("aupreset"))
            file = file.withFileExtension ("aupreset");
        audioProcessor.saveAUPreset (file, file.getFileNameWithoutExtension());
    });
}

void HearingCorrectionAUv2AudioProcessorEditor::loadPreset()
{
    auto dir = HearingCorrectionAUv2AudioProcessor::getPresetsDirectory();
    presetChooser = std::make_unique<juce::FileChooser> (
        "Load EarFix preset", dir, "*.aupreset");

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectFiles;

    presetChooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (! file.existsAsFile())
            return;
        if (audioProcessor.loadAUPresetFile (file))
        {
            // APVTS-attached controls update themselves; refresh the non-parameter bits.
            populateHeadphoneList();
            updateHeadphoneInfo();
        }
    });
}

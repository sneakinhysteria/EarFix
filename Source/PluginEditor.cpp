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

    // Max boost slider (vertical fader in control section)
    maxBoostSlider.setSliderStyle (juce::Slider::LinearVertical);
    maxBoostSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 18);
    maxBoostSlider.setTextValueSuffix (" dB");
    maxBoostSlider.setColour (juce::Slider::textBoxTextColourId, CustomLookAndFeel::textDark);
    maxBoostSlider.setColour (juce::Slider::textBoxBackgroundColourId, CustomLookAndFeel::panelWhite);
    maxBoostSlider.setColour (juce::Slider::textBoxOutlineColourId, CustomLookAndFeel::borderNeutral);
    addAndMakeVisible (maxBoostSlider);
    maxBoostLabel.setText ("MAX", juce::dontSendNotification);  // Short label
    maxBoostLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    maxBoostLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    maxBoostLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (maxBoostLabel);

    // Auto-gain button - styled to match UI
    autoGainButton.setButtonText ("AUTO\nGAIN");
    autoGainButton.setColour (juce::TextButton::buttonColourId, CustomLookAndFeel::panelWhite);
    autoGainButton.setColour (juce::TextButton::buttonOnColourId, CustomLookAndFeel::accentBlue);
    autoGainButton.setColour (juce::TextButton::textColourOffId, CustomLookAndFeel::textDark);
    autoGainButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible (autoGainButton);

    // Meter labels (same style as fader labels for consistency)
    inputMeterLabel.setText ("INPUT", juce::dontSendNotification);
    inputMeterLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    inputMeterLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    inputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (inputMeterLabel);

    outputMeterLabel.setText ("", juce::dontSendNotification);  // No label - flows from OUTPUT fader
    outputMeterLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    outputMeterLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    outputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputMeterLabel);

    // Enable buttons
    rightEnableButton.setName ("right");
    leftEnableButton.setName ("left");
    addAndMakeVisible (rightEnableButton);
    addAndMakeVisible (leftEnableButton);

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

    // Auto-gain logic
    if (autoGainButton.isDown())
    {
        float inLevel = std::max (displayInputL, displayInputR);
        float outLevel = std::max (displayOutputL, displayOutputR);
        if (inLevel > 0.0001f && outLevel > 0.0001f)
        {
            float inDb = juce::Decibels::gainToDecibels (inLevel);
            float outDb = juce::Decibels::gainToDecibels (outLevel);
            float diff = inDb - outDb;
            float currentGain = outputGainSlider.getValue();
            float newGain = juce::jlimit (-24.0f, 24.0f, static_cast<float> (currentGain + diff * 0.1f));
            outputGainSlider.setValue (newGain, juce::sendNotificationAsync);
        }
    }

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

        // Auto-gain hint text
        g.setColour (CustomLookAndFeel::textMuted);
        g.setFont (juce::FontOptions (9.0f));
        auto btnBounds = autoGainButton.getBounds();
        g.drawText ("press to adjust", btnBounds.getX() - 10, btnBounds.getBottom() + 2,
                   btnBounds.getWidth() + 20, 10, juce::Justification::centred);
        g.drawText ("release to set", btnBounds.getX() - 10, btnBounds.getBottom() + 11,
                   btnBounds.getWidth() + 20, 10, juce::Justification::centred);
    }

    // Version footer
    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("v1.3.0", 0, getHeight() - 24, getWidth(), 20, juce::Justification::centred);
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

    // ============ LAYOUT CALCULATION ============
    auto bounds = getLocalBounds().reduced (MARGIN);
    bounds.removeFromBottom (VERSION_H);  // Reserve for version label

    // Fixed heights
    const int HP_PANEL_H = 60;       // Headphone panel (room for dropdown + info)
    const int CTRL_PANEL_H = 160;    // Control panel

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

    // Row 1: icon, dropdown, toggle, refresh
    int iconW = 28, toggleW = 40, refreshW = 50, elemH = 26;  // Wider refresh for text
    int refreshX = hpX + hpW - refreshW;
    int toggleX = refreshX - 8 - toggleW;
    int dropX = hpX + iconW + 8;
    int dropW = toggleX - 8 - dropX;

    headphoneSelector.setBounds (dropX, hpY, dropW, elemH);
    headphoneEnableButton.setBounds (toggleX, hpY + 3, toggleW, 20);
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

    // Right ear panel (left side)
    auto rPanel = agArea.removeFromLeft (chartW);
    int agContentY = rPanel.getY() + PANEL_PAD;
    rightEnableButton.setBounds (rPanel.getX() + PANEL_PAD, agContentY, 36, 20);
    rightEarLabel.setBounds (rPanel.getX() + PANEL_PAD + 36 + 24 + 4, agContentY, 80, 20);
    // Chart starts after toggle row + 10px gap (PANEL_PAD)
    int chartTop = agContentY + toggleRowH + PANEL_PAD;
    rightAudiogram.setBounds (rPanel.getX(), chartTop,
                              rPanel.getWidth(), rPanel.getBottom() - chartTop);

    agArea.removeFromLeft (chartGap);

    // Left ear panel (right side)
    auto lPanel = agArea;
    leftEnableButton.setBounds (lPanel.getX() + PANEL_PAD, agContentY, 36, 20);
    leftEarLabel.setBounds (lPanel.getX() + PANEL_PAD + 36 + 24 + 4, agContentY, 80, 20);
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
    int totalDDH = 3 * (lblH + ddH) + 2 * ddGap;
    int ddStartY = ddArea.getY() + (ddArea.getHeight() - totalDDH) / 2;

    modelLabel.setBounds (ddArea.getX(), ddStartY, ddArea.getWidth(), lblH);
    modelSelector.setBounds (ddArea.getX(), ddStartY + lblH, ddArea.getWidth(), ddH);

    int y2 = ddStartY + lblH + ddH + ddGap;
    compressionSpeedLabel.setBounds (ddArea.getX(), y2, ddArea.getWidth(), lblH);
    compressionSpeedSelector.setBounds (ddArea.getX(), y2 + lblH, ddArea.getWidth(), ddH);

    int y3 = y2 + lblH + ddH + ddGap;
    experienceLevelLabel.setBounds (ddArea.getX(), y3, ddArea.getWidth(), lblH);
    experienceLevelSelector.setBounds (ddArea.getX(), y3 + lblH, ddArea.getWidth(), ddH);

    // --- Right side: meters/faders/button with PANEL_PAD after divider ---
    ctrlArea.removeFromLeft (PANEL_PAD);  // Gap for divider
    auto mfArea = ctrlArea;

    // Layout: 5 elements evenly spaced: INPUT, STRENGTH, MAX, OUTPUT pair, AUTO_GAIN
    const int LBL_H = 14;
    const int TEXT_BOX_H = 20;
    int mfY = mfArea.getY();
    int mfH = mfArea.getHeight();

    // Track dimensions
    const int TRACK_TOP = mfY + LBL_H + 6;
    const int TRACK_H = mfH - LBL_H - 6 - TEXT_BOX_H - 8;

    // Element widths
    const int meterW = 22;
    const int faderW = 40;
    const int outputPairGap = 16;
    const int outputPairW = faderW + outputPairGap + meterW;
    const int btnW = 48;

    // Calculate 5 evenly spaced center points
    // Total width divided into 6 gaps (edges + between elements)
    int totalW = mfArea.getWidth();
    int spacing = totalW / 5;  // Distance between element centers
    int startX = mfArea.getX() + spacing / 2;  // First element center

    int col0 = startX;                    // INPUT
    int col1 = startX + spacing;          // STRENGTH
    int col2 = startX + spacing * 2;      // MAX
    int col3 = startX + spacing * 3;      // OUTPUT pair
    int col4 = startX + spacing * 4;      // AUTO GAIN

    // INPUT meter
    inputMeterLabel.setBounds (col0 - 30, mfY, 60, LBL_H);
    inputMeterBounds = juce::Rectangle<float> (col0 - meterW / 2.0f, TRACK_TOP, meterW, TRACK_H);

    // STRENGTH fader
    correctionLabel.setBounds (col1 - 45, mfY, 90, LBL_H);
    correctionStrengthSlider.setBounds (col1 - faderW / 2, TRACK_TOP, faderW, TRACK_H + TEXT_BOX_H);

    // MAX BOOST fader
    maxBoostLabel.setBounds (col2 - 30, mfY, 60, LBL_H);
    maxBoostSlider.setBounds (col2 - faderW / 2, TRACK_TOP, faderW, TRACK_H + TEXT_BOX_H);

    // OUTPUT pair: centered as one unit
    outputGainLabel.setBounds (col3 - 45, mfY, 90, LBL_H);
    int outputFaderX = col3 - outputPairW / 2;
    int outputMeterX = outputFaderX + faderW + outputPairGap;
    outputGainSlider.setBounds (outputFaderX, TRACK_TOP, faderW, TRACK_H + TEXT_BOX_H);
    outputMeterLabel.setBounds (0, 0, 0, 0);
    outputMeterBounds = juce::Rectangle<float> (outputMeterX, TRACK_TOP, meterW, TRACK_H);

    // AUTO GAIN button
    int btnH = 40;
    int btnY = mfY + (mfH - btnH - 20) / 2;
    autoGainButton.setBounds (col4 - btnW / 2, btnY, btnW, btnH);
}

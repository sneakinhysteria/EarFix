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
    correctionStrengthSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 44, 18);
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
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 44, 18);
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

    outputMeterLabel.setText ("OUTPUT", juce::dontSendNotification);
    outputMeterLabel.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    outputMeterLabel.setColour (juce::Label::textColourId, CustomLookAndFeel::textMuted);
    outputMeterLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outputMeterLabel);

    // Enable buttons
    rightEnableButton.setName ("right");
    leftEnableButton.setName ("left");
    addAndMakeVisible (rightEnableButton);
    addAndMakeVisible (leftEnableButton);

    // Ear labels
    rightEarLabel.setText ("Right ear", juce::dontSendNotification);
    rightEarLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (rightEarLabel);

    leftEarLabel.setText ("Left ear", juce::dontSendNotification);
    leftEarLabel.setJustificationType (juce::Justification::centredRight);
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

    // Listen for model changes
    audioProcessor.parameters.addParameterListener ("modelSelect", this);
    updateNALOptionsVisibility();

    // Start timer for meter updates
    startTimerHz (30);

    setSize (560, 500);
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

//==============================================================================
void HearingCorrectionAUv2AudioProcessorEditor::paint (juce::Graphics& g)
{
    CustomLookAndFeel::drawAluminumBackground (g, getLocalBounds());

    auto bounds = getLocalBounds().toFloat();

    // Section header
    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    auto headerBounds = bounds.removeFromTop (20.0f).reduced (16.0f, 0);
    g.drawText ("MODEL & OPTIONS", headerBounds, juce::Justification::centred);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("v1.2.4", headerBounds, juce::Justification::centredRight);

    // Draw control panel
    if (!controlPanelBounds.isEmpty())
    {
        CustomLookAndFeel::drawMachinedPanel (g, controlPanelBounds, 10.0f);

        // Draw divider between dropdowns and meters/faders
        auto dividerX = controlPanelBounds.getX() + controlPanelBounds.getWidth() * 0.33f;
        g.setColour (CustomLookAndFeel::borderNeutral);
        g.drawVerticalLine (static_cast<int> (dividerX),
                           controlPanelBounds.getY() + 12,
                           controlPanelBounds.getBottom() - 12);

        // Draw meters using stored bounds from resized()
        if (!inputMeterBounds.isEmpty())
        {
            drawMeter (g, inputMeterBounds.getX(), inputMeterBounds.getY(),
                      10, inputMeterBounds.getHeight(), displayInputL);
            drawMeter (g, inputMeterBounds.getX() + 12, inputMeterBounds.getY(),
                      10, inputMeterBounds.getHeight(), displayInputR);
        }

        if (!outputMeterBounds.isEmpty())
        {
            drawMeter (g, outputMeterBounds.getX(), outputMeterBounds.getY(),
                      10, outputMeterBounds.getHeight(), displayOutputL);
            drawMeter (g, outputMeterBounds.getX() + 12, outputMeterBounds.getY(),
                      10, outputMeterBounds.getHeight(), displayOutputR);
        }

        // Draw flow arrows centered on meter/fader tracks
        g.setColour (CustomLookAndFeel::textMuted.withAlpha (0.6f));
        g.setFont (juce::FontOptions (14.0f));
        float arrowY = inputMeterBounds.getCentreY() - 7;
        float arrowX1 = inputMeterBounds.getRight() + 8;
        float arrowX2 = correctionStrengthSlider.getRight() + 4;
        float arrowX3 = outputGainSlider.getRight() + 4;
        g.drawText (juce::String::charToString (0x203A), arrowX1, arrowY, 20, 14, juce::Justification::centred);
        g.drawText (juce::String::charToString (0x203A), arrowX2, arrowY, 20, 14, juce::Justification::centred);
        g.drawText (juce::String::charToString (0x203A), arrowX3, arrowY, 20, 14, juce::Justification::centred);

        // Auto-gain hint
        g.setColour (CustomLookAndFeel::textMuted);
        g.setFont (juce::FontOptions (8.0f));
        g.drawText ("hold to", autoGainButton.getBounds().toFloat().getX(),
                   autoGainButton.getBounds().getBottom() + 2,
                   autoGainButton.getWidth(), 10, juce::Justification::centred);
        g.drawText ("match", autoGainButton.getBounds().toFloat().getX(),
                   autoGainButton.getBounds().getBottom() + 11,
                   autoGainButton.getWidth(), 10, juce::Justification::centred);
    }

    // Audiogram header
    auto audiogramHeaderY = controlPanelBounds.getBottom() + 6;
    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    g.drawText ("AUDIOGRAM", 0, audiogramHeaderY, getWidth(), 24, juce::Justification::centred);
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
    auto bounds = getLocalBounds().reduced (16);
    bounds.removeFromTop (20); // Header

    // Control panel - same height as audiogram (200px)
    controlPanelBounds = bounds.removeFromTop (200).toFloat();
    auto controlArea = controlPanelBounds.reduced (12.0f).toNearestInt();

    // Left 1/3: Dropdowns
    auto dropdownSection = controlArea.removeFromLeft (static_cast<int> (controlArea.getWidth() * 0.33f));
    dropdownSection.removeFromRight (12); // Gap before divider

    const int dropdownHeight = 28;
    const int labelHeight = 16;
    const int gap = 8;

    // Center dropdowns vertically
    int totalDropdownHeight = 3 * (labelHeight + dropdownHeight) + 2 * gap;
    int dropdownY = dropdownSection.getY() + (dropdownSection.getHeight() - totalDropdownHeight) / 2;

    // Model
    modelLabel.setBounds (dropdownSection.getX(), dropdownY, dropdownSection.getWidth(), labelHeight);
    modelSelector.setBounds (dropdownSection.getX(), dropdownY + labelHeight, dropdownSection.getWidth(), dropdownHeight);

    // Speed
    compressionSpeedLabel.setBounds (dropdownSection.getX(), dropdownY + labelHeight + dropdownHeight + gap,
                                     dropdownSection.getWidth(), labelHeight);
    compressionSpeedSelector.setBounds (dropdownSection.getX(), dropdownY + labelHeight * 2 + dropdownHeight + gap,
                                        dropdownSection.getWidth(), dropdownHeight);

    // Level
    experienceLevelLabel.setBounds (dropdownSection.getX(), dropdownY + (labelHeight + dropdownHeight + gap) * 2,
                                    dropdownSection.getWidth(), labelHeight);
    experienceLevelSelector.setBounds (dropdownSection.getX(), dropdownY + labelHeight + (labelHeight + dropdownHeight + gap) * 2,
                                       dropdownSection.getWidth(), dropdownHeight);

    // Right 2/3: Meters, Faders, Auto-gain
    controlArea.removeFromLeft (12); // Gap after divider
    auto metersSection = controlArea;

    // Layout constants - calculate from available space
    const int meterLabelHeight = 16;
    const int meterLabelWidth = 60;  // Wide enough for "OUTPUT" and "STRENGTH"
    const int textBoxHeight = 20;
    const int bottomPadding = 8;

    // Calculate meter/fader track height from available space
    // Total height = label + gap + track + gap + textbox + bottomPadding
    const int trackHeight = metersSection.getHeight() - meterLabelHeight - 4 - textBoxHeight - bottomPadding;

    // For sliders: track = bounds - 14px internal padding
    const int faderBoundsHeight = trackHeight + 14;
    const int faderWidth = 44;

    // Calculate strip positions (5 elements: input, strength, output fader, output meter, autogain)
    const float stripWidth = (metersSection.getWidth() - 60) / 4.5f; // Leave room for auto-gain

    int trackTop = metersSection.getY() + meterLabelHeight + 4;
    int textBoxTop = trackTop + trackHeight + 4;

    // === INPUT METERS ===
    int inputX = metersSection.getX() + 4;
    inputMeterLabel.setBounds (inputX, metersSection.getY(), meterLabelWidth, meterLabelHeight);
    inputMeterBounds = juce::Rectangle<float> (inputX + 6, trackTop, 22, trackHeight);

    // === STRENGTH FADER ===
    int strengthX = inputX + static_cast<int> (stripWidth);
    correctionLabel.setBounds (strengthX - 6, metersSection.getY(), meterLabelWidth + 16, meterLabelHeight);
    // Slider bounds: position so track aligns with meters, text box goes below
    correctionStrengthSlider.setBounds (strengthX, trackTop - 7, faderWidth, faderBoundsHeight + textBoxHeight);

    // === OUTPUT FADER ===
    int outputFaderX = strengthX + static_cast<int> (stripWidth);
    outputGainLabel.setBounds (outputFaderX - 4, metersSection.getY(), meterLabelWidth, meterLabelHeight);
    outputGainSlider.setBounds (outputFaderX, trackTop - 7, faderWidth, faderBoundsHeight + textBoxHeight);

    // === OUTPUT METERS ===
    int outputMeterX = outputFaderX + static_cast<int> (stripWidth);
    outputMeterLabel.setBounds (outputMeterX, metersSection.getY(), meterLabelWidth, meterLabelHeight);
    outputMeterBounds = juce::Rectangle<float> (outputMeterX + 6, trackTop, 22, trackHeight);

    // === AUTO-GAIN BUTTON === (vertically centered in remaining space)
    int autoGainX = metersSection.getRight() - 56;
    int autoGainY = metersSection.getY() + (metersSection.getHeight() - 60) / 2;
    autoGainButton.setBounds (autoGainX, autoGainY, 52, 44);

    bounds.removeFromTop (6);

    // Audiogram section
    auto audiogramSection = bounds;
    auto headerRow = audiogramSection.removeFromTop (24);
    const int chartWidth = (audiogramSection.getWidth() - 12) / 2;

    // Right ear toggle
    auto rightEarArea = headerRow.removeFromLeft (chartWidth);
    rightEnableButton.setBounds (rightEarArea.removeFromLeft (36).reduced (0, 2));
    rightEarArea.removeFromLeft (6);
    rightEarLabel.setBounds (rightEarArea.removeFromLeft (70));

    headerRow.removeFromLeft (12);

    // Left ear toggle
    auto leftEarArea = headerRow;
    leftEnableButton.setBounds (leftEarArea.removeFromRight (36).reduced (0, 2));
    leftEarArea.removeFromRight (6);
    leftEarLabel.setBounds (leftEarArea.removeFromRight (70));

    audiogramSection.removeFromTop (4);

    // Audiogram charts
    rightAudiogram.setBounds (audiogramSection.removeFromLeft (chartWidth));
    audiogramSection.removeFromLeft (12);
    leftAudiogram.setBounds (audiogramSection);
}

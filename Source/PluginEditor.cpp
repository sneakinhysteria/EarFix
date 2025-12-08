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
    // Apply custom look and feel
    setLookAndFeel (&customLookAndFeel);

    // Output gain slider
    outputGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 55, 22);
    outputGainSlider.setTextValueSuffix (" dB");
    outputGainSlider.setColour (juce::Slider::textBoxTextColourId, CustomLookAndFeel::textDark);
    outputGainSlider.setColour (juce::Slider::textBoxBackgroundColourId, CustomLookAndFeel::panelWhite);
    outputGainSlider.setColour (juce::Slider::textBoxOutlineColourId, CustomLookAndFeel::borderNeutral);
    outputGainSlider.setColour (juce::Slider::textBoxHighlightColourId, CustomLookAndFeel::textDark);
    addAndMakeVisible (outputGainSlider);
    outputGainLabel.setText ("Output", juce::dontSendNotification);
    outputGainLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (outputGainLabel);

    // Correction strength slider
    correctionStrengthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    correctionStrengthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 55, 22);
    correctionStrengthSlider.setTextValueSuffix ("%");
    correctionStrengthSlider.setColour (juce::Slider::textBoxTextColourId, CustomLookAndFeel::textDark);
    correctionStrengthSlider.setColour (juce::Slider::textBoxBackgroundColourId, CustomLookAndFeel::panelWhite);
    correctionStrengthSlider.setColour (juce::Slider::textBoxOutlineColourId, CustomLookAndFeel::borderNeutral);
    correctionStrengthSlider.setColour (juce::Slider::textBoxHighlightColourId, CustomLookAndFeel::textDark);
    addAndMakeVisible (correctionStrengthSlider);
    correctionLabel.setText ("Strength", juce::dontSendNotification);
    correctionLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (correctionLabel);

    // Model selector
    modelSelector.addItem ("Half-Gain", 1);
    modelSelector.addItem ("NAL-NL2", 2);
    addAndMakeVisible (modelSelector);
    modelLabel.setText ("Model", juce::dontSendNotification);
    modelLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (modelLabel);

    // Compression speed selector (only affects NAL model)
    compressionSpeedSelector.addItem ("Fast", 1);
    compressionSpeedSelector.addItem ("Slow", 2);
    addAndMakeVisible (compressionSpeedSelector);
    compressionSpeedLabel.setText ("Speed", juce::dontSendNotification);
    compressionSpeedLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (compressionSpeedLabel);

    // Experience level selector (only affects NAL model)
    experienceLevelSelector.addItem ("New", 1);
    experienceLevelSelector.addItem ("Some", 2);
    experienceLevelSelector.addItem ("Experienced", 3);
    addAndMakeVisible (experienceLevelSelector);
    experienceLevelLabel.setText ("Level", juce::dontSendNotification);
    experienceLevelLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (experienceLevelLabel);

    // Enable buttons (named for LookAndFeel color detection)
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

    // Listen for model changes to show/hide NAL options
    audioProcessor.parameters.addParameterListener ("modelSelect", this);

    // Initial visibility update
    updateNALOptionsVisibility();

    setSize (560, 420);
}

HearingCorrectionAUv2AudioProcessorEditor::~HearingCorrectionAUv2AudioProcessorEditor()
{
    audioProcessor.parameters.removeParameterListener ("modelSelect", this);
    setLookAndFeel (nullptr);
}

//==============================================================================
void HearingCorrectionAUv2AudioProcessorEditor::parameterChanged (const juce::String& parameterID, float /*newValue*/)
{
    if (parameterID == "modelSelect")
    {
        juce::MessageManager::callAsync ([this]() { updateNALOptionsVisibility(); });
    }
}

void HearingCorrectionAUv2AudioProcessorEditor::updateNALOptionsVisibility()
{
    auto* modelParam = audioProcessor.parameters.getRawParameterValue ("modelSelect");
    bool isNAL = modelParam != nullptr && modelParam->load() > 0.5f;

    compressionSpeedLabel.setVisible (isNAL);
    compressionSpeedSelector.setVisible (isNAL);
    experienceLevelLabel.setVisible (isNAL);
    experienceLevelSelector.setVisible (isNAL);

    repaint();
}

//==============================================================================
void HearingCorrectionAUv2AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Draw aluminum background
    CustomLookAndFeel::drawAluminumBackground (g, getLocalBounds());

    auto bounds = getLocalBounds().toFloat();

    // Section header: "Model & Options" with version
    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    auto headerBounds = bounds.removeFromTop (20.0f).reduced (16.0f, 0);
    g.drawText ("MODEL & OPTIONS", headerBounds, juce::Justification::centred);

    // Version text on the right
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("v1.0.1", headerBounds, juce::Justification::centredRight);

    // Draw machined panel for controls section
    if (!controlPanelBounds.isEmpty())
    {
        CustomLookAndFeel::drawMachinedPanel (g, controlPanelBounds, 10.0f);
    }

    // Section header: "Audiogram" centered between ear toggles
    auto audiogramHeaderBounds = getLocalBounds().toFloat();
    audiogramHeaderBounds.removeFromTop (16.0f + 20.0f + 124.0f + 6.0f); // Adjust based on layout
    audiogramHeaderBounds = audiogramHeaderBounds.removeFromTop (24.0f).reduced (16.0f, 0);

    g.setColour (CustomLookAndFeel::textMuted);
    g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    g.drawText ("AUDIOGRAM", audiogramHeaderBounds, juce::Justification::centred);
}

void HearingCorrectionAUv2AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);

    // Section header space
    bounds.removeFromTop (20);

    // Control panel area (two columns)
    // Height: 3 rows x 28px + 2 gaps x 8px + 24px padding = 124px
    controlPanelBounds = bounds.removeFromTop (124).toFloat();
    auto controlArea = controlPanelBounds.reduced (12.0f).toNearestInt();

    // Left column for Model + NAL options
    const int leftColumnWidth = 175;
    auto leftColumn = controlArea.removeFromLeft (leftColumnWidth);

    // Right column for Output + Strength sliders
    controlArea.removeFromLeft (16); // Gap between columns
    auto rightColumn = controlArea;

    // Left column rows
    auto modelRow = leftColumn.removeFromTop (28);
    modelLabel.setBounds (modelRow.removeFromLeft (55));
    modelRow.removeFromLeft (8);
    modelSelector.setBounds (modelRow);

    leftColumn.removeFromTop (8);

    auto speedRow = leftColumn.removeFromTop (28);
    compressionSpeedLabel.setBounds (speedRow.removeFromLeft (55));
    speedRow.removeFromLeft (8);
    compressionSpeedSelector.setBounds (speedRow);

    leftColumn.removeFromTop (8);

    auto levelRow = leftColumn.removeFromTop (28);
    experienceLevelLabel.setBounds (levelRow.removeFromLeft (55));
    levelRow.removeFromLeft (8);
    experienceLevelSelector.setBounds (levelRow);

    // Right column rows
    auto outputRow = rightColumn.removeFromTop (28);
    outputGainLabel.setBounds (outputRow.removeFromLeft (55));
    outputRow.removeFromLeft (8);
    outputGainSlider.setBounds (outputRow);

    rightColumn.removeFromTop (8);

    auto strengthRow = rightColumn.removeFromTop (28);
    correctionLabel.setBounds (strengthRow.removeFromLeft (55));
    strengthRow.removeFromLeft (8);
    correctionStrengthSlider.setBounds (strengthRow);

    bounds.removeFromTop (6);

    // Audiogram section
    auto audiogramSection = bounds;

    // Header row with ear toggles
    auto headerRow = audiogramSection.removeFromTop (24);
    const int chartWidth = (audiogramSection.getWidth() - 12) / 2;

    // Right ear toggle + label on left side
    auto rightEarArea = headerRow.removeFromLeft (chartWidth);
    rightEnableButton.setBounds (rightEarArea.removeFromLeft (36).reduced (0, 2));
    rightEarArea.removeFromLeft (6);
    rightEarLabel.setBounds (rightEarArea.removeFromLeft (70));

    // Gap for "AUDIOGRAM" title (drawn in paint)
    headerRow.removeFromLeft (12);

    // Left ear label + toggle on right side (reversed order)
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

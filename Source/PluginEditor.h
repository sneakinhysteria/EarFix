/*
  ==============================================================================

    EarFix Hearing Correction - Alpha
    Plugin Editor (UI) - Premium machined aluminum styling

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AudiogramComponent.h"
#include "CustomLookAndFeel.h"

//==============================================================================
class HearingCorrectionAUv2AudioProcessorEditor  : public juce::AudioProcessorEditor,
                                                    private juce::AudioProcessorValueTreeState::Listener,
                                                    private juce::Timer
{
public:
    HearingCorrectionAUv2AudioProcessorEditor (HearingCorrectionAUv2AudioProcessor&);
    ~HearingCorrectionAUv2AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void updateNALOptionsVisibility();
    void drawMeter (juce::Graphics& g, float x, float y, float w, float h, float level);

    HearingCorrectionAUv2AudioProcessor& audioProcessor;
    CustomLookAndFeel customLookAndFeel;

    // Right column sliders
    juce::Slider       outputGainSlider;
    juce::Slider       correctionStrengthSlider;
    juce::Label        outputGainLabel;
    juce::Label        correctionLabel;

    // Left column: Model selection
    juce::ComboBox     modelSelector;
    juce::Label        modelLabel;

    // Left column: Compression speed (NAL model only)
    juce::ComboBox     compressionSpeedSelector;
    juce::Label        compressionSpeedLabel;

    // Left column: Experience level (NAL model only)
    juce::ComboBox     experienceLevelSelector;
    juce::Label        experienceLevelLabel;

    // Per-ear enable toggles
    juce::ToggleButton rightEnableButton { "right" };
    juce::ToggleButton leftEnableButton { "left" };
    juce::Label        rightEarLabel;
    juce::Label        leftEarLabel;

    // Audiogram charts (side by side: Right | Left)
    AudiogramComponent rightAudiogram { AudiogramComponent::Ear::Right, CustomLookAndFeel::accentRed };
    AudiogramComponent leftAudiogram  { AudiogramComponent::Ear::Left, CustomLookAndFeel::accentBlue };

    // Meter labels (as proper Label components for consistent rendering)
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;

    // Control panel bounds (for painting)
    juce::Rectangle<float> controlPanelBounds;

    // Meter bounds (for drawing in paint)
    juce::Rectangle<float> inputMeterBounds;
    juce::Rectangle<float> outputMeterBounds;

    // APVTS Attachments
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment>   outputGainAttachment;
    std::unique_ptr<SliderAttachment>   correctionStrengthAttachment;
    std::unique_ptr<ComboBoxAttachment> modelSelectAttachment;
    std::unique_ptr<ComboBoxAttachment> compressionSpeedAttachment;
    std::unique_ptr<ComboBoxAttachment> experienceLevelAttachment;
    std::unique_ptr<ButtonAttachment>   rightEnableAttachment;
    std::unique_ptr<ButtonAttachment>   leftEnableAttachment;

    // Auto-gain button
    juce::TextButton autoGainButton { "AUTO\nGAIN" };
    bool autoGainActive = false;
    float autoGainOffset = 0.0f;

    // Smoothed meter levels for display
    float displayInputL = 0.0f, displayInputR = 0.0f;
    float displayOutputL = 0.0f, displayOutputR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HearingCorrectionAUv2AudioProcessorEditor)
};

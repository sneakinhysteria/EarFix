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

    // Basic/Advanced UI mode: Basic swaps the full dropdown/fader control set for two
    // curated preset buttons (Speech/Music) + a live summary of what they set. Advanced
    // is the existing full control set, unchanged.
    void updateUIModeVisibility();
    void updatePresetSummaryLabel();

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

    // Left column: Loudness mode (Centered / Boost Only)
    juce::ComboBox     loudnessModeSelector;
    juce::Label        loudnessModeLabel;

    // Fader section: Max boost limiter (correction ceiling)
    juce::Slider       maxBoostSlider;
    juce::Label        maxBoostLabel;

    // Per-ear enable toggles
    juce::ToggleButton rightEnableButton { "right" };
    juce::ToggleButton leftEnableButton { "left" };
    juce::Label        rightEarLabel;
    juce::Label        leftEarLabel;

    // Link toggle: when active, enabling/disabling one ear also does the other.
    // One shared state, mirrored by a small button in each ear card.
    // Text left empty -- CustomLookAndFeel draws a vector chain-link glyph for any
    // TextButton with componentID "linkIcon" instead of relying on an emoji glyph.
    juce::TextButton   rightLinkButton;
    juce::TextButton   leftLinkButton;
    bool earsLinked = false;
    void setEarsLinked (bool linked);
    void onEarEnableClicked (bool isRight);

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
    std::unique_ptr<SliderAttachment>   maxBoostAttachment;
    std::unique_ptr<ComboBoxAttachment> modelSelectAttachment;
    std::unique_ptr<ComboBoxAttachment> compressionSpeedAttachment;
    std::unique_ptr<ComboBoxAttachment> experienceLevelAttachment;
    std::unique_ptr<ComboBoxAttachment> loudnessModeAttachment;
    std::unique_ptr<ButtonAttachment>   rightEnableAttachment;
    std::unique_ptr<ButtonAttachment>   leftEnableAttachment;

    // Headphone EQ section
    juce::ComboBox     headphoneSelector;
    juce::ToggleButton headphoneEnableButton { "headphoneEQ" };
    juce::TextButton   headphoneRefreshButton { "Refresh" };  // Text button - icon too small
    juce::TextButton   headphoneImportButton { "Import" };    // Paste a ParametricEQ profile
    juce::Label        headphoneInfoLabel;

    void openHeadphoneImportDialog();
    std::unique_ptr<ButtonAttachment> headphoneEnableAttachment;

    void populateHeadphoneList();
    void updateHeadphoneInfo();

    // Preset management (host-independent)
    juce::TextButton savePresetButton { "Save" };
    juce::TextButton loadPresetButton { "Load" };
    std::unique_ptr<juce::FileChooser> presetChooser;
    void savePreset();
    void loadPreset();

    // Basic/Advanced mode toggle (segmented pair, top-left, mirroring Save/Load)
    juce::TextButton basicModeButton    { "Basic" };
    juce::TextButton advancedModeButton { "Advanced" };

    // Basic-mode curated preset buttons + live summary of what they set
    juce::TextButton speechPresetButton { "Speech" };
    juce::TextButton musicPresetButton  { "Music" };
    juce::Label      presetSummaryLabel;

    // Section bounds for painting
    juce::Rectangle<float> headphonePanelBounds;
    juce::Rectangle<float> audiogramPanelBounds;

    // Smoothed meter levels for display
    float displayInputL = 0.0f, displayInputR = 0.0f;
    float displayOutputL = 0.0f, displayOutputR = 0.0f;

    // Last value pushed to maxBoostSlider's "rangeCeiling" property, to avoid
    // repainting the active-range marker every timer tick when it hasn't moved.
    float lastMaxBoostCeiling = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HearingCorrectionAUv2AudioProcessorEditor)
};

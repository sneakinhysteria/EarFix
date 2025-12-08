/*
  ==============================================================================

    CustomLookAndFeel.h
    Premium machined aluminum UI styling - EarFix

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Color palette - satin aluminum theme
    static inline const juce::Colour backgroundAluminum { 0xffd4d4d4 };
    static inline const juce::Colour panelWhite { 0xfffafafa };
    static inline const juce::Colour borderLight { 0x80ffffff };
    static inline const juce::Colour borderDark { 0xffb0b0b0 };
    static inline const juce::Colour borderNeutral { 0xffc0c0c0 };
    static inline const juce::Colour textDark { 0xff2a2a2a };
    static inline const juce::Colour textMuted { 0xff707070 };
    static inline const juce::Colour accentBlue { 0xff4a90d9 };
    static inline const juce::Colour accentRed { 0xffd94a4a };
    static inline const juce::Colour sliderTrack { 0xffd0d0d0 };
    static inline const juce::Colour sliderFill { 0xff4a90d9 };
    static inline const juce::Colour gridLine { 0xffc8c8c8 };

    CustomLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, backgroundAluminum);
        setColour (juce::Label::textColourId, textDark);
        setColour (juce::ComboBox::backgroundColourId, panelWhite);
        setColour (juce::ComboBox::textColourId, textDark);
        setColour (juce::ComboBox::outlineColourId, borderNeutral);
        setColour (juce::PopupMenu::backgroundColourId, panelWhite);
        setColour (juce::PopupMenu::textColourId, textDark);
        setColour (juce::TextEditor::backgroundColourId, textDark);
        setColour (juce::TextEditor::textColourId, juce::Colours::white);
        setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);

        // Slider text box colors (white background with dark text, matching other fields)
        setColour (juce::Slider::textBoxBackgroundColourId, panelWhite);
        setColour (juce::Slider::textBoxTextColourId, textDark);
        setColour (juce::Slider::textBoxOutlineColourId, borderNeutral);

        // Also set Label background for slider text boxes
        setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    }

    //==========================================================================
    // Custom slider text box with proper colors
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox (slider);
        // Display mode: dark text on white background
        label->setColour (juce::Label::textColourId, textDark);
        label->setColour (juce::Label::backgroundColourId, panelWhite);
        label->setColour (juce::Label::outlineColourId, borderNeutral);
        // Edit mode: white text on dark background
        label->setColour (juce::TextEditor::textColourId, juce::Colours::white);
        label->setColour (juce::TextEditor::backgroundColourId, textDark);
        label->setColour (juce::TextEditor::highlightColourId, accentBlue);
        label->setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
        return label;
    }

    //==========================================================================
    // Toggle Button (iOS-style switch with ear colors)
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();

        // Determine color based on component name or default
        juce::Colour activeColour = accentBlue;
        if (button.getName().containsIgnoreCase ("right"))
            activeColour = accentRed;

        // Calculate toggle size
        float toggleWidth = 36.0f;
        float toggleHeight = 20.0f;
        auto toggleBounds = juce::Rectangle<float> (0, (bounds.getHeight() - toggleHeight) / 2,
                                                     toggleWidth, toggleHeight);

        bool isOn = button.getToggleState();

        // Track background
        g.setColour (isOn ? activeColour : juce::Colour (0xffc0c0c0));
        g.fillRoundedRectangle (toggleBounds, toggleHeight * 0.5f);

        // Track inner shadow
        g.setColour (juce::Colour (0x18000000));
        g.drawRoundedRectangle (toggleBounds.reduced (0.5f), toggleHeight * 0.5f, 1.0f);

        // Thumb
        float thumbSize = toggleHeight - 4.0f;
        float thumbX = isOn ? toggleBounds.getRight() - thumbSize - 2.0f : toggleBounds.getX() + 2.0f;
        auto thumbBounds = juce::Rectangle<float> (thumbX, toggleBounds.getCentreY() - thumbSize * 0.5f,
                                                    thumbSize, thumbSize);

        // Thumb shadow
        g.setColour (juce::Colour (0x20000000));
        g.fillEllipse (thumbBounds.translated (0.5f, 0.5f));

        // Thumb
        g.setColour (juce::Colours::white);
        g.fillEllipse (thumbBounds);
    }

    //==========================================================================
    // Linear Slider (fader style with rectangular thumb)
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        bool isHorizontal = (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar);

        // Track
        float trackThickness = 6.0f;
        juce::Rectangle<float> track;

        if (isHorizontal)
        {
            track = bounds.withSizeKeepingCentre (bounds.getWidth() - 14.0f, trackThickness);
        }
        else
        {
            track = bounds.withSizeKeepingCentre (trackThickness, bounds.getHeight() - 14.0f);
        }

        // Track background
        g.setColour (sliderTrack);
        g.fillRoundedRectangle (track, trackThickness * 0.5f);

        // Track inner shadow
        g.setColour (juce::Colour (0x15000000));
        g.drawRoundedRectangle (track, trackThickness * 0.5f, 1.0f);

        // Filled portion
        juce::Rectangle<float> filledTrack;
        if (isHorizontal)
        {
            float fillStart = track.getX();
            float fillEnd = sliderPos;
            filledTrack = track.withX (fillStart).withRight (fillEnd);
        }
        else
        {
            filledTrack = track.withTop (sliderPos);
        }

        g.setColour (sliderFill.withAlpha (0.7f));
        g.fillRoundedRectangle (filledTrack, trackThickness * 0.5f);

        // Thumb (rectangular fader style)
        float thumbWidth = 14.0f;
        float thumbHeight = 22.0f;
        juce::Rectangle<float> thumbBounds;

        if (isHorizontal)
        {
            thumbBounds = juce::Rectangle<float> (thumbWidth, thumbHeight)
                .withCentre ({ sliderPos, bounds.getCentreY() });
        }
        else
        {
            thumbBounds = juce::Rectangle<float> (thumbHeight, thumbWidth)
                .withCentre ({ bounds.getCentreX(), sliderPos });
        }

        // Thumb shadow
        g.setColour (juce::Colour (0x20000000));
        g.fillRoundedRectangle (thumbBounds.translated (1.0f, 1.0f), 3.0f);

        // Thumb body with gradient
        juce::ColourGradient thumbGradient (juce::Colours::white, thumbBounds.getX(), thumbBounds.getY(),
                                             juce::Colour (0xfff0f0f0), thumbBounds.getX(), thumbBounds.getBottom(),
                                             false);
        g.setGradientFill (thumbGradient);
        g.fillRoundedRectangle (thumbBounds, 3.0f);

        // Thumb border
        g.setColour (juce::Colour (0xffb0b0b0));
        g.drawRoundedRectangle (thumbBounds, 3.0f, 1.0f);

        // Thumb grip lines
        g.setColour (juce::Colour (0xffc0c0c0));
        float cx = thumbBounds.getCentreX();
        float cy = thumbBounds.getCentreY();
        g.drawLine (cx - 3.0f, cy - 3.0f, cx + 3.0f, cy - 3.0f, 1.0f);
        g.drawLine (cx - 3.0f, cy, cx + 3.0f, cy, 1.0f);
        g.drawLine (cx - 3.0f, cy + 3.0f, cx + 3.0f, cy + 3.0f, 1.0f);
    }

    //==========================================================================
    // ComboBox (clean dropdown)
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height);

        // Background
        g.setColour (panelWhite);
        g.fillRoundedRectangle (bounds, 6.0f);

        // Border
        g.setColour (borderNeutral);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

        // Arrow
        auto arrowBounds = juce::Rectangle<float> ((float) width - 20.0f, 0.0f, 20.0f, (float) height);
        juce::Path arrow;
        auto arrowSize = 4.0f;
        auto centre = arrowBounds.getCentre();
        arrow.addTriangle (centre.x - arrowSize, centre.y - arrowSize * 0.5f,
                           centre.x + arrowSize, centre.y - arrowSize * 0.5f,
                           centre.x, centre.y + arrowSize * 0.5f);

        g.setColour (textMuted);
        g.fillPath (arrow);
    }

    //==========================================================================
    // Label
    juce::Font getLabelFont (juce::Label& label) override
    {
        return juce::Font (juce::FontOptions (13.0f));
    }

    //==========================================================================
    // Popup Menu
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override
    {
        if (isSeparator)
        {
            auto r = area.reduced (5, 0).toFloat();
            g.setColour (juce::Colour (0xffe0e0e0));
            g.fillRect (r.removeFromTop (1));
            return;
        }

        auto r = area.reduced (1);

        if (isHighlighted)
        {
            g.setColour (accentBlue.withAlpha (0.1f));
            g.fillRoundedRectangle (r.toFloat(), 4.0f);
        }

        g.setColour (isActive ? textDark : textMuted);
        g.setFont (juce::FontOptions (13.0f));

        auto textBounds = r.reduced (10, 0);
        g.drawFittedText (text, textBounds, juce::Justification::centredLeft, 1);

        if (isTicked)
        {
            g.setColour (accentBlue);
            auto tickBounds = r.removeFromRight (r.getHeight()).reduced (5).toFloat();
            juce::Path tick;
            tick.startNewSubPath (tickBounds.getX(), tickBounds.getCentreY());
            tick.lineTo (tickBounds.getCentreX(), tickBounds.getBottom() - 2);
            tick.lineTo (tickBounds.getRight(), tickBounds.getY() + 2);
            g.strokePath (tick, juce::PathStrokeType (2.0f));
        }
    }

    //==========================================================================
    // Draw machined panel with unified lighting
    static void drawMachinedPanel (juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius = 10.0f)
    {
        // Limit corner radius to avoid half-circle edges on small panels
        float maxRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.2f;
        cornerRadius = juce::jmin (cornerRadius, maxRadius);
        cornerRadius = juce::jmax (cornerRadius, 4.0f); // Minimum 4px

        // Main panel fill
        g.setColour (panelWhite);
        g.fillRoundedRectangle (bounds, cornerRadius);

        // Draw uniform border
        g.setColour (borderNeutral);
        g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.5f);

        // Inner highlight at top (subtle machined effect)
        g.setColour (juce::Colour (0x60ffffff));
        g.drawHorizontalLine ((int) (bounds.getY() + 2),
                              bounds.getX() + cornerRadius,
                              bounds.getRight() - cornerRadius);

        // Subtle inner shadow at bottom
        g.setColour (juce::Colour (0x10000000));
        g.drawHorizontalLine ((int) (bounds.getBottom() - 3),
                              bounds.getX() + cornerRadius,
                              bounds.getRight() - cornerRadius);
    }

    //==========================================================================
    // Draw aluminum background texture
    static void drawAluminumBackground (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        // Base color
        g.setColour (backgroundAluminum);
        g.fillRect (bounds);

        // Subtle radial highlights for satin finish
        juce::ColourGradient highlight1 (juce::Colour (0x18ffffff),
                                          bounds.getWidth() * 0.3f, bounds.getHeight() * 0.2f,
                                          juce::Colours::transparentWhite,
                                          bounds.getWidth() * 0.3f + 200, bounds.getHeight() * 0.2f + 200,
                                          true);
        g.setGradientFill (highlight1);
        g.fillRect (bounds);

        juce::ColourGradient shadow1 (juce::Colour (0x10000000),
                                       bounds.getWidth() * 0.7f, bounds.getHeight() * 0.8f,
                                       juce::Colours::transparentBlack,
                                       bounds.getWidth() * 0.7f + 200, bounds.getHeight() * 0.8f + 200,
                                       true);
        g.setGradientFill (shadow1);
        g.fillRect (bounds);

        // Top-to-bottom subtle gradient
        juce::ColourGradient vertGradient (juce::Colour (0x0affffff), 0, 0,
                                            juce::Colour (0x06000000), 0, (float) bounds.getHeight(),
                                            false);
        g.setGradientFill (vertGradient);
        g.fillRect (bounds);
    }
};

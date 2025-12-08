/*
  ==============================================================================

    AudiogramComponent.h
    Visual audiogram chart with draggable points
    Premium machined aluminum styling - EarFix

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"

//==============================================================================
class AudiogramComponent : public juce::Component
{
public:
    enum class Ear { Right, Left };

    AudiogramComponent (Ear ear, juce::Colour colour)
        : earSide (ear), earColour (colour)
    {
        setOpaque (false);
    }

    void setParameterValues (std::array<std::atomic<float>*, 6>& params)
    {
        for (int i = 0; i < 6; ++i)
            paramPointers[i] = params[i];
    }

    void setParameterAttachments (juce::AudioProcessorValueTreeState& apvts,
                                   const juce::StringArray& paramIds)
    {
        apvtsPtr = &apvts;
        parameterIds = paramIds;

        for (int i = 0; i < 6; ++i)
        {
            auto* param = apvts.getRawParameterValue (paramIds[i]);
            if (param)
                pointValues[i] = param->load();

            attachments[i] = std::make_unique<juce::ParameterAttachment> (
                *apvts.getParameter (paramIds[i]),
                [this, i] (float value) {
                    pointValues[i] = value;
                    repaint();
                },
                nullptr);
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Chart margins (space for labels)
        const float leftMargin = 38.0f;   // Space for dB HL label + dB values
        const float rightMargin = 10.0f;
        const float topMargin = 10.0f;
        const float bottomMargin = 28.0f; // Space for Hz label + freq values

        // Draw machined panel background
        CustomLookAndFeel::drawMachinedPanel (g, bounds, 10.0f);

        // Chart area
        auto chartBounds = bounds;
        chartBounds.removeFromLeft (leftMargin);
        chartBounds.removeFromRight (rightMargin);
        chartBounds.removeFromTop (topMargin);
        chartBounds.removeFromBottom (bottomMargin);

        const float chartLeft = chartBounds.getX();
        const float chartTop = chartBounds.getY();
        const float chartRight = chartBounds.getRight();
        const float chartBottom = chartBounds.getBottom();
        const float chartWidth = chartBounds.getWidth();
        const float chartHeight = chartBounds.getHeight();

        // dB range: -20 to 120 (140 dB total)
        const float dbMin = -20.0f;
        const float dbMax = 120.0f;
        const float dbRange = dbMax - dbMin;

        // Draw grid lines (every 10dB) and labels (every 20dB)
        g.setFont (juce::FontOptions (9.0f));

        for (int db = -20; db <= 120; db += 10)
        {
            float y = chartTop + ((db - dbMin) / dbRange) * chartHeight;

            if (y >= chartTop && y <= chartBottom)
            {
                // Grid line (dashed)
                bool isLabeledLine = (db % 20 == 0);
                g.setColour (isLabeledLine ? CustomLookAndFeel::gridLine : CustomLookAndFeel::gridLine.withAlpha (0.5f));

                juce::Path dashPath;
                dashPath.startNewSubPath (chartLeft, y);
                dashPath.lineTo (chartRight, y);

                const float dashPattern[] = { 3.0f, 3.0f };
                juce::PathStrokeType strokeType (0.5f);
                strokeType.createDashedStroke (dashPath, dashPath, dashPattern, 2);
                g.strokePath (dashPath, strokeType);

                // dB label (only at 20dB intervals, right-aligned)
                if (isLabeledLine)
                {
                    g.setColour (CustomLookAndFeel::textMuted);
                    g.drawText (juce::String (db), 14.0f, y - 5.0f, leftMargin - 18.0f, 10.0f,
                                juce::Justification::centredRight);
                }
            }
        }

        // Y-axis label: "dB HL" (rotated)
        g.setColour (CustomLookAndFeel::textMuted);
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));

        // Save transform, rotate, draw, restore
        g.saveState();
        float yLabelX = 6.0f;
        float yLabelY = chartTop + chartHeight * 0.5f;
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, yLabelX, yLabelY));
        g.drawText ("dB HL", yLabelX - 15.0f, yLabelY - 5.0f, 30.0f, 10.0f, juce::Justification::centred);
        g.restoreState();

        // X-axis label: "Hz" (centered below chart)
        g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        g.drawText ("Hz", chartLeft, bounds.getBottom() - 12.0f, chartWidth, 10.0f,
                    juce::Justification::centred);

        // Frequency labels at bottom
        const char* freqLabels[] = { "250", "500", "1k", "2k", "4k", "8k" };
        g.setColour (CustomLookAndFeel::textMuted);
        g.setFont (juce::FontOptions (9.0f));

        for (int i = 0; i < 6; ++i)
        {
            float x = getXForFrequencyIndex (i, chartLeft, chartWidth);
            g.drawText (freqLabels[i], x - 16.0f, chartBottom + 4.0f, 32.0f, 12.0f,
                        juce::Justification::centred);
        }

        // Build the curve path
        juce::Path curvePath;
        bool pathStarted = false;

        for (int i = 0; i < 6; ++i)
        {
            float x = getXForFrequencyIndex (i, chartLeft, chartWidth);
            float y = chartTop + ((pointValues[i] - dbMin) / dbRange) * chartHeight;
            y = juce::jlimit (chartTop, chartBottom, y);

            if (!pathStarted)
            {
                curvePath.startNewSubPath (x, y);
                pathStarted = true;
            }
            else
            {
                curvePath.lineTo (x, y);
            }
        }

        // Draw fill under curve (subtle gradient in ear color)
        if (!curvePath.isEmpty())
        {
            juce::Path fillPath (curvePath);

            float lastX = getXForFrequencyIndex (5, chartLeft, chartWidth);
            float lastY = chartTop + ((pointValues[5] - dbMin) / dbRange) * chartHeight;
            lastY = juce::jlimit (chartTop, chartBottom, lastY);

            float firstX = getXForFrequencyIndex (0, chartLeft, chartWidth);

            fillPath.lineTo (lastX, chartBottom);
            fillPath.lineTo (firstX, chartBottom);
            fillPath.closeSubPath();

            // Gradient fill in ear color
            juce::ColourGradient fillGradient (earColour.withAlpha (0.15f), 0, chartTop,
                                                earColour.withAlpha (0.02f), 0, chartBottom,
                                                false);
            g.setGradientFill (fillGradient);
            g.fillPath (fillPath);
        }

        // Draw the curve stroke in ear color
        g.setColour (earColour);
        g.strokePath (curvePath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));

        // Draw points
        const float pointRadius = 5.0f;
        for (int i = 0; i < 6; ++i)
        {
            float x = getXForFrequencyIndex (i, chartLeft, chartWidth);
            float y = chartTop + ((pointValues[i] - dbMin) / dbRange) * chartHeight;
            y = juce::jlimit (chartTop, chartBottom, y);

            bool isHovered = (hoverPoint == i) || (draggingPoint == i);

            // Hover highlight
            if (isHovered)
            {
                g.setColour (earColour.withAlpha (0.2f));
                g.fillEllipse (x - pointRadius * 2, y - pointRadius * 2,
                               pointRadius * 4, pointRadius * 4);
            }

            // Point fill (white)
            g.setColour (juce::Colours::white);
            g.fillEllipse (x - pointRadius, y - pointRadius,
                           pointRadius * 2, pointRadius * 2);

            // Point stroke in ear color
            g.setColour (earColour);
            g.drawEllipse (x - pointRadius, y - pointRadius,
                           pointRadius * 2, pointRadius * 2,
                           draggingPoint == i ? 2.5f : 2.0f);
        }

        // Value tooltip when dragging
        if (draggingPoint >= 0 && draggingPoint < 6)
        {
            float x = getXForFrequencyIndex (draggingPoint, chartLeft, chartWidth);
            float y = chartTop + ((pointValues[draggingPoint] - dbMin) / dbRange) * chartHeight;
            y = juce::jlimit (chartTop, chartBottom, y);

            juce::String valueText = juce::String (static_cast<int> (pointValues[draggingPoint])) + " dB";

            // Tooltip background
            auto tooltipBounds = juce::Rectangle<float> (x - 22, y - 26, 44, 18);
            g.setColour (CustomLookAndFeel::textDark);
            g.fillRoundedRectangle (tooltipBounds, 4.0f);

            // Tooltip text
            g.setColour (juce::Colours::white);
            g.setFont (juce::FontOptions (11.0f));
            g.drawText (valueText, tooltipBounds, juce::Justification::centred);
        }
    }

    void mouseMove (const juce::MouseEvent& event) override
    {
        int newHover = getPointAtPosition (event.position);
        if (newHover != hoverPoint)
        {
            hoverPoint = newHover;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (hoverPoint >= 0)
        {
            hoverPoint = -1;
            repaint();
        }
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        draggingPoint = getPointAtPosition (event.position);
        if (draggingPoint >= 0 && apvtsPtr && draggingPoint < parameterIds.size())
        {
            if (auto* param = apvtsPtr->getParameter (parameterIds[draggingPoint]))
                param->beginChangeGesture();
            repaint();
        }
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        if (draggingPoint >= 0 && draggingPoint < 6 && apvtsPtr)
        {
            auto bounds = getLocalBounds().toFloat();
            const float topMargin = 10.0f;
            const float bottomMargin = 28.0f;
            const float chartTop = topMargin;
            const float chartBottom = bounds.getHeight() - bottomMargin;
            const float chartHeight = chartBottom - chartTop;

            const float dbMin = -20.0f;
            const float dbMax = 120.0f;
            const float dbRange = dbMax - dbMin;

            float normalizedY = (event.position.y - chartTop) / chartHeight;
            float dbValue = normalizedY * dbRange + dbMin;
            dbValue = std::round (dbValue / 5.0f) * 5.0f;
            dbValue = juce::jlimit (dbMin, dbMax, dbValue);

            pointValues[draggingPoint] = dbValue;

            if (auto* param = apvtsPtr->getParameter (parameterIds[draggingPoint]))
            {
                float normalizedValue = (dbValue - dbMin) / dbRange;
                param->setValueNotifyingHost (normalizedValue);
            }

            repaint();
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (draggingPoint >= 0 && apvtsPtr && draggingPoint < parameterIds.size())
        {
            if (auto* param = apvtsPtr->getParameter (parameterIds[draggingPoint]))
                param->endChangeGesture();
        }

        draggingPoint = -1;
        repaint();
    }

private:
    Ear earSide;
    juce::Colour earColour;

    std::array<float, 6> pointValues = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<std::atomic<float>*, 6> paramPointers = { nullptr };
    std::array<std::unique_ptr<juce::ParameterAttachment>, 6> attachments;
    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;
    juce::StringArray parameterIds;

    int draggingPoint = -1;
    int hoverPoint = -1;

    float getXForFrequencyIndex (int index, float chartLeft, float chartWidth) const
    {
        return chartLeft + (static_cast<float> (index) + 0.5f) * (chartWidth / 6.0f);
    }

    int getPointAtPosition (juce::Point<float> pos) const
    {
        auto bounds = getLocalBounds().toFloat();
        const float leftMargin = 38.0f;
        const float rightMargin = 10.0f;
        const float topMargin = 10.0f;
        const float bottomMargin = 28.0f;

        auto chartBounds = bounds;
        chartBounds.removeFromLeft (leftMargin);
        chartBounds.removeFromRight (rightMargin);
        chartBounds.removeFromTop (topMargin);
        chartBounds.removeFromBottom (bottomMargin);

        const float chartLeft = chartBounds.getX();
        const float chartTop = chartBounds.getY();
        const float chartBottom = chartBounds.getBottom();
        const float chartWidth = chartBounds.getWidth();
        const float chartHeight = chartBounds.getHeight();

        const float dbMin = -20.0f;
        const float dbRange = 140.0f;
        const float hitRadius = 14.0f;

        for (int i = 0; i < 6; ++i)
        {
            float x = getXForFrequencyIndex (i, chartLeft, chartWidth);
            float y = chartTop + ((pointValues[i] - dbMin) / dbRange) * chartHeight;
            y = juce::jlimit (chartTop, chartBottom, y);

            if (pos.getDistanceFrom (juce::Point<float> (x, y)) < hitRadius)
                return i;
        }
        return -1;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudiogramComponent)
};

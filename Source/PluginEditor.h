#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class ObysGridLookAndFeel : public juce::LookAndFeel_V4
{
public:
    bool crazyMode = false;

    juce::Colour getBgColour() const       { return crazyMode ? juce::Colour (0xFF0D0D0D) : juce::Colour (0xFFF4F3EF); }
    juce::Colour getInkColour() const      { return crazyMode ? juce::Colour (0xFFF4F3EF) : juce::Colour (0xFF111111); }
    juce::Colour getAccentColour() const   { return crazyMode ? juce::Colour (0xFFD8FF00) : juce::Colour (0xFFFF3B00); }
    juce::Colour getSubtleGrid() const     { return crazyMode ? juce::Colour (0x26FFFFFF) : juce::Colour (0x22111111); }
    juce::Colour getCardFill() const       { return crazyMode ? juce::Colour (0xFF161616) : juce::Colour (0xFFEBE9E2); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class ObysMoodAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit ObysMoodAudioProcessorEditor (ObysMoodAudioProcessor&);
    ~ObysMoodAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureKnob (juce::Slider& s, const juce::String& name);

    ObysMoodAudioProcessor& audioProcessor;
    ObysGridLookAndFeel obysLaf;

    bool showEditorialGrid = true;

    juce::TextButton gridOverlayBtn { "GRID: ON" };
    juce::TextButton crazyModeBtn   { "CRAZY MODE" };

    juce::Slider mixSlider, clockSlider;
    juce::Slider wetTimeSlider, wetModSlider;
    juce::Slider loopLenSlider, loopModSlider;

    juce::ComboBox wetModeBox, routingBox, loopModeBox, rampTargetBox;

    juce::ToggleButton wetEngageBtn   { "WET // OBNE" };
    juce::ToggleButton loopEngageBtn  { "LOOP // DROLO" };
    juce::ToggleButton clockStepBtn   { "STEPPED" };
    juce::ToggleButton rampBounceBtn  { "BOUNCE" };
    juce::ToggleButton rampSquareBtn  { "SQUARE" };

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAtt> mixAtt, clockAtt, wetTimeAtt, wetModAtt, loopLenAtt, loopModAtt;
    std::unique_ptr<ComboAtt>  wetModeAtt, routingAtt, loopModeAtt, rampTargetAtt;
    std::unique_ptr<ButtonAtt> wetEngageAtt, loopEngageAtt, clockStepAtt, rampBounceAtt, rampSquareAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObysMoodAudioProcessorEditor)
};

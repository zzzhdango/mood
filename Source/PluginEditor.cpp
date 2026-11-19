#include "PluginEditor.h"
#include <cmath>

void ObysGridLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const int numTicks = 17;
    for (int i = 0; i < numTicks; ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (numTicks - 1);
        const float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        const bool isActive = (t <= sliderPos + 0.001f);
        const float innerR = radius * 0.84f;
        const float outerR = (i % 4 == 0) ? radius * 1.02f : radius * 0.94f;

        g.setColour (isActive ? getAccentColour() : getInkColour().withAlpha (0.35f));
        g.drawLine (centre.x + innerR * std::sin (tickAngle),
                    centre.y - innerR * std::cos (tickAngle),
                    centre.x + outerR * std::sin (tickAngle),
                    centre.y - outerR * std::cos (tickAngle),
                    (i % 4 == 0) ? 1.5f : 1.0f);
    }

    const float discR = radius * 0.72f;
    g.setColour (getCardFill());
    g.fillEllipse (centre.x - discR, centre.y - discR, discR * 2.0f, discR * 2.0f);

    g.setColour (getInkColour());
    g.drawEllipse (centre.x - discR, centre.y - discR, discR * 2.0f, discR * 2.0f, 1.5f);

    const float pInner = discR * 0.22f;
    const float pOuter = discR * 0.92f;
    g.setColour (getAccentColour());
    g.drawLine (centre.x + pInner * std::sin (angle),
                centre.y - pInner * std::cos (angle),
                centre.x + pOuter * std::sin (angle),
                centre.y - pOuter * std::cos (angle),
                2.5f);

    g.setColour (getInkColour());
    g.fillRect (centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

void ObysGridLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool, bool)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool isOn = button.getToggleState();

    g.setColour (isOn ? getAccentColour() : getCardFill());
    g.fillRect (bounds);

    g.setColour (getInkColour());
    g.drawRect (bounds, 1.25f);

    const float sq = 8.0f;
    g.setColour (isOn ? juce::Colour (0xFF111111) : getInkColour().withAlpha (0.35f));
    g.fillRect (bounds.getX() + 8.0f, bounds.getCentreY() - sq * 0.5f, sq, sq);

    g.setColour (isOn && crazyMode ? juce::Colour (0xFF0D0D0D) : getInkColour());
    g.setFont (juce::FontOptions ("Helvetica Neue", 11.5f, juce::Font::bold));
    g.drawText (button.getButtonText(),
                bounds.withTrimmedLeft (22.0f),
                juce::Justification::centredLeft, true);
}

ObysMoodAudioProcessorEditor::ObysMoodAudioProcessorEditor (ObysMoodAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&obysLaf);
    setSize (920, 580);

    addAndMakeVisible (gridOverlayBtn);
    gridOverlayBtn.onClick = [this]
    {
        showEditorialGrid = !showEditorialGrid;
        gridOverlayBtn.setButtonText (showEditorialGrid ? "GRID: ON" : "GRID: OFF");
        repaint();
    };

    addAndMakeVisible (crazyModeBtn);
    crazyModeBtn.onClick = [this]
    {
        obysLaf.crazyMode = !obysLaf.crazyMode;
        crazyModeBtn.setButtonText (obysLaf.crazyMode ? "CLASSIC MODE" : "CRAZY MODE");
        repaint();
    };

    configureKnob (mixSlider,     "MIX / RAMP");
    configureKnob (clockSlider,   "CLOCK");
    configureKnob (wetTimeSlider, "TIME");
    configureKnob (wetModSlider,  "MODIFY");
    configureKnob (loopLenSlider, "LENGTH");
    configureKnob (loopModSlider, "MODIFY");

    auto setupCombo = [this] (juce::ComboBox& box, const juce::StringArray& items)
    {
        box.addItemList (items, 1);
        addAndMakeVisible (box);
    };

    setupCombo (wetModeBox,    { "REVERB", "DELAY", "SLIP" });
    setupCombo (routingBox,    { "IN [PARALLEL]", "M -> W [LOOP TO WET]", "W -> M [RESAMPLE]" });
    setupCombo (loopModeBox,   { "ENV", "TAPE", "STRETCH" });
    setupCombo (rampTargetBox, { "RAMP: OFF", "RAMP: CLOCK", "RAMP: WET TIME", "RAMP: WET MOD", "RAMP: LOOP LEN", "RAMP: LOOP MOD" });

    addAndMakeVisible (wetEngageBtn);
    addAndMakeVisible (loopEngageBtn);
    addAndMakeVisible (clockStepBtn);
    addAndMakeVisible (rampBounceBtn);
    addAndMakeVisible (rampSquareBtn);

    mixAtt        = std::make_unique<SliderAtt> (audioProcessor.apvts, "mix", mixSlider);
    clockAtt      = std::make_unique<SliderAtt> (audioProcessor.apvts, "clock", clockSlider);
    wetTimeAtt    = std::make_unique<SliderAtt> (audioProcessor.apvts, "wet_time", wetTimeSlider);
    wetModAtt     = std::make_unique<SliderAtt> (audioProcessor.apvts, "wet_modify", wetModSlider);
    loopLenAtt    = std::make_unique<SliderAtt> (audioProcessor.apvts, "loop_length", loopLenSlider);
    loopModAtt    = std::make_unique<SliderAtt> (audioProcessor.apvts, "loop_modify", loopModSlider);

    wetModeAtt    = std::make_unique<ComboAtt> (audioProcessor.apvts, "wet_mode", wetModeBox);
    routingAtt    = std::make_unique<ComboAtt> (audioProcessor.apvts, "routing", routingBox);
    loopModeAtt   = std::make_unique<ComboAtt> (audioProcessor.apvts, "loop_mode", loopModeBox);
    rampTargetAtt = std::make_unique<ComboAtt> (audioProcessor.apvts, "ramp_target", rampTargetBox);

    wetEngageAtt  = std::make_unique<ButtonAtt> (audioProcessor.apvts, "wet_engage", wetEngageBtn);
    loopEngageAtt = std::make_unique<ButtonAtt> (audioProcessor.apvts, "loop_engage", loopEngageBtn);
    clockStepAtt  = std::make_unique<ButtonAtt> (audioProcessor.apvts, "clock_stepped", clockStepBtn);
    rampBounceAtt = std::make_unique<ButtonAtt> (audioProcessor.apvts, "ramp_bounce", rampBounceBtn);
    rampSquareAtt = std::make_unique<ButtonAtt> (audioProcessor.apvts, "ramp_square", rampSquareBtn);

    startTimerHz (30);
}

ObysMoodAudioProcessorEditor::~ObysMoodAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ObysMoodAudioProcessorEditor::configureKnob (juce::Slider& s, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    s.setName (name);
    addAndMakeVisible (s);
}

void ObysMoodAudioProcessorEditor::timerCallback()
{
    repaint();
}

void ObysMoodAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bg     = obysLaf.getBgColour();
    const auto ink    = obysLaf.getInkColour();
    const auto accent = obysLaf.getAccentColour();

    g.fillAll (bg);

    if (showEditorialGrid)
    {
        g.setColour (obysLaf.getSubtleGrid());
        const int cols = 12;
        const float colW = static_cast<float> (getWidth()) / static_cast<float> (cols);
        for (int c = 1; c < cols; ++c)
            g.drawVerticalLine (static_cast<int> (c * colW), 0.0f, static_cast<float> (getHeight()));

        const int baselineStep = 20;
        for (int y = baselineStep; y < getHeight(); y += baselineStep)
            g.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));
    }

    const int margin = 20;
    const int headerH = 56;
    const int topRowY = margin + headerH;
    const int topRowH = 190;
    const int botRowY = topRowY + topRowH;
    const int botRowH = getHeight() - botRowY - margin;
    const int totalW  = getWidth() - margin * 2;
    const int col3W   = totalW / 3;

    g.setColour (ink);
    g.drawRect (margin, margin, totalW, getHeight() - margin * 2, 1.5f);
    g.drawHorizontalLine (topRowY, static_cast<float> (margin), static_cast<float> (getWidth() - margin));
    g.drawHorizontalLine (botRowY, static_cast<float> (margin), static_cast<float> (getWidth() - margin));

    g.drawVerticalLine (margin + col3W,     static_cast<float> (topRowY), static_cast<float> (getHeight() - margin));
    g.drawVerticalLine (margin + col3W * 2, static_cast<float> (topRowY), static_cast<float> (getHeight() - margin));

    g.setColour (ink);
    g.fillRect (margin + 16, margin + 16, 24, 24);
    g.setColour (accent);
    g.fillRect (margin + 32, margin + 16, 8, 8);

    g.setColour (ink);
    g.setFont (juce::FontOptions ("Helvetica Neue", 20.0f, juce::Font::bold));
    g.drawText ("GRIDS // MOOD", margin + 52, margin + 12, 240, 32, juce::Justification::centredLeft);

    g.setFont (juce::FontOptions ("Courier New", 11.0f, juce::Font::plain));
    g.drawText ("SPATIAL MICRO-LOOPER ARCHITECTURE  [OBNE + DROLO DSP]", margin + 235, margin + 14, 390, 28, juce::Justification::centredLeft);

    auto drawModuleLabel = [&] (int cellX, int cellY, const juce::String& idx, const juce::String& title)
    {
        g.setColour (accent);
        g.setFont (juce::FontOptions ("Courier New", 11.0f, juce::Font::bold));
        g.drawText (idx, cellX + 12, cellY + 8, 40, 16, juce::Justification::centredLeft);

        g.setColour (ink);
        g.setFont (juce::FontOptions ("Helvetica Neue", 12.0f, juce::Font::bold));
        g.drawText (title, cellX + 42, cellY + 8, col3W - 54, 16, juce::Justification::centredLeft);
    };

    drawModuleLabel (margin,             topRowY, "01 /", "WET CHANNEL [OBNE]");
    drawModuleLabel (margin + col3W,     topRowY, "02 /", "MASTER CLOCK & MIX");
    drawModuleLabel (margin + col3W * 2, topRowY, "03 /", "MICRO-LOOPER [DROLO]");

    drawModuleLabel (margin,             botRowY, "04 /", "SIGNAL ROUTING & ENGAGE");
    drawModuleLabel (margin + col3W,     botRowY, "05 /", "MODULAR BUFFER TELEMETRY");
    drawModuleLabel (margin + col3W * 2, botRowY, "06 /", "DIP RAMPING MATRIX");

    g.setFont (juce::FontOptions ("Courier New", 10.5f, juce::Font::bold));
    g.setColour (ink.withAlpha (0.75f));
    const int knobW = (col3W - 24) / 2;
    g.drawText ("TIME",      margin + 12,               topRowY + 30, knobW, 14, juce::Justification::centred);
    g.drawText ("MODIFY",    margin + 12 + knobW,       topRowY + 30, knobW, 14, juce::Justification::centred);
    g.drawText ("MIX/RAMP",  margin + col3W + 12,       topRowY + 30, knobW, 14, juce::Justification::centred);
    g.drawText ("CLOCK",     margin + col3W + 12 + knobW, topRowY + 30, knobW, 14, juce::Justification::centred);
    g.drawText ("LENGTH",    margin + col3W * 2 + 12,   topRowY + 30, knobW, 14, juce::Justification::centred);
    g.drawText ("MODIFY",    margin + col3W * 2 + 12 + knobW, topRowY + 30, knobW, 14, juce::Justification::centred);

    const auto vizBounds = juce::Rectangle<int> (margin + col3W + 14, botRowY + 32, col3W - 28, botRowH - 46).toFloat();
    g.setColour (obysLaf.getCardFill());
    g.fillRect (vizBounds);
    g.setColour (ink);
    g.drawRect (vizBounds, 1.0f);

    const auto loopBar = vizBounds.reduced (10.0f).removeFromTop (38.0f);
    g.setFont (juce::FontOptions ("Courier New", 10.0f, juce::Font::bold));
    g.drawText (audioProcessor.telemetry.loopActive.load() ? "LOOP BUFFER: FROZEN / PLAYING" : "LOOP BUFFER: ALWAYS-LISTENING",
                loopBar.withHeight (14.0f), juce::Justification::centredLeft);

    const auto trackRect = loopBar.withTrimmedTop (16.0f);
    g.drawRect (trackRect, 1.0f);

    const float playNorm = audioProcessor.telemetry.loopPlayhead.load();
    const float winNorm  = juce::jlimit (0.04f, 1.0f, audioProcessor.telemetry.loopWindowSize.load());
    g.setColour (accent.withAlpha (0.28f));
    g.fillRect (trackRect.getX(), trackRect.getY(), trackRect.getWidth() * winNorm, trackRect.getHeight());

    g.setColour (accent);
    const float headX = trackRect.getX() + playNorm * (trackRect.getWidth() - 4.0f);
    g.fillRect (headX, trackRect.getY(), 4.0f, trackRect.getHeight());

    const auto metricsArea = vizBounds.reduced (10.0f).withTrimmedTop (48.0f);
    const float hz = audioProcessor.telemetry.effectiveClockHz.load();
    const float rampV = audioProcessor.telemetry.rampValue.load();
    const float envV  = juce::jlimit (0.0f, 1.0f, audioProcessor.telemetry.envLevel.load() * 3.0f);

    g.setColour (ink);
    g.drawText ("SAMPLE CLOCK: " + juce::String (hz / 1000.0f, 2) + " kHz",
                metricsArea.withHeight (16.0f), juce::Justification::centredLeft);

    auto envBar = metricsArea.withTrimmedTop (22.0f).withHeight (14.0f);
    g.drawRect (envBar, 1.0f);
    g.setColour (ink);
    g.fillRect (envBar.getX(), envBar.getY(), envBar.getWidth() * envV, envBar.getHeight());
    g.setFont (juce::FontOptions ("Courier New", 9.5f, juce::Font::bold));
    g.drawText ("ENV DETECTOR", envBar.translated (0.0f, 16.0f), juce::Justification::centredLeft);

    auto rampBar = metricsArea.withTrimmedTop (56.0f).withHeight (14.0f);
    g.setColour (ink);
    g.drawRect (rampBar, 1.0f);
    g.setColour (accent);
    g.fillRect (rampBar.getX(), rampBar.getY(), rampBar.getWidth() * rampV, rampBar.getHeight());
    g.setColour (ink);
    g.drawText ("RAMP / LFO SWEEP", rampBar.translated (0.0f, 16.0f), juce::Justification::centredLeft);
}

void ObysMoodAudioProcessorEditor::resized()
{
    const int margin = 20;
    const int headerH = 56;
    const int topRowY = margin + headerH;
    const int topRowH = 190;
    const int botRowY = topRowY + topRowH;
    const int totalW  = getWidth() - margin * 2;
    const int col3W   = totalW / 3;
    const int knobW   = (col3W - 24) / 2;

    gridOverlayBtn.setBounds (getWidth() - margin - 216, margin + 14, 96, 28);
    crazyModeBtn.setBounds   (getWidth() - margin - 112, margin + 14, 98, 28);

    wetTimeSlider.setBounds (margin + 12,                     topRowY + 46, knobW, 130);
    wetModSlider.setBounds  (margin + 12 + knobW,             topRowY + 46, knobW, 130);

    mixSlider.setBounds     (margin + col3W + 12,             topRowY + 46, knobW, 130);
    clockSlider.setBounds   (margin + col3W + 12 + knobW,     topRowY + 46, knobW, 130);

    loopLenSlider.setBounds (margin + col3W * 2 + 12,         topRowY + 46, knobW, 130);
    loopModSlider.setBounds (margin + col3W * 2 + 12 + knobW, topRowY + 46, knobW, 130);

    const int bLeftX = margin + 14;
    const int bCtrlW = col3W - 28;
    routingBox.setBounds    (bLeftX, botRowY + 34, bCtrlW, 28);
    wetModeBox.setBounds    (bLeftX, botRowY + 70, (bCtrlW - 8) / 2, 28);
    loopModeBox.setBounds   (bLeftX + (bCtrlW - 8) / 2 + 8, botRowY + 70, (bCtrlW - 8) / 2, 28);

    wetEngageBtn.setBounds  (bLeftX, botRowY + 112, bCtrlW, 38);
    loopEngageBtn.setBounds (bLeftX, botRowY + 158, bCtrlW, 38);

    const int bRightX = margin + col3W * 2 + 14;
    rampTargetBox.setBounds (bRightX, botRowY + 34, bCtrlW, 28);
    clockStepBtn.setBounds  (bRightX, botRowY + 72, bCtrlW, 34);
    rampBounceBtn.setBounds (bRightX, botRowY + 114, (bCtrlW - 8) / 2, 34);
    rampSquareBtn.setBounds (bRightX + (bCtrlW - 8) / 2 + 8, botRowY + 114, (bCtrlW - 8) / 2, 34);
}

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <atomic>

class ObysMoodAudioProcessor : public juce::AudioProcessor
{
public:
    ObysMoodAudioProcessor();
    ~ObysMoodAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    struct Telemetry
    {
        std::atomic<float> loopPlayhead { 0.0f };
        std::atomic<float> loopWindowStart { 0.0f };
        std::atomic<float> loopWindowSize { 0.25f };
        std::atomic<float> envLevel { 0.0f };
        std::atomic<float> rampValue { 0.0f };
        std::atomic<float> effectiveClockHz { 16000.0f };
        std::atomic<bool>  loopActive { false };
        std::atomic<bool>  wetActive { true };
    } telemetry;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double hostSampleRate = 44100.0;
    double clockPhase = 0.0;
    float heldInputL = 0.0f, heldInputR = 0.0f;
    float heldWetOutL = 0.0f, heldWetOutR = 0.0f;
    float heldLoopOutL = 0.0f, heldLoopOutR = 0.0f;

    static constexpr int kMaxDelaySamples = 192000;
    std::vector<float> wetBufL, wetBufR;
    int wetWritePos = 0;

    static constexpr int kNumReverbTaps = 8;
    const std::array<float, kNumReverbTaps> tapRatios { 0.131f, 0.217f, 0.319f, 0.443f, 0.571f, 0.709f, 0.853f, 0.987f };
    float smearStateL = 0.0f, smearStateR = 0.0f;
    float wetFeedbackL = 0.0f, wetFeedbackR = 0.0f;

    double slipReadOffset = 0.0;
    int slipRefreshCounter = 0;

    static constexpr int kMaxLoopSamples = 524288;
    std::vector<float> loopBufL, loopBufR;
    int loopWritePos = 0;
    int capturedLoopLength = 65536;
    double loopPlayPos = 0.0;
    double stretchAnchor = 0.0;
    double stretchGrainPos = 0.0;
    float envFollower = 0.0f;
    bool prevLoopEngaged = false;

    double rampPhase = 0.0;
    float currentRamp = 0.0f;

    float readBufferLinear (const std::vector<float>& buf, double index, int maxLen) const;
    void processClockTick (float inL, float inR,
                           float clockNorm, float mixVal,
                           int routingMode,
                           bool wetOn, int wetMode, float wetTime, float wetMod,
                           bool loopOn, int loopMode, float loopLen, float loopMod,
                           int rampTarget, bool rampBounce, bool rampSquare);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ObysMoodAudioProcessor)
};

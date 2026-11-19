#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

ObysMoodAudioProcessor::ObysMoodAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    wetBufL.resize (kMaxDelaySamples, 0.0f);
    wetBufR.resize (kMaxDelaySamples, 0.0f);
    loopBufL.resize (kMaxLoopSamples, 0.0f);
    loopBufR.resize (kMaxLoopSamples, 0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout ObysMoodAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "MIX / RAMP",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "clock", 1 }, "CLOCK",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.72f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "clock_stepped", 1 }, "HARMONIZED STEPS", true));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "routing", 1 }, "ROUTING",
        juce::StringArray { "IN", "M -> W", "W -> M" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "wet_time", 1 }, "TIME",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "wet_modify", 1 }, "WET MODIFY",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.60f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "wet_mode", 1 }, "WET MODE",
        juce::StringArray { "REVERB", "DELAY", "SLIP" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "wet_engage", 1 }, "WET ENGAGE", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "loop_length", 1 }, "LENGTH",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.65f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "loop_modify", 1 }, "LOOP MODIFY",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.68f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "loop_mode", 1 }, "LOOP MODE",
        juce::StringArray { "ENV", "TAPE", "STRETCH" }, 1));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "loop_engage", 1 }, "LOOP ENGAGE", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ramp_target", 1 }, "RAMP TARGET",
        juce::StringArray { "OFF", "CLOCK", "WET TIME", "WET MOD", "LOOP LEN", "LOOP MOD" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ramp_bounce", 1 }, "BOUNCE", true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ramp_square", 1 }, "SQUARE WAVE", false));

    return { params.begin(), params.end() };
}

void ObysMoodAudioProcessor::prepareToPlay (double sampleRate, int)
{
    hostSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    std::fill (wetBufL.begin(), wetBufL.end(), 0.0f);
    std::fill (wetBufR.begin(), wetBufR.end(), 0.0f);
    std::fill (loopBufL.begin(), loopBufL.end(), 0.0f);
    std::fill (loopBufR.begin(), loopBufR.end(), 0.0f);

    wetWritePos = 0;
    loopWritePos = 0;
    clockPhase = 0.0;
    smearStateL = smearStateR = 0.0f;
    wetFeedbackL = wetFeedbackR = 0.0f;
    envFollower = 0.0f;
    prevLoopEngaged = false;
}

void ObysMoodAudioProcessor::releaseResources() {}

bool ObysMoodAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

float ObysMoodAudioProcessor::readBufferLinear (const std::vector<float>& buf, double index, int maxLen) const
{
    while (index < 0.0) index += static_cast<double> (maxLen);
    while (index >= static_cast<double> (maxLen)) index -= static_cast<double> (maxLen);

    const int i0 = static_cast<int> (index);
    const int i1 = (i0 + 1) % maxLen;
    const float frac = static_cast<float> (index - static_cast<double> (i0));
    return buf[i0] + frac * (buf[i1] - buf[i0]);
}

void ObysMoodAudioProcessor::processClockTick (float inL, float inR,
                                               float clockNorm, float mixVal,
                                               int routingMode,
                                               bool wetOn, int wetMode, float wetTime, float wetMod,
                                               bool loopOn, int loopMode, float loopLen, float loopMod,
                                               int rampTarget, bool rampBounce, bool rampSquare)
{
    juce::ignoreUnused (mixVal, rampBounce, rampSquare);

    if (rampTarget > 0)
    {
        const float modVal = currentRamp;
        switch (rampTarget)
        {
            case 1: clockNorm = juce::jlimit (0.0f, 1.0f, clockNorm + (modVal - 0.5f) * 0.65f); break;
            case 2: wetTime   = juce::jlimit (0.0f, 1.0f, wetTime   + (modVal - 0.5f) * 0.80f); break;
            case 3: wetMod    = juce::jlimit (0.0f, 1.0f, wetMod    + (modVal - 0.5f) * 0.80f); break;
            case 4: loopLen   = juce::jlimit (0.0f, 1.0f, loopLen   + (modVal - 0.5f) * 0.80f); break;
            case 5: loopMod   = juce::jlimit (0.0f, 1.0f, loopMod   + (modVal - 0.5f) * 0.80f); break;
            default: break;
        }
    }

    const float peak = std::max (std::abs (inL), std::abs (inR));
    const float envCoeff = (peak > envFollower) ? 0.05f : 0.0015f;
    envFollower += envCoeff * (peak - envFollower);
    telemetry.envLevel.store (envFollower, std::memory_order_relaxed);

    if (loopOn && !prevLoopEngaged)
    {
        const int baseCap = static_cast<int> (16384.0f + loopLen * 114688.0f);
        capturedLoopLength = juce::jlimit (2048, kMaxLoopSamples - 4, baseCap);
        loopPlayPos = 0.0;
        stretchAnchor = 0.0;
        stretchGrainPos = 0.0;
    }
    prevLoopEngaged = loopOn;

    float loopOutL = 0.0f, loopOutR = 0.0f;
    if (loopOn)
    {
        const int activeLen = std::max (512, static_cast<int> (capturedLoopLength * (0.06f + 0.94f * loopLen)));

        if (loopMode == 0)
        {
            const float threshold = 0.45f * (1.05f - loopMod);
            const bool triggered = (envFollower > threshold);

            if (!triggered)
            {
                loopPlayPos += 1.0;
                if (loopPlayPos >= static_cast<double> (activeLen))
                    loopPlayPos -= static_cast<double> (activeLen);
                stretchAnchor = loopPlayPos;
            }
            else
            {
                const double sliceLen = std::max (128.0, static_cast<double> (activeLen) * (0.03 + 0.25 * loopLen));
                stretchGrainPos += 1.0;
                if (stretchGrainPos >= sliceLen)
                    stretchGrainPos = 0.0;
                loopPlayPos = std::fmod (stretchAnchor + stretchGrainPos, static_cast<double> (activeLen));
            }
        }
        else if (loopMode == 1)
        {
            static constexpr std::array<double, 8> tapeSpeeds { -4.0, -2.0, -1.0, -0.5, 0.5, 1.0, 2.0, 4.0 };
            const int idx = juce::jlimit (0, 7, static_cast<int> (loopMod * 7.99f));
            const double speed = tapeSpeeds[idx];

            loopPlayPos += speed;
            while (loopPlayPos < 0.0) loopPlayPos += static_cast<double> (activeLen);
            while (loopPlayPos >= static_cast<double> (activeLen)) loopPlayPos -= static_cast<double> (activeLen);
        }
        else
        {
            const double grainSize = std::max (512.0, static_cast<double> (activeLen) * (0.08 + 0.35 * loopLen));
            const double advanceRate = 0.02 + 0.98 * (1.0 - loopMod);

            stretchGrainPos += 1.0;
            if (stretchGrainPos >= grainSize)
            {
                stretchGrainPos = 0.0;
                stretchAnchor += grainSize * advanceRate;
                if (stretchAnchor >= static_cast<double> (activeLen))
                    stretchAnchor -= static_cast<double> (activeLen);
            }
            loopPlayPos = std::fmod (stretchAnchor + stretchGrainPos, static_cast<double> (activeLen));
        }

        double readIdx = static_cast<double> (loopWritePos) - static_cast<double> (activeLen) + loopPlayPos;
        float rawL = readBufferLinear (loopBufL, readIdx, kMaxLoopSamples);
        float rawR = readBufferLinear (loopBufR, readIdx, kMaxLoopSamples);

        const double edgeSamples = 96.0;
        float envWin = 1.0f;
        if (loopPlayPos < edgeSamples)
            envWin = static_cast<float> (loopPlayPos / edgeSamples);
        else if (static_cast<double> (activeLen) - loopPlayPos < edgeSamples)
            envWin = static_cast<float> ((static_cast<double> (activeLen) - loopPlayPos) / edgeSamples);

        if (loopMode == 1 && loopLen < 0.65f)
        {
            const double grainMod = std::fmod (loopPlayPos, std::max (256.0, 4096.0 * loopLen));
            const float gEnv = std::sin (juce::MathConstants<float>::pi * static_cast<float> (grainMod / std::max (256.0, 4096.0 * loopLen)));
            envWin *= (0.25f + 0.75f * gEnv);
        }

        loopOutL = rawL * envWin;
        loopOutR = rawR * envWin;

        telemetry.loopPlayhead.store (static_cast<float> (loopPlayPos / static_cast<double> (activeLen)), std::memory_order_relaxed);
        telemetry.loopWindowSize.store (static_cast<float> (activeLen) / static_cast<float> (kMaxLoopSamples), std::memory_order_relaxed);
    }
    else
    {
        float recL = inL;
        float recR = inR;

        if (routingMode == 2 && wetOn)
        {
            recL += heldWetOutL * 0.85f;
            recR += heldWetOutR * 0.85f;
        }

        loopBufL[loopWritePos] = std::tanh (recL);
        loopBufR[loopWritePos] = std::tanh (recR);
        loopWritePos = (loopWritePos + 1) % kMaxLoopSamples;
        telemetry.loopPlayhead.store (static_cast<float> (loopWritePos) / static_cast<float> (kMaxLoopSamples), std::memory_order_relaxed);
    }

    float wetInL = inL;
    float wetInR = inR;

    if (routingMode == 1 && loopOn)
    {
        wetInL = inL * 0.45f + loopOutL;
        wetInR = inR * 0.45f + loopOutR;
    }

    float wetOutL = 0.0f, wetOutR = 0.0f;
    if (wetOn)
    {
        const double baseDelaySamples = 400.0 + static_cast<double> (wetTime) * 28000.0;

        if (wetMode == 0)
        {
            const float smear = wetMod;
            const float fb = 0.25f + smear * 0.58f;

            wetBufL[wetWritePos] = std::tanh (wetInL + wetFeedbackR * fb);
            wetBufR[wetWritePos] = std::tanh (wetInR + wetFeedbackL * fb);

            float sumL = 0.0f, sumR = 0.0f;
            for (int t = 0; t < kNumReverbTaps; ++t)
            {
                const double tapOffset = baseDelaySamples * tapRatios[t];
                const float tapL = readBufferLinear (wetBufL, static_cast<double> (wetWritePos) - tapOffset, kMaxDelaySamples);
                const float tapR = readBufferLinear (wetBufR, static_cast<double> (wetWritePos) - tapOffset * 1.03, kMaxDelaySamples);
                const float sign = (t % 2 == 0) ? 1.0f : -0.88f;
                sumL += tapL * sign;
                sumR += tapR * (-sign);
            }
            sumL *= (1.0f / std::sqrt (static_cast<float> (kNumReverbTaps)));
            sumR *= (1.0f / std::sqrt (static_cast<float> (kNumReverbTaps)));

            const float lpCoeff = 0.15f + (1.0f - smear) * 0.75f;
            smearStateL += lpCoeff * (sumL - smearStateL);
            smearStateR += lpCoeff * (sumR - smearStateR);

            wetFeedbackL = smearStateL;
            wetFeedbackR = smearStateR;
            wetOutL = smearStateL;
            wetOutR = smearStateR;
        }
        else if (wetMode == 1)
        {
            const float fb = wetMod * 0.92f;
            wetBufL[wetWritePos] = std::tanh (wetInL + wetFeedbackL * fb);
            wetBufR[wetWritePos] = std::tanh (wetInR + wetFeedbackR * fb);

            const float delL = readBufferLinear (wetBufL, static_cast<double> (wetWritePos) - baseDelaySamples, kMaxDelaySamples);
            const float delR = readBufferLinear (wetBufR, static_cast<double> (wetWritePos) - baseDelaySamples * 1.015, kMaxDelaySamples);

            smearStateL += 0.45f * (delL - smearStateL);
            smearStateR += 0.45f * (delR - smearStateR);

            wetFeedbackL = smearStateL;
            wetFeedbackR = smearStateR;
            wetOutL = delL;
            wetOutR = delR;
        }
        else
        {
            wetBufL[wetWritePos] = wetInL;
            wetBufR[wetWritePos] = wetInR;

            const int refreshPeriod = static_cast<int> (800.0f + wetTime * 16000.0f);
            if (++slipRefreshCounter >= refreshPeriod)
            {
                slipRefreshCounter = 0;
                slipReadOffset = 64.0;
            }

            const double slipSpeed = (static_cast<double> (wetMod) - 0.5) * 4.0;
            slipReadOffset += (1.0 - slipSpeed);
            slipReadOffset = juce::jlimit (4.0, static_cast<double> (kMaxDelaySamples - 16), slipReadOffset);

            const float sL = readBufferLinear (wetBufL, static_cast<double> (wetWritePos) - slipReadOffset, kMaxDelaySamples);
            const float sR = readBufferLinear (wetBufR, static_cast<double> (wetWritePos) - slipReadOffset, kMaxDelaySamples);

            const float phaseNorm = static_cast<float> (slipRefreshCounter) / static_cast<float> (std::max (1, refreshPeriod));
            const float win = std::sin (juce::MathConstants<float>::pi * phaseNorm);

            wetOutL = sL * win;
            wetOutR = sR * win;
        }

        wetWritePos = (wetWritePos + 1) % kMaxDelaySamples;
    }

    float totalWetL = 0.0f, totalWetR = 0.0f;
    if (routingMode == 1)
    {
        totalWetL = (loopOn ? loopOutL * 0.55f : 0.0f) + (wetOn ? wetOutL : 0.0f);
        totalWetR = (loopOn ? loopOutR * 0.55f : 0.0f) + (wetOn ? wetOutR : 0.0f);
    }
    else
    {
        totalWetL = (loopOn ? loopOutL : 0.0f) + (wetOn ? wetOutL : 0.0f);
        totalWetR = (loopOn ? loopOutR : 0.0f) + (wetOn ? wetOutR : 0.0f);
    }

    heldLoopOutL = loopOutL;
    heldLoopOutR = loopOutR;
    heldWetOutL = totalWetL;
    heldWetOutR = totalWetR;
}

void ObysMoodAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numChannels == 0 || numSamples == 0)
        return;

    float mixVal      = apvts.getRawParameterValue ("mix")->load();
    float clockNorm   = apvts.getRawParameterValue ("clock")->load();
    bool clockStepped = apvts.getRawParameterValue ("clock_stepped")->load() > 0.5f;
    int routingMode   = static_cast<int> (apvts.getRawParameterValue ("routing")->load());

    float wetTime     = apvts.getRawParameterValue ("wet_time")->load();
    float wetMod      = apvts.getRawParameterValue ("wet_modify")->load();
    int wetMode       = static_cast<int> (apvts.getRawParameterValue ("wet_mode")->load());
    bool wetOn        = apvts.getRawParameterValue ("wet_engage")->load() > 0.5f;

    float loopLen     = apvts.getRawParameterValue ("loop_length")->load();
    float loopMod     = apvts.getRawParameterValue ("loop_modify")->load();
    int loopMode      = static_cast<int> (apvts.getRawParameterValue ("loop_mode")->load());
    bool loopOn       = apvts.getRawParameterValue ("loop_engage")->load() > 0.5f;

    int rampTarget    = static_cast<int> (apvts.getRawParameterValue ("ramp_target")->load());
    bool rampBounce   = apvts.getRawParameterValue ("ramp_bounce")->load() > 0.5f;
    bool rampSquare   = apvts.getRawParameterValue ("ramp_square")->load() > 0.5f;

    telemetry.loopActive.store (loopOn, std::memory_order_relaxed);
    telemetry.wetActive.store (wetOn, std::memory_order_relaxed);

    float effectiveClockNorm = clockNorm;
    if (clockStepped)
    {
        static constexpr std::array<float, 9> musicalSteps { 0.0f, 0.14f, 0.25f, 0.38f, 0.50f, 0.62f, 0.75f, 0.88f, 1.0f };
        const int stepIdx = juce::jlimit (0, 8, static_cast<int> (clockNorm * 8.99f));
        effectiveClockNorm = musicalSteps[stepIdx];
    }

    const double targetClockHz = 2200.0 * std::pow (15.5, static_cast<double> (effectiveClockNorm));
    telemetry.effectiveClockHz.store (static_cast<float> (targetClockHz), std::memory_order_relaxed);

    const double clockInc = juce::jlimit (0.02, 1.0, targetClockHz / hostSampleRate);

    const double rampFreqHz = 0.08 + static_cast<double> (mixVal) * 6.5;
    const double rampInc = rampFreqHz / hostSampleRate;
    const float dryWetMix = (rampTarget > 0) ? 0.55f : mixVal;

    auto* chL = buffer.getWritePointer (0);
    auto* chR = (numChannels > 1) ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        if (rampTarget > 0)
        {
            rampPhase += rampInc;
            if (rampBounce)
            {
                if (rampPhase >= 1.0) rampPhase -= 1.0;
                const float tri = 1.0f - std::abs (2.0f * static_cast<float> (rampPhase) - 1.0f);
                currentRamp = rampSquare ? (rampPhase < 0.5 ? 1.0f : 0.0f) : tri;
            }
            else
            {
                if (rampPhase > 1.0) rampPhase = 1.0;
                currentRamp = static_cast<float> (rampPhase);
            }
            telemetry.rampValue.store (currentRamp, std::memory_order_relaxed);
        }
        else
        {
            rampPhase = 0.0;
            currentRamp = 0.0f;
            telemetry.rampValue.store (0.0f, std::memory_order_relaxed);
        }

        const float inL = chL[i];
        const float inR = (chR != nullptr) ? chR[i] : inL;

        clockPhase += clockInc;
        if (clockPhase >= 1.0)
        {
            clockPhase -= 1.0;
            heldInputL = inL;
            heldInputR = inR;
            processClockTick (heldInputL, heldInputR,
                              effectiveClockNorm, mixVal,
                              routingMode,
                              wetOn, wetMode, wetTime, wetMod,
                              loopOn, loopMode, loopLen, loopMod,
                              rampTarget, rampBounce, rampSquare);
        }

        const float outL = inL * (1.0f - dryWetMix) + heldWetOutL * dryWetMix;
        const float outR = inR * (1.0f - dryWetMix) + heldWetOutR * dryWetMix;

        chL[i] = outL;
        if (chR != nullptr)
            chR[i] = outR;
    }
}

juce::AudioProcessorEditor* ObysMoodAudioProcessor::createEditor()
{
    return new ObysMoodAudioProcessorEditor (*this);
}

void ObysMoodAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ObysMoodAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ObysMoodAudioProcessor();
}

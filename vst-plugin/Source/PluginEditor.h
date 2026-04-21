#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class CompressorStrip : public juce::Component, public juce::Timer
{
public:
    CompressorStrip(MultiColorAudioProcessor& p, int index);
    ~CompressorStrip() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    MultiColorAudioProcessor& proc;
    int idx;

    juce::ToggleButton enableBtn;
    juce::Slider       blendSlider;
    juce::Slider       trimSlider;
    juce::Label        nameLabel;
    juce::Label        trimLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trimAttach;

    float displayGR = 0.0f;
};

// ─── Filter knob + enable toggle ─────────────────────────────────────────────
class FilterControl : public juce::Component
{
public:
    FilterControl(juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& freqParamID,
                  const juce::String& onParamID,
                  const juce::String& label);

    void resized() override;

private:
    juce::Slider       freqKnob;
    juce::ToggleButton onBtn;
    juce::Label        label;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  freqAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>  onAttach;
};

// ─── Main editor ─────────────────────────────────────────────────────────────
class MultiColorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MultiColorEditor(MultiColorAudioProcessor&);
    ~MultiColorEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    MultiColorAudioProcessor& proc;

    std::array<std::unique_ptr<CompressorStrip>, kNumCompressors> strips;

    // Global controls row
    FilterControl preHPF, preLPF, postHPF, postLPF;

    juce::Slider atkMultKnob, relMultKnob;
    juce::Label  atkMultLabel, relMultLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> atkMultAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> relMultAttach;

    juce::Slider outGainSlider;
    juce::Label  outGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiColorEditor)
};

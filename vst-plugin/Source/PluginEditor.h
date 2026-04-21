#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// One vertical strip per compressor
class CompressorStrip : public juce::Component,
                        public juce::Timer
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

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>  enableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  blendAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>  trimAttach;

    float displayGR = 0.0f;
};

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

    juce::Slider outGainSlider;
    juce::Label  outGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiColorEditor)
};

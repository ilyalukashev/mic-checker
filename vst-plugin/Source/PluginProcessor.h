#pragma once
#include <JuceHeader.h>
#include "Compressors/LA4Compressor.h"
#include "Compressors/VSC3Compressor.h"
#include "Compressors/DistressorCompressor.h"
#include "Compressors/Fairchild670.h"
#include "Compressors/SSLOverdrive.h"
#include "Compressors/LA2ACompressor.h"
#include "Compressors/Spanky1176.h"

static constexpr int kNumCompressors = 7;

class MultiColorAudioProcessor : public juce::AudioProcessor
{
public:
    MultiColorAudioProcessor();
    ~MultiColorAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MultiColor Comp"; }
    bool  acceptsMidi()  const override { return false; }
    bool  producesMidi() const override { return false; }
    bool  isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()   override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Indexed access for editor
    CompressorBase* getCompressor(int i) { return comps[i].get(); }

private:
    std::array<std::unique_ptr<CompressorBase>, kNumCompressors> comps;
    juce::AudioBuffer<float> tempBuffer;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiColorAudioProcessor)
};

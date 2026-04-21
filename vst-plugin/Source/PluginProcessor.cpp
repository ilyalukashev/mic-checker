#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout MultiColorAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < kNumCompressors; ++i)
    {
        auto prefix = juce::String("comp") + juce::String(i);

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "_blend",
            "Blend " + juce::String(i),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
            0.75f));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            prefix + "_enabled",
            "Enable " + juce::String(i),
            true));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "_trim",
            "Trim " + juce::String(i),
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
            0.0f));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "output_gain", "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f));

    return layout;
}

MultiColorAudioProcessor::MultiColorAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "State", createLayout())
{
    comps[0] = std::make_unique<LA4Compressor>();
    comps[1] = std::make_unique<VSC3Compressor>();
    comps[2] = std::make_unique<DistressorCompressor>();
    comps[3] = std::make_unique<Fairchild670>();
    comps[4] = std::make_unique<SSLOverdrive>();
    comps[5] = std::make_unique<LA2ACompressor>();
    comps[6] = std::make_unique<Spanky1176>();
}

void MultiColorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    for (auto& c : comps)
        c->prepare(sampleRate, samplesPerBlock);

    tempBuffer.setSize(2, samplesPerBlock);
}

void MultiColorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    // Read parameters
    float totalBlend = 0.0f;
    for (int i = 0; i < kNumCompressors; ++i)
    {
        auto prefix  = juce::String("comp") + juce::String(i);
        bool  enabled = apvts.getRawParameterValue(prefix + "_enabled")->load() > 0.5f;
        float blend   = apvts.getRawParameterValue(prefix + "_blend")->load();
        float trim    = apvts.getRawParameterValue(prefix + "_trim")->load();

        comps[i]->setEnabled(enabled);
        comps[i]->setBlend(blend);
        comps[i]->setInputTrim(trim);

        if (enabled) totalBlend += blend;
    }

    if (totalBlend < 0.0001f)
    {
        buffer.clear();
        return;
    }

    // Mix compressor outputs in parallel
    juce::AudioBuffer<float> mixBuffer(numCh, n);
    mixBuffer.clear();

    for (int i = 0; i < kNumCompressors; ++i)
    {
        if (!comps[i]->isEnabled() || comps[i]->getBlend() < 0.001f)
            continue;

        // Clone input into tempBuffer
        tempBuffer.setSize(numCh, n, false, false, true);
        for (int ch = 0; ch < numCh; ++ch)
            tempBuffer.copyFrom(ch, 0, buffer, ch, 0, n);

        comps[i]->process(tempBuffer);

        float w = comps[i]->getBlend() / totalBlend;
        for (int ch = 0; ch < numCh; ++ch)
            mixBuffer.addFrom(ch, 0, tempBuffer, ch, 0, n, w);
    }

    // Write to output
    for (int ch = 0; ch < numCh; ++ch)
        buffer.copyFrom(ch, 0, mixBuffer, ch, 0, n);

    // Apply output gain
    const float outGain = std::pow(10.0f,
        apvts.getRawParameterValue("output_gain")->load() * 0.05f);
    buffer.applyGain(outGain);
}

void MultiColorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MultiColorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultiColorAudioProcessor();
}

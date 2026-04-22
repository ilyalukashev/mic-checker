#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout MultiColorAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < kNumCompressors; ++i)
    {
        auto px = juce::String("comp") + juce::String(i);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            px + "_blend", "Blend "  + juce::String(i),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.75f));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            px + "_enabled", "Enable " + juce::String(i), true));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            px + "_trim", "Trim " + juce::String(i),
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "output_gain", "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "attack_mult", "Attack Mult",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "release_mult", "Release Mult",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));

    // Pre filters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "pre_hpf_freq", "Pre HPF",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.4f), 80.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "pre_hpf_on", "Pre HPF On", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "pre_lpf_freq", "Pre LPF",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.4f), 18000.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "pre_lpf_on", "Pre LPF On", false));

    // Post filters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "post_hpf_freq", "Post HPF",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.4f), 80.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "post_hpf_on", "Post HPF On", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "post_lpf_freq", "Post LPF",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.4f), 18000.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "post_lpf_on", "Post LPF On", false));

    return layout;
}

MultiColorAudioProcessor::MultiColorAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
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

// ─── Calibration ─────────────────────────────────────────────────────────────

float MultiColorAudioProcessor::measureSteadyGR(CompressorBase* comp,
                                                  float inputDB,
                                                  double sr)
{
    const int settleLen  = int(sr * 0.5);  // 500 ms to reach steady state
    const int measureLen = int(sr * 0.2);  // measure over 200 ms
    const int totalLen   = settleLen + measureLen;

    const float amp = CompressorBase::dBToLinear(inputDB);
    const double inc = 2.0 * juce::MathConstants<double>::pi * 440.0 / sr;

    juce::AudioBuffer<float> buf(2, totalLen);
    double phase = 0.0;
    for (int k = 0; k < totalLen; ++k)
    {
        float s = amp * float(std::sin(phase));
        phase += inc;
        buf.setSample(0, k, s);
        buf.setSample(1, k, s);
    }

    comp->reset();

    // Settle phase
    juce::AudioBuffer<float> settle(2, settleLen);
    for (int ch = 0; ch < 2; ++ch)
        settle.copyFrom(ch, 0, buf, ch, 0, settleLen);
    comp->process(settle);

    // Measure phase
    juce::AudioBuffer<float> meas(2, measureLen);
    for (int ch = 0; ch < 2; ++ch)
        meas.copyFrom(ch, 0, buf, ch, settleLen, measureLen);
    comp->process(meas);

    comp->reset();
    return comp->getCurrentGR();
}

void MultiColorAudioProcessor::runCalibration(double sr)
{
    for (int i = 0; i < kNumCompressors; ++i)
    {
        auto* comp = comps[i].get();
        if (dynamic_cast<SSLOverdrive*>(comp))
        {
            // SSLOverdrive has no threshold; apply a fixed -1 dB internal gain
            // so its output level is consistent with the other compressors.
            comp->setCalibTrim(-1.0f);
            continue;
        }

        // Binary search for calibTrim that makes measuredGR == kCalTarget
        float lo = -24.0f, hi = 24.0f;

        for (int iter = 0; iter < 28; ++iter)
        {
            float mid = (lo + hi) * 0.5f;
            comp->setCalibTrim(mid);
            float gr = measureSteadyGR(comp, kCalInputDB, sr);

            if (std::abs(gr - kCalTarget) < 0.01f) break;

            // More trim → higher effective input → more GR
            if (gr < kCalTarget) lo = mid;
            else                 hi = mid;
        }
    }

    // Fixed makeup: compensate for the kCalTarget dB of GR
    makeupGainLin = CompressorBase::dBToLinear(kCalTarget);
}

// ─── prepare / process ───────────────────────────────────────────────────────

void MultiColorAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    for (auto& c : comps) c->prepare(sr, samplesPerBlock);
    tempBuffer.setSize(2, samplesPerBlock);
    preFilter.reset();
    postFilter.reset();
    runCalibration(sr);
}

void MultiColorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    // Read global multipliers
    const float atkM = apvts.getRawParameterValue("attack_mult")->load();
    const float relM = apvts.getRawParameterValue("release_mult")->load();

    // Read filter params and rebuild coefficients (cheap)
    {
        const float phf = apvts.getRawParameterValue("pre_hpf_freq")->load();
        const float plf = apvts.getRawParameterValue("pre_lpf_freq")->load();
        const float ohf = apvts.getRawParameterValue("post_hpf_freq")->load();
        const float olf = apvts.getRawParameterValue("post_lpf_freq")->load();
        const bool  pho = apvts.getRawParameterValue("pre_hpf_on")->load()  > 0.5f;
        const bool  plo = apvts.getRawParameterValue("pre_lpf_on")->load()  > 0.5f;
        const bool  oho = apvts.getRawParameterValue("post_hpf_on")->load() > 0.5f;
        const bool  olo = apvts.getRawParameterValue("post_lpf_on")->load() > 0.5f;

        preFilter.updateCoeffs(getSampleRate(), phf, pho, plf, plo, preHPFOn, preLPFOn);
        postFilter.updateCoeffs(getSampleRate(), ohf, oho, olf, olo, postHPFOn, postLPFOn);
    }

    // Pre-filter
    preFilter.process(buffer, preHPFOn, preLPFOn);

    // Parallel compressor mix
    float totalBlend = 0.0f;
    for (int i = 0; i < kNumCompressors; ++i)
    {
        auto px  = juce::String("comp") + juce::String(i);
        bool  en = apvts.getRawParameterValue(px + "_enabled")->load() > 0.5f;
        float bl = apvts.getRawParameterValue(px + "_blend")->load();
        float tr = apvts.getRawParameterValue(px + "_trim")->load();

        comps[i]->setEnabled(en);
        comps[i]->setBlend(bl);
        comps[i]->setUserTrim(tr);
        comps[i]->setAttackMult(atkM);
        comps[i]->setReleaseMult(relM);

        if (en) totalBlend += bl;
    }

    if (totalBlend < 0.0001f) { buffer.clear(); return; }

    juce::AudioBuffer<float> mixBuffer(numCh, n);
    mixBuffer.clear();

    for (int i = 0; i < kNumCompressors; ++i)
    {
        if (!comps[i]->isEnabled() || comps[i]->getBlend() < 0.001f) continue;

        tempBuffer.setSize(numCh, n, false, false, true);
        for (int ch = 0; ch < numCh; ++ch)
            tempBuffer.copyFrom(ch, 0, buffer, ch, 0, n);

        comps[i]->process(tempBuffer);

        float w = comps[i]->getBlend() / totalBlend;
        for (int ch = 0; ch < numCh; ++ch)
            mixBuffer.addFrom(ch, 0, tempBuffer, ch, 0, n, w);
    }

    for (int ch = 0; ch < numCh; ++ch)
        buffer.copyFrom(ch, 0, mixBuffer, ch, 0, n);

    // Post-filter
    postFilter.process(buffer, postHPFOn, postLPFOn);

    // Makeup (calibration) + user output gain
    const float outGain = makeupGainLin *
        std::pow(10.0f, apvts.getRawParameterValue("output_gain")->load() * 0.05f);
    buffer.applyGain(outGain);
}

// ─── State ───────────────────────────────────────────────────────────────────

void MultiColorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = apvts.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void MultiColorAudioProcessor::setStateInformation(const void* data, int sz)
{
    auto xml = getXmlFromBinary(data, sz);
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultiColorAudioProcessor();
}

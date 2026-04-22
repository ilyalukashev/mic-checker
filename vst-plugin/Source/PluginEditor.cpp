#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  CompressorStrip
// ─────────────────────────────────────────────────────────────────────────────

CompressorStrip::CompressorStrip(MultiColorAudioProcessor& p, int index)
    : proc(p), idx(index)
{
    auto* comp   = proc.getCompressor(idx);
    auto  prefix = juce::String("comp") + juce::String(idx);

    nameLabel.setText(comp->getName(), juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, comp->getColour());
    addAndMakeVisible(nameLabel);

    enableBtn.setButtonText("");
    enableBtn.setColour(juce::ToggleButton::tickColourId, comp->getColour());
    addAndMakeVisible(enableBtn);
    enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, prefix + "_enabled", enableBtn);

    blendSlider.setSliderStyle(juce::Slider::LinearVertical);
    blendSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    blendSlider.setColour(juce::Slider::thumbColourId,      comp->getColour());
    blendSlider.setColour(juce::Slider::trackColourId,      comp->getColour().withAlpha(0.5f));
    blendSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF2A2A2A));
    addAndMakeVisible(blendSlider);
    blendAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, prefix + "_blend", blendSlider);

    trimSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    trimSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    trimSlider.setColour(juce::Slider::thumbColourId,              comp->getColour().brighter(0.2f));
    trimSlider.setColour(juce::Slider::rotarySliderFillColourId,   comp->getColour().withAlpha(0.7f));
    trimSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF404040));
    trimSlider.setColour(juce::Slider::textBoxTextColourId,        juce::Colours::lightgrey);
    trimSlider.setColour(juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    addAndMakeVisible(trimSlider);
    trimAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, prefix + "_trim", trimSlider);

    trimLabel.setText("TRIM", juce::dontSendNotification);
    trimLabel.setJustificationType(juce::Justification::centred);
    trimLabel.setFont(juce::FontOptions(9.0f));
    trimLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(trimLabel);

    startTimerHz(30);
}

CompressorStrip::~CompressorStrip() { stopTimer(); }

void CompressorStrip::timerCallback()
{
    float gr = proc.getCompressor(idx)->getCurrentGR();
    if (std::abs(gr - displayGR) > 0.05f) { displayGR = gr; repaint(); }
}

void CompressorStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto colour = proc.getCompressor(idx)->getColour();

    g.setColour(juce::Colour(0xFF1C1C1C));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(colour.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    const float mX = 10.0f, mW = bounds.getWidth() - 20.0f;
    const float mY = bounds.getHeight() - 36.0f, mH = 8.0f;

    g.setColour(juce::Colour(0xFF2A2A2A));
    g.fillRoundedRectangle(mX, mY, mW, mH, 3.0f);

    float frac = juce::jlimit(0.0f, 1.0f, displayGR / 12.0f);
    if (frac > 0.001f)
    {
        juce::ColourGradient grad(colour.brighter(0.3f), mX, mY,
                                  juce::Colours::red.withAlpha(0.9f),
                                  mX + mW, mY, false);
        grad.addColour(0.5, colour);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(mX, mY, mW * frac, mH, 3.0f);
    }

    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText(juce::String(displayGR, 1) + " dB",
               juce::Rectangle<float>(mX, mY + mH + 1.0f, mW, 12.0f),
               juce::Justification::centred, false);
}

void CompressorStrip::resized()
{
    auto r = getLocalBounds().reduced(6);
    nameLabel.setBounds(r.removeFromTop(20));
    enableBtn.setBounds(r.removeFromTop(22).withSizeKeepingCentre(80, 20));
    r.removeFromTop(4);
    auto bottom = r.removeFromBottom(80);
    blendSlider.setBounds(r);
    trimLabel.setBounds(bottom.removeFromTop(14));
    trimSlider.setBounds(bottom);
}

// ─────────────────────────────────────────────────────────────────────────────
//  FilterControl
// ─────────────────────────────────────────────────────────────────────────────

FilterControl::FilterControl(juce::AudioProcessorValueTreeState& apvts,
                               const juce::String& freqID,
                               const juce::String& onID,
                               const juce::String& labelText)
{
    freqKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 14);
    freqKnob.setColour(juce::Slider::thumbColourId,              juce::Colours::lightblue);
    freqKnob.setColour(juce::Slider::rotarySliderFillColourId,   juce::Colour(0xFF4488AA));
    freqKnob.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF303030));
    freqKnob.setColour(juce::Slider::textBoxTextColourId,        juce::Colours::lightgrey);
    freqKnob.setColour(juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    addAndMakeVisible(freqKnob);
    freqAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, freqID, freqKnob);

    onBtn.setButtonText("ON");
    onBtn.setColour(juce::ToggleButton::tickColourId, juce::Colours::lightblue);
    addAndMakeVisible(onBtn);
    onAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, onID, onBtn);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(label);
}

void FilterControl::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromTop(14));
    onBtn.setBounds(r.removeFromTop(20).withSizeKeepingCentre(40, 18));
    freqKnob.setBounds(r);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers for knob styling
// ─────────────────────────────────────────────────────────────────────────────

static void styleGlobalKnob(juce::Slider& s, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 14);
    s.setColour(juce::Slider::thumbColourId,              juce::Colours::white);
    s.setColour(juce::Slider::rotarySliderFillColourId,   juce::Colours::white.withAlpha(0.6f));
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF404040));
    s.setColour(juce::Slider::textBoxTextColourId,        juce::Colours::lightgrey);
    s.setColour(juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
    (void)name;
}

static void styleLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, juce::Colours::grey);
}

// ─────────────────────────────────────────────────────────────────────────────
//  MultiColorEditor
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioProcessorEditor* MultiColorAudioProcessor::createEditor()
{
    return new MultiColorEditor(*this);
}

MultiColorEditor::MultiColorEditor(MultiColorAudioProcessor& p)
    : AudioProcessorEditor(p), proc(p),
      preHPF (p.apvts, "pre_hpf_freq",  "pre_hpf_on",  "PRE HPF"),
      preLPF (p.apvts, "pre_lpf_freq",  "pre_lpf_on",  "PRE LPF"),
      postHPF(p.apvts, "post_hpf_freq", "post_hpf_on", "POST HPF"),
      postLPF(p.apvts, "post_lpf_freq", "post_lpf_on", "POST LPF")
{
    for (int i = 0; i < kNumCompressors; ++i)
    {
        strips[i] = std::make_unique<CompressorStrip>(proc, i);
        addAndMakeVisible(*strips[i]);
    }

    addAndMakeVisible(preHPF);
    addAndMakeVisible(preLPF);
    addAndMakeVisible(postHPF);
    addAndMakeVisible(postLPF);

    styleGlobalKnob(atkMultKnob, "attack_mult");
    styleGlobalKnob(relMultKnob, "release_mult");
    styleLabel(atkMultLabel, "ATK x");
    styleLabel(relMultLabel, "REL x");
    addAndMakeVisible(atkMultKnob);
    addAndMakeVisible(relMultKnob);
    addAndMakeVisible(atkMultLabel);
    addAndMakeVisible(relMultLabel);

    atkMultAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "attack_mult", atkMultKnob);
    relMultAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "release_mult", relMultKnob);

    styleGlobalKnob(outGainSlider, "output_gain");
    outGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    styleLabel(outGainLabel, "OUT GAIN");
    addAndMakeVisible(outGainSlider);
    addAndMakeVisible(outGainLabel);

    outGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "output_gain", outGainSlider);

    // setSize must come LAST — it triggers resized() immediately
    setSize(1120, 460);
}

void MultiColorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF141414));

    // Separator line above global controls
    auto r = getLocalBounds();
    g.setColour(juce::Colour(0xFF303030));
    g.fillRect(r.removeFromBottom(82).removeFromTop(1));

    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("MultiColor Comp", getLocalBounds().removeFromTop(28),
               juce::Justification::centred, false);
}

void MultiColorEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop(28);                          // title

    auto globalBar = r.removeFromBottom(80);      // global controls
    auto filterBar = globalBar.reduced(6, 4);

    // Filter controls — left block (pre) and right block (post)
    int filterW = 84;
    preHPF .setBounds(filterBar.removeFromLeft(filterW));
    preLPF .setBounds(filterBar.removeFromLeft(filterW));

    filterBar.removeFromLeft(8); // gap

    // Attack / Release multipliers in center
    auto multArea = filterBar.removeFromLeft(180);
    {
        auto la = multArea.removeFromLeft(90).reduced(2);
        atkMultLabel.setBounds(la.removeFromTop(14));
        atkMultKnob .setBounds(la);
    }
    {
        auto la = multArea.removeFromLeft(90).reduced(2);
        relMultLabel.setBounds(la.removeFromTop(14));
        relMultKnob .setBounds(la);
    }

    filterBar.removeFromLeft(8);
    postHPF.setBounds(filterBar.removeFromLeft(filterW));
    postLPF.setBounds(filterBar.removeFromLeft(filterW));

    // Output gain — right edge
    auto outArea = filterBar.removeFromRight(100).reduced(2);
    outGainLabel .setBounds(outArea.removeFromTop(14));
    outGainSlider.setBounds(outArea);

    // Compressor strips fill the rest
    const int stripW = r.getWidth() / kNumCompressors;
    for (auto& strip : strips)
        strip->setBounds(r.removeFromLeft(stripW).reduced(4));
}

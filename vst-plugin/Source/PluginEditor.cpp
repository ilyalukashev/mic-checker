#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  CompressorStrip
// ─────────────────────────────────────────────────────────────────────────────

CompressorStrip::CompressorStrip(MultiColorAudioProcessor& p, int index)
    : proc(p), idx(index)
{
    auto* comp   = proc.getCompressor(idx);
    auto  prefix = juce::String("comp") + juce::String(idx);

    // Name
    nameLabel.setText(comp->getName(), juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, comp->getColour());
    addAndMakeVisible(nameLabel);

    // Enable toggle
    enableBtn.setButtonText("");
    enableBtn.setColour(juce::ToggleButton::tickColourId, comp->getColour());
    addAndMakeVisible(enableBtn);
    enableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, prefix + "_enabled", enableBtn);

    // Blend fader (vertical)
    blendSlider.setSliderStyle(juce::Slider::LinearVertical);
    blendSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    blendSlider.setColour(juce::Slider::thumbColourId,     comp->getColour());
    blendSlider.setColour(juce::Slider::trackColourId,     comp->getColour().withAlpha(0.5f));
    blendSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF2A2A2A));
    addAndMakeVisible(blendSlider);
    blendAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, prefix + "_blend", blendSlider);

    // Trim knob
    trimSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    trimSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    trimSlider.setColour(juce::Slider::thumbColourId, comp->getColour().brighter(0.2f));
    trimSlider.setColour(juce::Slider::rotarySliderFillColourId, comp->getColour().withAlpha(0.7f));
    trimSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF404040));
    trimSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::lightgrey);
    trimSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
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

CompressorStrip::~CompressorStrip()
{
    stopTimer();
}

void CompressorStrip::timerCallback()
{
    float gr = proc.getCompressor(idx)->getCurrentGR();
    if (std::abs(gr - displayGR) > 0.05f)
    {
        displayGR = gr;
        repaint();
    }
}

void CompressorStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto colour = proc.getCompressor(idx)->getColour();

    // Background
    g.setColour(juce::Colour(0xFF1C1C1C));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Border
    g.setColour(colour.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    // GR meter bar (below blend slider)
    const float meterX = 10.0f;
    const float meterW = bounds.getWidth() - 20.0f;
    const float meterY = bounds.getHeight() - 36.0f;
    const float meterH = 8.0f;

    g.setColour(juce::Colour(0xFF2A2A2A));
    g.fillRoundedRectangle(meterX, meterY, meterW, meterH, 3.0f);

    float grFraction = juce::jlimit(0.0f, 1.0f, displayGR / 12.0f);
    if (grFraction > 0.001f)
    {
        juce::ColourGradient grad(colour.brighter(0.3f), meterX, meterY,
                                  juce::Colours::red.withAlpha(0.9f),
                                  meterX + meterW, meterY, false);
        grad.addColour(0.5, colour);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(meterX, meterY, meterW * grFraction, meterH, 3.0f);
    }

    // GR readout
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText(juce::String(displayGR, 1) + " dB",
               juce::Rectangle<float>(meterX, meterY + meterH + 1.0f, meterW, 12.0f),
               juce::Justification::centred, false);
}

void CompressorStrip::resized()
{
    auto r = getLocalBounds().reduced(6);

    nameLabel.setBounds(r.removeFromTop(20));
    enableBtn.setBounds(r.removeFromTop(22).withSizeKeepingCentre(80, 20));
    r.removeFromTop(4);

    // Blend fader takes most of the height
    auto bottomArea = r.removeFromBottom(80);
    blendSlider.setBounds(r);

    // Trim knob at bottom
    trimLabel.setBounds(bottomArea.removeFromTop(14));
    trimSlider.setBounds(bottomArea);
}

// ─────────────────────────────────────────────────────────────────────────────
//  MultiColorEditor
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioProcessorEditor* MultiColorAudioProcessor::createEditor()
{
    return new MultiColorEditor(*this);
}

MultiColorEditor::MultiColorEditor(MultiColorAudioProcessor& p)
    : AudioProcessorEditor(p), proc(p)
{
    setSize(1120, 400);

    for (int i = 0; i < kNumCompressors; ++i)
    {
        strips[i] = std::make_unique<CompressorStrip>(proc, i);
        addAndMakeVisible(*strips[i]);
    }

    // Output gain
    outGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    outGainSlider.setColour(juce::Slider::thumbColourId,            juce::Colours::white);
    outGainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white.withAlpha(0.6f));
    outGainSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF404040));
    outGainSlider.setColour(juce::Slider::textBoxTextColourId,     juce::Colours::lightgrey);
    outGainSlider.setColour(juce::Slider::textBoxOutlineColourId,  juce::Colours::transparentBlack);
    addAndMakeVisible(outGainSlider);
    outGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "output_gain", outGainSlider);

    outGainLabel.setText("OUT GAIN", juce::dontSendNotification);
    outGainLabel.setJustificationType(juce::Justification::centred);
    outGainLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    outGainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(outGainLabel);
}

void MultiColorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF141414));

    // Title
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText("MultiColor Comp", getLocalBounds().removeFromTop(28),
               juce::Justification::centred, false);
}

void MultiColorEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop(28);                        // title
    auto bottomBar = r.removeFromBottom(50);    // output gain area

    const int stripW = r.getWidth() / kNumCompressors;
    for (auto& strip : strips)
        strip->setBounds(r.removeFromLeft(stripW).reduced(4));

    // Output gain
    outGainLabel.setBounds(bottomBar.removeFromLeft(100).withTrimmedTop(6));
    outGainSlider.setBounds(bottomBar.removeFromLeft(90).withSizeKeepingCentre(90, 46));
}

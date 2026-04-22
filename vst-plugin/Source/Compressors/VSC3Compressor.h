#pragma once
#include "CompressorBase.h"

// Vertigo VSC-3 / SSL G-Bus VCA character
// Hard-knee RMS detector, fast attack, tight punch, minimal saturation
class VSC3Compressor : public CompressorBase
{
public:
    juce::String getName()   const override { return "VSC-3"; }
    juce::Colour getColour() const override { return juce::Colour(0xFF3A8FD4); }

    void prepare(double sr, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset()  override;

private:
    static constexpr float kThreshold  = -18.0f;
    static constexpr float kRatioRecip = 1.0f / 4.0f;
    static constexpr float kKnee       = 1.0f;  // near-hard knee

    static constexpr int kRmsWindow = 512;
    float rmsBuffer[kRmsWindow] = {};
    int   rmsPos  = 0;
    float rmsSumSq = 0.0f;

    float gainSmooth = 0.0f;
};

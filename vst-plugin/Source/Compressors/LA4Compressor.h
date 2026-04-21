#pragma once
#include "CompressorBase.h"

// UREI LA-4 / Fuse Audio VCL-4 character
// Opto/VCA hybrid: slow dual-TC photocell, soft knee, warm 2nd-harmonic saturation
class LA4Compressor : public CompressorBase
{
public:
    juce::String getName()   const override { return "LA-4"; }
    juce::Colour getColour() const override { return juce::Colour(0xFFD4A843); }

    void prepare(double sr, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset()  override;

private:
    static constexpr float kThreshold  = -18.0f;
    static constexpr float kRatioRecip = 1.0f / 3.5f;
    static constexpr float kKnee       = 10.0f;

    float envFast = 0.0f, envSlow = 0.0f;
    float gainSmooth = 0.0f;
};

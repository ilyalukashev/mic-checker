#pragma once
#include "CompressorBase.h"

// Fairchild 670 character
// Tube vari-mu: ratio grows with GR, program-dependent release (6-TC blend),
// transformer even-harmonic saturation, very slow and musical
class Fairchild670 : public CompressorBase
{
public:
    juce::String getName()   const override { return "670"; }
    juce::Colour getColour() const override { return juce::Colour(0xFF7A4F8F); }

    void prepare(double sr, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset()  override;

private:
    static constexpr float kThreshold = -16.0f;
    static constexpr float kKnee      = 12.0f;
    static constexpr float kBaseRatio = 2.0f;   // ratio at threshold (grows with GR)

    // 6 release time constants (ms), blended by signal level
    static constexpr float kTCs[6] = { 300.0f, 800.0f, 2000.0f, 5000.0f, 12000.0f, 25000.0f };

    float envL = 0.0f, envR = 0.0f;
    float gainSmooth = 0.0f;
    float prevGR = 0.0f;
};

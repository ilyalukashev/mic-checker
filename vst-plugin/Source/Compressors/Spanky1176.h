#pragma once
#include "CompressorBase.h"

// Universal Audio 1176 "All Buttons" / Spanky mode
// FET, instant attack, all-buttons ~12:1 equivalent, 3kHz "honk" resonance,
// aggressive pumping, 3rd-harmonic FET distortion
class Spanky1176 : public CompressorBase
{
public:
    juce::String getName()   const override { return "Spanky 1176"; }
    juce::Colour getColour() const override { return juce::Colour(0xFFCB4040); }

    void prepare(double sr, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset()  override;

private:
    static constexpr float kThreshold  = -18.0f;
    static constexpr float kRatioRecip = 1.0f / 12.0f;
    static constexpr float kKnee       = 1.5f;

    float envL = 0.0f, envR = 0.0f;
    float gainSmooth = 0.0f;

    // Resonant 3kHz "honk" filter (biquad bandpass), per channel
    struct BQState { float x1=0,x2=0,y1=0,y2=0; } honkL, honkR;
    struct BQCoeffs { float b0,b1,b2,a1,a2; } honkC;
};

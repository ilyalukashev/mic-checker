#pragma once
#include "CompressorBase.h"

// Empirical Labs Distressor character
// Ultra-fast FET peak detector, 2nd+3rd harmonic DIST mode, aggressive pumping
class DistressorCompressor : public CompressorBase
{
public:
    juce::String getName()   const override { return "Distressor"; }
    juce::Colour getColour() const override { return juce::Colour(0xFFE05C2A); }

    void prepare(double sr, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset()  override;

private:
    static constexpr float kThreshold  = -18.0f;
    static constexpr float kRatioRecip = 1.0f / 6.0f;
    static constexpr float kKnee       = 2.0f;

    float envL = 0.0f, envR = 0.0f;
    float gainSmooth = 0.0f;

    // HP sidechain filter state (200 Hz highpass to avoid bass-triggered pumping)
    float hpX1L = 0.0f, hpX1R = 0.0f;
    float hpY1L = 0.0f, hpY1R = 0.0f;
    float hpA1 = 0.0f, hpB0 = 0.0f, hpB1 = 0.0f;
};

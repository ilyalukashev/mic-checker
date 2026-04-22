#pragma once
#include "CompressorBase.h"

// Teletronix LA-2A character
// T4B photo-cell: program-dependent 3-TC release, 3:1 soft-limiting knee,
// smooth opto response, classic optical warmth
class LA2ACompressor : public CompressorBase
{
public:
    juce::String getName()   const override { return "LA-2A"; }
    juce::Colour getColour() const override { return juce::Colour(0xFFB0B0B0); }

    void prepare(double sr, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset()  override;

private:
    static constexpr float kThreshold  = -18.0f;
    static constexpr float kKnee       = 15.0f;  // wide soft knee = limiting character

    // Photo-cell envelope: three parallel decay paths
    float cellFast   = 0.0f;   // 50 ms
    float cellMed    = 0.0f;   // 500 ms
    float cellSlow   = 0.0f;   // 4500 ms
    float gainSmooth = 0.0f;
};

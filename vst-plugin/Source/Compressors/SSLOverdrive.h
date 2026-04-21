#pragma once
#include "CompressorBase.h"

// SSL 4000 mic-preamp overdrive character
// HPF 80 Hz + LPF 12 kHz, then soft-clip saturation — adds grit & midrange presence
// Not a compressor; currentGR_dB reflects drive amount for meter display
class SSLOverdrive : public CompressorBase
{
public:
    juce::String getName()   const override { return "SSL Pre"; }
    juce::Colour getColour() const override { return juce::Colour(0xFF2EB87A); }

    void prepare(double sr, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset()  override;

    void setDrive(float dB) { drive = dBToLinear(dB); }

private:
    float drive = dBToLinear(12.0f); // ~12 dB into saturation

    // Biquad state for HPF & LPF (per channel)
    struct BiquadState { float x1=0,x2=0,y1=0,y2=0; };
    BiquadState hpfL, hpfR, lpfL, lpfR;

    struct BiquadCoeffs { float b0,b1,b2,a1,a2; };
    BiquadCoeffs hpfC, lpfC;

    static BiquadCoeffs makeHPF(double fc, double sr);
    static BiquadCoeffs makeLPF(double fc, double sr);
    static float biquad(float x, BiquadState& s, const BiquadCoeffs& c);
};

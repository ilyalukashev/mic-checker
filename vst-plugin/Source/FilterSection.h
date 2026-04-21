#pragma once
#include <JuceHeader.h>
#include <cmath>

struct ButterworthBiquad
{
    float b0=1,b1=0,b2=0,a1=0,a2=0;
    float x1=0,x2=0,y1=0,y2=0;

    void makeHPF(double fc, double sr)
    {
        const double w  = 2.0 * juce::MathConstants<double>::pi * fc / sr;
        const double c  = std::cos(w), s = std::sin(w);
        const double al = s / (2.0 * 0.70710678);
        const double a0 = 1.0 + al;
        b0 = float( (1.0+c)*0.5 / a0);
        b1 = float(-(1.0+c)     / a0);
        b2 = b0;
        a1 = float(-2.0*c / a0);
        a2 = float((1.0-al) / a0);
    }

    void makeLPF(double fc, double sr)
    {
        const double w  = 2.0 * juce::MathConstants<double>::pi * fc / sr;
        const double c  = std::cos(w), s = std::sin(w);
        const double al = s / (2.0 * 0.70710678);
        const double a0 = 1.0 + al;
        b0 = float( (1.0-c)*0.5 / a0);
        b1 = float( (1.0-c)     / a0);
        b2 = b0;
        a1 = float(-2.0*c / a0);
        a2 = float((1.0-al) / a0);
    }

    float tick(float x)
    {
        float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2=x1; x1=x; y2=y1; y1=y;
        return y;
    }

    void reset() { x1=x2=y1=y2=0.0f; }
};

// Stereo HPF + LPF pair for a single stage (pre or post)
struct FilterSection
{
    ButterworthBiquad hpfL, hpfR, lpfL, lpfR;

    void updateCoeffs(double sr,
                      float hpfFreq, bool hpfOn,
                      float lpfFreq, bool lpfOn,
                      bool& outHPFOn, bool& outLPFOn)
    {
        outHPFOn = hpfOn;
        outLPFOn = lpfOn;
        if (hpfOn) { hpfL.makeHPF(hpfFreq, sr); hpfR.makeHPF(hpfFreq, sr); }
        if (lpfOn) { lpfL.makeLPF(lpfFreq, sr); lpfR.makeLPF(lpfFreq, sr); }
    }

    void reset() { hpfL.reset(); hpfR.reset(); lpfL.reset(); lpfR.reset(); }

    void process(juce::AudioBuffer<float>& buf, bool hpfOn, bool lpfOn)
    {
        auto* L = buf.getWritePointer(0);
        auto* R = buf.getNumChannels() > 1 ? buf.getWritePointer(1) : nullptr;
        const int n = buf.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            if (hpfOn) { L[i] = hpfL.tick(L[i]); if (R) R[i] = hpfR.tick(R[i]); }
            if (lpfOn) { L[i] = lpfL.tick(L[i]); if (R) R[i] = lpfR.tick(R[i]); }
        }
    }
};

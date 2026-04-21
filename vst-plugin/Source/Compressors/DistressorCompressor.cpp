#include "DistressorCompressor.h"

void DistressorCompressor::prepare(double sr, int blockSize)
{
    CompressorBase::prepare(sr, blockSize);
    const float w  = 2.0f * juce::MathConstants<float>::pi * 200.0f / float(sr);
    const float c  = std::cos(w), s2 = std::sin(w);
    const float al = s2 / (2.0f * 0.70710678f);
    const float a0 = 1.0f + al;
    hpB0 =  (1.0f + c) * 0.5f / a0;
    hpB1 = -(1.0f + c)         / a0;
    hpA1 = -2.0f * c            / a0;
    reset();
}

void DistressorCompressor::reset()
{
    envL = envR = gainSmooth = 0.0f;
    hpX1L = hpX1R = hpY1L = hpY1R = 0.0f;
}

void DistressorCompressor::process(juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    const float envAtk = timeToCoeff(0.05f  * attackMult,  sampleRate);
    const float envRel = timeToCoeff(12.0f  * releaseMult, sampleRate);
    const float gAtk   = timeToCoeff(0.1f   * attackMult,  sampleRate);
    const float gRel   = timeToCoeff(100.0f * releaseMult, sampleRate);

    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;
    float totalGR = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i] * inputTrimLin;
        const float inR = R ? R[i] * inputTrimLin : inL;

        float scL = hpB0 * inL + hpB1 * hpX1L - hpA1 * hpY1L;
        hpX1L = inL; hpY1L = scL;
        float scR = hpB0 * inR + hpB1 * hpX1R - hpA1 * hpY1R;
        hpX1R = inR; hpY1R = scR;

        float levelL = std::abs(scL), levelR = std::abs(scR);

        if (levelL > envL) envL = envAtk * envL + (1.0f - envAtk) * levelL;
        else               envL = envRel * envL + (1.0f - envRel) * levelL;
        if (levelR > envR) envR = envAtk * envR + (1.0f - envAtk) * levelR;
        else               envR = envRel * envR + (1.0f - envRel) * levelR;

        float detDB    = linearTodB(std::max(envL, envR) + 1e-7f);
        float targetGR = gainComputeDB(detDB, kThreshold, kRatioRecip, kKnee);

        if (targetGR < gainSmooth) gainSmooth = gAtk * gainSmooth + (1.0f - gAtk) * targetGR;
        else                       gainSmooth = gRel * gainSmooth + (1.0f - gRel) * targetGR;

        float gain = dBToLinear(gainSmooth);
        float outL = inL * gain, outR = inR * gain;
        L[i] = softSat(outL * 1.04f, 0.8f) * 0.96f;
        if (R) R[i] = softSat(outR * 1.04f, 0.8f) * 0.96f;
        totalGR += gainSmooth;
    }

    currentGR_dB = -(totalGR / float(n));
}

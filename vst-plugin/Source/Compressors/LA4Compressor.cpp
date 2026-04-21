#include "LA4Compressor.h"

void LA4Compressor::prepare(double sr, int blockSize)
{
    CompressorBase::prepare(sr, blockSize);
    reset();
}

void LA4Compressor::reset()
{
    envFast = envSlow = gainSmooth = 0.0f;
}

void LA4Compressor::process(juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    const float fastAtk = timeToCoeff(3.0f  * attackMult,  sampleRate);
    const float fastRel = timeToCoeff(80.0f  * releaseMult, sampleRate);
    const float slowAtk = timeToCoeff(40.0f  * attackMult,  sampleRate);
    const float slowRel = timeToCoeff(1800.0f* releaseMult, sampleRate);
    const float gAtk    = timeToCoeff(5.0f   * attackMult,  sampleRate);
    const float gRel    = timeToCoeff(380.0f * releaseMult, sampleRate);

    float totalGR = 0.0f;
    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i] * inputTrimLin;
        const float inR = R ? R[i] * inputTrimLin : inL;

        float level = std::max(std::abs(inL), std::abs(inR));

        if (level > envFast) envFast = fastAtk * envFast + (1.0f - fastAtk) * level;
        else                 envFast = fastRel * envFast + (1.0f - fastRel) * level;

        if (level > envSlow) envSlow = slowAtk * envSlow + (1.0f - slowAtk) * level;
        else                 envSlow = slowRel * envSlow + (1.0f - slowRel) * level;

        float detector = envFast * 0.30f + envSlow * 0.70f;
        float detDB    = linearTodB(detector + 1e-7f);
        float targetGR = gainComputeDB(detDB, kThreshold, kRatioRecip, kKnee);

        if (targetGR < gainSmooth) gainSmooth = gAtk * gainSmooth + (1.0f - gAtk) * targetGR;
        else                       gainSmooth = gRel * gainSmooth + (1.0f - gRel) * targetGR;

        float gain = dBToLinear(gainSmooth);
        L[i] = softSat(inL * gain * 1.015f, 0.4f) * 0.985f;
        if (R) R[i] = softSat(inR * gain * 1.015f, 0.4f) * 0.985f;
        totalGR += gainSmooth;
    }

    currentGR_dB = -(totalGR / float(n));
}

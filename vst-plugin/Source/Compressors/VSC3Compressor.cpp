#include "VSC3Compressor.h"

void VSC3Compressor::prepare(double sr, int blockSize)
{
    CompressorBase::prepare(sr, blockSize);
    reset();
    // Scale RMS window to ~3 ms
    // (using fixed array; window effectively kRmsWindow samples)
}

void VSC3Compressor::reset()
{
    std::fill(std::begin(rmsBuffer), std::end(rmsBuffer), 0.0f);
    rmsPos = 0;
    rmsSumSq = 0.0f;
    gainSmooth = 0.0f;
}

void VSC3Compressor::process(juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    const float gAtk = timeToCoeff(0.3f,   sampleRate);
    const float gRel = timeToCoeff(150.0f, sampleRate);

    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;

    float totalGR = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i] * inputTrimLin;
        const float inR = R ? R[i] * inputTrimLin : inL;

        // Stereo-linked RMS (circular buffer, fixed 512-sample window)
        float peak = 0.5f * (inL * inL + inR * inR);
        rmsSumSq  -= rmsBuffer[rmsPos];
        rmsSumSq  += peak;
        rmsSumSq   = std::max(rmsSumSq, 0.0f);
        rmsBuffer[rmsPos] = peak;
        rmsPos = (rmsPos + 1) % kRmsWindow;

        float rmsLin = std::sqrt(rmsSumSq / float(kRmsWindow));
        float rmsDB  = linearTodB(rmsLin + 1e-7f);

        float targetGR = gainComputeDB(rmsDB, kThreshold, kRatioRecip, kKnee);

        if (targetGR < gainSmooth) gainSmooth = gAtk * gainSmooth + (1.0f - gAtk) * targetGR;
        else                       gainSmooth = gRel * gainSmooth + (1.0f - gRel) * targetGR;

        float gain = dBToLinear(gainSmooth);

        L[i] = inL * gain;
        if (R) R[i] = inR * gain;

        totalGR += gainSmooth;
    }

    currentGR_dB = -(totalGR / float(n));
}

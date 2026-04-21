#include "LA2ACompressor.h"

void LA2ACompressor::prepare(double sr, int blockSize)
{
    CompressorBase::prepare(sr, blockSize);
    reset();
}

void LA2ACompressor::reset()
{
    cellFast = cellMed = cellSlow = gainSmooth = 0.0f;
}

void LA2ACompressor::process(juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    const float cellAtk     = timeToCoeff(10.0f    * attackMult,  sampleRate);
    const float cellFastRel = timeToCoeff(50.0f    * releaseMult, sampleRate);
    const float cellMedRel  = timeToCoeff(500.0f   * releaseMult, sampleRate);
    const float cellSlowRel = timeToCoeff(4500.0f  * releaseMult, sampleRate);
    const float gAtk        = timeToCoeff(8.0f     * attackMult,  sampleRate);

    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;
    float totalGR = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i] * inputTrimLin;
        const float inR = R ? R[i] * inputTrimLin : inL;

        float level = std::max(std::abs(inL), std::abs(inR));

        if (level > cellFast)
        {
            cellFast = cellAtk * cellFast + (1.0f - cellAtk) * level;
            cellMed  = cellAtk * cellMed  + (1.0f - cellAtk) * level;
            cellSlow = cellAtk * cellSlow + (1.0f - cellAtk) * level;
        }
        else
        {
            cellFast = cellFastRel * cellFast;
            cellMed  = cellMedRel  * cellMed;
            cellSlow = cellSlowRel * cellSlow;
        }

        float detector = cellFast * 0.20f + cellMed * 0.45f + cellSlow * 0.35f;
        float detDB    = linearTodB(detector + 1e-7f);

        float overshoot  = std::max(0.0f, detDB - kThreshold);
        float dynRatio   = 3.0f + overshoot * 0.25f;
        float targetGR   = gainComputeDB(detDB, kThreshold, 1.0f / dynRatio, kKnee);

        if (targetGR < gainSmooth) gainSmooth = gAtk * gainSmooth + (1.0f - gAtk) * targetGR;
        else
        {
            float relCoeff = timeToCoeff(600.0f * releaseMult, sampleRate);
            gainSmooth = relCoeff * gainSmooth + (1.0f - relCoeff) * targetGR;
        }

        float gain = dBToLinear(gainSmooth);
        L[i] = inL * gain;
        if (R) R[i] = inR * gain;
        totalGR += gainSmooth;
    }

    currentGR_dB = -(totalGR / float(n));
}

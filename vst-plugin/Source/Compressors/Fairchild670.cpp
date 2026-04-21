#include "Fairchild670.h"

constexpr float Fairchild670::kTCs[6];

void Fairchild670::prepare(double sr, int blockSize)
{
    CompressorBase::prepare(sr, blockSize);
    reset();
}

void Fairchild670::reset()
{
    envL = envR = gainSmooth = prevGR = 0.0f;
}

void Fairchild670::process(juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    const float envAtk = timeToCoeff(0.2f  * attackMult, sampleRate);
    const float gAtk   = timeToCoeff(0.2f  * attackMult, sampleRate);

    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;
    float totalGR = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i] * inputTrimLin;
        const float inR = R ? R[i] * inputTrimLin : inL;

        float level = std::max(std::abs(inL), std::abs(inR));

        if (level > envL) envL = envAtk * envL + (1.0f - envAtk) * level;

        float detDB     = linearTodB(envL + 1e-7f);
        float overshoot = std::max(0.0f, detDB - kThreshold);
        float effRatio  = kBaseRatio + overshoot * 0.65f;
        float targetGR  = gainComputeDB(detDB, kThreshold, 1.0f / effRatio, kKnee);

        float grDepth = juce::jlimit(0.0f, 1.0f, -prevGR / 12.0f);
        float relMS   = (kTCs[0] + grDepth * (kTCs[5] - kTCs[0])) * releaseMult;
        float gRel    = timeToCoeff(relMS, sampleRate);

        if (targetGR < gainSmooth) gainSmooth = gAtk * gainSmooth + (1.0f - gAtk) * targetGR;
        else                       gainSmooth = gRel * gainSmooth + (1.0f - gRel) * targetGR;

        prevGR = gainSmooth;
        float gain = dBToLinear(gainSmooth);

        auto tubeSat = [](float x) -> float {
            float y = x + 0.04f * std::abs(x) * x;
            return std::tanh(y * 0.9f) / std::tanh(0.9f);
        };

        L[i] = tubeSat(inL * gain);
        if (R) R[i] = tubeSat(inR * gain);
        totalGR += gainSmooth;
    }

    currentGR_dB = -(totalGR / float(n));
}

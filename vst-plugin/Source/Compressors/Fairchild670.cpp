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

    // Very slow vari-mu attack (~0.2ms)
    const float envAtk = timeToCoeff(0.2f, sampleRate);
    // Release: program-dependent, computed per-sample from prevGR
    const float gAtk   = timeToCoeff(0.2f, sampleRate);

    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;

    float totalGR = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i] * inputTrimLin;
        const float inR = R ? R[i] * inputTrimLin : inL;

        float level = std::max(std::abs(inL), std::abs(inR));

        if (level > envL) envL = envAtk * envL + (1.0f - envAtk) * level;
        else              envL = envL; // release handled below via gainSmooth

        float detDB = linearTodB(envL + 1e-7f);

        // Vari-mu: effective ratio increases with compression depth
        // At 0dB above threshold: ratio ~2:1
        // At 6dB above threshold: ratio ~6:1 (tube grid bias changes)
        float overshoot = std::max(0.0f, detDB - kThreshold);
        float effRatio  = kBaseRatio + overshoot * 0.65f;
        float ratioRecip = 1.0f / effRatio;

        float targetGR = gainComputeDB(detDB, kThreshold, ratioRecip, kKnee);

        // Program-dependent release: lighter GR → faster release, heavier GR → slower
        // Interpolate between TC[0] (fast, light) and TC[5] (slow, heavy)
        float grDepth  = juce::jlimit(0.0f, 1.0f, -prevGR / 12.0f);
        float relMS    = kTCs[0] + grDepth * (kTCs[5] - kTCs[0]);
        float gRel     = timeToCoeff(relMS, sampleRate);

        if (targetGR < gainSmooth) gainSmooth = gAtk * gainSmooth + (1.0f - gAtk) * targetGR;
        else                       gainSmooth = gRel * gainSmooth + (1.0f - gRel) * targetGR;

        prevGR = gainSmooth;
        float gain = dBToLinear(gainSmooth);

        // Transformer saturation: even-harmonic (tube warmth)
        float outL = inL * gain;
        float outR = inR * gain;

        // Subtle even-harmonic: x + 0.05*x^2 soft-clipped
        auto tubeSat = [](float x) -> float
        {
            float y = x + 0.04f * x * x * (x < 0.0f ? -1.0f : 1.0f);
            return std::tanh(y * 0.9f) * (1.0f / std::tanh(0.9f));
        };

        L[i] = tubeSat(outL);
        if (R) R[i] = tubeSat(outR);

        totalGR += gainSmooth;
    }

    currentGR_dB = -(totalGR / float(n));
}

#include "Spanky1176.h"

void Spanky1176::prepare(double sr, int blockSize)
{
    CompressorBase::prepare(sr, blockSize);

    // Peaking EQ at 3kHz, Q=1.5, +3dB — the classic 1176 all-buttons "honk"
    const double fc  = 3000.0;
    const double w0  = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double c   = std::cos(w0);
    const double s   = std::sin(w0);
    const double Q   = 1.5;
    const double A   = std::pow(10.0, 3.0 / 40.0); // +3dB peak
    const double alp = s / (2.0 * Q);
    const double a0  = 1.0 + alp / A;
    honkC.b0 = float((1.0 + alp * A) / a0);
    honkC.b1 = float(-2.0 * c       / a0);
    honkC.b2 = float((1.0 - alp * A) / a0);
    honkC.a1 = float(-2.0 * c       / a0);
    honkC.a2 = float((1.0 - alp / A) / a0);

    reset();
}

void Spanky1176::reset()
{
    envL = envR = gainSmooth = 0.0f;
    honkL = honkR = {};
}

void Spanky1176::process(juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    // Instantaneous FET attack, fast release with pumping
    const float envAtk = timeToCoeff(0.02f,  sampleRate);
    const float envRel = timeToCoeff(8.0f,   sampleRate);
    const float gAtk   = timeToCoeff(0.05f,  sampleRate);
    const float gRel   = timeToCoeff(55.0f,  sampleRate);

    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;

    float totalGR = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i] * inputTrimLin;
        const float inR = R ? R[i] * inputTrimLin : inL;

        float levelL = std::abs(inL);
        float levelR = std::abs(inR);

        if (levelL > envL) envL = envAtk * envL + (1.0f - envAtk) * levelL;
        else               envL = envRel * envL + (1.0f - envRel) * levelL;

        if (levelR > envR) envR = envAtk * envR + (1.0f - envAtk) * levelR;
        else               envR = envRel * envR + (1.0f - envRel) * levelR;

        float detDB  = linearTodB(std::max(envL, envR) + 1e-7f);
        float targetGR = gainComputeDB(detDB, kThreshold, kRatioRecip, kKnee);

        if (targetGR < gainSmooth) gainSmooth = gAtk * gainSmooth + (1.0f - gAtk) * targetGR;
        else                       gainSmooth = gRel * gainSmooth + (1.0f - gRel) * targetGR;

        float gain = dBToLinear(gainSmooth);

        float outL = inL * gain;
        float outR = inR * gain;

        // 3kHz honk resonance (biquad peaking)
        auto applyHonk = [&](float x, BQState& s) -> float
        {
            float y = honkC.b0 * x + honkC.b1 * s.x1 + honkC.b2 * s.x2
                                   - honkC.a1 * s.y1 - honkC.a2 * s.y2;
            s.x2 = s.x1; s.x1 = x;
            s.y2 = s.y1; s.y1 = y;
            return y;
        };

        outL = applyHonk(outL, honkL);
        if (R) outR = applyHonk(outR, honkR);

        // FET odd-harmonic (3rd) clipping
        L[i] = softSat(outL * 1.06f, 1.2f) * 0.94f;
        if (R) R[i] = softSat(outR * 1.06f, 1.2f) * 0.94f;

        totalGR += gainSmooth;
    }

    currentGR_dB = -(totalGR / float(n));
}

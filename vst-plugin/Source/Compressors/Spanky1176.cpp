#include "Spanky1176.h"

void Spanky1176::prepare(double sr, int blockSize)
{
    CompressorBase::prepare(sr, blockSize);
    const double fc  = 3000.0;
    const double w0  = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double c   = std::cos(w0), s = std::sin(w0);
    const double Q   = 1.5;
    const double A   = std::pow(10.0, 3.0 / 40.0);
    const double alp = s / (2.0 * Q);
    const double a0  = 1.0 + alp / A;
    honkC.b0 = float((1.0 + alp * A) / a0);
    honkC.b1 = float(-2.0 * c        / a0);
    honkC.b2 = float((1.0 - alp * A) / a0);
    honkC.a1 = float(-2.0 * c        / a0);
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

    const float envAtk = timeToCoeff(0.02f  * attackMult,  sampleRate);
    const float envRel = timeToCoeff(8.0f   * releaseMult, sampleRate);
    const float gAtk   = timeToCoeff(0.05f  * attackMult,  sampleRate);
    const float gRel   = timeToCoeff(55.0f  * releaseMult, sampleRate);

    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;
    float totalGR = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i] * inputTrimLin;
        const float inR = R ? R[i] * inputTrimLin : inL;

        float lL = std::abs(inL), lR = std::abs(inR);
        if (lL > envL) envL = envAtk * envL + (1.0f - envAtk) * lL;
        else           envL = envRel * envL + (1.0f - envRel) * lL;
        if (lR > envR) envR = envAtk * envR + (1.0f - envAtk) * lR;
        else           envR = envRel * envR + (1.0f - envRel) * lR;

        float detDB    = linearTodB(std::max(envL, envR) + 1e-7f);
        float targetGR = gainComputeDB(detDB, kThreshold, kRatioRecip, kKnee);

        if (targetGR < gainSmooth) gainSmooth = gAtk * gainSmooth + (1.0f - gAtk) * targetGR;
        else                       gainSmooth = gRel * gainSmooth + (1.0f - gRel) * targetGR;

        float gain = dBToLinear(gainSmooth);
        float outL = inL * gain, outR = inR * gain;

        auto applyHonk = [&](float x, BQState& st) -> float {
            float y = honkC.b0*x + honkC.b1*st.x1 + honkC.b2*st.x2
                                  - honkC.a1*st.y1 - honkC.a2*st.y2;
            st.x2=st.x1; st.x1=x; st.y2=st.y1; st.y1=y;
            return y;
        };

        outL = applyHonk(outL, honkL);
        if (R) outR = applyHonk(outR, honkR);

        L[i] = softSat(outL * 1.06f, 1.2f) * 0.94f;
        if (R) R[i] = softSat(outR * 1.06f, 1.2f) * 0.94f;
        totalGR += gainSmooth;
    }

    currentGR_dB = -(totalGR / float(n));
}

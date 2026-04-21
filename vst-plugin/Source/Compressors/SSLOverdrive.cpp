#include "SSLOverdrive.h"

SSLOverdrive::BiquadCoeffs SSLOverdrive::makeHPF(double fc, double sr)
{
    const double w0  = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double c   = std::cos(w0);
    const double s   = std::sin(w0);
    const double q   = 0.707;
    const double alp = s / (2.0 * q);
    const double a0  = 1.0 + alp;
    BiquadCoeffs co;
    co.b0 = float((1.0 + c) * 0.5 / a0);
    co.b1 = float(-(1.0 + c) / a0);
    co.b2 = float((1.0 + c) * 0.5 / a0);
    co.a1 = float(-2.0 * c / a0);
    co.a2 = float((1.0 - alp) / a0);
    return co;
}

SSLOverdrive::BiquadCoeffs SSLOverdrive::makeLPF(double fc, double sr)
{
    const double w0  = 2.0 * juce::MathConstants<double>::pi * fc / sr;
    const double c   = std::cos(w0);
    const double s   = std::sin(w0);
    const double q   = 0.707;
    const double alp = s / (2.0 * q);
    const double a0  = 1.0 + alp;
    BiquadCoeffs co;
    co.b0 = float((1.0 - c) * 0.5 / a0);
    co.b1 = float((1.0 - c) / a0);
    co.b2 = float((1.0 - c) * 0.5 / a0);
    co.a1 = float(-2.0 * c / a0);
    co.a2 = float((1.0 - alp) / a0);
    return co;
}

float SSLOverdrive::biquad(float x, BiquadState& s, const BiquadCoeffs& c)
{
    float y = c.b0 * x + c.b1 * s.x1 + c.b2 * s.x2
                       - c.a1 * s.y1 - c.a2 * s.y2;
    s.x2 = s.x1; s.x1 = x;
    s.y2 = s.y1; s.y1 = y;
    return y;
}

void SSLOverdrive::prepare(double sr, int blockSize)
{
    CompressorBase::prepare(sr, blockSize);
    hpfC = makeHPF(80.0,    sr);
    lpfC = makeLPF(12000.0, sr);
    reset();
}

void SSLOverdrive::reset()
{
    hpfL = hpfR = lpfL = lpfR = {};
}

void SSLOverdrive::process(juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    auto* L = buffer.getWritePointer(0);
    auto* R = numCh > 1 ? buffer.getWritePointer(1) : nullptr;

    float totalSat = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float inL = L[i] * inputTrimLin;
        float inR = R ? R[i] * inputTrimLin : inL;

        // Band-limit signal (mic-pre EQ shape)
        float fL = biquad(biquad(inL, hpfL, hpfC), lpfL, lpfC);
        float fR = biquad(biquad(inR, hpfR, hpfC), lpfR, lpfC);

        // Soft saturation — emulates transformer + gain stage clipping
        L[i] = softSat(fL * drive, 1.5f) / std::tanh(1.5f);
        if (R) R[i] = softSat(fR * drive, 1.5f) / std::tanh(1.5f);

        totalSat += std::abs(fL * drive) - std::abs(fL);
    }

    // Display "GR" as average clamp amount for meter
    currentGR_dB = juce::jlimit(0.0f, 12.0f, totalSat / float(n) * 20.0f);
}

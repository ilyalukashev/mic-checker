#pragma once
#include <JuceHeader.h>
#include <cmath>

class CompressorBase
{
public:
    virtual ~CompressorBase() = default;

    virtual void prepare(double sr, int /*blockSize*/) { sampleRate = sr; }
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    virtual juce::String getName()   const = 0;
    virtual juce::Colour getColour() const = 0;

    // User trim (additive on top of calibration offset)
    void setUserTrim(float dB)        { userTrimDB = dB;  updateInputGain(); }
    // Called by auto-calibration; not exposed to user
    void setCalibTrim(float dB)       { calibTrimDB = dB; updateInputGain(); }

    void setEnabled(bool e)           { enabled = e; }
    bool isEnabled()       const      { return enabled; }

    void  setBlend(float b)           { blend = juce::jlimit(0.0f, 1.0f, b); }
    float getBlend()       const      { return blend; }

    void  setAttackMult(float m)      { attackMult  = std::max(0.01f, m); }
    void  setReleaseMult(float m)     { releaseMult = std::max(0.01f, m); }

    float getCurrentGR()   const      { return currentGR_dB; }
    float getCalibTrim()   const      { return calibTrimDB; }

    static float dBToLinear(float dB) { return std::pow(10.0f, dB * 0.05f); }
    static float linearTodB(float l)  { return l > 1e-7f ? 20.0f * std::log10(l) : -140.0f; }

protected:
    double sampleRate   = 44100.0;
    float  blend        = 1.0f;
    bool   enabled      = true;
    float  inputTrimLin = 1.0f;
    float  currentGR_dB = 0.0f;
    float  attackMult   = 1.0f;
    float  releaseMult  = 1.0f;

    static float timeToCoeff(float ms, double sr)
    {
        if (ms <= 0.0f) return 0.0f;
        return std::exp(-1.0f / float(ms * 0.001 * sr));
    }

    // Returns gain change in dB (always <= 0)
    static float gainComputeDB(float inputDB, float threshDB,
                                float ratioRecip, float kneeDB)
    {
        float diff = inputDB - threshDB;
        if (kneeDB > 0.0f)
        {
            float hk = kneeDB * 0.5f;
            if (diff <= -hk) return 0.0f;
            if (diff <   hk)
            {
                float t = (diff + hk) / kneeDB;
                return (ratioRecip - 1.0f) * t * t * kneeDB * 0.5f;
            }
        }
        else if (diff <= 0.0f) return 0.0f;

        return diff * (ratioRecip - 1.0f);
    }

    static float softSat(float x, float drive = 1.0f)
    {
        return std::tanh(x * drive) / std::tanh(drive + 1e-6f);
    }

private:
    float userTrimDB  = 0.0f;
    float calibTrimDB = 0.0f;

    void updateInputGain() { inputTrimLin = dBToLinear(userTrimDB + calibTrimDB); }
};

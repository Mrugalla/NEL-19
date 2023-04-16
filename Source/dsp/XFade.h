#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

namespace dsp
{
    template<typename Float>
    inline Float msInInc(Float ms, Float Fs) noexcept
    {
        return static_cast<Float>(1) / msInSamples(ms, Fs);
    }
    
    struct XFade
    {
        using AudioBuffer = juce::AudioBuffer<float>;
		static constexpr float Pi = 3.141592653589f;
        
        XFade() :
            buffer(),
            phase(0.),
            inc(0.),
            idx(0),
            fading(false)
        {}

        void prepare(double sampleRate, double lengthMs, int blockSize)
        {
            buffer.setSize(3, blockSize, false, true, false);
            inc = msInInc(lengthMs, sampleRate);
        }

        void init() noexcept
        {
            idx = 1 - idx;
            phase = 0.;
            fading = true;
        }

        float* const* getSamples() noexcept
        {
            return buffer.getArrayOfWritePointers();
        }

        void synthesizePhase(int numSamples) noexcept
        {
            auto xSamples = getSamples();
            auto xBuf = xSamples[2];

            for (auto s = 0; s < numSamples; ++s)
            {
                xBuf[s] = static_cast<float>(phase);
                phase += inc;
                if (phase > 1.)
                {
                    fading = false;
                    phase = 1.;
                    for (; s < numSamples; ++s)
                        xBuf[s] = 1.f;
                    return;
                }
            }
        }

        void operator()(float* const* samples, int numChannels, int numSamples) noexcept
        {
            synthesizePhase(numSamples);

            auto xSamples = getSamples();
            auto xBuf = xSamples[2];

            for (auto ch = 0; ch < numChannels; ++ch)
            {
                auto smpls = samples[ch];
                const auto xSmpls = xSamples[ch];

                for (auto s = 0; s < numSamples; ++s)
                {
                    const auto smpl = smpls[s];
                    const auto xSmpl = xSmpls[s];

                    const auto xFade = xBuf[s];
                    const auto xPi = xFade * Pi;
                    const auto frac = std::cos(xPi + Pi) + 1.f;
                    const auto xFrac = std::cos(xPi) + 1.f;

                    smpls[s] = (smpl * frac + xSmpl * xFrac) * .5f;
                }
            }
        }

        AudioBuffer buffer;
        double phase, inc;
        int idx;
        bool fading;
    };

}
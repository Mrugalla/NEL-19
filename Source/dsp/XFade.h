#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace dsp
{
    using SIMD = juce::FloatVectorOperations;

    template<typename Float>
    inline Float msInSamples(Float ms, Float Fs) noexcept
    {
        return ms * Fs * static_cast<Float>(0.001);
    }

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

    template<size_t NumTracks>
    struct XFadeMixer
    {
        using AudioBuffer = juce::AudioBuffer<float>;
        static constexpr float Pi = 3.141592653589f;

        struct Track
        {
            Track() :
                gain(0.),
                dest(0.)
            {}

            const bool isFading() const noexcept
            {
				return dest != gain;
            }

            void synthesizeGainValues(float* xBuf, 
                double inc, int numSamples) noexcept
            {
                if (!isFading())
                    return SIMD::fill(xBuf, dest, numSamples);

				if(dest == 1.)
                    for (auto s = 0; s < numSamples; ++s)
                    {
                        xBuf[s] = static_cast<float>(gain);
                        gain += inc;
                        if (gain >= 1.)
                        {
                            gain = 1.;
                            for (; s < numSamples; ++s)
                                xBuf[s] = 1.f;
                            return;
                        }
                    }
                else
                    for (auto s = 0; s < numSamples; ++s)
                    {
                        xBuf[s] = static_cast<float>(gain);
                        gain -= inc;
                        if (gain <= 0.)
                        {
                            gain = 0.;
                            for (; s < numSamples; ++s)
                                xBuf[s] = 0.f;
                            return;
                        }
                    }
            }
            
            void operator()(float* const* samples, float* const* xSamples,
                int numChannels, int numSamples) noexcept
            {
                
            }
            
            void copy(float* const* dest, const float* const* src,
                int numChannels, int numSamples) const noexcept
            {
                const auto xBuf = src[2];
                for (auto ch = 0; ch < numChannels; ++ch)
                    for(auto s = 0; s < numSamples; ++s)
						dest[ch][s] = src[ch][s] * xBuf[s];
            }

            void add(float* const* dest, const float* const* src,
                int numChannels, int numSamples) const noexcept
            {
				const auto xBuf = src[2];
				for (auto ch = 0; ch < numChannels; ++ch)
					SIMD::addWithMultiply(dest[ch], src[ch], xBuf, numSamples);
            }

            double gain, dest;
        };

        XFadeMixer() :
            buffer(),
            tracks(),
            inc(0.),
            idx(0)
        {
			tracks[idx].dest = tracks[idx].gain = 1.;
        }

        void prepare(double sampleRate, double lengthMs, int blockSize)
        {
            buffer.setSize(3 * NumTracks, blockSize, false, true, false);
            inc = msInInc(lengthMs, sampleRate);
        }

        void init() noexcept
        {
            idx = (idx + 1) % NumTracks;
            for (auto& track : tracks)
                track.dest = 0.;
            tracks[idx].dest = 1.;
        }

        float* const* getSamples(int i) noexcept
        {
            return &buffer.getArrayOfWritePointers()[i * 3];
        }

        const float* const* getSamples(int i) const noexcept
        {
            return &buffer.getArrayOfReadPointers()[i * 3];
        }

        void synthesizeGainValues(int i, int numSamples) noexcept
        {
            auto xSamples = getSamples(i);
            auto xBuf = xSamples[2];
            auto& track = tracks[i];
            
            track.synthesizeGainValues(xBuf, inc, numSamples);
        }
        
        const bool isEnabled(int i) const noexcept
        {
			return tracks[i].gain + tracks[i].dest != 0.;
		}

        const bool isFading(int i) const noexcept
        {
			return tracks[i].gain != tracks[i].dest;
        }
        
        void copy(float* const* dest, int i, int numChannels, int numSamples) const noexcept
        {
			tracks[i].copy(dest, getSamples(i), numChannels, numSamples);
        }

        void add(float* const* dest, int i, int numChannels, int numSamples) const noexcept
        {
			tracks[i].add(dest, getSamples(i), numChannels, numSamples);
        }

        void copyAndAdd(float* const* dest, int numChannels, int numSamples) noexcept
        {
            copy(dest, 0, numChannels, numSamples);
			for (auto i = 1; i < NumTracks; ++i)
				add(dest, i, numChannels, numSamples);
        }

        const int numTracksEnabled() const noexcept
        {
            auto sum = 0;
			for (auto i = 0; i < NumTracks; ++i)
				sum += isEnabled(i) ? 1 : 0;
			return sum;
        }

    protected:
		AudioBuffer buffer;
        std::array<Track, NumTracks> tracks;
        double inc;
    public:
        int idx;
    };

}
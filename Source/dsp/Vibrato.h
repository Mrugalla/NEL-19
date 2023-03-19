#pragma once
#include "WHead.h"
#include <JuceHeader.h>
#include <limits>

namespace vibrato
{
	using AudioBuffer = juce::AudioBuffer<float>;
	using WHead = dsp::WHead;

	inline float mix(float a, float b, float m) noexcept
	{
		return std::sqrt(1.f - m) * a + std::sqrt(m) * b;
	}

	enum class InterpolationType
	{
		Lerp, Spline,
		NumInterpolationTypes
	};
	
	inline juce::String toString(InterpolationType t)
	{
		switch (t)
		{
		case InterpolationType::Lerp: return "lerp";
		case InterpolationType::Spline: return "spline";
		//case InterpolationType::LagRange: return "lagrange";
		//case InterpolationType::Sinc: return "sinc";
		default: return "";
		}
	}
	
	inline InterpolationType toType(const juce::String& t)
	{
		const auto numTypes = static_cast<int>(InterpolationType::NumInterpolationTypes);
		for (auto i = 0; i < numTypes; ++i)
		{
			const auto type = static_cast<InterpolationType>(i);
			if (t == toString(type))
				return type;
		}
		return InterpolationType::NumInterpolationTypes;
	}

	struct Delay
	{
		Delay() :
			ringBuffer(),
			delaySize(0.f), delayMid(0.f), delayMax(0.f),
			delaySizeInt(0)
		{
		}

		void prepare(size_t s)
		{
			delaySizeInt = s;
			ringBuffer.setSize(2, delaySizeInt, false, true, false);
			delaySize = static_cast<float>(s);
			delayMid = s * .5f;
			delayMax = delaySize - 1.f;
		}

		void operator()(float* const* samples, int numChannels, int numSamples,
			float* const* vibBuf, const int* wHead, InterpolationType interpolationType) noexcept
		{
			synthesizeReadHead(numChannels, numSamples, vibBuf, wHead);
			
			switch (interpolationType)
			{
			case InterpolationType::Lerp: return processBlockDelayLERP(samples, numChannels, numSamples, vibBuf, wHead);
			case InterpolationType::Spline: return processBlockDelaySPLINE(samples, numChannels, numSamples, vibBuf, wHead);
			}
		}

	private:
		AudioBuffer ringBuffer;
		float delaySize, delayMid, delayMax;
		int delaySizeInt;

		void synthesizeReadHead(int numChannels, int numSamples, float* const* vibBuf, const int* wHead) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto buf = vibBuf[ch];
				// map buffer [-1, 1] to [0, delaySize]
				juce::FloatVectorOperations::multiply(buf, delayMid, numSamples);
				juce::FloatVectorOperations::add(buf, delayMid, numSamples);
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto dly = buf[s];
					auto rh = static_cast<float>(wHead[s]) - dly;
					if (rh < 0.f)
						rh += delaySize;
					if (rh >= delaySize)
						rh -= delaySize;

					buf[s] = rh;
				}
			}
		}
		
		void processBlockDelayLERP(float* const* samples, int numChannels,
			int numSamples, float* const* readHead, const int* wHead) noexcept
		{
			auto ringBuf = ringBuffer.getArrayOfWritePointers();

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto ring = ringBuf[ch];
				const auto rHead = readHead[ch];
				auto smpls = samples[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = rHead[s];

					ring[w] = smpls[s];
					smpls[s] = interpolation::lerp(ring, r, delaySizeInt);
				}
			}
		}
		
		void processBlockDelaySPLINE(float* const* samples, int numChannels,
			int numSamples, float* const* readHead, const int* wHead) noexcept
		{
			auto ringBuf = ringBuffer.getArrayOfWritePointers();

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto ring = ringBuf[ch];
				const auto rHead = readHead[ch];
				auto smpls = samples[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = rHead[s];

					ring[w] = smpls[s];
					smpls[s] = interpolation::cubicHermiteSpline(ring, r, delaySizeInt);
				}
			}
		}
	};

	struct Processor
	{
		Processor() :
			wHead(),
			delay(),
			size(0),
			interpolationType(InterpolationType::Lerp)
		{
		}
		
		void prepare(const int blockSize, int _delaySize)
		{
			size = _delaySize;
			wHead.prepare(blockSize, size);
			delay.prepare(size);
		}

		// PROCESS
		void operator()(float* const* samples, int numChannels, int numSamples, float* const* vibBuf) noexcept
		{
			wHead(numSamples);
			delay
			(
				samples, numChannels, numSamples,
				vibBuf,
				wHead.data(),
				interpolationType.load()
			);
		}
		
		float getSizeInMs(float Fs) const noexcept
		{
			return 1000.f * static_cast<float>(size) / Fs;
		}
		
		int getLatency() const noexcept
		{
			return static_cast<int>(size) / 2;
		}
		
	protected:
		WHead wHead;
		Delay delay;
		size_t size;
	public:
		std::atomic<InterpolationType> interpolationType;
		
		const size_t ringBufferSize() const noexcept { return size; }
	};
}

/*

lagrange interpolator
	worse in lowend
	better in highend
	>> brittle noise in highend when used on mixed signal (inter-modulation?)

feature ideas:
	allpass instead of delay (or in feedback loop, considering fb delay)

*/
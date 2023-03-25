#pragma once
#include <array>
#include <limits>
#include "WHead.h"
#include "PRM.h"
#include <JuceHeader.h>

namespace vibrato
{
	using AudioBuffer = juce::AudioBuffer<float>;
	using WHead = dsp::WHead;
	using PRM = dsp::PRM;
	/* buffer, x, size */
	using InterpolationFunc = float(*)(const float*, float, int) noexcept;

	inline float fastTanh(float x) noexcept
	{
		const auto xx = x * x;
		auto a = x * (27.f + xx);
		auto b = 27.f + 9.f * xx;
		return a / b;
	}

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

	inline float lerp(const float* buffer, float x, int size) noexcept
	{
		return interpolation::lerp(buffer, x, size);
	}

	inline float cubic(const float* buffer, float x, int size) noexcept
	{
		return interpolation::cubicHermiteSpline(buffer, x, size);
	}

	struct SamplePair
	{
		float sIn, sOut;
	};

	inline SamplePair getDelayPair(float* smpls, const float* ring, const InterpolationFunc& interpolate,
		float r, float feedback, int size, int s) noexcept
	{
		const auto sOut = interpolate(ring, r, size);
		const auto sFb = fastTanh(feedback * sOut);
		const auto sIn = smpls[s] + sFb;
		return { sIn, sOut };
	}
	
	/* this method is likely not correct yet lol */
	inline SamplePair getAllpassPair(float* smpls, const float* ring, const InterpolationFunc& interpolate,
		float r, float feedback, int size, int s) noexcept
	{
		const auto sOut = interpolate(ring, r, size);
		const auto sFb = fastTanh(feedback * sOut);
		const auto sIn = smpls[s] + sFb;
		return { sOut, sIn };
	}

	struct Delay
	{
		Delay() :
			interpolationFuncs{ &lerp, &cubic },
			ringBuffer(),
			delaySize(0.f), delayMid(0.f), delayMax(0.f),
			delaySizeInt(0)
		{
		}

		void prepare(int s)
		{
			delaySizeInt = s;
			ringBuffer.setSize(2, delaySizeInt, false, true, false);
			delaySize = static_cast<float>(s);
			delayMid = s * .5f;
			delayMax = delaySize - 1.f;
		}

		void operator()(float* const* samples, int numChannels, int numSamples,
			float* const* vibBuf, const int* wHead, const float* fbBuf,
			InterpolationType interpolationType) noexcept
		{
			synthesizeReadHead(numChannels, numSamples, vibBuf, wHead);
			
			auto ringBuf = ringBuffer.getArrayOfWritePointers();
			const auto& interpolate = interpolationFuncs[static_cast<int>(interpolationType)];

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto ring = ringBuf[ch];
				const auto rHead = vibBuf[ch];
				auto smpls = samples[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = rHead[s];
					const auto fb = fbBuf[s];

					const auto pair = getDelayPair(smpls, ring, interpolate, r, fb, delaySizeInt, s);

					ring[w] = pair.sIn;
					smpls[s] = pair.sOut;
				}
			}
		}

	private:
		std::array<InterpolationFunc, 2> interpolationFuncs;
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
	};

	struct Processor
	{
		Processor() :
			feedbackPRM(0.f),
			wHead(),
			delay(),
			size(0)
		{
		}
		
		void prepare(float Fs, int blockSize, int _delaySize)
		{
			size = _delaySize;
			wHead.prepare(blockSize, size);
			delay.prepare(size);
			feedbackPRM.prepare(Fs, blockSize, 8.f);
		}

		/* samples, numChannels, numSamples, vibBuf, feedback[-1,1] */
		void operator()(float* const* samples, int numChannels, int numSamples,
			float* const* vibBuf, float feedback, InterpolationType interpolationType) noexcept
		{
			auto fbBuf = feedbackPRM(feedback, numSamples);
			if(!feedbackPRM.smoothing)
				juce::FloatVectorOperations::fill(fbBuf, feedback, numSamples);

			wHead(numSamples);
			delay
			(
				samples, numChannels, numSamples,
				vibBuf,
				wHead.data(),
				fbBuf,
				interpolationType
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
		PRM feedbackPRM;
		WHead wHead;
		Delay delay;
		int size;
		
		const size_t ringBufferSize() const noexcept
		{
			return size;
		}
	};
}

/*

feature ideas:
	allpass instead of delay (or in feedback loop, considering fb delay)

*/
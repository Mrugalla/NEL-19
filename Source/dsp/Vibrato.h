#pragma once
#include <array>
#include <limits>
#include "WHead.h"
#include "PRM.h"
#include <JuceHeader.h>

namespace vibrato
{
	static constexpr float Pi = 3.14159265358979323846f;
	static constexpr float PiHalf = Pi / 2.f;

	using AudioBuffer = juce::AudioBuffer<float>;
	using WHead = dsp::WHead;
	using PRMInfo = dsp::PRMInfo;
	using PRM = dsp::PRM;
	using LP = smooth::Lowpass<float>;

	/* buffer, x, size */
	using InterpolationFunc = float(*)(const float*, float, int) noexcept;
	using FilterUpdateFunc = void(*)(LP&, float dampFc) noexcept;

	inline float fastTanh2(float x) noexcept
	{
		return x / (1.f + std::abs(x));
	}

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

	inline void noUpdate(LP&, float) noexcept {} // yes, this is important

	inline void updateFilter(LP& lp, float dampFc) noexcept
	{
		lp.makeFromDecayInFc(dampFc);
	}

	inline float waveshape(float x) noexcept
	{
		return -.405548f * x * x * x + 1.34908f * x;
	}

	struct SamplePair
	{
		float sIn, sOut;
	};

	inline SamplePair getDelayPair(float* smpls, const float* ring, const InterpolationFunc& interpolate,
		LP& lp, float r, float feedback, int size, int s) noexcept
	{
		const auto sOut = interpolate(ring, r, size);
		const auto sLP = lp(sOut);
		const auto sFb = waveshape(feedback * sLP);
		const auto sIn = smpls[s] + sFb;
		return { sIn, sOut };
	}
	
	inline SamplePair getAllpassPair(float*, const float*, const InterpolationFunc&,
		float, float, int, int) noexcept
	{
		return { 0.f, 0.f };
	}

	struct Delay
	{
		Delay() :
			interpolationFuncs{ &lerp, &cubic },
			filterUpdateFuncs{ &noUpdate, &updateFilter },
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
			float* const* vibBuf, const int* wHead, const float* fbBuf, const PRMInfo& dampFcInfo,
			InterpolationType interpolationType) noexcept
		{
			synthesizeReadHead(numChannels, numSamples, vibBuf, wHead);
			
			auto ringBuf = ringBuffer.getArrayOfWritePointers();
			const auto& interpolate = interpolationFuncs[static_cast<int>(interpolationType)];
			const auto& updateFilter = filterUpdateFuncs[dampFcInfo.smoothing ? 1 : 0];

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto ring = ringBuf[ch];
				const auto rHead = vibBuf[ch];
				auto smpls = samples[ch];
				auto& lp = lps[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					updateFilter(lp, dampFcInfo[s]);
					
					const auto w = wHead[s];
					const auto r = rHead[s];
					const auto fb = fbBuf[s];

					const auto pair = getDelayPair(smpls, ring, interpolate, lp, r, fb, delaySizeInt, s);

					ring[w] = pair.sIn;
					smpls[s] = pair.sOut;
				}
			}
		}

		void processNoDepth(float* const* samples, int numChannels, int numSamples,
			const int* wHead)
		{
			auto ringBuf = ringBuffer.getArrayOfWritePointers();

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto ring = ringBuf[ch];
				auto smpls = samples[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = w;

					ring[w] = smpls[s];
					smpls[s] = ring[r];
				}
			}
		}
		
		void processFF(float* const* samples, int numChannels, int numSamples,
			float* depthBuf, const int* wHead,
			InterpolationType interpolationType) noexcept
		{
			synthesizeReadHeadFF(numSamples, depthBuf, wHead);

			auto ringBuf = ringBuffer.getArrayOfWritePointers();
			const auto& interpolate = interpolationFuncs[static_cast<int>(interpolationType)];

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto ring = ringBuf[ch];
				auto smpls = samples[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = depthBuf[s];

					ring[w] = smpls[s];
					smpls[s] = interpolate(ring, r, delaySizeInt);
				}
			}
		}

	private:
		std::array<InterpolationFunc, 2> interpolationFuncs;
		std::array<FilterUpdateFunc, 2> filterUpdateFuncs;
		std::array<LP, 2> lps;
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
				synthesizeReadHead(buf, numSamples, wHead);
			}
		}

		void synthesizeReadHeadFF(int numSamples, float* depthBuf, const int* wHead) noexcept
		{
			// map from [0, 1] to [delaySizeHalf, 0]
			for (auto s = 0; s < numSamples; ++s)
				depthBuf[s] = 1.f - depthBuf[s];
			juce::FloatVectorOperations::multiply(depthBuf, delayMid, numSamples);
			synthesizeReadHead(depthBuf, numSamples, wHead);
		}

		void synthesizeReadHead(float* buf, int numSamples, const int* wHead) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto dly = buf[s];
				auto rh = static_cast<float>(wHead[s]) - dly;
				if (rh < 0.f)
					rh += delaySize;

				buf[s] = rh;
			}
		}
	};

	struct Processor
	{
		Processor() :
			feedbackPRM(0.f),
			dampPRM(1.f),
			wHead(),
			vibrato(),
			delayFF(),
			fsInv(1.f),
			size(0)
		{
		}
		
		void prepare(float Fs, int blockSize, int _delaySize)
		{
			size = _delaySize;
			wHead.prepare(blockSize, size);
			vibrato.prepare(size);
			delayFF.prepare(size);
			feedbackPRM.prepare(Fs, blockSize, 8.f);
			dampPRM.prepare(Fs, blockSize, 13.f);

			fsInv = 1.f / Fs;
		}

		/* samples, numChannels, numSamples, vibBuf, depthBuf[0,1], feedback[-1,1], dampHz[1, N], lookaheadEnabled */
		void operator()(float* const* samples, int numChannels, int numSamples,
			float* const* vibBuf, float* depthBuf, float feedback, float dampHz, InterpolationType interpolationType,
			bool lookaheadEnabled) noexcept
		{
			wHead(numSamples);
			
			const bool isVibrating = depthBuf[0] != 0.f;

			if (isVibrating)
			{
				const auto fbInfo = feedbackPRM(feedback, numSamples);
				if (!fbInfo.smoothing)
					juce::FloatVectorOperations::fill(fbInfo.buf, feedback, numSamples);

				const auto dampFc = dampHz * fsInv;
				const auto dampInfo = dampPRM(dampFc, numSamples);

				vibrato
				(
					samples, numChannels, numSamples,
					vibBuf,
					wHead.data(),
					fbInfo.buf, dampInfo,
					interpolationType
				);
			}
			else
				vibrato.processNoDepth
				(
					samples, numChannels, numSamples,
					wHead.data()
				);

			if(lookaheadEnabled)
				delayFF.processFF
				(
					samples, numChannels, numSamples,
					depthBuf,
					wHead.data(),
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
		PRM feedbackPRM, dampPRM;
		WHead wHead;
		Delay vibrato, delayFF;
		float fsInv;
		int size;
		
		const size_t ringBufferSize() const noexcept
		{
			return size;
		}
	};
}

/*

feature ideas:
	allpass instead of delay

*/
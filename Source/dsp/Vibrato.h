#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <limits>
#include "WHead.h"
#include "PRM.h"

namespace vibrato
{
	//static constexpr double Pi = 3.1415926535897932384626433832795;
	//static constexpr double PiHalf = Pi / 2.;

	using AudioBufferD = juce::AudioBuffer<double>;
	using WHead = dsp::WHead;
	using PRMInfo = dsp::PRMInfo<double>;
	using PRM = dsp::PRM<double>;
	using LP = smooth::Lowpass<double>;

	/* buffer, x, size */
	using InterpolationFunc = double(*)(const double*, double, int) noexcept;
	using FilterUpdateFunc = void(*)(LP&, double dampFc) noexcept;

	template<typename Float>
	inline Float fastTanh2(Float x) noexcept
	{
		return x / (static_cast<Float>(1) + std::abs(x));
	}

	template<typename Float>
	inline Float fastTanh(Float x) noexcept
	{
		const auto xx = x * x;
		auto a = x * (static_cast<Float>(27) + xx);
		auto b = static_cast<Float>(27) + static_cast<Float>(9) * xx;
		return a / b;
	}

	template<typename Float>
	inline Float mix(Float a, Float b, Float m) noexcept
	{
		return std::sqrt(static_cast<Float>(1) - m) * a + std::sqrt(m) * b;
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

	inline double lerp(const double* buffer, double x, int size) noexcept
	{
		return interpolation::lerp(buffer, x, size);
	}

	inline double cubic(const double* buffer, double x, int size) noexcept
	{
		return interpolation::cubicHermiteSpline(buffer, x, size);
	}

	inline void noUpdate(LP&, double) noexcept {} // yes, this is important

	inline void updateFilter(LP& lp, double dampFc) noexcept
	{
		lp.makeFromDecayInFc(dampFc);
	}

	inline double waveshape(double x) noexcept
	{
		return -.405548 * x * x * x + 1.34908 * x;
	}

	struct SamplePair
	{
		double sIn, sOut;
	};

	inline SamplePair getDelayPair(double* smpls, const double* ring, const InterpolationFunc& interpolate,
		LP& lp, double r, double feedback, int size, int s) noexcept
	{
		const auto sOut = interpolate(ring, r, size);
		const auto sLP = lp(sOut);
		const auto sFb = waveshape(feedback * sLP);
		const auto sIn = smpls[s] + sFb;
		return { sIn, sOut };
	}
	
	inline SamplePair getAllpassPair(double*, const double*, const InterpolationFunc&,
		double, double, int, int) noexcept
	{
		return { 0.f, 0.f };
	}

	struct Delay
	{
		Delay() :
			interpolationFuncs{ &lerp, &cubic },
			filterUpdateFuncs{ &noUpdate, &updateFilter },
			ringBuffer(),
			delaySize(0.), delayMid(0.), delayMax(0.),
			delaySizeInt(0)
		{
		}

		void prepare(int s)
		{
			delaySizeInt = s;
			ringBuffer.setSize(2, delaySizeInt, false, true, false);
			delaySize = static_cast<double>(delaySizeInt);
			delayMax = delaySize - 1.;
			delayMid = delaySize * .5;
		}

		void operator()(double* const* samples, int numChannels, int numSamples,
			double* const* vibBuf, const int* wHead, const double* fbBuf, const PRMInfo& dampFcInfo,
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

		void processNoDepth(double* const* samples, int numChannels, int numSamples,
			const int* wHead) noexcept
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
		
		void processFF(double* const* samples, int numChannels, int numSamples,
			double* depthBuf, const int* wHead,
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
		AudioBufferD ringBuffer;
		double delaySize, delayMid, delayMax;
		int delaySizeInt;

		void synthesizeReadHead(int numChannels, int numSamples, double* const* vibBuf, const int* wHead) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto buf = vibBuf[ch];
				// map buffer [-1, 1] to [0, 2]
				juce::FloatVectorOperations::add(buf, 1., numSamples);
				// map buffer [0, 2] to [0, delayMax]
				juce::FloatVectorOperations::multiply(buf, .5 * delayMax, numSamples);
					
				synthesizeReadHead(buf, numSamples, wHead);
			}
		}

		void synthesizeReadHeadFF(int numSamples, double* depthBuf, const int* wHead) noexcept
		{
			// map from [0, 1] to [1, 0]
			for (auto s = 0; s < numSamples; ++s)
				depthBuf[s] = 1. - depthBuf[s];
			// map from [1, 0] to [delayMid, 0]
			juce::FloatVectorOperations::multiply(depthBuf, delayMid, numSamples);
			synthesizeReadHead(depthBuf, numSamples, wHead);
		}

		void synthesizeReadHead(double* buf, int numSamples, const int* wHead) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto dly = buf[s];
				auto rh = static_cast<double>(wHead[s]) - dly;
				if (rh < 0.)
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
		
		void prepare(double Fs, int blockSize, int _delaySize)
		{
			size = _delaySize;
			wHead.prepare(blockSize, size);
			vibrato.prepare(size);
			delayFF.prepare(size);
			feedbackPRM.prepare(Fs, blockSize, 8.);
			dampPRM.prepare(Fs, blockSize, 13.);

			fsInv = 1. / Fs;
		}

		/* samples, numChannels, numSamples, vibBuf, depthBuf[0,1], feedback[-1,1], dampHz[1, N], lookaheadEnabled */
		void operator()(double* const* samples, int numChannels, int numSamples,
			double* const* vibBuf, double* depthBuf, double feedback, double dampHz, InterpolationType interpolationType,
			bool lookaheadEnabled) noexcept
		{
			wHead(numSamples);
			
			const bool isVibrating = depthBuf[0] != 0.;

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
		
		double getSizeInMs(double Fs) const noexcept
		{
			return 1000. * static_cast<double>(size) / Fs;
		}
		
		int getLatency() const noexcept
		{
			return static_cast<int>(size) / 2;
		}
		
	protected:
		PRM feedbackPRM, dampPRM;
		WHead wHead;
		Delay vibrato, delayFF;
		double fsInv;
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
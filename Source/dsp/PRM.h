#pragma once
#include "Smooth.h"
#include <vector>

namespace dsp
{
	template<typename Float>
	struct PRMInfo
	{
		PRMInfo(Float* _buf, Float _val, bool _smoothing) :
			buf(_buf),
			val(_val),
			smoothing(_smoothing)
		{}

		/* idx */
		Float operator[](int i) const noexcept
		{
			return buf[i];
		}

		Float* buf;
		Float val;
		bool smoothing;
	};

	template<typename Float>
	struct PRM
	{
		/* startVal */
		PRM(Float startVal) :
			smooth(startVal),
			buf(),
			value(startVal)
		{}

		/* Fs, blockSize, smoothLenMs */
		void prepare(Float Fs, int blockSize, Float smoothLenMs)
		{
			buf.resize(blockSize);
			smooth.makeFromDecayInMs(smoothLenMs, Fs);
		}

		/* value, numSamples */
		PRMInfo<Float> operator()(Float val, int numSamples) noexcept
		{
			value = val;
			bool smoothing = smooth(buf.data(), value, numSamples);
			return { buf.data(), value, smoothing};
		}

		/* numSamples */
		PRMInfo<Float> operator()(int numSamples) noexcept
		{
			bool smoothing = smooth(buf.data(), numSamples);
			return { buf.data(), value, smoothing };
		}

		/* idx */
		Float operator[](int i) const noexcept
		{
			return buf[i];
		}

	protected:
		smooth::Smooth<Float> smooth;
		std::vector<Float> buf;
		Float value;
	};

	using PRMInfoF = PRMInfo<float>;
	using PRMInfoD = PRMInfo<double>;

	using PRMF = PRM<float>;
	using PRMD = PRM<double>;
}
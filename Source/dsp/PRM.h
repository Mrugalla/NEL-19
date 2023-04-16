#pragma once
#include "Smooth.h"
#include <vector>

namespace dsp
{
	struct PRMInfo
	{
		PRMInfo(float* _buf, float _val, bool _smoothing) :
			buf(_buf),
			val(_val),
			smoothing(_smoothing)
		{}

		/* idx */
		float operator[](int i) const noexcept
		{
			return buf[i];
		}

		float* buf;
		float val;
		bool smoothing;
	};

	struct PRM
	{
		/* startVal */
		PRM(float startVal) :
			smooth(startVal),
			buf(),
			value(startVal)
		{}

		/* Fs, blockSize, smoothLenMs */
		void prepare(float Fs, int blockSize, float smoothLenMs)
		{
			buf.resize(blockSize);
			smooth.makeFromDecayInMs(smoothLenMs, Fs);
		}

		/* value, numSamples */
		PRMInfo operator()(float val, int numSamples) noexcept
		{
			value = val;
			bool smoothing = smooth(buf.data(), value, numSamples);
			return { buf.data(), value, smoothing};
		}

		/* numSamples */
		PRMInfo operator()(int numSamples) noexcept
		{
			bool smoothing = smooth(buf.data(), numSamples);
			return { buf.data(), value, smoothing };
		}

		/* idx */
		float operator[](int i) const noexcept
		{
			return buf[i];
		}

	protected:
		smooth::Smooth<float> smooth;
		std::vector<float> buf;
		float value;
	};
}
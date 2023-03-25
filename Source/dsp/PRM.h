#pragma once
#include "Smooth.h"
#include <vector>

namespace dsp
{
	struct PRM
	{
		/* startVal */
		PRM(float startVal) :
			smooth(startVal),
			buf(),
			smoothing(false)
		{}

		/* Fs, blockSize, smoothLenMs */
		void prepare(float Fs, int blockSize, float smoothLenMs)
		{
			buf.resize(blockSize);
			smooth.makeFromDecayInMs(smoothLenMs, Fs);
		}

		/* value, numSamples */
		float* operator()(float value, int numSamples) noexcept
		{
			smoothing = smooth(buf.data(), value, numSamples);
			return buf.data();
		}

		/* numSamples */
		float* operator()(int numSamples) noexcept
		{
			smoothing = smooth(buf.data(), numSamples);
			return buf.data();
		}

		/* idx */
		float operator[](int i) const noexcept
		{
			return buf[i];
		}

		smooth::Smooth<float> smooth;
		std::vector<float> buf;
		bool smoothing;
	};
}
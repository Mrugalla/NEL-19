#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "PRM.h"

namespace macro
{
	using PRM = dsp::PRMD;
	using PRMInfo = dsp::PRMInfoD;
	using SIMD = juce::FloatVectorOperations;
	using Smooth = smooth::Lowpass<double>;

	inline double dbToGain(double db, double minDb) noexcept
	{
		if (db <= minDb)
			return 0.;
		return std::pow(10., db / 20.);
	}

	struct Macro
	{
		Macro() :
			smoothies{ 0., 0. },
			sampleRate(1.),
			macro(0.),
			smoothingHz(20.),
			scGain(0.),
			scGainPRM(scGain)
		{}

		void prepare(double _sampleRate, int blockSize) noexcept
		{
			sampleRate = _sampleRate;

			scGainPRM.prepare(sampleRate, blockSize, 14.);
		}

		void setParameters(double _macro, double _smoothingHz, double _scGain) noexcept
		{
			macro = _macro;
			smoothingHz = _smoothingHz;
			scGain = _scGain;
		}

		void operator()(double* const* samples,
			const double* const* scSamples,
			int numChannels, int numSamples) noexcept
		{
			smoothies[0].makeFromDecayInHz(smoothingHz, sampleRate);
			if (numChannels == 2)
				smoothies[1].copyCutoffFrom(smoothies[0]);

			const auto scGainAmp = dbToGain(scGain, -120.);
			const auto scGainInfo = scGainPRM(scGainAmp, numSamples);

			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::fill(samples[ch], macro, numSamples);

			if (scGainInfo.smoothing)
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto smpls = samples[ch];
					auto scSmpls = scSamples[ch];

					SIMD::addWithMultiply(smpls, scSmpls, scGainInfo.buf, numSamples);
					limit(smpls, numSamples);
				}
			}
			else if (scGainAmp != 0.)
			{
				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto smpls = samples[ch];
					auto scSmpls = scSamples[ch];

					SIMD::addWithMultiply(smpls, scSmpls, scGainAmp, numSamples);
					limit(smpls, numSamples);
				}
			}
			
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				auto& smooth = smoothies[ch];
				
				smooth(smpls, numSamples);
			}
		}

	protected:
		std::array<Smooth, 2> smoothies;
		double sampleRate, macro, smoothingHz, scGain;
		PRM scGainPRM;

		void limit(double* smpls, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				if (smpls[s] > 1.)
					smpls[s] = 1.;
				else if (smpls[s] < -1.)
					smpls[s] = -1.;
		}
	};
}
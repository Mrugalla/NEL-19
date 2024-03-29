#pragma once
#include "WHead.h"
#include "../modsys/ModSys.h"
#include "Smooth.h"

namespace drywet
{
	using AudioBufferD = juce::AudioBuffer<double>;

	struct FFDelay
	{
		FFDelay() :
			wHead(),
			ringBuffer(),
			rHead(0)
		{}
		
		void prepare(int blockSize, int size)
		{
			wHead.prepare(blockSize, size);
			ringBuffer.setSize(2, size, false, true, false);
			rHead.resize(blockSize);
		}
		
		void operator()(double* const* samplesDry, int numChannels, int numSamples) noexcept
		{
			synthesizeHeads(numSamples);
			auto ringBuf = ringBuffer.getArrayOfWritePointers();

			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto dry = samplesDry[ch];
				auto ring = ringBuf[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = rHead[s];

					ring[w] = dry[s];
					dry[s] = ring[r];
				}
			}
		}
		
		void operator()(double* const* samplesDest, const double* const* samplesSrc,
			int numChannels, int numSamples) noexcept
		{
			synthesizeHeads(numSamples);
			auto ringBuf = ringBuffer.getArrayOfWritePointers();
			
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				const auto src = samplesSrc[ch];
				auto dest = samplesDest[ch];
				auto ring = ringBuf[ch];

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					const auto r = rHead[s];

					ring[w] = src[s];
					dest[s] = ring[r];
				}
			}
		}
	
	protected:
		dsp::WHead wHead;
		AudioBufferD ringBuffer;
		std::vector<int> rHead;

		void synthesizeHeads(int numSamples) noexcept
		{
			wHead(numSamples);
			for (auto s = 0; s < numSamples; ++s)
			{
				rHead[s] = wHead[s] + 1;
				if (rHead[s] >= wHead.delaySize)
					rHead[s] -= wHead.delaySize;
			}
		}
	};

	struct Processor
	{
		enum
		{
			kL,
			kR,
			kMix,
			kMixDry,
			kMixWet,
			kGainWet,
			kNumChannels
		};

		Processor() :
			mixSmooth(0.f),
			delay(),
			buffers(),
			gainWet(420.f), gainWetVal(1.f),
			gainWetSmooth(0.f)
		{
		}
		
		void prepare(double sampleRate, int blockSize, int latency)
		{
			mixSmooth.makeFromDecayInMs(10., sampleRate);
			gainWetSmooth.makeFromDecayInMs(4., sampleRate);
			buffers.setSize(kNumChannels, blockSize, false, true, false);
			delay.prepare(blockSize, latency);
		}
		
		void saveDry(const double* const* samples, double mixVal, int numChannels, int numSamples,
			bool lookaheadEnabled) noexcept
		{
			auto bufs = buffers.getArrayOfWritePointers();

			{ // SMOOTHEN MIX PARAMETER VALUE
				auto mixSmoothing = mixSmooth(bufs[kMix], mixVal, numSamples);
				if(!mixSmoothing)
					juce::FloatVectorOperations::fill(bufs[kMix], mixVal, numSamples);
			}
			{ // MAKING EQUAL LOUDNESS CURVES
				for (auto s = 0; s < numSamples; ++s)
					bufs[kMixDry][s] = std::sqrt(1. - bufs[kMix][s]);
				for (auto s = 0; s < numSamples; ++s)
					bufs[kMixWet][s] = std::sqrt(bufs[kMix][s]);
			}
			
			if(lookaheadEnabled)
				delay(bufs, samples, numChannels, numSamples);
			else
			{
				for (auto ch = 0; ch < numChannels; ++ch)
					juce::FloatVectorOperations::copy(bufs[kL + ch], samples[ch], numSamples);
			}
		}
		
		void processWet(double* const* samples, double _gainWet, int numChannels, int numSamples) noexcept
		{
			auto bufs = buffers.getArrayOfWritePointers();

			if (gainWet != _gainWet)
			{
				gainWet = _gainWet;
				gainWetVal = juce::Decibels::decibelsToGain(gainWet, -120.);
			}
			{
				auto gainWetSmoothing = gainWetSmooth(bufs[kGainWet], gainWetVal, numSamples);
				if (!gainWetSmoothing)
					juce::FloatVectorOperations::fill(bufs[kGainWet], gainWetVal, numSamples);
			}
			
			for (auto ch = 0; ch < numChannels; ++ch)
			{
				auto smpls = samples[ch];
				const auto dry = bufs[kL + ch];

				for (auto s = 0; s < numSamples; ++s)
					smpls[s] = dry[s] * bufs[kMixDry][s] + smpls[s] * bufs[kMixWet][s] * bufs[kGainWet][s];
			}
		}
	
		void processBypass(double* const* samples, int numChannels, int numSamples,
			bool lookaheadEnabled) noexcept
		{
			if (lookaheadEnabled)
				delay(samples, numChannels, numSamples);
			else
			{
				auto bufs = buffers.getArrayOfWritePointers();
				for (auto ch = 0; ch < numChannels; ++ch)
					juce::FloatVectorOperations::copy(bufs[kL + ch], samples[ch], numSamples);
			}
		}

	protected:
		smooth::Smooth<double> mixSmooth;
		FFDelay delay;
		AudioBufferD buffers;
		double gainWet, gainWetVal;
		smooth::Smooth<double> gainWetSmooth;
	};
}

/*

*/
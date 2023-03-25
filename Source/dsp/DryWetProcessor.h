#pragma once
#include "WHead.h"
#include "../modsys/ModSys.h"
#include "Smooth.h"

namespace drywet
{
	using AudioBuffer = juce::AudioBuffer<float>;

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
		
		void operator()(float* const* samplesDry, int numChannels, int numSamples) noexcept
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
		
		void operator()(float* const* samplesDest, const float* const* samplesSrc, int numChannels, int numSamples) noexcept
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
		AudioBuffer ringBuffer;
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
		
		void prepare(float sampleRate, int blockSize, int latency)
		{
			mixSmooth.makeFromDecayInMs(10.f, sampleRate);
			gainWetSmooth.makeFromDecayInMs(4.f, sampleRate);
			buffers.setSize(kNumChannels, blockSize, false, true, false);
			delay.prepare(blockSize, latency);
		}
		
		void processBypass(float* const* samples, int numChannels, int numSamples) noexcept
		{
			delay(samples, numChannels, numSamples);
		}
		
		void saveDry(const float* const* samples, float mixVal, int numChannels, int numSamples) noexcept
		{
			auto bufs = buffers.getArrayOfWritePointers();

			{ // SMOOTHEN MIX PARAMETER VALUE
				auto mixSmoothing = mixSmooth(bufs[kMix], mixVal, numSamples);
				if(!mixSmoothing)
					juce::FloatVectorOperations::fill(bufs[kMix], mixVal, numSamples);
			}
			{ // MAKING EQUAL LOUDNESS CURVES
				for (auto s = 0; s < numSamples; ++s)
					bufs[kMixDry][s] = std::sqrt(1.f - bufs[kMix][s]);
				for (auto s = 0; s < numSamples; ++s)
					bufs[kMixWet][s] = std::sqrt(bufs[kMix][s]);
			}
			
			delay(bufs, samples, numChannels, numSamples);
		}
		
		void processWet(float* const* samples, float _gainWet, int numChannels, int numSamples) noexcept
		{
			auto bufs = buffers.getArrayOfWritePointers();

			if (gainWet != _gainWet)
			{
				gainWet = _gainWet;
				gainWetVal = juce::Decibels::decibelsToGain(gainWet);
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
	
	protected:
		smooth::Smooth<float> mixSmooth;
		FFDelay delay;
		AudioBuffer buffers;
		float gainWet, gainWetVal;
		smooth::Smooth<float> gainWetSmooth;
	};
}

/*

*/
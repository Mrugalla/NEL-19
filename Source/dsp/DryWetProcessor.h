#pragma once
#include "../modsys/ModSys.h"

namespace drywet
{
	inline juce::String getLookaheadID() { return "lookaheadEnabled"; }

	struct FFDelay
	{
		FFDelay() :
			ringBuffer(),
			wHead(0), rHead(0)
		{}
		void resize(const int size)
		{
			ringBuffer.resize(size, 0.f);
			wHead = 0;
			rHead = 1;
		}
		void processBlock(float* dry, const int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				ringBuffer[wHead] = dry[s];
				dry[s] = ringBuffer[rHead];
				++wHead;
				if (wHead == ringBuffer.size())
					wHead = 0;
				rHead = wHead + 1;
				if (rHead == ringBuffer.size())
					rHead = 0;
			}
		}
		void processBlock(float* dest, const float* src, const int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				ringBuffer[wHead] = src[s];
				dest[s] = ringBuffer[rHead];
				++wHead;
				if (wHead == ringBuffer.size())
					wHead = 0;
				rHead = wHead + 1;
				if (rHead == ringBuffer.size())
					rHead = 0;
			}
		}
	protected:
		std::vector<float> ringBuffer;
		size_t wHead, rHead;
	};

	struct Processor
	{
		using Buffer = std::array<std::vector<float>, 3>;

		Processor(int _numChannels) :
			mixSmooth(),
			dryBuffer(), paramBuffer(),
			numChannels(_numChannels),
			
			gainWet(420.f), gainWetVal(1.f),
			gainWetSmooth(),

			lookaheadEnabled(true),
			lookaheadState(true)
		{
		}
		void setLookaheadEnabled(bool e) noexcept
		{
			lookaheadEnabled.store(e);
		}
		void prepare(float sampleRate, int maxBufferSize, int latency)
		{
			modSys6::Smooth::makeFromDecayInMs(mixSmooth, 20.f, sampleRate);
			modSys6::Smooth::makeFromDecayInMs(gainWetSmooth, 12.f, sampleRate);
			for (auto& b : dryBuffer)
				b.resize(maxBufferSize, 0.f);
			for (auto& b : paramBuffer)
				b.resize(maxBufferSize, 0.f);
			for (auto ch = 0; ch < numChannels; ++ch)
				dryDelay[ch].resize(latency);
		}
		bool processBypass(float** samples, int numChannelsIn, int numChannelsOut, int numSamples) noexcept
		{
			{ // CHECK IF LOOKAHEAD STATE CHANGED
				const auto e = lookaheadEnabled.load();
				if (lookaheadState != e)
				{
					lookaheadState = e;
					return false;
				}
			}
			if (lookaheadState)
			{
				{
					auto& dly = dryDelay[0];
					auto dry = dryBuffer[0].data();
					auto smpls = samples[0];

					dly.processBlock(dry, smpls, numSamples);
					juce::FloatVectorOperations::copy(smpls, dry, numSamples);
				}
				if (numChannelsOut == 2)
				{
					if(numChannelsIn < numChannelsOut)
						juce::FloatVectorOperations::copy(samples[1], samples[0], numSamples);
					else
					{
						auto& dly = dryDelay[1];
						auto dry = dryBuffer[1].data();
						auto smpls = samples[1];

						dly.processBlock(dry, smpls, numSamples);
						juce::FloatVectorOperations::copy(smpls, dry, numSamples);
					}
					
				}
			}
			return true;
		}
		bool saveDry(const float** samples, float mixVal, int numChannelsIn, int numChannelsOut, int numSamples) noexcept
		{
			{ // CHECK IF LOOKAHEAD STATE CHANGED
				const auto e = lookaheadEnabled.load();
				if (lookaheadState != e)
				{
					lookaheadState = e;
					return false;
				}
			}
			auto mixBuf = paramBuffer[2].data();
			{ // SMOOTHEN PARAMETER VALUE pVAL
				for (auto s = 0; s < numSamples; ++s)
					mixBuf[s] = mixSmooth(mixVal);
			}
			{ // MAKING EQUAL LOUDNESS CURVES
				for (auto s = 0; s < numSamples; ++s)
					paramBuffer[0][s] = std::sqrt(1.f - mixBuf[s]);
				for (auto s = 0; s < numSamples; ++s)
					paramBuffer[1][s] = std::sqrt(mixBuf[s]);
			}
			{ // SAVE DRY BUFFER
				if (lookaheadState)
				{
					{
						auto& dly = dryDelay[0];
						auto dry = dryBuffer[0].data();
						auto smpls = samples[0];

						dly.processBlock(dry, smpls, numSamples);
					}
					if (numChannelsOut == 2 && numChannelsIn == numChannels)
					{
						auto& dly = dryDelay[1];
						auto dry = dryBuffer[1].data();
						auto smpls = samples[1];

						dly.processBlock(dry, smpls, numSamples);
					}
				}
				else
				{
					for (auto ch = 0; ch < numChannelsIn; ++ch)
					{
						auto dry = dryBuffer[ch].data();
						const auto smpls = samples[ch];

						juce::FloatVectorOperations::copy(dry, smpls, numSamples);
					}
				}
			}
			return true;
		}
		void processWet(float** samples, float _gainWet, int numChannelsIn, int numChannelsOut, int numSamples) noexcept
		{
			if (gainWet != _gainWet)
			{
				gainWet = _gainWet;
				gainWetVal = juce::Decibels::decibelsToGain(gainWet);
			}
			auto gainBuf = paramBuffer[2].data();
			{
				for (auto s = 0; s < numSamples; ++s)
					gainBuf[s] = gainWetSmooth(gainWetVal);
			}
			const auto pBuf0 = paramBuffer[0].data();
			const auto pBuf1 = paramBuffer[1].data();
			{
				auto smpls = samples[0];
				const auto dry = dryBuffer[0].data();

				for (auto s = 0; s < numSamples; ++s)
					smpls[s] = dry[s] * pBuf0[s] + smpls[s] * pBuf1[s] * gainBuf[s];
			}

			if (numChannelsOut == 2)
			{
				auto smpls = samples[1];
				const auto dry = dryBuffer[1 % numChannelsIn].data();

				for (auto s = 0; s < numSamples; ++s)
					smpls[s] = dry[s] * pBuf0[s] + smpls[s] * pBuf1[s] * gainBuf[s];
			}
		}
		bool isLookaheadEnabled() const noexcept { return lookaheadEnabled.load(); }
	protected:
		modSys6::Smooth mixSmooth;
		std::array<FFDelay, 2> dryDelay;
		Buffer dryBuffer, paramBuffer;
		const int numChannels;

		float gainWet, gainWetVal;
		modSys6::Smooth gainWetSmooth;

		std::atomic<bool> lookaheadEnabled;
		bool lookaheadState;
	};
}

/*

*/
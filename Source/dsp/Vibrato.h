#pragma once
#define DebugVibrato false
#define LookAheadEnabled true
#include <JuceHeader.h>
#include <limits>

namespace vibrato {
	static inline float mix(const float a, float const b, const float m) noexcept {
		return std::sqrt(1.f - m) * a + std::sqrt(m) * b;
	}

	struct FFDelay {
		FFDelay() :
			ringBuffer(),
			wHead(0), rHead(0)
		{}
		// SET
		void setDelaySize(const int delaySize) {
			ringBuffer.resize(delaySize / 2, 0.f);
			wHead = 0;
			rHead = 1;
		}
		// PROCESS
		void processBlock(float* dry, const int numSamples) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
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
	protected:
		std::vector<float> ringBuffer;
		size_t wHead, rHead;
	};

	struct Delay {
		Delay(juce::AudioBuffer<float>& vibBuf, int channel) :
			delayBuffer(vibBuf),
			ringBuffer(),
			delaySize(0.f), delayMid(0.f),
			ch(channel)
		{
		}
		void setDelaySize(const size_t s) {
			ringBuffer.resize(s, 0.f);
			delaySize = static_cast<float>(s);
			delayMid = s * .5f;
		}
		// PROCESS
		void processBlock(float* samples, const float* dry, const float* dryWetMix, const int numSamples, const size_t* writeHead) noexcept {
			
#if DebugVibrato
			processBlockDebug(samples, numSamples);
#elif !DebugVibrato
			processBlockReadHead(numSamples, writeHead);
			processBlockDelay(samples, dry, dryWetMix, numSamples, writeHead);
#endif
		}
		// GET
		const size_t size() const noexcept { return ringBuffer.size(); }
	private:
		juce::AudioBuffer<float>& delayBuffer;
		std::vector<float> ringBuffer;
		float delaySize, delayMid;
		int ch;

		inline void processBlockDebug(float* samples, const int numSamples) noexcept {
			auto delay = delayBuffer.getWritePointer(ch);
			for (auto s = 0; s < numSamples; ++s)
				samples[s] = delay[s];
		}

		inline void processBlockReadHead(const int numSamples, const size_t* writeHead) noexcept {
			auto delay = delayBuffer.getWritePointer(ch);
			for (auto s = 0; s < numSamples; ++s) {
				// map delay [-1, 1] to [0, delaySize]
				const auto d = delay[s] * delayMid + delayMid;
				// calc readHead
				const auto rh = static_cast<float>(writeHead[s]) - d;
				delay[s] = rh < 0.f ? rh + delaySize : rh;
			}
		}
		
		inline void processBlockDelay(float* samples, const float* dry, const float* dryWetMix, const int numSamples, const size_t* writeHead) noexcept {
			const auto delay = delayBuffer.getReadPointer(ch);
			for (auto s = 0; s < numSamples; ++s) {
				ringBuffer[writeHead[s]] = samples[s];
				//const auto val = interpolation::lerp(ringBuffer.data(), delay[s], size());
				//const auto val = interpolation::lanczosSinc(ringBuffer.data(), delay[s], size(), 8);
				const auto val = interpolation::cubicHermiteSpline(ringBuffer.data(), delay[s], size());
				//const auto val = interpolation::lagrange(ringBuffer.data(), delay[s], size(), 8);
				samples[s] = mix(dry[s], val, dryWetMix[s]);
			}
		}
	};

	struct Processor {
		Processor(juce::AudioBuffer<float>& vibBuf, juce::AudioBuffer<float>& dryWetMixP, const int channelCount) :
			dryDelay(),
			writeHead(),
			delay(),
			dryWetMixParam(dryWetMixP),
			wHead(-1)
		{
			dryDelay.resize(channelCount);
			delay.reserve(channelCount);
			for (auto ch = 0; ch < channelCount; ++ch)
				delay.emplace_back(vibBuf, ch);
		}
		// PREPARE
		void prepareToPlay(const int blockSize) {
			writeHead.resize(blockSize, 0);
			dryBuffer.setSize(numChannels(), blockSize, false, false, false);
		}
		void resizeDelay(juce::AudioProcessor& p, const size_t size) {
#if LookAheadEnabled
			p.setLatencySamples(size / 2);
#endif
			for (auto& d : delay)
				d.setDelaySize(size);
			for (auto& d : dryDelay)
				d.setDelaySize(size);
		}
		// PROCESS
		void processBlock(juce::AudioBuffer<float>& audioBuffer) noexcept {
			auto samples = audioBuffer.getArrayOfWritePointers();
			auto dry = dryBuffer.getArrayOfWritePointers();
			const auto numSamples = audioBuffer.getNumSamples();
			for (auto ch = 0; ch < numChannels(); ++ch)
				juce::FloatVectorOperations::copy(dry[ch], samples[ch], numSamples);
#if LookAheadEnabled
			for (auto ch = 0; ch < numChannels(); ++ch)
				dryDelay[ch].processBlock(dry[ch], numSamples);
#endif	
			processBlockWriteHead(numSamples);
			const auto dwmp = dryWetMixParam.getReadPointer(0);
			for (auto ch = 0; ch < numChannels(); ++ch)
				delay[ch].processBlock(samples[ch], dry[ch], dwmp, numSamples, writeHead.data());
		}
	private:
		juce::AudioBuffer<float> dryBuffer;
		std::vector<FFDelay> dryDelay;
		std::vector<size_t> writeHead;
		std::vector<Delay> delay;
		juce::AudioBuffer<float>& dryWetMixParam;
		size_t wHead;

		inline void processBlockWriteHead(const int numSamples) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
				++wHead;
				if (wHead >= ringBufferSize())
					wHead = 0;
				writeHead[s] = wHead;
			}
		}
		
		const size_t numChannels() const noexcept { return delay.size(); }
		const size_t ringBufferSize() const noexcept { return delay[0].size(); }
	};
}

#undef DebugVibrato
#undef LookAheadEnabled

/*

fix the lagrange interpolator and try implement lagrange with lower aliasing than hermite

dryWetMix uses 2 sqrt in method. whole buffer calculates through twice, because buffer is const

feature ideas:
	m/s encoding
	allpass instead delay (or in feedback loop, considering fb delay)

	advanced settings
		change fps of visualizer
		change interpolation type of vibrato

	post processing modulation output before going into vibrato
		lowpass
		waveshaper
		wavefolder
		bitcrusher

*/
#pragma once
#define DebugVibrato false
#define LookAheadEnabled true
#include <JuceHeader.h>
#include <limits>

namespace vibrato {
	static inline float mix(const float a, float const b, const float m) noexcept {
		return std::sqrt(1.f - m) * a + std::sqrt(m) * b;
	}

	enum InterpolationType { Lerp, Spline, LagRange, Sinc, NumInterpolationTypes };

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
		Delay(juce::AudioBuffer<float>& vibBuf, int channel, InterpolationType it = InterpolationType::Spline) :
			delayBuffer(vibBuf),
			ringBuffer(),
			delaySize(0.f), delayMid(0.f),
			interpolationType(it),
			ch(channel)
		{
		}
		void setDelaySize(const size_t s) {
			ringBuffer.resize(s, 0.f);
			delaySize = static_cast<float>(s);
			delayMid = s * .5f;
		}
		void setInterpolationType(const InterpolationType t) noexcept { interpolationType = t; }
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
		const InterpolationType getInterpolationType() const noexcept { return interpolationType; }
	private:
		juce::AudioBuffer<float>& delayBuffer;
		std::vector<float> ringBuffer;
		float delaySize, delayMid;
		InterpolationType interpolationType;
		int ch;

		void processBlockDebug(float* samples, const int numSamples) noexcept {
			auto delay = delayBuffer.getWritePointer(ch);
			for (auto s = 0; s < numSamples; ++s)
				samples[s] = delay[s];
		}

		void processBlockReadHead(const int numSamples, const size_t* writeHead) noexcept {
			auto delay = delayBuffer.getWritePointer(ch);
			for (auto s = 0; s < numSamples; ++s) {
				// map delay [-1, 1] to [0, delaySize]
				const auto d = delay[s] * delayMid + delayMid;
				// calc readHead
				const auto rh = static_cast<float>(writeHead[s]) - d;
				delay[s] = rh < 0.f ? rh + delaySize : rh;
			}
		}
		
		void processBlockDelay(float* samples, const float* dry, const float* dryWetMix, const int numSamples, const size_t* writeHead) noexcept {
			switch (interpolationType) {
			case InterpolationType::Lerp: return processBlockDelayLERP(samples, dry, dryWetMix, numSamples, writeHead);
			case InterpolationType::Spline: return processBlockDelaySPLINE(samples, dry, dryWetMix, numSamples, writeHead);
			case InterpolationType::LagRange: return processBlockDelayLAGRANGE(samples, dry, dryWetMix, numSamples, writeHead);
			case InterpolationType::Sinc: return processBlockDelaySINC(samples, dry, dryWetMix, numSamples, writeHead);
			}
		}
		void processBlockDelayLERP(float* samples, const float* dry, const float* dryWetMix, const int numSamples, const size_t* writeHead) noexcept {
			const auto delay = delayBuffer.getReadPointer(ch);
			for (auto s = 0; s < numSamples; ++s) {
				ringBuffer[writeHead[s]] = samples[s];
				const auto val = interpolation::lerp(ringBuffer.data(), delay[s], size());
				samples[s] = mix(dry[s], val, dryWetMix[s]);
			}
		}
		void processBlockDelaySPLINE(float* samples, const float* dry, const float* dryWetMix, const int numSamples, const size_t* writeHead) noexcept {
			const auto delay = delayBuffer.getReadPointer(ch);
			for (auto s = 0; s < numSamples; ++s) {
				ringBuffer[writeHead[s]] = samples[s];
				const auto val = interpolation::cubicHermiteSpline(ringBuffer.data(), delay[s], size());
				samples[s] = mix(dry[s], val, dryWetMix[s]);
			}
		}
		void processBlockDelayLAGRANGE(float* samples, const float* dry, const float* dryWetMix, const int numSamples, const size_t* writeHead) noexcept {
			const auto delay = delayBuffer.getReadPointer(ch);
			for (auto s = 0; s < numSamples; ++s) {
				ringBuffer[writeHead[s]] = samples[s];
				const auto val = interpolation::lagrange(ringBuffer.data(), delay[s], size(), 9);
				samples[s] = mix(dry[s], val, dryWetMix[s]);
			}
		}
		void processBlockDelaySINC(float* samples, const float* dry, const float* dryWetMix, const int numSamples, const size_t* writeHead) noexcept {
			const auto delay = delayBuffer.getReadPointer(ch);
			for (auto s = 0; s < numSamples; ++s) {
				ringBuffer[writeHead[s]] = samples[s];
				const auto val = interpolation::lanczosSinc(ringBuffer.data(), delay[s], size(), 9);
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
			wHead(-1),
			rBufferSize(0)
		{
			dryDelay.resize(channelCount);
			delay.reserve(channelCount);
			for (auto ch = 0; ch < channelCount; ++ch)
				delay.emplace_back(vibBuf, ch);
		}
		Processor(Processor& other) :
			dryBuffer(other.dryBuffer),
			dryDelay(other.dryDelay),
			writeHead(other.writeHead),
			delay(other.delay),
			dryWetMixParam(other.dryWetMixParam),
			wHead(-1),
			rBufferSize(other.ringBufferSize())
		{}
		// PREPARE
		void prepareToPlay(const int blockSize) {
			writeHead.resize(blockSize, 0);
			dryBuffer.setSize(numChannels(), blockSize, false, false, false);
		}
		void resizeDelay(juce::AudioProcessor& p, const size_t size) {
			rBufferSize = size;
			for (auto& d : delay)
				d.setDelaySize(rBufferSize);
			for (auto& d : dryDelay)
				d.setDelaySize(rBufferSize);
#if LookAheadEnabled
			p.setLatencySamples(rBufferSize / 2);
#endif
		}
		// PARAMETERS
		void resizeDelaySafe(juce::AudioProcessor& p, const float ms) {
			rBufferSize = static_cast<size_t>(ms * static_cast<float>(p.getSampleRate()) / 1000.f);
		}
		void setInterpolationType(InterpolationType t) noexcept {
			for (auto& d : delay) d.setInterpolationType(t);
		}
		// PROCESS
		void processBlock(juce::AudioProcessor& p, juce::AudioBuffer<float>& audioBuffer) noexcept {
			if (rBufferSize != ringBufferSize())
				return resizeDelay(p, rBufferSize);

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
		// GET
		const float getSizeInMs(const juce::AudioProcessor& p) const noexcept {
			auto dlyTime = static_cast<double>(1000. * ringBufferSize()) / p.getSampleRate();
			return static_cast<float>(dlyTime);
		}
		const InterpolationType getInterpolationType() const noexcept { return delay[0].getInterpolationType(); }
	private:
		juce::AudioBuffer<float> dryBuffer;
		std::vector<FFDelay> dryDelay;
		std::vector<size_t> writeHead;
		std::vector<Delay> delay;
		juce::AudioBuffer<float>& dryWetMixParam;
		size_t wHead, rBufferSize;

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
lagrange interpolator
	worse in lowend
	better in highend
	>> brittle noise in highend when used on mixed signal (inter-modulation?)

dryWetMix uses 2 sqrt in method. whole buffer calculates through twice, because buffer is const

feature ideas:
	m/s encoding
	allpass instead delay (or in feedback loop, considering fb delay)

	advanced settings
		change fps of visualizer

	post processing modulation output before going into vibrato
		lowpass
		waveshaper
		wavefolder
		bitcrusher
*/
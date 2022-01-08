#pragma once
#include <JuceHeader.h>
#include <limits>

namespace vibrato
{
	static inline float mix(float a, float b, float m) noexcept
	{
		return std::sqrt(1.f - m) * a + std::sqrt(m) * b;
	}

	enum class InterpolationType
	{
		Lerp, Spline, LagRange, Sinc,
		NumInterpolationTypes
	};
	inline juce::String toString(InterpolationType t)
	{
		switch (t)
		{
		case InterpolationType::Lerp: return "lerp";
		case InterpolationType::Spline: return "spline";
		case InterpolationType::LagRange: return "lagrange";
		case InterpolationType::Sinc: return "sinc";
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
	
	using Buffer = std::array<std::vector<float>, 2>;

	struct Delay
	{
		Delay(Buffer& vibBuf, int channel, InterpolationType it) :
			delayBuffer(vibBuf),
			ringBuffer(),
			delaySize(0.f), delayMid(0.f), delayMax(0.f),
			interpolationType(it),
			ch(channel)
		{
		}
		void setDelaySize(size_t s)
		{
			ringBuffer.resize(s, 0.f);
			delaySize = static_cast<float>(s);
			delayMid = s * .5f;
			delayMax = delaySize - 1.f;
		}
		void setInterpolationType(InterpolationType t) noexcept { interpolationType = t; }
		// PROCESS
		void processBlockBypassed() noexcept
		{
			for (auto& s : ringBuffer)
				s = 0.f;
		}
		void processBlock(float* samples,
			int numSamples, const size_t* writeHead) noexcept
		{
			processBlockReadHead(numSamples, writeHead);
			processBlockDelay(samples, numSamples, writeHead);
		}
		// GET
		size_t size() const noexcept { return ringBuffer.size(); }
		InterpolationType getInterpolationType() const noexcept { return interpolationType; }
	private:
		Buffer& delayBuffer;
		std::vector<float> ringBuffer;
		float delaySize, delayMid, delayMax;
		InterpolationType interpolationType;
		int ch;

		void processBlockReadHead(int numSamples, const size_t* writeHead) noexcept
		{
			auto& buf = delayBuffer[ch];
			// map buffer [-1, 1] to [0, delaySize]
			juce::FloatVectorOperations::multiply(buf.data(), delayMid, numSamples);
			juce::FloatVectorOperations::add(buf.data(), delayMid, numSamples);
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto dly = buf[s];
				auto rh = static_cast<float>(writeHead[s]) - dly;
				if (rh < 0.f)
					rh += delaySize;
				buf[s] = juce::jlimit(0.f, delayMax, rh);
			}
		}
		
		void processBlockDelay(float* samples,
			int numSamples, const size_t* writeHead) noexcept
		{
			switch (interpolationType)
			{
			case InterpolationType::Lerp: return processBlockDelayLERP(samples, numSamples, writeHead);
			case InterpolationType::Spline: return processBlockDelaySPLINE(samples, numSamples, writeHead);
			case InterpolationType::LagRange: return processBlockDelayLAGRANGE(samples, numSamples, writeHead);
			case InterpolationType::Sinc: return processBlockDelaySINC(samples, numSamples, writeHead);
			}
		}
		void processBlockDelayLERP(float* samples,
			const int numSamples, const size_t* writeHead) noexcept
		{
			const auto sizeInt = static_cast<int>(size());
			for (auto s = 0; s < numSamples; ++s)
			{
				ringBuffer[writeHead[s]] = samples[s];
				const auto val = interpolation::lerp(ringBuffer.data(), delayBuffer[ch][s], sizeInt);
				samples[s] = val;
			}
		}
		void processBlockDelaySPLINE(float* samples,
			const int numSamples, const size_t* writeHead) noexcept
		{
			const auto sizeInt = static_cast<int>(size());
			for (auto s = 0; s < numSamples; ++s)
			{
				ringBuffer[writeHead[s]] = samples[s];
				const auto val = interpolation::cubicHermiteSpline(ringBuffer.data(), delayBuffer[ch][s], sizeInt);
				samples[s] = val;
			}
		}
		void processBlockDelayLAGRANGE(float* samples,
			const int numSamples, const size_t* writeHead) noexcept
		{
			const auto sizeInt = static_cast<int>(size());
			for (auto s = 0; s < numSamples; ++s)
			{
				ringBuffer[writeHead[s]] = samples[s];
				const auto val = interpolation::lagrange(ringBuffer.data(), delayBuffer[ch][s], sizeInt, 9);
				samples[s] = val;
			}
		}
		void processBlockDelaySINC(float* samples,
			const int numSamples, const size_t* writeHead) noexcept
		{
			const auto sizeInt = static_cast<int>(size());
			for (auto s = 0; s < numSamples; ++s)
			{
				ringBuffer[writeHead[s]] = samples[s];
				const auto val = interpolation::lanczosSinc(ringBuffer.data(), delayBuffer[ch][s], sizeInt, 9);
				samples[s] = val;
			}
		}
	};

	struct Processor
	{
		Processor(Buffer& vibBuf, int _numChannels) :
			writeHead(),
			delay
			{
				Delay(vibBuf, 0, InterpolationType::Spline),
				Delay(vibBuf, 1, InterpolationType::Spline)
			},
			wHead(static_cast<size_t>(-1)),
			rBufferSize(0),
			numChannels(_numChannels),
			wannaUpdate(false),
			interpolationType(InterpolationType::Spline)
		{
		}
		// PREPARE
		void prepareToPlay(const int blockSize)
		{
			writeHead.resize(blockSize, 0);
		}
		void resizeDelay(size_t size)
		{
			rBufferSize = size;
			for (auto ch = 0; ch < numChannels; ++ch)
				delay[ch].setDelaySize(rBufferSize);
		}
		void triggerUpdate() noexcept
		{
			wannaUpdate.store(true);
		}
		void setInterpolationType(InterpolationType t) noexcept
		{
			interpolationType.store(t);
			for (auto& d : delay)
				d.setInterpolationType(t);
		}
		// PROCESS
		bool processBlock(juce::AudioBuffer<float>& audioBuffer, juce::AudioProcessor* p, int numChannelsOut)
		{
			if (wannaUpdate.load())
			{
				p->prepareToPlay(p->getSampleRate(), p->getBlockSize());
				wannaUpdate.store(false);
				return false;
			}
			processBlock(audioBuffer, numChannelsOut);
			return true;
		}
		bool processBlockBypassed(juce::AudioProcessor* p, int numChannelsOut)
		{
			if (wannaUpdate.load())
			{
				p->prepareToPlay(p->getSampleRate(), p->getBlockSize());
				wannaUpdate.store(false);
				return false;
			}
			for (auto ch = 0; ch < numChannelsOut; ++ch)
				delay[ch].processBlockBypassed();
			return true;
		}
		// GET
		float getSizeInMs(float Fs) const noexcept
		{
			return 1000.f * static_cast<float>(ringBufferSize()) / Fs;
		}
		InterpolationType getInterpolationType() const noexcept
		{
			return interpolationType.load();
		}
		int getLatency() const noexcept
		{
			return static_cast<int>(ringBufferSize()) / 2;
		}
	protected:
		std::vector<size_t> writeHead;
		std::array<Delay, 2> delay;
		size_t wHead, rBufferSize;
		const int numChannels;
		std::atomic<bool> wannaUpdate;
		std::atomic<InterpolationType> interpolationType;

		void processBlock(juce::AudioBuffer<float>& audioBuffer, int numChannelsOut) noexcept
		{
			auto samples = audioBuffer.getArrayOfWritePointers();
			const auto numSamples = audioBuffer.getNumSamples();
			processBlockWriteHead(numSamples);
			for (auto ch = 0; ch < numChannelsOut; ++ch)
				delay[ch].processBlock(samples[ch], numSamples, writeHead.data());
		}

		void processBlockWriteHead(const int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				++wHead;
				if (wHead >= ringBufferSize())
					wHead = 0;
				writeHead[s] = wHead;
			}
		}
		
		const size_t ringBufferSize() const noexcept { return delay[0].size(); }
	};
}

/*

lagrange interpolator
	worse in lowend
	better in highend
	>> brittle noise in highend when used on mixed signal (inter-modulation?)

feature ideas:
	allpass instead of delay (or in feedback loop, considering fb delay)

*/
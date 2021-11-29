#pragma once
#include <array>

#include "juce_audio_basics/juce_audio_basics.h"

#include "Filter.h"
#include "ConvolutionFilter.h"

namespace oversampling {
	constexpr size_t MaxNumStages = 2;
	static constexpr size_t MaxOrder = 1 << MaxNumStages;

	struct Processor
	{
		Processor(const int _numChannels, juce::AudioProcessor* p) :
			audioProcessor(p),
			Fs(0.),
			numChannels(_numChannels),
			blockSize(0),

			buffer(),

			filterUp2(  numChannels, 88200.f,  19000.f, 3049.f,  true),
			filterUp4(  numChannels, 176400.f, 22050.f, 44100.f, true),
			filterDown4(numChannels, 176400.f, 22050.f, 44100.f),
			filterDown2(numChannels, 88200.f,  19000.f, 3049.f),

			FsUp(0.),
			blockSizeUp(0),

			enabled(true), wannaUpdate(false),
			enabledTmp(true),

			numSamples1x(0), numSamples2x(0), numSamples4x(0)
		{
		}

		Processor(Processor& p) :
			audioProcessor(p.audioProcessor),
			Fs(p.Fs),
			numChannels(p.numChannels), blockSize(p.blockSize),
			buffer(p.buffer),
			filterUp2(p.filterUp2), filterUp4(p.filterUp4),
			filterDown4(p.filterDown4), filterDown2(p.filterDown2),
			FsUp(p.FsUp), blockSizeUp(p.blockSizeUp),
			enabled(p.enabled.load()),
			wannaUpdate(p.wannaUpdate.load()),
			enabledTmp(p.enabledTmp),
			numSamples1x(0), numSamples2x(0), numSamples4x(0)
		{
		}
		// prepare & params
		void prepareToPlay(const double sampleRate, const int _blockSize) {
			Fs = sampleRate;
			blockSize = _blockSize;
			if (enabled.load()) {
				FsUp = sampleRate * static_cast<double>(MaxOrder);
				blockSizeUp = blockSize * MaxOrder;
			}
			else {
				FsUp = sampleRate;
				blockSizeUp = blockSize;
			}
			buffer.setSize(numChannels, blockSize * MaxOrder, false, false, false);
		}
		/* processing methods */
		juce::AudioBuffer<float>* upsample(juce::AudioBuffer<float>& input) {
			if (wannaUpdate.load()) {
				enabled.store(enabledTmp);
				audioProcessor->prepareToPlay(Fs, blockSize);
				wannaUpdate.store(false);
				return nullptr;
			}
			if (enabled.load()) {
				numSamples1x = input.getNumSamples();
				numSamples2x = numSamples1x * 2;
				numSamples4x = numSamples1x * 4;

				buffer.setSize(numChannels, numSamples4x, true, false, true);
				auto samplesUp = buffer.getArrayOfWritePointers();
				const auto samplesIn = input.getArrayOfReadPointers();
				// zero stuffing + filter 2x
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples1x; ++s) {
						const auto s2 = s * 2;
						samplesUp[ch][s2] = filterUp2.processSampleUpEven(samplesIn[ch][s], ch);
						samplesUp[ch][s2 + 1] = filterUp2.processSampleUpOdd(ch);
					}
				// zero stuffing + filter 4x
				const auto maxSample2x = numSamples2x - 1;
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = maxSample2x; s > -1; --s) {
						const auto s2 = s * 2;
						samplesUp[ch][s2] = samplesUp[ch][s];
						samplesUp[ch][s2 + 1] = 0.f;
					}
				// filter 4x
				filterUp4.processBlockUp(samplesUp, numSamples4x);
				return &buffer;
			}
			return &input;
		}
		void downsample(juce::AudioBuffer<float>& outBuf) noexcept {
			if (enabled.load()) {
				auto samplesUp = buffer.getArrayOfWritePointers();
				auto samplesOut = outBuf.getArrayOfWritePointers();
				filterDown4.processBlockDown(samplesUp, numSamples4x);
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples2x; ++s)
						samplesUp[ch][s] = samplesUp[ch][s * 2];
				filterDown2.processBlockDown(samplesUp, numSamples2x);
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples1x; ++s)
						samplesOut[ch][s] = samplesUp[ch][s * 2];
			}
		}
		////////////////////////////////////////
		const double getSampleRateUpsampled() const noexcept { return FsUp; }
		const int getBlockSizeUp() const noexcept { return blockSizeUp; }
		/* returns true if changing the state was successful */
		bool setEnabled(const bool e) noexcept {
			if (wannaUpdate.load()) return false;
			enabledTmp = e;
			wannaUpdate.store(true);
			return true;
		}
		bool isEnabled() const noexcept { return enabled.load(); }
		const int getLatency() const noexcept {
			if (enabled.load())
				return filterUp2.getLatency() + filterDown2.getLatency() + filterUp4.getLatency() + filterDown4.getLatency();
			return 0;
		}
	protected:
		juce::AudioProcessor* audioProcessor;
		double Fs;
		int numChannels, blockSize;

		juce::AudioBuffer<float> buffer;

		ConvolutionFilter filterUp2, filterUp4;
		ConvolutionFilter filterDown4, filterDown2;

		double FsUp;
		int blockSizeUp;

		std::atomic<bool> enabled;
		std::atomic<bool> wannaUpdate;
		bool enabledTmp;

		int numSamples1x, numSamples2x, numSamples4x;
	};
}

/*
try other filter types:
	polyphase IIR
	butterworth low pass filter
automatic reaction to different sampleRates

optimisations:
convolution filter simd

*/
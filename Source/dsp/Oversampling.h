#pragma once
#include <JuceHeader.h>

namespace oversampling {
	
	struct Processor {
		Processor(const int _numChannels) :
			Fs(0.),
			numChannels(_numChannels),
			blockSize(0),

			buffer(),
			FsUp(0.),
			blockSizeUp(0)
		{}
		void prepareToPlay(const double sampleRate, const int _blockSize) {
			Fs = sampleRate;
			blockSize = _blockSize;
			Fs = sampleRate * 2.;
			blockSizeUp = blockSize * 2;
			buffer.setSize(numChannels, blockSizeUp);
		}
		juce::AudioBuffer<float>& upsample(const juce::AudioBuffer<float>& input) noexcept {
			auto samplesUp = buffer.getArrayOfWritePointers();
			auto samplesIn = input.getArrayOfReadPointers();
			zeroStuffing(samplesUp, samplesIn, input.getNumSamples());
			// PUT UPSAMPLING FILTER HERE
		}
		void downsample(juce::AudioBuffer<float>& outBuf) noexcept {
			auto samplesUp = buffer.getArrayOfReadPointers();
			auto samplesOut = outBuf.getArrayOfWritePointers();
			// PUT ANTI-ALIASING FILTER HERE
			downsample(samplesOut, samplesUp, outBuf.getNumSamples());
		}

	protected:
		double Fs;
		int numChannels, blockSize;
		juce::AudioBuffer<float> buffer;
		double FsUp;
		int blockSizeUp;

	private:
		void zeroStuffing(float** samplesUp, const float** samplesIn, const int numInputSamples) noexcept {
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numInputSamples; ++s) {
					const auto upIdx = s * 2;
					samplesUp[ch][upIdx] = samplesIn[ch][s];
					samplesUp[ch][upIdx + 1] = 0.f;
				}
		}
		void downsample(float** samplesOut, const float** samplesUp, const int numOutputSamples) {
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numOutputSamples; ++s)
					samplesOut[ch][s] = samplesUp[ch][s * 2];
		}
	};
}
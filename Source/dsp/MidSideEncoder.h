#pragma once

namespace midSide {
	struct Processor {
		Processor(int numChannels) :
			enabled(numChannels == 2)
		{}
		void setEnabled(bool e) { enabled = e; }
		void processBlockEncode(juce::AudioBuffer<float>& audioBuffer) noexcept {
			if (enabled) {
				auto samples = audioBuffer.getArrayOfWritePointers();
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s) {
					const auto mid = (samples[0][s] + samples[1][s]) * .5f;
					const auto side = (samples[0][s] - samples[1][s]) * .5f;
					samples[0][s] = mid;
					samples[1][s] = side;
				}
			}
		}
		void processBlockDecode(juce::AudioBuffer<float>& audioBuffer) noexcept {
			if (enabled) {
				auto samples = audioBuffer.getArrayOfWritePointers();
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s) {
					const auto left = samples[0][s] + samples[1][s];
					const auto right = samples[0][s] - samples[1][s];
					samples[0][s] = left;
					samples[1][s] = right;
				}
			}
		}
	protected:
		bool enabled;
	};
}

/*

midside encoder before mod matrix or after?

*/
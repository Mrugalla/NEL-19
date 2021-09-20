#pragma once
#include "Utils.h"

namespace modSys2 {
	/*
	* a parameter, its block and a lowpass filter
	*/
	struct Parameter :
		public Identifiable
	{
		struct Smoothing {
			Smoothing() :
				env(0.f),
				startValue(0.f), endValue(0.f), rangeValue(0.f),
				idx(0.f), length(0.f),
				isWorking(false)
			{}
			void setLength(const float samples) noexcept { length = samples; }
			void processBlock(float* block, const float dest, const int numSamples) noexcept {
				if (!isWorking) {
					if (env == dest)
						return bypass(block, dest, numSamples);
					setNewDestination(dest);
				}
				else if (length == 0)
					return bypass(block, dest, numSamples);
				processWork(block, dest, numSamples);
			}
		protected:
			float env, startValue, endValue, rangeValue, idx, length;
			bool isWorking;

			void setNewDestination(const float dest) noexcept {
				startValue = env;
				endValue = dest;
				rangeValue = endValue - startValue;
				idx = 0.f;
				isWorking = true;
			}
			void processWork(float* block, const float dest, const int numSamples) noexcept {
				const auto lenInv = 1.f / length;
				for (auto s = 0; s < numSamples; ++s) {
					if (idx >= length) {
						if (dest != endValue)
							setNewDestination(dest);
						else {
							for (auto s1 = s; s1 < numSamples; ++s1)
								block[s1] = endValue;
							isWorking = false;
							return;
						}
					}
					const auto curve = .5f - approx::taylor_cos(idx * pi * lenInv) * .5f;
					block[s] = env = startValue + curve * rangeValue;
					++idx;
				}
			}
			void bypass(float* block, const float dest, const int numSamples) noexcept {
				for (auto s = 0; s < numSamples; ++s)
					block[s] = dest;
			}
		};

		Parameter(juce::AudioProcessorValueTreeState& apvts, juce::Identifier&& pID) :
			Identifiable(std::move(pID)),
			parameter(apvts.getRawParameterValue(pID)),
			rap(apvts.getParameter(pID)),
			attachedModulator(nullptr),
			sumValue(0.f),
			block(),
			smoothing(),
			Fs(1.f),
			blockSize(0),
			active(false)
		{}
		// SET
		void prepareToPlay(const int blockS, double sampleRate) {
			Fs = static_cast<float>(sampleRate);
			blockSize = blockS;
			block.setSize(1, blockSize, false, false);
		}
		void setSmoothingLengthInSamples(const float length) noexcept { smoothing.setLength(length); }
		void setActive(bool a) { active = a; }
		// PROCESS
		void attachTo(juce::Identifier* mID) noexcept { attachedModulator = mID; }
		void processBlock(const int numSamples) noexcept {
			const auto targetValue = parameter->load();
			const auto normalised = rap->convertTo0to1(targetValue);
			smoothing.processBlock(block.getWritePointer(0), normalised, numSamples);
		}
		void processBlockEmpty() noexcept {
			const auto targetValue = parameter->load();
			const auto normalised = rap->convertTo0to1(targetValue);
			*block.getWritePointer(0, 0) = normalised;
		}
		void storeSumValue(const int lastSample) noexcept { sumValue.set(get(lastSample)); }
		void set(const float value, const int s) noexcept {
			auto samples = block.getWritePointer(0);
			samples[s] = value;
		}
		void limit(const int numSamples) noexcept {
			auto blockData = data().getWritePointer(0);
			for (auto s = 0; s < numSamples; ++s)
				blockData[s] = blockData[s] < 0.f ? 0.f : blockData[s] > 1.f ? 1.f : blockData[s];
		}
		// GET NORMAL
		float getSumValue() const noexcept { return sumValue.get(); }
		inline const float get(const int s = 0) const noexcept { return *block.getReadPointer(0, s); }
		juce::AudioBuffer<float>& data() noexcept { return block; }
		const juce::AudioBuffer<float>& data() const noexcept { return block; }
		bool isActive() const noexcept { return active; }
		const juce::Identifier* getAttachedModulatorID() const noexcept { return attachedModulator; }
		// GET CONVERTED
		float denormalized(const int s = 0) const noexcept {
			return rap->convertFrom0to1(juce::jlimit(0.f, 1.f, get(s)));
		}
	protected:
		std::atomic<float>* parameter;
		const juce::RangedAudioParameter* rap;
		juce::Identifier* attachedModulator;
		juce::Atomic<float> sumValue;
		juce::AudioBuffer<float> block;
		Smoothing smoothing;
		float Fs;
		int blockSize;
		bool active;
	};
}

/*

make parameter without buffer
	(a lot of them don't really need their buffers)
	(some would perform better if certain operations came before buffering)
	(make sure optionally wanting buffers is a thing)

*/
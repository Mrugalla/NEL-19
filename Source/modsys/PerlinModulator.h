#pragma once
#include "Utils.h"
#include "Parameter.h"
#include "Modulator.h"

namespace modSys2 {
	/*
* a randomized modulator (1d perlin noise)
*/
	class PerlinModulator :
		public Modulator
	{
		enum { Rate, Octaves, Width };
	public:
		PerlinModulator(const juce::String& mID, const std::shared_ptr<Parameter> rateParam,
			const std::shared_ptr<Parameter> octavesParam, const std::shared_ptr<Parameter> widthParam,
			const param::MultiRange& ranges, const int maxNumOctaves) :
			Modulator(mID),
			multiRange(ranges),
			freeID(multiRange.getID("free")),
			syncID(multiRange.getID("sync")),
			seed(),
			seedSize(1 << maxNumOctaves),
			maxOctaves(maxNumOctaves),
			phase(0), fsInv(0), gainAccum(1),
			octaves(-1)
		{
			this->params.push_back(rateParam);
			this->params.push_back(octavesParam);
			this->params.push_back(widthParam);
			this->informParametersAboutAttachment();

			seed.resize(seedSize + spline::Size, 0.f);
			juce::Random rand;
			for (auto s = 0; s < seedSize; ++s)
				seed[s] = rand.nextFloat();
			for (auto s = seedSize; s < seed.size(); ++s)
				seed[s] = seed[s - seedSize];
		}
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			fsInv = 1.f / static_cast<float>(Fs);
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo& playHead) noexcept override {
			auto block = _block.getArrayOfWritePointers();
			auto numChannels = audioBuffer.getNumChannels();
			numChannels = numChannels > 3 ? numChannels : 2;
			const auto maxChannel = numChannels - 1;
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;

			setOctaves(juce::jlimit(0, maxOctaves, static_cast<int>(this->params[Octaves]->denormalized())));

			const auto rateValue = juce::jlimit(0.f, 1.f, this->params[Rate]->get(0));
			const auto rateInHz = multiRange(freeID).convertFrom0to1(rateValue);
			const auto inc = rateInHz * fsInv;
			synthesizePhase(block[maxChannel], inc, numSamples);
			synthesizeRandSignal(block, numChannels, numSamples);

			processWidth(block, numChannels, numSamples);
		}
	protected:
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> seed;
		const int seedSize, maxOctaves;
		float phase, fsInv, gainAccum;
		int octaves;

		void setOctaves(const int oct) noexcept {
			if (octaves == oct) return;
			octaves = oct;
			gainAccum = 0.f;
			for (auto o = 0; o < octaves; ++o) {
				const auto scl = static_cast<float>(1 << o);
				const auto gain = 1.f / scl;
				gainAccum += gain;
			}
			gainAccum = 1.f / gainAccum;
		}

		inline void synthesizePhase(float* block, const float inc, const int numSamples) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
				phase += inc;
				if (phase >= seedSize)
					phase -= seedSize;
				block[s] = phase;
			}
		}
		inline void synthesizeRandSignal(float** block, const int numChannels, const int numSamples) noexcept {
			const auto maxChannel = numChannels - 1;
			for (auto ch = 0; ch < numChannels; ++ch) {
				auto offset = ch * seedSize * .5f;
				for (auto s = 0; s < numSamples; ++s) {
					auto noise = 0.f;
					for (int o = 0; o < octaves; ++o) {
						const auto scl = static_cast<float>(1 << o);
						auto x = block[maxChannel][s] * scl + offset;
						while (x >= seedSize)
							x -= seedSize;
						const auto gain = 1.f / scl;
						noise += spline::process(seed.data(), x) * gain;
					}
					noise *= gainAccum;
					block[ch][s] = noise;
				}
			}
		}
		inline void processWidth(float** block, const int numChannels, const int numSamples) noexcept {
			const auto width = this->params[Width]->get();
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block[ch][s] = block[0][s] + width * (block[ch][s] - block[0][s]);
		}
	};
}


/*

width of perlin noise mod goes way too hard on values below 5%

*/
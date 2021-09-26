#pragma once
#include <JuceHeader.h>
#include "Utils.h"
#include "Parameter.h"
#include "Modulator.h"

namespace modSys2 {
	/*
	* a randomized modulator (by lowpassing an edgy signal)
	*/
	class RandomModulator :
		public Modulator
	{
		enum { Sync, Rate, Bias, Width, Smooth };
		struct LP1Pole {
			LP1Pole() :
				env(0.f),
				cutoff(.0001f)
			{}
			void processBlock(float* block, const int numSamples) noexcept {
				for (auto s = 0; s < numSamples; ++s)
					block[s] = process(block[s]);
			}
			const float process(const float sample) noexcept {
				env += cutoff * (sample - env);
				return env;
			}
			float env, cutoff;
		};
		struct LP1PoleOrder {
			LP1PoleOrder(int order) :
				filters()
			{
				filters.resize(order);
			}
			void setCutoff(float amount, float rateInSlew) {
				const auto cutoff = 1.f + amount * (rateInSlew - 1.f);
				for (auto& f : filters)
					f.cutoff = cutoff;
			}
			void processBlock(float* block, const int numSamples) noexcept {
				for (auto f = 0; f < filters.size(); ++f)
					filters[f].processBlock(block, numSamples);
			}
			const float process(float sample) noexcept {
				for (auto& f : filters)
					sample = f.process(sample);
				return sample;
			}
			std::vector<LP1Pole> filters;
		};
	public:
		RandomModulator(const juce::String& mID, const std::shared_ptr<Parameter>& syncParam,
			const std::shared_ptr<Parameter>& rateParam, const std::shared_ptr<Parameter>& biasParam,
			const std::shared_ptr<Parameter>& widthParam, const std::shared_ptr<Parameter>& smoothParam,
			const param::MultiRange& ranges) :
			Modulator(mID),
			multiRange(ranges),
			freeID(multiRange.getID("free")),
			syncID(multiRange.getID("sync")),
			randValue(),
			smoothing(),
			rand(juce::Time::currentTimeMillis()),
			externalLatency(0),
			phase(1.f), fsInv(1.f), rateInHz(1.f)
		{
			this->params.push_back(syncParam);
			this->params.push_back(rateParam);
			this->params.push_back(biasParam);
			this->params.push_back(widthParam);
			this->params.push_back(smoothParam);
			this->informParametersAboutAttachment();
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			static constexpr auto filterOrder = 3;
			smoothing.resize(numChannels, filterOrder);
			randValue.resize(numChannels, 0);
			fsInv = 1.f / Fs;
			externalLatency = static_cast<double>(latency);
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo& playHead, const juce::MidiBuffer&) noexcept override {
			auto block = _block.getArrayOfWritePointers();
			auto numChannels = audioBuffer.getNumChannels();
			numChannels = numChannels < 3 ? numChannels : 2;
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;

			const bool isFree = this->params[Sync]->get(0) < .5f;
			const auto biasValue = juce::jlimit(0.f, 1.f, this->params[Bias]->get(0));

			if (isFree) {
				const auto rateValue = juce::jlimit(0.f, 1.f, this->params[Rate]->get(0));
				rateInHz = multiRange(freeID).convertFrom0to1(rateValue);
				const auto inc = rateInHz * fsInv;
				synthesizePhase(block, inc, numSamples);
				synthesizeRandomSignal(block, biasValue, numSamples, 0);
			}
			else {
				const auto bpm = playHead.bpm;
				const auto bps = bpm / 60.;
				const auto quarterNoteLengthInSamples = Fs / bps;
				const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
				const auto rateValue = juce::jlimit(0.f, 1.f, this->params[Rate]->get());
				rateInHz = multiRange(syncID).convertFrom0to1(rateValue);
				const auto inc = 1.f / (static_cast<float>(barLengthInSamples) * rateInHz);
				if (playHead.isPlaying) {
					// latency stuff
					const auto latencyLengthInQuarterNotes = externalLatency / quarterNoteLengthInSamples;
					auto ppq = (playHead.ppqPosition - latencyLengthInQuarterNotes) * .25;
					while (ppq < 0.f) ++ppq;
					// latency stuff end
					const auto ppqCh = static_cast<float>(ppq) / rateInHz;
					auto newPhase = (ppqCh - std::floor(ppqCh));
					phase = newPhase;
				}
				synthesizePhase(block, inc, numSamples);
				synthesizeRandomSignal(block, biasValue, numSamples, 0);
			}
			processWidth(block, biasValue, numChannels, numSamples);
			processSmoothing(block, numChannels, numSamples);
		}
	protected:
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> randValue;
		std::vector<LP1PoleOrder> smoothing;
		juce::Random rand;
		double externalLatency;
		float phase, fsInv, rateInHz;
	private:
		inline void synthesizePhase(float** block, const float inc,
			const int numSamples) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
				phase += inc;
				if (phase >= 1.f) {
					block[1][s] = 1.f;
					--phase;
				}
				else
					block[1][s] = 0.f;
			}
		}
		inline void synthesizeRandomSignal(float** block, const float bias,
			const int numSamples, const int ch) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
				if (block[1][s] == 1.f)
					randValue[ch] = getBiasedValue(rand.nextFloat(), bias);
				block[ch][s] = randValue[ch];
			}
		}
		inline void processWidth(float** block, const float biasValue,
			const int numChannels, const int numSamples) noexcept {
			const auto widthValue = juce::jlimit(0.f, 1.f, this->params[Width]->get(0));
			if (widthValue != 0.f)
				for (auto ch = 1; ch < numChannels; ++ch) {
					synthesizeRandomSignal(block, biasValue, numSamples, ch);
					for (auto s = 0; s < numSamples; ++s)
						block[ch][s] = block[0][s] + widthValue * (block[ch][s] - block[0][s]);
				}
			else
				for (auto ch = 1; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						block[ch][s] = block[0][s];
		}

		inline void processSmoothing(float** block, const int numChannels, const int numSamples) {
			const auto rateInSlew = hzInSlewRate(rateInHz, Fs);
			static constexpr auto magicNumber = .9998f;
			const auto smoothValue = weight(this->params[Smooth]->get(0), magicNumber);
			for (auto ch = 0; ch < numChannels; ++ch) {
				smoothing[ch].setCutoff(smoothValue, rateInSlew);
				smoothing[ch].processBlock(block[ch], numSamples);
			}
		}
		const float getBiasedValue(float value, float bias) const noexcept {
			if (bias < .5f) {
				const auto a = bias * 2.f;
				return std::atan(std::tan(value * pi - .5f * pi) * a) / pi + .5f;
			}
			const auto a = 1.f - (2.f * bias - 1.f);
			return std::atan(std::tan(value * pi - .5f * pi) / a) / pi + .5f;
		}
	};
}


/*

instead of 1pole filter use cos approx to smoothen the thing
	or spline!

*/
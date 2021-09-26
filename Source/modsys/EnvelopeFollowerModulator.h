#pragma once
#include "Utils.h"
#include "Parameter.h"
#include "Modulator.h"

namespace modSys2 {
	/*
	* an envelope follower modulator
	*/
	class EnvelopeFollowerModulator :
		public Modulator
	{
		enum { Gain, Attack, Release, Bias, Width };
	public:
		EnvelopeFollowerModulator(const juce::String& mID, std::shared_ptr<Parameter> inputGain,
			std::shared_ptr<Parameter> atkParam, std::shared_ptr<Parameter> rlsParam,
			std::shared_ptr<Parameter> biasParam, std::shared_ptr<Parameter> widthParam) :
			Modulator(mID),
			env()
		{
			this->params.push_back(inputGain);
			this->params.push_back(atkParam);
			this->params.push_back(rlsParam);
			this->params.push_back(biasParam);
			this->params.push_back(widthParam);
			this->informParametersAboutAttachment();
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			env.resize(numChannels, 0.f);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo&, const juce::MidiBuffer&) noexcept override {
			auto block = _block.getArrayOfWritePointers();
			auto numChannels = audioBuffer.getNumChannels();
			numChannels = numChannels < 3 ? numChannels : 2;
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			const auto samples = audioBuffer.getArrayOfReadPointers();

			const auto atkInMs = this->params[Attack]->denormalized(0);
			const auto rlsInMs = this->params[Release]->denormalized(0);
			const auto bias = 1.f - juce::jlimit(0.f, 1.f, this->params[Bias]->get());
			const auto atkInSamples = msInSamples(atkInMs, Fs);
			const auto rlsInSamples = msInSamples(rlsInMs, Fs);
			const auto atkSpeed = 1.f / atkInSamples;
			const auto rlsSpeed = 1.f / rlsInSamples;
			const auto gain = makeAutoGain(atkSpeed, rlsSpeed);
			for (auto ch = 0; ch < numChannels; ++ch)
				processEnvelope(block, samples, ch, numSamples, atkSpeed, rlsSpeed, gain, bias);
			const auto narrow = 1.f - juce::jlimit(0.f, 1.f, this->params[Width]->get());
			const auto channelInv = 1.f / numChannels;
			for (auto s = 0; s < numSamples; ++s) {
				auto mid = 0.f;
				for (auto ch = 0; ch < numChannels; ++ch)
					mid += block[ch][s];
				mid *= channelInv;
				for (auto ch = 0; ch < numChannels; ++ch)
					block[ch][s] += narrow * (mid - block[ch][s]);
			}
		}
	protected:
		std::vector<float> env;
	private:
		inline void getSamples(const juce::AudioBuffer<float>& audioBuffer, float** block) {
			const auto samples = audioBuffer.getArrayOfReadPointers();
			for (auto ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
					block[ch][s] = std::abs(samples[ch][s]) * dbInGain(this->params[Gain]->denormalized(s));
		}

		const inline float makeAutoGain(const float atkSpeed, const float rlsSpeed) const noexcept {
			return 1.f + std::sqrt(rlsSpeed / atkSpeed);
		}
		const inline float processBias(const float value, const float biasV) const noexcept {
			return std::pow(value, biasV);
		}
		inline void processEnvelope(float** block, const float** samples, const int ch, const int numSamples,
			const float atkSpeed, const float rlsSpeed, const float gain, const float bias) {
			for (auto s = 0; s < numSamples; ++s)
				processEnvelopeSample(block, samples, ch, s, atkSpeed, rlsSpeed, gain, bias);
		}
		inline void processEnvelopeSample(float** block, const float** samples, const int ch, const int s,
			const float atkSpeed, const float rlsSpeed, const float gain, const float bias) {
			const auto gainVal = juce::Decibels::decibelsToGain(this->params[Gain]->denormalized(s));
			const auto envSample = processBias(gain * std::abs(samples[ch][s]) * gainVal, bias);
			if (env[ch] < envSample)
				env[ch] += atkSpeed * (envSample - env[ch]);
			else if (env[ch] > envSample)
				env[ch] += rlsSpeed * (envSample - env[ch]);
			block[ch][s] = env[ch];
		}
	};
}


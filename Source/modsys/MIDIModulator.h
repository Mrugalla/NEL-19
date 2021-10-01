#pragma once
#include "Utils.h"
#include "Parameter.h"
#include "Modulator.h"

namespace modSys2 {
	class MIDIPitchbendModulator :
		public Modulator
	{
		struct Filter {
			Filter() :
				env(0.f),
				p(0.f)
			{}
			void processBlock(float* block, int numSamples) noexcept {
				for (auto s = 0; s < numSamples; ++s) {
					env += p * (block[s] - env);
					block[s] = env;
				}
			}
			float env, p;
		};

		enum Param { NumParams };
	public:
		MIDIPitchbendModulator(juce::Identifier&& mID, float& _signal) :
			Modulator(std::move(mID)),
			filter(),
			signal(_signal),
			midiValue(0.f)
		{
			//this->params.push_back(lowLimitParam);
			//this->informParametersAboutAttachment();
		}
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			const auto secs = .05;
			filter.p = 1.f / static_cast<float>(sampleRate * secs);
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& block, juce::AudioPlayHead::CurrentPositionInfo&, const juce::MidiBuffer& midi) noexcept override {
			const auto numSamples = audioBuffer.getNumSamples();
			const auto numChannels = audioBuffer.getNumChannels();
			processBlockPitchbend(midi, block, numSamples);
			filter.processBlock(block.getWritePointer(0), numSamples);
			signal = *block.getReadPointer(0, numSamples - 1);
			processWidth(block, numChannels, numSamples);
		}
	protected:
		Filter filter;
		float& signal;
		float midiValue;

		void processBlockPitchbend(const juce::MidiBuffer& midi, juce::AudioBuffer<float>& _block, int numSamples) noexcept {
			static constexpr float pitchWheelInv = 1.f / static_cast<float>(0x3fff);
			auto block = _block.getWritePointer(0);
			int s = 0;
			for (auto m : midi) {
				const auto msg = m.getMessage();
				if (msg.isPitchWheel()) {
					const auto ts = static_cast<int>(msg.getTimeStamp());
					while (s <= ts) {
						midiValue = static_cast<float>(msg.getPitchWheelValue()) * pitchWheelInv;
						block[s] = midiValue;
						++s;
					}
				}
			}
			while (s < numSamples) {
				block[s] = midiValue;
				++s;
			}
		}

		void processWidth(juce::AudioBuffer<float>& block, int numChannels, int numSamples) noexcept {
			for (auto ch = 1; ch < numChannels; ++ch)
				juce::FloatVectorOperations::copy(block.getWritePointer(ch), block.getReadPointer(0), numSamples);
		}
	};

	class MIDINoteModulator :
		public Modulator
	{
		struct Filter {
			Filter() :
				env(0.f),
				p(0.f)
			{}
			void processBlock(float* block, int numSamples) noexcept {
				for (auto s = 0; s < numSamples; ++s) {
					env += p * (block[s] - env);
					block[s] = env;
				}
			}
			float env, p;
		};
		struct Osc {
			Osc() :
				fsInv(0.f),
				phase(0.f),
				inc(0.f)
			{}
			void setFreq(float f) noexcept { inc = tau * f * fsInv; }
			float processSample() noexcept {
				phase += inc;
				if (phase >= tau)
					phase -= tau;
				return approx::taylor_sin(phase);
			}
			float fsInv;
		protected:
			float phase, inc;
		};

		enum Param { Octaves, Semi, Fine, PhaseDist, RetuneSpeed, NumParams };
	public:
		MIDINoteModulator(juce::Identifier&& mID,
			const std::shared_ptr<Parameter>& octavesParam, const std::shared_ptr<Parameter>& semiParam,
			const std::shared_ptr<Parameter>& fineParam, const std::shared_ptr<Parameter>& phaseDistParam,
			const std::shared_ptr<Parameter>& retuneSpeedParam) :
			Modulator(std::move(mID)),
			osc(),
			filter(),
			noteValue(69.f)
		{
			this->params.push_back(octavesParam);
			this->params.push_back(semiParam);
			this->params.push_back(fineParam);
			this->params.push_back(phaseDistParam);
			this->params.push_back(retuneSpeedParam);
			this->informParametersAboutAttachment();
		}
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			const auto fsInv = 1.f / static_cast<float>(sampleRate);
			osc.fsInv = fsInv;
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo&, const juce::MidiBuffer& midi) noexcept override {
			const auto numSamples = audioBuffer.getNumSamples();
			const auto numChannels = audioBuffer.getNumChannels();
			auto block = _block.getWritePointer(0);
			int s = 0;
			for (auto m : midi) {
				const auto msg = m.getMessage();
				if (msg.isNoteOn()) {
					auto ts = static_cast<int>(msg.getTimeStamp());
					while (s <= ts) {
						noteValue = static_cast<float>(msg.getNoteNumber());
						block[s] = noteValue;
						++s;
					}
				}
			}
			while (s < numSamples) {
				block[s] = noteValue;
				++s;
			}
			prepareFilterFrequency();
			for (auto s = 0; s < numSamples; ++s) {
				const auto oct = params[Param::Octaves]->denormalized() * 12.f;
				const auto semi = params[Param::Semi]->denormalized();
				const auto fine = params[Param::Fine]->get() * 2.f - 1.f;
				block[s] = juce::jlimit(0.f, 127.f, block[s] + oct + semi + fine);
			}
			filter.processBlock(block, numSamples);
			for (auto s = 0; s < numSamples; ++s) {
				osc.setFreq(noteInHz(block[s])); // only recalc if freq changed tho
				block[s] = osc.processSample();
			}
			for (auto ch = 1; ch < numChannels; ++ch)
				juce::FloatVectorOperations::copy(_block.getWritePointer(ch), block, numSamples);
		}
	protected:
		Osc osc;
		Filter filter;
		float fsInv, noteValue;

		void prepareFilterFrequency() {
			const auto retuneSpeedInMs = params[Param::RetuneSpeed]->denormalized();
			if (retuneSpeedInMs == 0.f)
				filter.p = 1.f;
			else
				filter.p = 1000.f / (retuneSpeedInMs * Fs);
		}
	};
}

/*

velocity? modwheel?

MIDI Pitchwheel
	Smoothing
	Overshoot (Bounce)

*/
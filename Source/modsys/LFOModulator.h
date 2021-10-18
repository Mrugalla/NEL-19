#pragma once

namespace modSys2 {
	/*
	* an lfo modulator
	*/
	class LFOModulator :
		public Modulator
	{
		enum { Sync, Rate, Width, WaveTable, Polarity, Phase };
	public:
		LFOModulator(const juce::String& mID, const std::shared_ptr<Parameter>& syncParam,
			const std::shared_ptr<Parameter>& rateParam, const std::shared_ptr<Parameter>& wdthParam,
			const std::shared_ptr<Parameter>& waveTableParam, const std::shared_ptr<Parameter>& polarityParam,
			const std::shared_ptr<Parameter>& phaseParam, const param::MultiRange& ranges, WaveTables* wts) :
			Modulator(mID),
			multiRange(ranges),
			freeID(multiRange.getID("free")),
			syncID(multiRange.getID("sync")),
			phase(),
			waveTables(wts),
			fsInv(0.f),
			externalLatency(0)
		{
			this->params.push_back(syncParam);
			this->params.push_back(rateParam);
			this->params.push_back(wdthParam);
			this->params.push_back(waveTableParam);
			this->params.push_back(polarityParam);
			this->params.push_back(phaseParam);
			this->informParametersAboutAttachment();
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			phase.resize(numChannels);
			fsInv = 1.f / this->Fs;
			externalLatency = latency;
			for (auto& p : phase) p = 0.f;
		}
		void addStuff(std::vector<void*>& stuff, int stuffID) override {
			if (stuffID == 0) // replaces the wavetables pointer
				waveTables = static_cast<WaveTables*>(stuff[0]);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo& playHead, const juce::MidiBuffer&) noexcept override {
			auto block = _block.getArrayOfWritePointers();
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			const bool isFree = this->params[Sync]->get() < .5f;

			if (isFree) {
				const auto rateValue = juce::jlimit(0.f, 1.f, this->params[Rate]->get());
				const auto rate = multiRange(freeID).convertFrom0to1(rateValue);
				const auto inc = rate * fsInv;
				processPhase(block, inc, 0, numSamples);
			}
			else {
				const auto bpm = playHead.bpm;
				const auto bps = bpm / 60.;
				const auto quarterNoteLengthInSamples = Fs / bps;
				const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
				const auto rateValue = juce::jlimit(0.f, 1.f, this->params[Rate]->get());
				const auto rate = multiRange(syncID).convertFrom0to1(rateValue);
				const auto inc = 1.f / (static_cast<float>(barLengthInSamples) * rate);
				if (playHead.isPlaying) {
					// latency stuff
					const auto latencyLengthInQuarterNotes = static_cast<double>(externalLatency) / quarterNoteLengthInSamples;
					auto ppq = (playHead.ppqPosition - latencyLengthInQuarterNotes) * .25;
					while (ppq < 0.f) ++ppq;
					// latency stuff end
					const auto ppqCh = static_cast<float>(ppq) / rate;
					auto newPhase = (ppqCh - std::floor(ppqCh));
					phase[0] = newPhase;
				}
				processPhase(block, inc, 0, numSamples);
			}
			processWidth(block, numChannels, numSamples);
			processWaveTable(block, numChannels, numSamples);
		}
	protected:
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> phase;
		WaveTables* waveTables;
		float fsInv;
		size_t externalLatency;

		inline void processPhase(float** block, const float inc,
			const int ch, const int numSamples) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
				phase[ch] += inc;
				if (phase[ch] >= 1.f)
					--phase[ch];
				const auto phaseVal = this->params[Phase]->denormalized(s) *.5f;
				auto withOffset = phase[ch] + phaseVal;
				if (withOffset < 0.f)
					++withOffset;
				else if(withOffset >= 1.f)
					--withOffset;
				block[ch][s] = withOffset;
			}
		}

		inline void processWidth(float** block, const int numChannels, const int numSamples) noexcept {
			const auto width = this->params[Width]->denormalized() * .5f;
			if (width != 0) {
				for (auto ch = 1; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s) {
						block[ch][s] = block[0][s] + width;
						if (block[ch][s] >= 1.f)
							block[ch][s] -= 1.f;
					}
				
			}
			else
				for (auto ch = 1; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						block[ch][s] = block[0][s];
		}

		void processWaveTable(float** block, const int numChannels, const int numSamples) noexcept {
			const auto wtValue = static_cast<int>(this->params[WaveTable]->denormalized());
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block[ch][s] = waveTables->operator()(block[ch][s], wtValue);
			const bool polarityFlipped = this->params[Polarity]->get() > .5f;
			if (polarityFlipped)
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						block[ch][s] = 1.f - block[ch][s];
		}
	};
}

/*

*/
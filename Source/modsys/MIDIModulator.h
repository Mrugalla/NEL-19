#pragma once
#include "Utils.h"
#include "Parameter.h"
#include "Modulator.h"

namespace modSys2 {
	struct MIDIModulator :
		public Modulator
	{
		//MIDIModulator(const std::shared_ptr<Parameter>& makroParam) :
		MIDIModulator(juce::Identifier&& mID) :
			Modulator(std::move(mID))
		{
			//this->params.push_back(makroParam);
			this->informParametersAboutAttachment();
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& block, juce::AudioPlayHead::CurrentPositionInfo&, const juce::MidiBuffer& midi) noexcept override {
			//choose: pitchbend, modwheel, cc, note, velocity
			//range low limit, high limit, bias
		}
	};
}

/*
design this modulator! :)
*/
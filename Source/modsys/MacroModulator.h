#pragma once
#include "Utils.h"
#include "Parameter.h"
#include "Modulator.h"

namespace modSys2 {
	/*
* a macro modulator
*/
	struct MacroModulator :
		public Modulator
	{
		MacroModulator(const std::shared_ptr<Parameter>& makroParam) :
			Modulator(makroParam->id)
		{
			this->params.push_back(makroParam);
			this->informParametersAboutAttachment();
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& block, juce::AudioPlayHead::CurrentPositionInfo&, const juce::MidiBuffer& midi) noexcept override {
			const auto paramData = this->params[0]->data().getReadPointer(0);
			auto blockData = block.getArrayOfWritePointers();
			for (auto ch = 0; ch < block.getNumChannels(); ++ch)
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
					blockData[0][s] = paramData[s];
		}
	};
}

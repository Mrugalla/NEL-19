#pragma once
#include "Utils.h"

namespace modSys2 {
	/*
	* base class for a modulator's destination. can be parameter (mono) or arbitrary buffer
	*/
	struct Destination :
		public Identifiable
	{
		Destination(const juce::Identifier& dID, juce::AudioBuffer<float>& destBlck,
			float defaultAtten = 1.f, bool defaultBidirectional = false) :
			Identifiable(dID),
			attenuvertor(defaultAtten),
			bidirectional(defaultBidirectional),
			destBlock(destBlck)
		{}
		Destination(juce::Identifier&& dID, juce::AudioBuffer<float>& destBlck,
			float defaultAtten = 1.f, bool defaultBidirectional = false) :
			Identifiable(std::move(dID)),
			attenuvertor(defaultAtten),
			bidirectional(defaultBidirectional),
			destBlock(destBlck)
		{}

		void processBlock(const juce::AudioBuffer<float>& modBlock, const int numSamples) noexcept {
			auto dest = destBlock.getArrayOfWritePointers();
			const auto mod = modBlock.getArrayOfReadPointers();
			const auto atten = attenuvertor.get();
			if (isBidirectional())
				for (auto ch = 0; ch < destBlock.getNumChannels(); ++ch) {
					const auto modCh = ch % modBlock.getNumChannels();
					for (auto s = 0; s < numSamples; ++s)
						dest[ch][s] += (2.f * mod[modCh][s] - 1.f) * atten;
				}
			else
				for (auto ch = 0; ch < destBlock.getNumChannels(); ++ch) {
					const auto modCh = ch % modBlock.getNumChannels();
					for (auto s = 0; s < numSamples; ++s)
						dest[ch][s] += mod[modCh][s] * atten;
				}
		}
		inline void setValue(float value) noexcept { attenuvertor.set(value); }
		inline float getValue() const noexcept { return attenuvertor.get(); }
		inline void setBirectional(bool b) noexcept { bidirectional.set(b); }
		inline bool isBidirectional() const noexcept { return bidirectional.get(); }
	protected:
		juce::Atomic<float> attenuvertor;
		juce::Atomic<bool> bidirectional;
		juce::AudioBuffer<float>& destBlock;
	};
}

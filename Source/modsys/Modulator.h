#pragma once
#include "Utils.h"

namespace modSys2 {
	/*
* a module that can modulate destinations
*/
	struct Modulator :
		public Identifiable
	{
		Modulator(const juce::Identifier& mID) :
			Identifiable(mID),
			params(),
			destinations(),
			outValue(),
			Fs(1),
			active(false)
		{
		}
		Modulator(juce::Identifier&& mID) :
			Identifiable(std::move(mID)),
			params(),
			destinations(),
			outValue(),
			Fs(1),
			active(false)
		{
		}
		Modulator(const juce::String& mID) :
			Identifiable(mID),
			params(),
			destinations(),
			outValue(),
			Fs(1),
			active(false)
		{
		}
		Modulator(const Modulator& other) :
			Identifiable(other.id),
			params(other.params),
			destinations(other.destinations),
			outValue(other.outValue),
			Fs(other.Fs),
			active(other.active)
		{
		}
		// SET
		void informParametersAboutAttachment() {
			for (auto param : params)
				param->attachTo(&this->id);
		}
		virtual void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) {
			if (outValue.size() != numChannels) {
				outValue.clear();
				for (auto c = 0; c < numChannels; ++c)
					outValue.push_back(juce::Atomic<float>(0.f));
			}
			Fs = static_cast<float>(sampleRate);
		}
		void addDestination(std::shared_ptr<Parameter> dest, const float atten = 1.f, const bool bidirec = false) {
			addDestination(dest->id, dest->data(), atten, bidirec);
		}
		void addDestination(const juce::Identifier& dID, juce::AudioBuffer<float>& destBlock, float atten = 1.f, bool bidirec = false) {
			if (hasDestination(dID)) return;
			destinations.push_back(std::make_shared<Destination>(dID, destBlock, atten, bidirec));
		}
		void addDestination(juce::Identifier&& dID, juce::AudioBuffer<float>& destBlock, const float atten = 1.f, const bool bidirec = false) {
			if (hasDestination(std::move(dID))) return;
			destinations.push_back(std::make_shared<Destination>(std::move(dID), destBlock, atten, bidirec));
		}
		void removeDestination(const juce::Identifier& dID) {
			for (auto d = 0; d < destinations.size(); ++d) {
				auto dest = destinations[d].get();
				if (dest->id == dID) {
					destinations.erase(destinations.begin() + d);
					return;
				}
			}
		}
		void removeDestinations(const Modulator* other) {
			if (other == nullptr) return;
			const auto& otherParams = other->getParameters();
			for (const auto op : otherParams)
				if (hasDestination(op->id))
					removeDestination(op->id);
		}
		void setActive(bool a) noexcept {
			active = a;
			for (auto param : params)
				param->setActive(active);
		}
		// can be used to transfer arbritrary stuff from specific mod through generic mod
		virtual void addStuff(std::vector<void*>& stuff, int stuffID) {}
		// PROCESS
		void setAttenuvertor(const juce::Identifier& pID, const float value) {
			getDestination(pID)->setValue(value);
		}
		void setAttenuvertor(juce::Identifier&& pID, const float value) {
			getDestination(std::move(pID))->setValue(value);
		}
		virtual void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) noexcept = 0;
		void processDestinations(const juce::AudioBuffer<float>& modBlock, const int numSamples) noexcept {
			for (auto destination : destinations)
				destination->processBlock(modBlock, numSamples);
		}
		void storeOutValue(float** blockData, const int numSamples) noexcept {
			const auto lastSample = numSamples - 1;
			for (auto ch = 0; ch < outValue.size(); ++ch)
				outValue[ch].set(blockData[ch][lastSample]);
		}
		// GET
		std::shared_ptr<Destination> getDestination(const juce::Identifier& pID) noexcept {
			for (auto d = 0; d < destinations.size(); ++d)
				if (destinations[d]->id == pID)
					return destinations[d];
			return nullptr;
		}
		const std::shared_ptr<Destination> getDestination(const juce::Identifier& pID) const noexcept {
			for (auto d = 0; d < destinations.size(); ++d)
				if (destinations[d]->id == pID)
					return destinations[d];
			return nullptr;
		}
		bool hasDestination(const juce::Identifier& pID) const noexcept {
			return getDestination(pID) != nullptr;
		}
		float getAttenuvertor(const juce::Identifier& pID) const noexcept {
			const auto d = getDestination(pID);
			return d != nullptr ? d->getValue() : 0.f;
		}
		const std::vector<std::shared_ptr<Destination>>& getDestinations() const noexcept {
			return destinations;
		}
		float getOutValue(const int ch) const noexcept { return outValue[ch].get(); }
		std::vector<std::shared_ptr<Parameter>>& getParameters() noexcept { return params; }
		const std::vector<std::shared_ptr<Parameter>>& getParameters() const noexcept { return params; }
		bool usesParameter(const juce::Identifier& pID) const noexcept {
			for (const auto p : params)
				if (p->id == pID)
					return true;
			return false;
		}
		bool usesParameter(const Parameter& parameter) const noexcept {
			for (const auto p : params)
				if (*p == parameter)
					return true;
			return false;
		}
		bool modulates(Modulator& other) const noexcept {
			for (const auto d : destinations)
				if (other.usesParameter(d->id))
					return true;
			return false;
		}
		bool isActive() const noexcept { return active; }
	protected:
		std::vector<std::shared_ptr<Parameter>> params;
		std::vector<std::shared_ptr<Destination>> destinations;
		std::vector<juce::Atomic<float>> outValue;
		float Fs;
		bool active;
	};
}


/*

processBlock noexcept-able?

*/
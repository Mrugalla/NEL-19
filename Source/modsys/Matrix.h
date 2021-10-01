#pragma once
#include "Utils.h"

namespace modSys2 {
	/*
* the thing that handles everything in the end
*/
	struct Matrix {
		Matrix(juce::AudioProcessorValueTreeState& apvts) :
			parameters(),
			modulators(),
			curPosInfo(getDefaultPlayHead()),
			block(),
			selectedModulator(),
			waveTables()
		{
			const Type type;
			auto state = apvts.state;
			const auto numChildren = state.getNumChildren();
			for (auto c = 0; c < numChildren; ++c) {
				const auto pChild = apvts.state.getChild(c);
				if (pChild.hasType(type.param)) {
					const auto pID = pChild.getProperty(type.id).toString();
					parameters.push_back(
						std::make_shared<Parameter>(apvts, pID)
					);
				}
			}
		}
		Matrix(const Matrix& other) :
			parameters(other.parameters),
			modulators(other.modulators),
			curPosInfo(getDefaultPlayHead()),
			block(other.block),
			selectedModulator(other.selectedModulator),
			waveTables(other.waveTables)
		{
			// updating the waveTables pointer in all lfo mods
			std::vector<void*> waveTablePtr;
			waveTablePtr.push_back(&waveTables);
			juce::String lfoStr("LFO");
			for (auto modulator : modulators)
				if (modulator->id.toString().contains(lfoStr))
					modulator->addStuff(waveTablePtr, 0);
		}
		// SET
		void setWavetables(const std::vector<std::function<float(float)>>& wts, const int samplesPerCycle) {
			waveTables.setSamplesPerCycle(samplesPerCycle);
			for (auto& wt : wts)
				waveTables.addWaveTable(wt);
		}
		void prepareToPlay(const int numChannels, int blockSize, const double sampleRate, const size_t latency = 0) {
			if (blockSize < 1) blockSize = 1;
			for (auto& p : parameters)
				p->prepareToPlay(blockSize, sampleRate);
			for (auto& m : modulators)
				m->prepareToPlay(numChannels, sampleRate, latency);
			const auto channelCount = numChannels * numChannels;
			block.setSize(channelCount, blockSize, false, false, false);
		}
		void setSmoothingLengthInSamples(const juce::Identifier& pID, float length) noexcept {
			auto param = getParameter(pID);
			if (param != nullptr)
				param->setSmoothingLengthInSamples(length);
		}
		// SERIALIZE
		void setState(juce::AudioProcessorValueTreeState& apvts) {
			// BINARY TO VALUETREE
			auto state = apvts.state;
			const Type type;
			auto modSysChild = state.getChildWithName(type.modSys);
			if (!modSysChild.isValid()) return;
			auto numModulators = modSysChild.getNumChildren();
			for (auto m = 0; m < numModulators; ++m) {
				const auto modChild = modSysChild.getChild(m);
				const auto mID = modChild.getProperty(type.id).toString();
				const auto numDestinations = modChild.getNumChildren();
				for (auto d = 0; d < numDestinations; ++d) {
					const auto destChild = modChild.getChild(d);
					const auto dID = destChild.getProperty(type.id).toString();
					const auto dValue = static_cast<float>(destChild.getProperty(type.atten));
					const auto bidirec = destChild.getProperty(type.bidirec).toString() == "0" ? false : true;
					const auto destParameter = getParameter(dID);
					if (destParameter != nullptr)
						addDestination(mID, dID, dValue, bidirec);
					// if destination not a parameter
					// serialization has to be done externally
				}
			}
		}
		void getState(juce::AudioProcessorValueTreeState& apvts) {
			// VALUETREE TO BINARY
			auto state = apvts.state;
			const Type type;
			auto modSysChild = state.getChildWithName(type.modSys);
			if (!modSysChild.isValid()) {
				modSysChild = juce::ValueTree(type.modSys);
				state.appendChild(modSysChild, nullptr);
			}
			modSysChild.removeAllChildren(nullptr);

			for (const auto mod : modulators) {
				juce::ValueTree modChild(type.modulator);
				modChild.setProperty(type.id, mod->id.toString(), nullptr);
				const auto& destVec = mod->getDestinations();
				for (const auto d : destVec) {
					juce::ValueTree destChild(type.destination);
					destChild.setProperty(type.id, d->id.toString(), nullptr);
					destChild.setProperty(type.atten, d->getValue(), nullptr);
					destChild.setProperty(type.bidirec, d->isBidirectional() ? 1 : 0, nullptr);
					modChild.appendChild(destChild, nullptr);
				}
				modSysChild.appendChild(modChild, nullptr);
			}
		}
		// PARAMETERS
		void activateParameter(const juce::Identifier& pID, bool active) {
			auto param = getParameter(pID);
			if (param == nullptr) return;
			param->setActive(active);
		}
		// MODULATORS
		std::shared_ptr<Modulator> addMacroModulator(const juce::Identifier& pID) {
			auto macroParam = getParameter(pID);
			modulators.push_back(std::make_shared<MacroModulator>(macroParam));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addEnvelopeFollowerModulator(const juce::Identifier& gainPID,
			const juce::Identifier& atkPID, const juce::Identifier& rlsPID,
			const juce::Identifier& biasPID, const juce::Identifier& widthPID, int idx) {
			const auto gainP = getParameter(gainPID);
			const auto atkP = getParameter(atkPID);
			const auto rlsP = getParameter(rlsPID);
			const auto biasP = getParameter(biasPID);
			const auto widthP = getParameter(widthPID);
			const juce::String idString("EnvFol" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<EnvelopeFollowerModulator>(idString, gainP, atkP, rlsP, biasP, widthP));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addLFOModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID,
			const juce::Identifier& widthPID, const juce::Identifier& waveTablePID, const juce::Identifier& polarityPID,
			const juce::Identifier& phasePID, const param::MultiRange& ranges, int idx)
		{
			const auto syncP = getParameter(syncPID);
			const auto rateP = getParameter(ratePID);
			const auto widthP = getParameter(widthPID);
			const auto waveTableP = getParameter(waveTablePID);
			const auto polarityP = getParameter(polarityPID);
			const auto phaseP = getParameter(phasePID);
			const juce::String idString("LFO" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<LFOModulator>(idString, syncP, rateP, widthP, waveTableP, polarityP, phaseP, ranges, &waveTables));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addRandomModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID,
			const juce::Identifier& biasPID, const juce::Identifier& widthPID, const juce::Identifier& smoothPID,
			const param::MultiRange& ranges, int idx) {
			const auto syncP = getParameter(syncPID);
			const auto rateP = getParameter(ratePID);
			const auto biasP = getParameter(biasPID);
			const auto widthP = getParameter(widthPID);
			const auto smoothP = getParameter(smoothPID);
			const juce::String idString("Rand" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<RandomModulator>(idString, syncP, rateP, biasP, widthP, smoothP, ranges));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addPerlinModulator(const juce::Identifier& ratePID,
			const juce::Identifier& octavesPID, const juce::Identifier& widthPID,
			const param::MultiRange& ranges, int maxOctaves, int idx) {
			const auto rateP = getParameter(ratePID);
			const auto octavesP = getParameter(octavesPID);
			const auto widthP = getParameter(widthPID);
			const juce::String idString("Perlin" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<PerlinModulator>(idString, rateP, octavesP, widthP, ranges, maxOctaves));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addNoteModulator(const juce::Identifier& octPID, const juce::Identifier& semiPID,
			const juce::Identifier& finePID, const juce::Identifier& phaseDistPID, const juce::Identifier& retunePID,
			int idx) {
			const auto octP = getParameter(octPID);
			const auto semiP = getParameter(semiPID);
			const auto fineP = getParameter(finePID);
			const auto phaseDistP = getParameter(phaseDistPID);
			const auto retuneP = getParameter(retunePID);
			const juce::String idString("Note" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<MIDINoteModulator>(idString, octP, semiP, fineP, phaseDistP, retuneP));
			return modulators.back();
		}
		std::shared_ptr<Modulator> addMIDIPitchbendModulator(float& signal, int idx) {
			const juce::String idString("Pitchbend" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<MIDIPitchbendModulator>(idString, signal));
			return modulators.back();
		}
		void setModulatorActive(const juce::Identifier& mID, bool active) {
			auto mod = getModulator(mID);
			if (mod == nullptr) return;
			mod->setActive(active);
		}
		// MODIFY / REPLACE
		void selectModulator(const juce::Identifier& mID) noexcept { selectedModulator = getModulator(mID); }
		void addDestination(const juce::Identifier& mID, const juce::Identifier& dID, const float atten = 1.f, const bool bidirec = false) {
			auto param = getParameter(dID);
			auto thisMod = getModulator(mID);
			if (param != nullptr) { // destination belongs to a parameter
				for (auto t = 0; t < modulators.size(); ++t) { // search for mod i wanna add a dest to
					auto maybeThisMod = modulators[t];
					if (maybeThisMod == thisMod)  // found it.
						for (auto m = 0; m < modulators.size(); ++m) { // search for mod that has the parameter
							auto otherMod = modulators[m];
							if (otherMod->usesParameter(dID)) // found it
								if (otherMod != thisMod) // does the parameter belong to another mod?
								{
									thisMod->addDestination(param, atten, bidirec); // add dest
									if (t > m) // swap if needed
										std::swap(modulators[t], modulators[m]);
									if (otherMod->modulates(*thisMod)) // remove conflicting dests from other mod
									{
										otherMod->removeDestinations(thisMod.get());
									}
									return;
								}
								else return;
						}
				}
				// destination's parameter doesn't belong to any modulator, so just add
				thisMod->addDestination(param, atten, bidirec);
				return;
			}
		}
		void addDestination(const juce::Identifier& mID, const juce::Identifier& dID, juce::AudioBuffer<float>& destBlock, const float atten = 1.f, const bool bidirec = false) {
			// destination is not a parameter, so add unchecked
			auto thisMod = getModulator(mID);
			if (thisMod == nullptr) return;
			thisMod->addDestination(dID, destBlock, atten, bidirec);
		}
		void removeDestination(const juce::Identifier& mID, const juce::Identifier& dID) {
			auto mod = getModulator(mID);
			if (mod == nullptr) return;
			mod->removeDestination(dID);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioPlayHead* playHead, const juce::MidiBuffer& midi) noexcept {
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			const auto modBlockData = block.getArrayOfWritePointers();
			if (playHead) playHead->getCurrentPosition(curPosInfo);
			for (auto p : parameters)
				if (p->isActive())
					p->processBlock(numSamples);
			for (auto m : modulators)
				if (m->isActive()) {
					m->processBlock(audioBuffer, block, curPosInfo, midi);
					m->storeOutValue(modBlockData, numSamples);
					m->processDestinations(block, numSamples);
				}
			const auto lastSample = numSamples - 1;
			for (auto p : parameters)
				if (p->isActive()) {
					p->limit(numSamples);
					p->storeSumValue(lastSample);
				}
		}
		void processBlockEmpty() {
			const auto modBlockData = block.getArrayOfWritePointers();
			for (auto p : parameters)
				if (p->isActive())
					p->processBlockEmpty();
			for (auto m : modulators)
				if (m->isActive()) {
					//m->processBlockEmpty(block);
					m->storeOutValue(modBlockData, 1);
					m->processDestinations(block, 1);
				}
			for (auto p : parameters)
				if (p->isActive()) {
					p->limit(1);
					p->storeSumValue(1);
				}
			//for (auto m : modulators)
				//if (m->isActive()) {
					//m->processBlockEmpty(block);
					//m->storeOutValue(modBlockData);
					//m->processDestinationsEmpty(block);
				//}
			/*
			const auto lastSample = numSamples - 1;
			for (auto p : parameters)
				if (p->isActive()) {
					p->limit(numSamples);
					p->storeSumValue(lastSample);
				}
			*/
		}
		// GET
		std::shared_ptr<Modulator> getSelectedModulator() noexcept { return selectedModulator; }
		std::shared_ptr<Modulator> getModulator(const juce::Identifier& mID) noexcept {
			for (auto mod : modulators) {
				auto m = mod.get();
				if (m->hasID(mID))
					return mod;
			}
			return nullptr;
		}
		std::shared_ptr<Parameter> getParameter(const juce::Identifier& pID) {
			for (auto parameter : parameters)
				if (parameter->hasID(pID))
					return parameter;
			return nullptr;
		}
		inline const int getModulatorIndex(const juce::Identifier& mID) const noexcept {
			for (auto m = 0; m < modulators.size(); ++m)
				if (modulators[m]->hasID(mID))
					return m;
			return -1;
		}
		inline const int getParameterIndex(const juce::Identifier& pID) const noexcept {
			for (auto p = 0; p < parameters.size(); ++p)
				if (parameters[p]->hasID(pID))
					return p;
			return -1;
		}
		inline const float getParameterValue(const int p, const int s = 0) const noexcept {
			return parameters[p]->get(s);
		}
		// UI
		void selectModulatorOf(const juce::Identifier& pID) {
			for (auto modulator : modulators) {
				auto params = modulator->getParameters();
				for (auto param : params)
					if (param->hasID(pID)) {
						selectedModulator = modulator;
						return;
					}
			}
		}
		const WaveTables& getWaveTables() const noexcept { return waveTables; }
	protected:
		std::vector<std::shared_ptr<Parameter>> parameters;
		std::vector<std::shared_ptr<Modulator>> modulators;
		juce::AudioPlayHead::CurrentPositionInfo curPosInfo;
		juce::AudioBuffer<float> block;
		std::shared_ptr<Modulator> selectedModulator;
		WaveTables waveTables;

		void dbg(int idx = 0) {
			if (idx == 0) {
				juce::String str("mods with dests:\n");
				for (auto m : modulators) {
					str += m->id.toString() + ": ";
					auto dests = m->getDestinations();
					for (auto d : dests)
						str += d->id.toString() + ", ";
					str += "\n";
				}
				DBG(str);
			}
			else {
				juce::String str("mods active:\n");
				for (auto m : modulators)
					str += m->id.toString() + ": " + (m->isActive() ? "Y\n" : "N\n");
				DBG(str);
			}
		}
	};
}

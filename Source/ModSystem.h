#pragma once
#include <JuceHeader.h>
#include <functional>
#include <array>

namespace modSys2 {
	static constexpr float pi = 3.14159265359f;
	static constexpr float tau = 6.28318530718f;
	static float msInSamples(float ms, float Fs) noexcept { return ms * Fs * .001f; }
	static float hzInSamples(float hz, float Fs) noexcept { return Fs / hz; }
	static float hzInSlewRate(float hz, float Fs) noexcept { return 1.f / hzInSamples(hz, Fs); }
	static float dbInGain(float db) noexcept { return std::pow(10.f, db * .05f); }
	static float gainInDb(float gain) noexcept { return 20.f * std::log10(gain); }
	/* values and bias are normalized [0,1] lin curve at around bias = .6 for some reason */
	static float weight(float value, float bias) noexcept {
		if (value == 0.f) return 0.f;
		const float b0 = std::pow(value, bias);
		const float b1 = std::pow(value, 1 / bias);
		return b1 / b0;
	}
	static juce::AudioPlayHead::CurrentPositionInfo getDefaultPlayHead() noexcept {
		juce::AudioPlayHead::CurrentPositionInfo cpi;
		cpi.bpm = 120;
		cpi.editOriginTime = 0;
		cpi.frameRate = juce::AudioPlayHead::FrameRateType::fps25;
		cpi.isLooping = false;
		cpi.isPlaying = true;
		cpi.isRecording = false;
		cpi.ppqLoopEnd = 1;
		cpi.ppqLoopStart = 0;
		cpi.ppqPosition = 420;
		cpi.ppqPositionOfLastBarStart = 69;
		cpi.timeInSamples = 0;
		cpi.timeInSeconds = 0;
		cpi.timeSigDenominator = 1;
		cpi.timeSigNumerator = 1;
		return cpi;
	}
	
	/*
	* simple lerp
	*/
	namespace lerp {
		static inline float process(const float* data, const float x) noexcept {
			const auto xFloor = std::floor(x);
			const auto frac = x - xFloor;
			const auto x0 = static_cast<int>(xFloor);
			const auto x1 = x0 + 1;
			return data[x0] + frac * (data[x1] - data[x0]);
		}
	}

	/*
	* hermit cubic spline interpolation
	*/
	namespace spline {
		static constexpr int Size = 4;
		// hermit cubic spline
		// hornersheme
		// thx peter
		static float process(const float* data, const float x) noexcept {
			const auto iFloor = std::floor(x);
			auto i0 = static_cast<int>(iFloor);
			auto i1 = i0 + 1;
			auto i2 = i0 + 2;
			auto i3 = i0 + 3;

			const auto frac = x - iFloor;
			const auto v0 = data[i0];
			const auto v1 = data[i1];
			const auto v2 = data[i2];
			const auto v3 = data[i3];

			const auto c0 = v1;
			const auto c1 = .5f * (v2 - v0);
			const auto c2 = v0 - 2.5f * v1 + 2.f * v2 - .5f * v3;
			const auto c3 = 1.5f * (v1 - v2) + .5f * (v3 - v0);

			return ((c3 * frac + c2) * frac + c1) * frac + c0;
		}
		static float processChecked(const float* data, const float x, const int size) noexcept {
			auto iFloor = std::floor(x);
			auto i0 = static_cast<int>(iFloor);
			auto i1 = (i0 + 1) % size;
			auto i2 = (i0 + 2) % size;
			auto i3 = (i0 + 3) % size;

			auto frac = x - iFloor;
			auto v0 = data[i0];
			auto v1 = data[i1];
			auto v2 = data[i2];
			auto v3 = data[i3];

			auto c0 = v1;
			auto c1 = .5f * (v2 - v0);
			auto c2 = v0 - 2.5f * v1 + 2.f * v2 - .5f * v3;
			auto c3 = 1.5f * (v1 - v2) + .5f * (v3 - v0);

			return ((c3 * frac + c2) * frac + c1) * frac + c0;
		}
	};

	/*
	* base class for identifiable classes (parameter, modulator, certain components etc.)
	*/
	struct Identifiable {
		Identifiable(const juce::Identifier& tID) : id(tID) {}
		Identifiable(juce::Identifier&& tID) : id(tID) {}
		Identifiable(const juce::String& tID) : id(tID) {}
		Identifiable(juce::String&& tID) : id(std::move(tID)) {}
		bool hasID(const juce::Identifier& otherID) const noexcept { return id == otherID; }
		bool operator==(const Identifiable& other) const noexcept { return id == other.id; }
		juce::Identifier id;
	};

	/*
	* a class for managing a set of wavetables to choose from in the LFOs
	*/
	class WaveTables {
		enum { TableIdx, SampleIdx };
	public:
		WaveTables(const int _samplesPerCycle = 0) :
			tables(),
			samplesPerCycle(static_cast<float>(_samplesPerCycle))
		{}
		void setSamplesPerCycle(const int spc) { samplesPerCycle = static_cast<float>(spc); }
		void addWaveTable(const std::function<float(float)>& func) {
			tables.push_back(std::vector<float>());
			const auto tableIdx = tables.size() - 1;
			const auto spc = static_cast<int>(samplesPerCycle);
			tables[tableIdx].reserve(spc + spline::Size);
			auto x = 0.f;
			const auto inc = 1.f / samplesPerCycle;
			for (int i = 0; i < spc; ++i, x += inc)
				tables[tableIdx].emplace_back(func(x));
			for (int i = 0; i < spline::Size; ++i)
				tables[tableIdx].emplace_back(tables[tableIdx][i]);
		}
		float operator()(const float phase, const int tableIdx) const noexcept {
			const auto x = phase * samplesPerCycle;
			return spline::process(tables[tableIdx].data(), x);
		}
		const inline size_t numTables() const noexcept { return tables.size(); }
	protected:
		std::vector<std::vector<float>> tables;
		float samplesPerCycle;
	};

	/*
	* a parameter, its block and a lowpass filter
	*/
	struct Parameter :
		public Identifiable
	{
		struct Smoothing {
			Smoothing() :
				env(0.f),
				startValue(0.f), endValue(0.f), rangeValue(0.f),
				idx(0.f), length(0.f),
				isWorking(false)
			{}
			void setLength(const float samples) noexcept { length = samples; }
			void processBlock(float* block, const float dest, const int numSamples) noexcept {
				if (!isWorking) {
					if (env == dest)
						return bypass(block, dest, numSamples);
					setNewDestination(dest);
				}
				else if (length == 0)
					return bypass(block, dest, numSamples);
				processWork(block, dest, numSamples);
			}
		protected:
			float env, startValue, endValue, rangeValue, idx, length;
			bool isWorking;

			void setNewDestination(const float dest) noexcept {
				startValue = env;
				endValue = dest;
				rangeValue = endValue - startValue;
				idx = 0.f;
				isWorking = true;
			}
			void processWork(float* block, const float dest, const int numSamples) noexcept {
				const auto lenInv = 1.f / length;
				for (auto s = 0; s < numSamples; ++s) {
					if (idx >= length) {
						if (dest != endValue)
							setNewDestination(dest);
						else {
							for (auto s1 = s; s1 < numSamples; ++s1)
								block[s1] = endValue;
							isWorking = false;
							return;
						}
					}
					const auto curve = .5f - approx::taylor_cos(idx * pi * lenInv) * .5f;
					block[s] = env = startValue + curve * rangeValue;
					++idx;
				}
			}
			void bypass(float* block, const float dest, const int numSamples) noexcept {
				for (auto s = 0; s < numSamples; ++s)
					block[s] = dest;
			}
		};

		Parameter(juce::AudioProcessorValueTreeState& apvts, juce::Identifier&& pID) :
			Identifiable(std::move(pID)),
			parameter(apvts.getRawParameterValue(pID)),
			rap(apvts.getParameter(pID)),
			attachedModulator(nullptr),
			sumValue(0.f),
			block(),
			smoothing(),
			Fs(1.f),
			blockSize(0),
			active(false)
		{}
		// SET
		void prepareToPlay(const int blockS, double sampleRate) {
			Fs = static_cast<float>(sampleRate);
			blockSize = blockS;
			block.setSize(1, blockSize, false, false);
		}
		void setSmoothingLengthInSamples(const float length) noexcept { smoothing.setLength(length); }
		void setActive(bool a) { active = a; }
		// PROCESS
		void attachTo(juce::Identifier* mID) noexcept { attachedModulator = mID; }
		void processBlock(const int numSamples) noexcept {
			const auto targetValue = parameter->load();
			const auto normalised = rap->convertTo0to1(targetValue);
			smoothing.processBlock(block.getWritePointer(0), normalised, numSamples);
		}
		void storeSumValue(const int lastSample) noexcept { sumValue.set(get(lastSample)); }
		void set(const float value, const int s) noexcept {
			auto samples = block.getWritePointer(0);
			samples[s] = value;
		}
		void limit(const int numSamples) noexcept {
			auto blockData = data().getWritePointer(0);
			for (auto s = 0; s < numSamples; ++s)
				blockData[s] = blockData[s] < 0.f ? 0.f : blockData[s] > 1.f ? 1.f : blockData[s];
		}
		// GET NORMAL
		float getSumValue() const noexcept { return sumValue.get(); }
		inline const float get(const int s = 0) const noexcept { return *block.getReadPointer(0, s); }
		juce::AudioBuffer<float>& data() noexcept { return block; }
		const juce::AudioBuffer<float>& data() const noexcept { return block; }
		bool isActive() const noexcept { return active; }
		const juce::Identifier* getAttachedModulatorID() const noexcept { return attachedModulator; }
		// GET CONVERTED
		float denormalized(const int s = 0) const noexcept {
			return rap->convertFrom0to1(juce::jlimit(0.f, 1.f, get(s)));
		}
	protected:
		std::atomic<float>* parameter;
		const juce::RangedAudioParameter* rap;
		juce::Identifier* attachedModulator;
		juce::Atomic<float> sumValue;
		juce::AudioBuffer<float> block;
		Smoothing smoothing;
		float Fs;
		int blockSize;
		bool active;
	};

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
		void addDestination(std::shared_ptr<Parameter>& dest, float atten = 1.f, bool bidirec = false) {
			addDestination(dest->id, dest->data(), atten, bidirec);
		}
		void addDestination(const juce::Identifier& dID, juce::AudioBuffer<float>& destBlock, float atten = 1.f, bool bidirec = false) {
			if (hasDestination(dID)) return;
			destinations.push_back(std::make_shared<Destination>(dID, destBlock, atten, bidirec));
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
		virtual void addStuff(std::vector<void*>& stuff, int stuffID){}
		// PROCESS
		void setAttenuvertor(const juce::Identifier& pID, const float value) {
			getDestination(pID)->setValue(value);
		}
		virtual void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& block, juce::AudioPlayHead::CurrentPositionInfo& playHead) = 0;
		void processDestinations(const juce::AudioBuffer<float>& modBlock, const int numSamples) noexcept {
			for (auto& destination : destinations) {
				destination->processBlock(modBlock, numSamples);
			}	
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

	/*
	* a macro modulator
	*/
	struct MacroModulator :
		public Modulator
	{
		MacroModulator(const std::shared_ptr<Parameter>& makroParam) :
			Modulator(makroParam->id)
		{
			params.push_back(makroParam);
			informParametersAboutAttachment();
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& block, juce::AudioPlayHead::CurrentPositionInfo&) override {
			const auto paramData = params[0]->data().getReadPointer(0);
			auto blockData = block.getArrayOfWritePointers();
			for(auto ch = 0; ch < block.getNumChannels(); ++ch)
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
					blockData[0][s] = paramData[s];
			storeOutValue(blockData, audioBuffer.getNumSamples());
		}
	};

	/*
	* an envelope follower modulator
	*/
	class EnvelopeFollowerModulator :
		public Modulator
	{
		enum { Gain, Attack, Release, Bias, Width };
	public:
		EnvelopeFollowerModulator(const juce::String& mID, const std::shared_ptr<Parameter>& inputGain,
			const std::shared_ptr<Parameter>& atkParam, const std::shared_ptr<Parameter>& rlsParam,
			const std::shared_ptr<Parameter>& biasParam, const std::shared_ptr<Parameter>& widthParam) :
			Modulator(mID),
			env()
		{
			params.push_back(inputGain);
			params.push_back(atkParam);
			params.push_back(rlsParam);
			params.push_back(biasParam);
			params.push_back(widthParam);
			informParametersAboutAttachment();
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			env.resize(numChannels, 0.f);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo&) override {
			auto block = _block.getArrayOfWritePointers();
			auto numChannels = audioBuffer.getNumChannels();
			numChannels = numChannels < 3 ? numChannels : 2;
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			const auto samples = audioBuffer.getArrayOfReadPointers();
			
			const auto atkInMs = params[Attack]->denormalized(0);
			const auto rlsInMs = params[Release]->denormalized(0);
			const auto bias = 1.f - juce::jlimit(0.f, 1.f, params[Bias]->get());
			const auto atkInSamples = msInSamples(atkInMs, Fs);
			const auto rlsInSamples = msInSamples(rlsInMs, Fs);
			const auto atkSpeed = 1.f / atkInSamples;
			const auto rlsSpeed = 1.f / rlsInSamples;
			const auto gain = makeAutoGain(atkSpeed, rlsSpeed);
			for (auto ch = 0; ch < numChannels; ++ch)
				processEnvelope(block, samples, ch, numSamples, atkSpeed, rlsSpeed, gain, bias);
			const auto narrow = 1.f - juce::jlimit(0.f, 1.f, params[Width]->get());
			const auto channelInv = 1.f / numChannels;
			for (auto s = 0; s < numSamples; ++s) {
				auto mid = 0.f;
				for (auto ch = 0; ch < numChannels; ++ch)
					mid += block[ch][s];
				mid *= channelInv;
				for (auto ch = 0; ch < numChannels; ++ch)
					block[ch][s] += narrow * (mid - block[ch][s]);
			}
			storeOutValue(block, numSamples);
		}
	protected:
		std::vector<float> env;
	private:
		inline void getSamples(const juce::AudioBuffer<float>& audioBuffer, float** block) {
			const auto samples = audioBuffer.getArrayOfReadPointers();
			for (auto ch = 0; ch < audioBuffer.getNumChannels(); ++ch)
				for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
					block[ch][s] = std::abs(samples[ch][s]) * dbInGain(params[Gain]->denormalized(s));
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
			const auto gainVal = juce::Decibels::decibelsToGain(params[Gain]->denormalized(s));
			const auto envSample = processBias(gain * std::abs(samples[ch][s]) * gainVal, bias);
			if (env[ch] < envSample)
				env[ch] += atkSpeed * (envSample - env[ch]);
			else if (env[ch] > envSample)
				env[ch] += rlsSpeed * (envSample - env[ch]);
			block[ch][s] = env[ch];
		}
	};

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
			fsInv(0.f)
		{
			params.push_back(syncParam);
			params.push_back(rateParam);
			params.push_back(wdthParam);
			params.push_back(waveTableParam);
			params.push_back(polarityParam);
			params.push_back(phaseParam);
			informParametersAboutAttachment();
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			phase.resize(numChannels);
			fsInv = 1.f / this->Fs;
			externalLatency = latency;
		}
		void addStuff(std::vector<void*>& stuff, int stuffID) override {
			if (stuffID == 0) // replace the wavetables pointer
				waveTables = static_cast<WaveTables*>(stuff[0]);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
			auto block = _block.getArrayOfWritePointers();
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			const bool isFree = params[Sync]->get() < .5f;

			if (isFree) {
				const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get());
				const auto rate = multiRange(freeID).convertFrom0to1(rateValue);
				const auto inc = rate * fsInv;
				processPhase(block, inc, 0, numSamples);
			}
			else {
				const auto bpm = playHead.bpm;
				const auto bps = bpm / 60.;
				const auto quarterNoteLengthInSamples = Fs / bps;
				const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
				const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get());
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
			storeOutValue(block, numSamples);
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
				const auto phaseVal = params[Phase]->get(s);
				const auto withOffset = phase[ch] + phaseVal;
				block[ch][s] = withOffset < 1.f ? withOffset : withOffset - 1.f;
			}
		}

		inline void processWidth(float** block, const int numChannels, const int numSamples) noexcept {
			const auto width = params[Width]->denormalized() * .5f;
			if (width != 0) {
				for (auto ch = 1; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						block[ch][s] = block[0][s] + width;
				if (width < 0.f)
					for (auto ch = 1; ch < numChannels; ++ch)
						for (auto s = 0; s < numSamples; ++s)
							while (block[ch][s] < 0.f) ++block[ch][s];
				else
					for (auto ch = 1; ch < numChannels; ++ch)
						for (auto s = 0; s < numSamples; ++s)
							while (block[ch][s] >= 1.f) --block[ch][s];
			}
			else
				for (auto ch = 1; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						block[ch][s] = block[0][s];
		}

		void processWaveTable(float** block, const int numChannels, const int numSamples) noexcept {
			const auto wtValue = static_cast<int>(params[WaveTable]->denormalized());
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block[ch][s] = waveTables->operator()(block[ch][s], wtValue);
			const bool polarityFlipped = params[Polarity]->get() > .5f;
			if (polarityFlipped)
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						block[ch][s] = 1.f - block[ch][s];
		}
	};

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
			phase(1.f), fsInv(1.f), rateInHz(1.f)
		{
			params.push_back(syncParam);
			params.push_back(rateParam);
			params.push_back(biasParam);
			params.push_back(widthParam);
			params.push_back(smoothParam);
			informParametersAboutAttachment();
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate, const size_t latency) override {
			Modulator::prepareToPlay(numChannels, sampleRate, latency);
			static constexpr auto filterOrder = 3;
			smoothing.resize(numChannels, filterOrder);
			randValue.resize(numChannels, 0);
			fsInv = 1.f / Fs;
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
			auto block = _block.getArrayOfWritePointers();
			auto numChannels = audioBuffer.getNumChannels();
			numChannels = numChannels < 3 ? numChannels : 2;
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;

			const bool isFree = params[Sync]->get(0) < .5f;
			const auto biasValue = juce::jlimit(0.f, 1.f, params[Bias]->get(0));

			if (isFree) {
				const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get(0));
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
				const auto ppq = playHead.ppqPosition * .25;
				const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get(0));
				rateInHz = multiRange(syncID).convertFrom0to1(rateValue);
				const auto inc = 1.f / (static_cast<float>(barLengthInSamples) * rateInHz);
				const auto ppqCh = static_cast<float>(ppq) / rateInHz;
				auto newPhase = (ppqCh - std::floor(ppqCh));
				phase = newPhase;
				synthesizePhase(block, inc, numSamples);
				synthesizeRandomSignal(block, biasValue, numSamples, 0);
			}
			processWidth(block, biasValue, numChannels, numSamples);
			processSmoothing(block, numChannels, numSamples);
			storeOutValue(block, numSamples);
		}
	protected:
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> randValue;
		std::vector<LP1PoleOrder> smoothing;
		juce::Random rand;
		float phase, fsInv, rateInHz;
	private:
		inline void synthesizePhase(float** block, const float inc,
			const int numSamples) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
				phase += inc;
				block[1][s] = phase >= 1.f ? 1.f : 0.f;
				if (block[1][s] == 1.f)
					--phase;
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
			const auto widthValue = juce::jlimit(0.f, 1.f, params[Width]->get(0));
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
			const auto smoothValue = weight(params[Smooth]->get(0), magicNumber);
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

	/*
	* a randomized modulator (1d perlin noise)
	*/
	class PerlinModulator :
		public Modulator
	{
		enum { Sync, Rate, Octaves, Width };
	public:
		PerlinModulator(const juce::String& mID, const std::shared_ptr<Parameter>& syncParam,
			const std::shared_ptr<Parameter>& rateParam, const std::shared_ptr<Parameter>& octavesParam,
			const std::shared_ptr<Parameter>& widthParam,
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
			params.push_back(syncParam);
			params.push_back(rateParam);
			params.push_back(octavesParam);
			params.push_back(widthParam);
			informParametersAboutAttachment();

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
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioBuffer<float>& _block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
			auto block = _block.getArrayOfWritePointers();
			auto numChannels = audioBuffer.getNumChannels();
			numChannels = numChannels > 3 ? numChannels : 2;
			const auto maxChannel = numChannels - 1;
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;

			const bool isFree = params[Sync]->get(0) < .5f;
			setOctaves(juce::jlimit(0, maxOctaves, static_cast<int>(params[Octaves]->denormalized())));

			if (isFree) {
				const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get(0));
				const auto rateInHz = multiRange(freeID).convertFrom0to1(rateValue);
				const auto inc = rateInHz * fsInv;
				synthesizePhase(block[maxChannel], inc, numSamples);
				synthesizeRandSignal(block, numChannels, numSamples);
			}
			else {
				const auto bpm = playHead.bpm;
				const auto bps = bpm / 60.;
				const auto quarterNoteLengthInSamples = Fs / bps;
				const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
				const auto ppq = playHead.ppqPosition * .25;
				const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get(0));
				const auto rateInHz = multiRange(syncID).convertFrom0to1(rateValue);
				const auto inc = 1.f / (static_cast<float>(barLengthInSamples) * rateInHz);
				const auto ppqCh = static_cast<float>(ppq) / rateInHz;
				auto newPhase = (ppqCh - std::floor(ppqCh));
				phase = newPhase;
				synthesizePhase(block[maxChannel], inc, numSamples);
				synthesizeRandSignal(block, numChannels, numSamples);
			}
			processWidth(block, numChannels, numSamples);
			storeOutValue(block, numSamples);
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
			const auto width = params[Width]->get();
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
					block[ch][s] = block[0][s] + width * (block[ch][s] - block[0][s]);
		}
	};

	/*
	* some identifiers used for serialization
	*/
	struct Type {
		Type() :
			modSys("MODSYS"),
			modulator("MOD"),
			destination("DEST"),
			id("id"),
			atten("atten"),
			bidirec("bidirec"),
			param("PARAM")
		{}
		const juce::Identifier modSys;
		const juce::Identifier modulator;
		const juce::Identifier destination;
		const juce::Identifier id;
		const juce::Identifier atten;
		const juce::Identifier bidirec;
		const juce::Identifier param;
	};
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
			for(auto modulator: modulators)
				if (modulator->id.toString().contains(lfoStr))
					modulator->addStuff(waveTablePtr, 0);
		}
		// SET
		void setWavetables(const std::vector<std::function<float(float)>>& wts, const int samplesPerCycle) {
			waveTables.setSamplesPerCycle(samplesPerCycle);
			for (auto& wt : wts)
				waveTables.addWaveTable(wt);
		}
		void prepareToPlay(const int numChannels, const int blockSize, const double sampleRate, const size_t latency = 0) {
			for (auto& p : parameters)
				p->prepareToPlay(blockSize, sampleRate);
			for (auto& m : modulators)
				m->prepareToPlay(numChannels, sampleRate, latency);
			const auto channelCount = numChannels * numChannels;
			block.setSize(channelCount, blockSize, false, false, false);
		}
		void setSmoothingLengthInSamples(const juce::Identifier& pID, float length) noexcept {
			auto param = getParameter(pID);
			if(param != nullptr)
				param->setSmoothingLengthInSamples(length);
		}
		// SERIALIZE
		void setState(juce::AudioProcessorValueTreeState& apvts) {
			// BINARY TO VALUETREE
			auto& state = apvts.state;
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
					if(destParameter != nullptr)
						addDestination(mID, dID, dValue, bidirec);
					// what if destination not parameter??
					// should non-parameter destinations be serializable?
				}
			}
		}
		void getState(juce::AudioProcessorValueTreeState& apvts) {
			// VALUETREE TO BINARY
			auto& state = apvts.state;
			const Type type;
			auto modSysChild = state.getChildWithName(type.modSys);
			if (!modSysChild.isValid()) {
				modSysChild = juce::ValueTree(type.modSys);
				state.appendChild(modSysChild, nullptr);
			}
			modSysChild.removeAllChildren(nullptr);

			for (const auto& mod : modulators) {
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
			if(param != nullptr)
				param->setActive(active);
		}
		// MODULATORS
		std::shared_ptr<Modulator> addMacroModulator(const juce::Identifier& pID) {
			auto macroParam = getParameter(pID);
			modulators.push_back(std::make_shared<MacroModulator>(macroParam));
			//auto lastMod = modulators[modulators.size() - 1];
			//macroParam->attachTo(&lastMod->id);
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
			/*
			auto lastMod = modulators[modulators.size() - 1];
			
			gainP->attachTo(&lastMod->id);
			atkP->attachTo(&lastMod->id);
			rlsP->attachTo(&lastMod->id);
			biasP->attachTo(&lastMod->id);
			widthP->attachTo(&lastMod->id);
			*/
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
			/*
			auto lastMod = modulators[modulators.size() - 1];
			syncP->attachTo(&lastMod->id);
			rateP->attachTo(&lastMod->id);
			widthP->attachTo(&lastMod->id);
			waveTableP->attachTo(&lastMod->id);
			polarityP->attachTo(&lastMod->id);
			phaseP->attachTo(&lastMod->id);
			*/
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
			/*
			auto lastMod = modulators[modulators.size() - 1];
			syncP->attachTo(&lastMod->id);
			rateP->attachTo(&lastMod->id);
			biasP->attachTo(&lastMod->id);
			widthP->attachTo(&lastMod->id);
			smoothP->attachTo(&lastMod->id);
			*/
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addPerlinModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID,
			const juce::Identifier& octavesPID, const juce::Identifier& widthPID,
			const param::MultiRange& ranges, int maxOctaves, int idx) {
			const auto syncP = getParameter(syncPID);
			const auto rateP = getParameter(ratePID);
			const auto octavesP = getParameter(octavesPID);
			const auto widthP = getParameter(widthPID);
			const juce::String idString("Perlin" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<PerlinModulator>(idString, syncP, rateP, octavesP, widthP, ranges, maxOctaves));
			/*
			auto lastMod = modulators[modulators.size() - 1];
			syncP->attachTo(&lastMod->id);
			rateP->attachTo(&lastMod->id);
			octavesP->attachTo(&lastMod->id);
			widthP->attachTo(&lastMod->id);
			*/
			return modulators[modulators.size() - 1];
		}
		void setModulatorActive(const juce::Identifier& mID, bool active) {
			auto mod = getModulator(mID);
			mod->setActive(active);
			return;
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
				thisMod->addDestination(dID, destBlock, atten, bidirec);
				return;
		}
		void removeDestination(const juce::Identifier& mID, const juce::Identifier& dID) {
			getModulator(mID)->removeDestination(dID);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, juce::AudioPlayHead* playHead) {
			const auto numChannels = audioBuffer.getNumChannels();
			const auto numSamples = audioBuffer.getNumSamples();
			if (playHead) playHead->getCurrentPosition(curPosInfo);
			for (auto& p : parameters)
				if(p->isActive())
					p->processBlock(numSamples);
			for (auto& m : modulators)
				if (m->isActive()) {
					m->processBlock(audioBuffer, block, curPosInfo);
					m->processDestinations(block, numSamples);
				}
			const auto lastSample = numSamples - 1;
			for (auto& p : parameters)
				if (p->isActive()) {
					p->limit(numSamples);
					p->storeSumValue(lastSample);
				}
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
				if (parameter.get()->hasID(pID))
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
		inline const float getParameterValue(const int p, const int s) const noexcept {
			return parameters[p]->get(s);
		}
		// UI
		void selectModulatorOf(const juce::Identifier& pID) {
			for (auto modulator : modulators) {
				auto params = modulator->getParameters();
				for (auto param : params)
					if (*param == pID) {
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

	/* to do:
	* 
	* signalsmith:
	* "if you're slowing down (a.k.a. upsampling) you need to lowpass at your original signal's Nyquist
	* but when you're speeding up, you need to lowpass at the new Nyquist."
	* 
	* temposync / free phase mod
	*	add phase offset parameter (rand)
	* 
	* write this-> in front of all things of base classes in modulator classes
	* 
	* perlin noise temposync
	*	even needed?
	*	only phasors atm
	* 
	* envelope follower mod makes no sense on [-1,1] range
	*	current solution: it goes [0,1] only
	* 
	* perlin noise modulator doesn't compensate for spline overshoot anymore (* .8)
	* 
	* dryWetMix needs sqrt(x) and sqrt(1 - x) all the time
	* currently in vibrato for each channel and sample (bad!)
	*	solution 1:
	*		rewrite modulation system to convert to sqrt before smoothing
	*	solution 2:
	*		make sqrt lookup table, so modSys can stay the way it is
	*	solution 3:
	*		make buffers for both calculations. (2)
			only calculate once per block
	* 
	* all destinations have: bias / weight
	*
	* rewrite parameter so has functions to give back min and max of range
	*	then implement maxOctaves in processBlock of perlinMod to get from there
	* 
	* envelopeFollowerModulator
	*	rewrite db to gain calculation so that it happens before parameter smoothing instead
	*	add lookahead parameter
	*
	* lfoModulator
	*	add pump curve wavetable
	*
	* randModulator
	*	try rewrite with spline interpolator instead lowpass
	* 
	* perlinModulator
	*	add method for calculating autogain spline overshoot
	* 
	* all wavy mods (especially temposync ones)
	*	phase parameter
	*/
}
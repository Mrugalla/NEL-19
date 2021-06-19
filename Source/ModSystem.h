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
	enum ChannelSetup { Left, Right, Mid, Side };

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
	* makes sure things can be identified
	*/
	struct Identifiable {
		Identifiable(const juce::Identifier& tID) : id(tID) {}
		Identifiable(const juce::String& tID) : id(tID) {}
		bool hasID(const juce::Identifier& otherID) const noexcept { return id == otherID; }
		bool operator==(const Identifiable& other) const noexcept { return id == other.id; }
		juce::Identifier id;
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
					const auto curve = .5f - std::cos(idx * pi / length) * .5f;
					block[s] = env = startValue + curve * rangeValue;
					++idx;
				}
			}
			void bypass(float* block, const float dest, const int numSamples) noexcept {
				for (auto s = 0; s < numSamples; ++s)
					block[s] = dest;
			}
		};

		Parameter(juce::AudioProcessorValueTreeState& apvts, const juce::String& pID) :
			Identifiable(pID),
			parameter(apvts.getRawParameterValue(pID)),
			rap(apvts.getParameter(pID)),
			sumValue(0.f),
			block(),
			smoothing(),
			Fs(1.f)
		{}
		// SET
		void prepareToPlay(const int blockSize, double sampleRate) {
			Fs = static_cast<float>(sampleRate);
			block.resize(blockSize);
		}
		void setSmoothingLengthInSamples(const float length) noexcept { smoothing.setLength(length); }
		// PROCESS
		void processBlock(const int numSamples) noexcept {
			const auto targetValue = parameter->load();
			const auto normalised = rap->convertTo0to1(targetValue);
			smoothing.processBlock(block.data(), normalised, numSamples);
		}
		void storeSumValue(const int lastSample) noexcept { sumValue.set(block[lastSample]); }
		void set(const float value, const int s) noexcept { block[s] = value; }
		void limit(const int numSamples) noexcept {
			for (auto s = 0; s < numSamples; ++s)
				block[s] = block[s] < 0.f ? 0.f : block[s] > 1.f ? 1.f : block[s];
		}
		// GET NORMAL
		float getSumValue() const noexcept { return sumValue.get(); }
		float get(const int s = 0) const noexcept { return block[s]; }
		std::vector<float>& data() noexcept { return block; }
		const std::vector<float>& data() const noexcept { return block; }
		// GET CONVERTED
		float denormalized(const int s = 0) const noexcept {
			return rap->convertFrom0to1(juce::jlimit(0.f, 1.f, block[s]));
		}
	protected:
		std::atomic<float>* parameter;
		const juce::RangedAudioParameter* rap;
		juce::Atomic<float> sumValue;
		std::vector<float> block;
		Smoothing smoothing;
		float Fs;
	};

	/*
	* base class for a modulator's destination. can be parameter (mono) or arbitrary buffer
	*/
	struct Destination :
		public Identifiable
	{
		Destination(const juce::Identifier& dID, std::vector<float>& destBlck, ChannelSetup defaultSetup, float defaultAtten = 1.f, bool defaultBidirectional = false) :
			Identifiable(dID),
			attenuvertor(defaultAtten),
			bidirectional(defaultBidirectional),
			destBlock(destBlck),
			channelSetup(defaultSetup)
		{
		}
		Destination(Destination&) = default;
		const ChannelSetup getChannelSetup() const noexcept { return channelSetup; }
		void processBlock(float* modBlock, const int numSamples) noexcept {
			const auto atten = attenuvertor.get();
			if (isBidirectional())
				for (auto s = 0; s < numSamples; ++s)
					destBlock[s] += (2.f * modBlock[s] - 1.f) * atten;
			else
				for (auto s = 0; s < numSamples; ++s)
					destBlock[s] += modBlock[s] * atten;
		}
		void setValue(float value) noexcept { attenuvertor.set(value); }
		float getValue() const noexcept { return attenuvertor.get(); }
		void setBirectional(bool b) noexcept { bidirectional.set(b); }
		bool isBidirectional() const noexcept { return bidirectional.get(); }
	protected:
		juce::Atomic<float> attenuvertor;
		juce::Atomic<bool> bidirectional;
		std::vector<float>& destBlock;
		const ChannelSetup channelSetup;
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
			Fs(1)
		{
		}
		Modulator(const juce::String& mID) :
			Identifiable(mID),
			params(),
			destinations(),
			outValue(),
			Fs(1)
		{
		}
		// SET
		virtual void prepareToPlay(const int numChannels, const double sampleRate) {
			if (outValue.size() != numChannels) {
				outValue.clear();
				for (auto c = 0; c < numChannels; ++c)
					outValue.push_back(juce::Atomic<float>(0.f));
			}
			Fs = static_cast<float>(sampleRate);
		}
		void addDestination(std::shared_ptr<Parameter>& dest, ChannelSetup channelSetup, float atten = 1.f, bool bidirec = false) {
			addDestination(dest->id, dest->data(), channelSetup, atten, bidirec);
		}
		void addDestination(const juce::Identifier& dID, std::vector<float>& destBlock, ChannelSetup channelSetup, float atten = 1.f, bool bidirec = false) {
			if (hasDestination(dID)) return;
			destinations.push_back(std::make_shared<Destination>(dID, destBlock, channelSetup, atten, bidirec));
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
		virtual void addStuff(const juce::String& /*sID*/, const VectorAnything& /*stuff*/) {}
		// PROCESS
		void setAttenuvertor(const juce::Identifier& pID, const float value) {
			getDestination(pID)->setValue(value);
		}
		virtual void processBlock(const juce::AudioBuffer<float>& audioBuffer, float** block, juce::AudioPlayHead::CurrentPositionInfo& playHead) = 0;
		void processDestinations(float** block, const int numSamples) noexcept {
			for (auto& destination : destinations) {
				const auto channelSetup = destination->getChannelSetup();
				destination->processBlock(block[channelSetup], numSamples);
			}	
		}
		void generateMidSide(float** block, const int numChannels, const int numSamples) noexcept {
			if (numChannels != 2) return;
			juce::FloatVectorOperations::copy(block[2], block[0], numSamples);
			juce::FloatVectorOperations::copy(block[3], block[0], numSamples);
			juce::FloatVectorOperations::add(block[2], block[1], numSamples);
			juce::FloatVectorOperations::subtract(block[3], block[1], numSamples);
			juce::FloatVectorOperations::multiply(block[2], .5f, numSamples);
			juce::FloatVectorOperations::multiply(block[3], .5f, numSamples);
		}
		void storeOutValue(float** block, const int lastSample) noexcept {
			for (auto ch = 0; ch < outValue.size(); ++ch)
				outValue[ch].set(block[ch][lastSample]);
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
		const float getAttenuvertor(const juce::Identifier& pID) const noexcept {
			return getDestination(pID)->getValue();
		}
		const std::vector<std::shared_ptr<Destination>>& getDestinations() const noexcept {
			return destinations;
		}
		float getOutValue(const int ch) const noexcept { return outValue[ch].get(); }
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
	protected:
		std::vector<std::shared_ptr<Parameter>> params;
		std::vector<std::shared_ptr<Destination>> destinations;
		std::vector<juce::Atomic<float>> outValue;
		float Fs;
	};

	/*
	* a macro modulator
	*/
	struct MacroModulator :
		public Modulator
	{
		MacroModulator(const std::shared_ptr<Parameter>& makroParam) :
			Modulator(makroParam->id)
		{ params.push_back(makroParam); }
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, float** block, juce::AudioPlayHead::CurrentPositionInfo&) override {
			for (auto s = 0; s < audioBuffer.getNumSamples(); ++s)
				block[0][s] = params[0]->get(s);
			const auto lastSample = audioBuffer.getNumSamples() - 1;
			storeOutValue(block, lastSample);
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
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate) override {
			Modulator::prepareToPlay(numChannels, sampleRate);
			env.resize(numChannels, 0.f);
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, float** block, juce::AudioPlayHead::CurrentPositionInfo&) override {
			auto numChannels = audioBuffer.getNumChannels();
			numChannels = numChannels < 3 ? numChannels : 2;
			const auto numSamples = audioBuffer.getNumSamples();
			const auto lastSample = numSamples - 1;
			const auto samples = audioBuffer.getArrayOfReadPointers();
			for (auto s = 0; s < numSamples; ++s)
				block[1][s] = dbInGain(params[Gain]->denormalized(s));
			for (auto s = 0; s < numSamples; ++s)
				block[0][s] = std::abs(samples[0][s]) * block[1][s];
			const auto atkInMs = params[Attack]->denormalized(0);
			const auto rlsInMs = params[Release]->denormalized(0);
			const auto bias = 1.f - juce::jlimit(0.f, 1.f, params[Bias]->get(0));
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
			generateMidSide(block, numChannels, numSamples);
			storeOutValue(block, lastSample);
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
			block[ch][s] = std::abs(samples[ch][s]) * block[1][s];
			if (env[ch] < block[ch][s])
				env[ch] += atkSpeed * (block[ch][s] - env[ch]);
			else if (env[ch] > block[ch][s])
				env[ch] += rlsSpeed * (block[ch][s] - env[ch]);
			block[ch][s] = env[ch] * gain;
			block[ch][s] = processBias(block[ch][s], bias);
		}
	};

	/*
	* an lfo modulator
	*/
	class LFOModulator :
		public Modulator
	{
		enum { Sync, Rate, Width, WaveTable };
		class WaveTables {
			enum { TableIdx, SampleIdx };
		public:
			WaveTables() :
				tables(),
				tableSize(0)
			{}
			void resize(const int tablesCount, const int _tableSize) {
				tableSize = _tableSize;
				tables.resize(tablesCount);
				for(auto& t: tables) t.resize(tableSize + spline::Size);
			}
			void addWaveTable(const std::function<float(float)>& func, const int tableIdx) noexcept {
				auto x = 0.f;
				const auto inc = 1.f / static_cast<float>(tableSize);
				for (auto i = 0; i < tableSize; ++i, x += inc)
					tables[tableIdx][i] = func(x);
				for (auto i = 0; i < spline::Size; ++i)
					tables[tableIdx][i + tableSize] = tables[tableIdx][i];
			}
			float operator()(const float phase, const int tableIdx) const noexcept {
				const auto x = phase * tableSize;
				return spline::process(tables[tableIdx].data(), x);
			}
			const size_t numTables() const { return tables.size(); }
		protected:
			std::vector<std::vector<float>> tables;
			int tableSize;
		};
	public:
		LFOModulator(const juce::String& mID, const std::shared_ptr<Parameter>& syncParam,
			const std::shared_ptr<Parameter>& rateParam, const std::shared_ptr<Parameter>& wdthParam,
			const std::shared_ptr<Parameter>& waveTableParam, const param::MultiRange& ranges) :
			Modulator(mID),
			multiRange(ranges),
			freeID(multiRange.getID("free")),
			syncID(multiRange.getID("sync")),
			phase(),
			waveTables(),
			fsInv(0.f)
		{
			params.push_back(syncParam);
			params.push_back(rateParam);
			params.push_back(wdthParam);
			params.push_back(waveTableParam);
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate) override {
			Modulator::prepareToPlay(numChannels, sampleRate);
			phase.resize(numChannels);
			fsInv = 1.f / Fs;
		}
		void addStuff(const juce::String& sID, const VectorAnything& stuff) override {
			if (sID == "wavetables") {
				const auto tablesCount = static_cast<int>(stuff.size()) - 1;
				const auto tableSize = stuff.get<int>(0);
				waveTables.resize(tablesCount, *tableSize);
				for (auto i = 1; i < stuff.size(); ++i) {
					const auto wtFunc = stuff.get<std::function<float(float)>>(i);
					waveTables.addWaveTable(*wtFunc, i - 1);
				}
			}
		}
		// PROCESS
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, float** block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
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
				const auto ppq = playHead.ppqPosition * .25;
				const auto rateValue = juce::jlimit(0.f, 1.f, params[Rate]->get());
				const auto rate = multiRange(syncID).convertFrom0to1(rateValue);
				const auto inc = 1.f / (static_cast<float>(barLengthInSamples) * rate);
				const auto ppqCh = static_cast<float>(ppq) / rate;
				auto newPhase = (ppqCh - std::floor(ppqCh));
				phase[0] = newPhase;
				processPhase(block, inc, 0, numSamples);
			}
			auto width = params[Width]->denormalized();
			if (width != 0) {
				width *= .5f;
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
			processWaveTable(block, numChannels, numSamples);
			generateMidSide(block, numChannels, numSamples);
			storeOutValue(block, lastSample);
		}
	protected:
		const param::MultiRange& multiRange;
		const juce::Identifier& freeID, syncID;
		std::vector<float> phase;
		WaveTables waveTables;
		float fsInv;

		inline void processPhase(float** block, const float inc,
			const int ch, const int numSamples) noexcept {
			for (auto s = 0; s < numSamples; ++s) {
				phase[ch] += inc;
				if (phase[ch] >= 1.f)
					--phase[ch];
				block[ch][s] = phase[ch];
			}
		}

		void processWaveTable(float** block, const int numChannels, const int numSamples) {
			for (auto ch = 0; ch < numChannels; ++ch) {
				auto wtValue = static_cast<int>(params[WaveTable]->denormalized());
				for (auto s = 0; s < numSamples; ++s)
					block[ch][s] = waveTables(block[ch][s], wtValue);
			}
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
		}
		// SET
		void prepareToPlay(const int numChannels, const double sampleRate) override {
			Modulator::prepareToPlay(numChannels, sampleRate);
			static constexpr auto filterOrder = 3;
			smoothing.resize(numChannels, filterOrder);
			randValue.resize(numChannels, 0);
			fsInv = 1.f / Fs;
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, float** block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
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
			generateMidSide(block, numChannels, numSamples);
			storeOutValue(block, numSamples - 1);
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

			seed.resize(seedSize + spline::Size, 0.f);
			juce::Random rand;
			for (auto s = 0; s < seedSize; ++s)
				seed[s] = rand.nextFloat() * .8f; // .8f compensates for spline overshoot
			for (auto s = seedSize; s < seed.size(); ++s)
				seed[s] = seed[s - seedSize];
		}
		void prepareToPlay(const int numChannels, const double sampleRate) override {
			Modulator::prepareToPlay(numChannels, sampleRate);
			fsInv = 1.f / static_cast<float>(Fs);
		}
		void processBlock(const juce::AudioBuffer<float>& audioBuffer, float** block, juce::AudioPlayHead::CurrentPositionInfo& playHead) override {
			auto numChannels = audioBuffer.getNumChannels();
			numChannels = numChannels < 3 ? numChannels : 2;
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
			generateMidSide(block, numChannels, numSamples);
			storeOutValue(block, numSamples - 1);
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
			const auto narrow = 1.f - params[Width]->get();
			for (auto ch = 1; ch < numChannels; ++ch)
				for (auto s = 1; s < numSamples; ++s)
					block[ch][s] += narrow * (block[0][s] - block[ch][s]);
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
			channelSetup("channelSetup"),
			id("id"),
			atten("atten"),
			bidirec("bidirec"),
			param("PARAM")
		{}
		const juce::Identifier modSys;
		const juce::Identifier modulator;
		const juce::Identifier destination;
		const juce::Identifier channelSetup;
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
			selectedModulator()
		{
			const Type type;
			auto& state = apvts.state;
			const auto numChildren = state.getNumChildren();
			for (auto c = 0; c < numChildren; ++c) {
				const auto& pChild = apvts.state.getChild(c);
				if (pChild.hasType(type.param)) {
					const auto pID = pChild.getProperty(type.id).toString();
					parameters.push_back(std::make_shared<Parameter>(apvts, pID));
				}
			}
		}
		Matrix(const Matrix& other) :
			parameters(other.parameters),
			modulators(other.modulators),
			curPosInfo(getDefaultPlayHead()),
			block(other.block),
			selectedModulator(other.selectedModulator)
		{}
		// SET
		void prepareToPlay(const int numChannels, const int blockSize, const double sampleRate) {
			for (auto& p : parameters)
				p->prepareToPlay(blockSize, sampleRate);
			for (auto& m : modulators)
				m->prepareToPlay(numChannels, sampleRate);
			const auto channelCount = numChannels * numChannels;
			block.setSize(channelCount, blockSize, false, false, false);
		}
		void setSmoothingLengthInSamples(const juce::Identifier& pID, float length) noexcept {
			getParameter(pID)->setSmoothingLengthInSamples(length);
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
					const auto channelSetup = static_cast<ChannelSetup>(static_cast<int>(destChild.getProperty(type.channelSetup, ChannelSetup::Left)));
					if(destParameter != nullptr)
						addDestination(mID, dID, channelSetup, dValue, bidirec);
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
					destChild.setProperty(type.channelSetup, static_cast<int>(d->getChannelSetup()), nullptr);
					modChild.appendChild(destChild, nullptr);
				}
				modSysChild.appendChild(modChild, nullptr);
			}
		}
		// ADD MODULATORS
		std::shared_ptr<Modulator> addMacroModulator(const juce::Identifier& pID) {
			modulators.push_back(std::make_shared<MacroModulator>(getParameter(pID)));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addEnvelopeFollowerModulator(const juce::Identifier& gainPID,
			const juce::Identifier& atkPID, const juce::Identifier& rlsPID,
			const juce::Identifier& biasPID, const juce::Identifier& wdthPID, int idx) {
			const auto gainP = getParameter(gainPID);
			const auto atkP = getParameter(atkPID);
			const auto rlsP = getParameter(rlsPID);
			const auto biasP = getParameter(biasPID);
			const auto wdthP = getParameter(wdthPID);
			const juce::String idString("EnvFol" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<EnvelopeFollowerModulator>(idString, gainP, atkP, rlsP, biasP, wdthP));
			return modulators[modulators.size() - 1];
		}
		std::shared_ptr<Modulator> addLFOModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID,
			const juce::Identifier& wdthPID, const juce::Identifier& waveTablePID,
			const param::MultiRange& ranges, int idx) {
			const auto syncP = getParameter(syncPID);
			const auto rateP = getParameter(ratePID);
			const auto wdthP = getParameter(wdthPID);
			const auto waveTableP = getParameter(waveTablePID);
			const juce::String idString("LFO" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<LFOModulator>(idString, syncP, rateP, wdthP, waveTableP, ranges));
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
		std::shared_ptr<Modulator> addPerlinModulator(const juce::Identifier& syncPID, const juce::Identifier& ratePID,
			const juce::Identifier& octavesPID, const juce::Identifier& widthPID,
			const param::MultiRange& ranges, int maxOctaves, int idx) {
			const auto syncP = getParameter(syncPID);
			const auto rateP = getParameter(ratePID);
			const auto octavesP = getParameter(octavesPID);
			const auto widthP = getParameter(widthPID);
			const juce::String idString("Perlin" + static_cast<juce::String>(idx));
			modulators.push_back(std::make_shared<PerlinModulator>(idString, syncP, rateP, octavesP, widthP, ranges, maxOctaves));
			return modulators[modulators.size() - 1];
		}
		// MODIFY / REPLACE
		void selectModulator(const juce::Identifier& mID) { selectedModulator = getModulator(mID); }
		void addDestination(const juce::Identifier& mID, const juce::Identifier& dID, ChannelSetup channelSetup, const float atten = 1.f, const bool bidirec = false) {
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
									thisMod->addDestination(param, channelSetup, atten, bidirec); // add dest
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
				thisMod->addDestination(param, channelSetup, atten, bidirec);
				return;
			}
		}
		void addDestination(const juce::Identifier& mID, const juce::Identifier& dID, std::vector<float>& destBlock, ChannelSetup channelSetup, const float atten = 1.f, const bool bidirec = false) {
				// destination is not a parameter, so add unchecked
				auto thisMod = getModulator(mID);
				thisMod->addDestination(dID, destBlock, channelSetup, atten, bidirec);
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
			for (auto& p : parameters) p.get()->processBlock(numSamples);
			auto modsBlock = block.getArrayOfWritePointers();
			for (auto& m : modulators) {
				m->processBlock(audioBuffer, modsBlock, curPosInfo);
				m->processDestinations(modsBlock, numSamples);
			}
			const auto lastSample = numSamples - 1;
			for (auto& parameter : parameters) {
				auto p = parameter.get();
				p->limit(numSamples);
				p->storeSumValue(lastSample);
			}
		}
		// GET
		std::shared_ptr<Modulator> getSelectedModulator() noexcept { return selectedModulator; }
		std::shared_ptr<Modulator> getModulator(const juce::Identifier& mID) noexcept {
			for (auto& mod : modulators) {
				auto m = mod.get();
				if (m->hasID(mID))
					return mod;
			}
			return nullptr;
		}
		std::shared_ptr<Parameter> getParameter(const juce::Identifier& pID) {
			for (auto& parameter : parameters)
				if (parameter.get()->hasID(pID))
					return parameter;
			return nullptr;
		}
	protected:
		std::vector<std::shared_ptr<Parameter>> parameters;
		std::vector<std::shared_ptr<Modulator>> modulators;
		juce::AudioPlayHead::CurrentPositionInfo curPosInfo;
		juce::AudioBuffer<float> block;
		std::shared_ptr<Modulator> selectedModulator;
	};

	/* to do:
	*
	* destination: bias / weight
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
	*
	* editor
	*	find more elegant way for vectorizing all modulatable parameters
	*	rather than making a ton of huge ass initializer lists, lol
	*
	*/
}
#pragma once
#include <array>
#include <functional>
#include <chrono>
#include <thread>

namespace nelDSP {
	// CERTAINTY LFO & SMOOTH CONST
	static constexpr float ShapeMax = 24;
	static constexpr int LowPassOrderDefault = 3;
	// DELAY CONST
	static constexpr float DelaySizeInMS = 5.f, DelaySizeInMSMin = 1, DelaySizeInMSMax = 1000;

	struct Utils {
		Utils(juce::AudioProcessor* p) :
			processor(p),
			sampleRate(1),
			Fs(1), FsInv(1),
			delaySizeInMs(DelaySizeInMS),
			numChannels(0), numSamples(0), maxBufferSize(0),
			delaySize(1), delayMax(1), delayCenter(1)
		{}
		// SET
		bool numChannelsChanged(const int num, bool forcedUpdate = false) {
			if (numChannels != num || forcedUpdate) {
				numChannels = num;
				return true;
			}
			return false;
		}
		bool sampleRateChanged(const double sr, bool forcedUpdate = false) {
			if (sampleRate != sr || forcedUpdate) {
				sampleRate = sr;
				//numChannels = numSamples = 0;
				Fs = static_cast<float>(sampleRate);
				FsInv = 1.f / Fs;
				setDelaySize(static_cast<int>(std::rint(msInsamples(delaySizeInMs))));
				//maxBufferSize = 0;
				return true;
			}
			return false;
		}
		bool maxBufferSizeChanged(const int b, bool forcedUpdate = false) {
			if (maxBufferSize != b || forcedUpdate) {
				maxBufferSize = b;
				return true;
			}
			return false;
		}
		bool delaySizeChanged(float s) {
			if (delaySizeInMs != s) {
				delaySizeInMs = s;
				setDelaySize(static_cast<int>(std::rint(msInsamples(delaySizeInMs))));
				return true;
			}
			return false;
		}
		void setLatency(const int i) { processor->setLatencySamples(i); }

		juce::AudioProcessor* processor;
		double sampleRate = 0;
		float Fs, FsInv;
		float delaySizeInMs;
		int numChannels, numSamples, maxBufferSize;

		const float msInsamples(const float ms) const { return ms * Fs * .001f; }
		const float hzInRatio(const float frequency) const { return frequency * FsInv; }

		void setDelaySize(const int size) {
			delaySize = size;
			delayMax = delaySize - 1;
			delayCenter = size / 2;
			setLatency(delayCenter);
		}
		
		int delaySize, delayMax, delayCenter;
	};

	namespace certainty {
		enum Term {
			AlmostCertainly,
			HighlyLikely,
			VeryGoodChance,
			Probable,
			Likely,
			Probably,
			WeBelieve,
			BetterThanEven,
			AboutEven,
			WeDoubt,
			Improbable,
			Unlikely,
			ProbablyNot,
			LittleChance,
			AlmostNoChance,
			HighlyUnlikely,
			ChancesAreSlight
		};

		static constexpr int CertaintiesCount = 17;
		static constexpr int CertaintiesPerCertainty = 46;
		static constexpr int CertaintiesOrder = 8;
		static constexpr bool ShouldSort = true;
		static constexpr bool ShouldLimit = true;
		static constexpr bool ShouldDeleteOutliers = true;
		static constexpr bool ShouldNormalize = true;

		static juce::String toString(Term t) {
			switch (t) {
			case Term::AlmostCertainly: return "Almost Certainly";
			case Term::HighlyLikely: return "Highly Likely";
			case Term::VeryGoodChance:return "Very Good Chance";
			case Term::Probable:return "Probable";
			case Term::Likely:return "Likely";
			case Term::Probably:return "Probably";
			case Term::WeBelieve:return "We Believe";
			case Term::BetterThanEven:return "Better Than Even";
			case Term::AboutEven:return "About Even";
			case Term::WeDoubt:return "We Doubt";
			case Term::Improbable:return "Improbable";
			case Term::Unlikely:return "Unlikely";
			case Term::ProbablyNot:return "Probably Not";
			case Term::LittleChance:return "Little Chance";
			case Term::AlmostNoChance:return "Almost No Chance";
			case Term::HighlyUnlikely:return "Highly Unlikely";
			case Term::ChancesAreSlight:return "Chances Are Slight";
			}
			return "";
		}
		
		struct Generator {
			Generator() :
				certainties(),
				rand(),
				size()
			{
				const auto rawData = juce::String::createStringFromData(
					BinaryData::likelyRawData_txt, BinaryData::likelyRawData_txtSize
				);

				const auto cert = prepareCertainties(rawData);
				certainties = cert[HighlyUnlikely];
				if(ShouldSort) sortCertainties();
				if(ShouldDeleteOutliers) deleteOutliers();
				upscaleCertainties(CertaintiesOrder);
				if (ShouldLimit) limitCertainties();
				if (ShouldNormalize) normalize();
			}

			const float operator()() { return certainties[rand(size)]; }
			const float operator()(const util::Range& range) {
				return range.start + certainties[rand(size)] * range.distance;
			}
			const float getAverage() const {
				auto sum = 0.f;
				for (auto& c : certainties)
					sum += c;
				return sum / static_cast<float>(size);
			}
		private:
			std::vector<float> certainties;
			util::Rand rand;
			int size;

			std::array<std::vector<float>, CertaintiesCount> prepareCertainties(const juce::String& rawData) {
				std::array<std::vector<float>, CertaintiesCount> cert;
				for (auto& c : cert) c.clear();

				juce::String word;
				auto row = 0;
				for (auto i = 0; i < rawData.length(); ++i) {
					auto character = rawData[i];
					if (character == '\n') { // next line (row restart)
						addWordToCertainties(cert, word, row);
						row = 0;
					}
					else if (character == ',') { // next argument (next row)
						addWordToCertainties(cert, word, row);
						++row;
					}
					else word += character; // keep on writing word
				}
				addWordToCertainties(cert, word, row);

				return cert;
			}
			void addWordToCertainties(std::array<std::vector<float>, CertaintiesCount>& cert, juce::String& word, const int row) {
				const auto certaintyInPercent = static_cast<float>(word.getIntValue());
				const auto certaintyAsDecimal = certaintyInPercent * .01f;
				cert[row].push_back(certaintyAsDecimal);
				word.clear();
			}

			void sortCertainties() { util::shakerSort2(certainties); }
			void deleteOutliers() {
				float averageInc = 0;
				for (auto i = 1; i < CertaintiesPerCertainty; ++i)
					averageInc += certainties[i] - certainties[i - 1];
				averageInc /= CertaintiesPerCertainty;
				const auto half = CertaintiesPerCertainty / 2;
				for (auto i = 0; i < half; ++i)
					if (certainties[i + 1] - certainties[i] > averageInc)
						certainties[i] = certainties[i + 1];
				for (auto i = half + 1; i < CertaintiesPerCertainty; ++i)
					if (certainties[i] - certainties[i - 1] > averageInc)
						certainties[i] = certainties[i - 1];
			}
			void upscaleCertainties(const int order) {
				size = order * CertaintiesPerCertainty;
				if (order != 1) {
					interpolation::Lanczos interpolator;
					std::vector<float> newData;
					for (auto c = 0; c < CertaintiesCount; ++c) {
						newData.reserve(size);
						for (auto n = 0; n < size; ++n) {
							auto idx = static_cast<float>(n) * CertaintiesPerCertainty / size;
							newData.emplace_back(interpolator(certainties, idx));
						}
					}
					certainties = newData;
				}
			}
			void limitCertainties() {
				for (auto& i : certainties)
					if (i < 0.f) i = 0.f;
					else if (i >= 1.f) i = 1.f - std::numeric_limits<float>::epsilon();
			}
			void normalize() {
				auto min = 1.f;
				auto max = 0.f;
				for (auto& s : certainties) {
					if (s < min) min = s;
					if (s > max) max = s;
				}
				util::Range r(min, max);
				for (auto& s : certainties)
					s = (s - r.start) / r.distance;
			}
		};
	}

	struct ASREnvelope {
		enum class State { Attack, Sustain, Release };
		ASREnvelope(const Utils& u) :
			utils(u),
			env(0),
			state(State::Release),
			attack(.001f),
			release(.001f)
		{}
		// SET
		void setMaxBufferSize() { data.resize(utils.maxBufferSize, 0); }
		// PARAM
		void setAttackInMs(float a) { setAttackInSamples(utils.msInsamples(a)); }
		void setReleaseInMs(float r) { setReleaseInSamples(utils.msInsamples(r)); }
		void setAttackInSamples(float a) { attack = 1.f / a; }
		void setReleaseInSamples(float r) { release = 1.f / r; }
		// PROCESS
		void processBlock(bool noteOn) {
			for (auto s = 0; s < utils.numSamples; ++s) {
				processSample(noteOn);
				data[s] = env;
			}
		}
		void processSample(bool noteOn) {
			switch (state) {
			case State::Release:
				if (noteOn) state = State::Attack;
				else
					if (env <= 0.f) env = 0.f;
					else env -= release;
				return;
			case State::Attack:
				if (!noteOn) state = State::Release;
				else
					if (env >= 1.f) {
						env = 1.f;
						state = State::Sustain;
					}
					else env += attack;
				return;
			case State::Sustain:
				if (!noteOn)
					state = State::Release;
				return;
			}
		}
		// GET
		const float operator[](int i) const { return data[i]; }
		const Utils& utils;
		std::vector<float> data;
		State state;
	private:
		float env, attack, release;
	};

	struct LowPass {
		LowPass(const Utils& utils, float start = 0) :
			utils(utils),
			value(start),
			inertia(1.f)
		{}
		// SET / PARAM
		void setInertiaInMs(const float ms) { setInertiaInSamples(utils.msInsamples(ms)); }
		void setInertiaInHz(const float hz) { inertia = utils.hzInRatio(hz); }
		void setInertiaInSamples(const float smpls) { inertia = 1.f / smpls; }
		// PROCESS
		const float operator()(const float x) {
			value += inertia * (x - value);
			return value;
		}
	private:
		const Utils& utils;
		float value, inertia;
	};

	struct MultiOrderLowPass {
		MultiOrderLowPass(const Utils& u, const int order = 1) :
			lowpass(),
			utils(u)
		{
			setOrder(order);
		}
		// PARAM
		void setOrder(int o) { lowpass.resize(o, { utils, 0 }); }
		void setInertiaInMs(float ms) {
			for (auto& s : lowpass)
				s.setInertiaInMs(ms);
		}
		void setInertiaInHz(float hz) {
			for (auto& s : lowpass)
				s.setInertiaInHz(hz);
		}
		void setInertiaInSamples(float smpls) {
			for (auto& s : lowpass)
				s.setInertiaInSamples(smpls);
		}
		// PROCESS
		void processBlock(std::vector<float>& data) {
			for (auto d = 0; d < utils.numSamples; ++d)
				for (auto& s : lowpass)
					data[d] = s(data[d]);
		}
	private:
		std::vector<LowPass> lowpass;
		const Utils& utils;
	};

	namespace vibrato {
		struct Phase {
			Phase(const Utils& u) :
				phase(1.f),
				inc(0.f),
				utils(u)
			{}
			void setFrequencyInHz(float hz) { inc = utils.hzInRatio(hz); }
			bool operator()() {
				phase += inc;
				if (phase < 1.f)
					return false;
				--phase;
				return true;
			}
			float phase;
		private:
			float inc;
			const Utils& utils;
		};

		struct CertaintySequencer {
			CertaintySequencer(const Utils& u, certainty::Generator& certainty) :
				phase(u),
				rand(),
				lowpass(u, LowPassOrderDefault),
				utils(u),
				certainty(certainty),
				freq(1.f), shape(1.f),
				curValue(0.f), depth(1.f)
			{}
			// PARAM
			void setFrequencyInHz(float hz) {
				freq = hz;
				phase.setFrequencyInHz(freq);
				updateLowpassFrequency();
			}
			void setDepth(float d) { depth = d; }
			void setShape(float s) {
				shape = s;
				updateLowpassFrequency();
			}
			// PROCESS
			void synthesizeBlock(std::vector<float>& data) { synthesizeRandomValues(data); }
			void filter(std::vector<float>& data) { lowpass.processBlock(data); }
			void scaleForDelay(float* data) {
				const auto dSize = static_cast<float>(utils.delaySize);
				juce::FloatVectorOperations::multiply(data, .5f, utils.numSamples);
				juce::FloatVectorOperations::add(data, .5f, utils.numSamples);
				juce::FloatVectorOperations::multiply(data, dSize, utils.numSamples);
			}
			void synthesizeBlockBypassed(std::vector<float>& data) {
				auto centre = static_cast<float>(utils.delayCenter);
				for (auto& d : data) d = centre;
			}
			// DBG
			void playback(juce::AudioBuffer<float>& buffer, const std::vector<float>& data, const int ch) {
				auto samples = buffer.getArrayOfWritePointers();
				for (auto s = 0; s < utils.numSamples; ++s)
					samples[ch][s] = 2.f * data[s] / utils.delaySize - 1.f;
			}
		private:
			Phase phase;
			util::Rand rand;
			MultiOrderLowPass lowpass;
			const Utils& utils;
			certainty::Generator& certainty;

			float freq, shape, curValue, depth;

			void updateLowpassFrequency() { lowpass.setInertiaInHz(shape * freq * LowPassOrderDefault); }

			/* synthesizes a smooth random curve (-1, 1) */
			void synthesizeRandomValues(std::vector<float>& data) {
				for (auto s = 0; s < utils.numSamples; ++s) {
					if (phase()) {
						const auto cert = certainty();
						curValue = cert * depth * (rand() > .5f ? 1.f : -1.f);
					}
					data[s] = curValue;
				}
			}
		};

		struct WriteHead {
			WriteHead(const Utils& utils) :
				data(),
				utils(utils),
				idx(-1)
			{}
			// SET
			void setMaxBufferSize() {
				data.resize(utils.maxBufferSize, 0);
				idx = -1;
			}
			// PROCESS
			void synthesizeBlock() {
				for (auto s = 0; s < utils.numSamples; ++s) {
					++idx;
					if (idx >= utils.delaySize)
						idx = 0;
					data[s] = idx;
				}
			}
			// GET
			const int operator[](const int i) const { return data[i]; }
			std::vector<int> data;
		private:
			const Utils& utils;
			int idx;
		};

		struct ReadHead {
			ReadHead(const Utils& u) :
				utils(u)
			{}
			// PROCESS
			void processBlock(std::vector<float>& data, const int* wHead) {
				for (auto s = 0; s < utils.numSamples; ++s) {
					data[s] = wHead[s] - data[s];
					if (data[s] < 0.f)
						data[s] += utils.delaySize;
				}
			}
		private:
			const Utils& utils;
		};

		struct FFDelay {
			FFDelay(const Utils& u) :
				buffer(),
				utils(u)
			{ setSampleRate(); }
			// SET
			void setSampleRate() { buffer.resize(utils.delaySize, 0); }
			// PROCESS
			void processBlock(juce::AudioBuffer<float>& b, const int* wHead, const std::vector<float>& rHead, const interpolation::Lanczos& interpolator, const int ch) {
				auto samples = b.getWritePointer(ch, 0);
				for (auto s = 0; s < utils.numSamples; ++s) {
					buffer[wHead[s]] = samples[s];
					samples[s] = interpolator(buffer, rHead[s]);
				}
			}
			void processBlock(juce::AudioBuffer<float>& b, const int* wHead, const std::vector<float>& rHead, const int ch) {
				auto samples = b.getWritePointer(ch, 0);
				for (auto s = 0; s < utils.numSamples; ++s) {
					buffer[wHead[s]] = samples[s];
					samples[s] = buffer[static_cast<int>(rHead[s])];
				}
			}
		private:
			std::vector<float> buffer;
			const Utils& utils;
		};
		
		struct MultiChannelModules {
			MultiChannelModules(Utils& u, certainty::Generator& certainty) :
				data(),
				seq(u, certainty),
				rHead(u),
				delay(u),
				utils(u)
			{ setMaxBufferSize(); }
			// SET
			void setSampleRate() { delay.setSampleRate(); }
			void setMaxBufferSize() { data.resize(utils.maxBufferSize, 0); }
			// PARAM
			void setDepth(const float d) { seq.setDepth(d); }
			void setFreq(const float f) { seq.setFrequencyInHz(f); }
			void setShape(const float s) { seq.setShape(s); }
			// PROCESS
			void synthesizeLFO() { seq.synthesizeBlock(data); }
			void synthesizeLFOBypassed() { seq.synthesizeBlockBypassed(data); }
			void copyLFO(const float* other) { juce::FloatVectorOperations::copy(data.data(), other, utils.numSamples); } // IF WIDTH == 0
			void saveLFOValue(std::atomic<float>& lfoValue) { lfoValue.store(data[0]); }
			void mixLFO(const float* other, const std::vector<float>& widthData) { // FOR LFOS of channel == 1 && WIDTH != 0
				for (auto s = 0; s < utils.numSamples; ++s)
					data[s] = other[s] + widthData[s] * (data[s] - other[s]);
			}
			void upscaleLFO() { seq.scaleForDelay(data.data()); }
			
			void filterDelay() { seq.filter(data); }
			void clampDelay() {
				const auto delaySize = static_cast<float>(utils.delaySize);
				for (auto s = 0; s < utils.numSamples; ++s)
					if (data[s] < 0.f) data[s] = 0.f;
					else if (data[s] >= delaySize) data[s] = delaySize - std::numeric_limits<float>::min();
			}
			void processBlock(juce::AudioBuffer<float>& buffer, const int* wHead, const interpolation::Lanczos& interpolator, const int ch) {
				//seq.playback(buffer, data, ch);
				rHead.processBlock(data, wHead);
				delay.processBlock(buffer, wHead, data, interpolator, ch);
			}
			void processBlock(juce::AudioBuffer<float>& buffer, const int* wHead, const int ch) {
				rHead.processBlock(data, wHead);
				delay.processBlock(buffer, wHead, data, ch);
			}
			// GET
			std::vector<float> data;
		private:
			CertaintySequencer seq;
			ReadHead rHead;
			FFDelay delay;
			const Utils& utils;
		};

		struct WidthProcessor {
			WidthProcessor(const Utils& u) :
				utils(u),
				lowpass(utils, 0),
				width(0.f), dest(1.f)
			{}
			// SET
			void setSampleRate() { lowpass.setInertiaInHz(17.f); }
			// PARAM
			void setWidth(float w) { dest = w; }
			// PROCESS
			void operator()(std::vector<float>& data, std::vector<MultiChannelModules>& chModules, std::atomic<float>& lfoValue) {
				for (auto s = 0; s < utils.numSamples; ++s)
					data[s] = lowpass(dest); // smoothen width transitions

				if (utils.numChannels == 1)
					processWidthDisabled(chModules);
				else
					processWidthEnabled(chModules, data, lfoValue);
			}
		private:
			const Utils& utils;
			LowPass lowpass;
			float width, dest;

			void processWidthDisabled(std::vector<MultiChannelModules>& chModules) { chModules[0].upscaleLFO(); }
			void processWidthEnabled(std::vector<MultiChannelModules>& chModules, const std::vector<float>& widthData, std::atomic<float>& lfoValue) {
				for (auto ch = 1; ch < utils.numChannels; ++ch) {
					chModules[ch].synthesizeLFO();
					chModules[ch].filterDelay();
					chModules[ch].mixLFO(chModules[0].data.data(), widthData);
					chModules[ch].saveLFOValue(lfoValue);
					chModules[ch].upscaleLFO();
				}
				chModules[0].upscaleLFO();
			}
		};

		struct MixProcessor {
			MixProcessor(const Utils& u) :
				dryBuffer(),
				utils(u),
				lowpassA(u, 0), lowpassB(u, 0),
				mix(1.f), mixA(0), mixB(1)
			{}
			// SET
			void setSampleRate() {
				const auto time = 17.f;
				lowpassA.setInertiaInMs(time);
				lowpassB.setInertiaInMs(time);
			}
			void setNumChannels() {
				dryBuffer.resize(utils.numChannels);
				setMaxBufferSize();
			}
			void setMaxBufferSize() { for (auto& d : dryBuffer) d.resize(utils.maxBufferSize, 0); }
			// PARAM
			void setMix(float m) {
				mix = m;
				mixA = std::sqrt(.5f * (1.f - m));
				mixB = std::sqrt(.5f * (1.f + m));
			}
			// PROCESS
			bool saveDryBuffer(const juce::AudioBuffer<float>& buffer) {
				if (mix != 0) {
					for (auto ch = 0; ch < utils.numChannels; ++ch)
						juce::FloatVectorOperations::copy(dryBuffer[ch].data(), buffer.getReadPointer(ch), utils.numSamples);
					return false;
				}
				return true; // 100% dry
			}
			void operator()(juce::AudioBuffer<float>& buffer, std::vector<float>& data) {
				// MIX
				auto samples = buffer.getArrayOfWritePointers();
				for (auto s = 0; s < utils.numSamples; ++s)
					data[s] = lowpassB(mixB);
				for (auto ch = 0; ch < utils.numChannels; ++ch)
					juce::FloatVectorOperations::multiply(samples[ch], data.data(), utils.numSamples);
				for (auto s = 0; s < utils.numSamples; ++s)
					data[s] = lowpassA(mixA);
				for (auto ch = 0; ch < utils.numChannels; ++ch)
					for (auto s = 0; s < utils.numSamples; ++s)
						samples[ch][s] += dryBuffer[ch][s] * data[s];
			}
		private:
			std::vector<std::vector<float>> dryBuffer;
			const Utils& utils;
			LowPass lowpassA, lowpassB;
			float mix, mixA, mixB;
		};

		struct Vibrato {
			Vibrato(Utils& u) :
				data(),
				interpolator(),
				lfoValues(),
				mixProcessor(u),
				chModules(),
				certainty(),
				wHead(u),
				widthProcessor(u),
				utils(u),

				depth(1.f), freq(1.f), shape(1), mix(-1.f), splineMix(0)
			{}
			// SET
			void setSampleRate() {
				mixProcessor.setSampleRate();
				widthProcessor.setSampleRate();
				for (auto& ch : chModules)
					ch.setSampleRate();
			}
			void setNumChannels() {
				chModules.resize(utils.numChannels, { utils, certainty });
				setDepth(depth);
				setFreq(freq);
				mixProcessor.setNumChannels();
			}
			void setMaxBufferSize() {
				data.resize(utils.maxBufferSize, 0);
				wHead.setMaxBufferSize();
				for (auto& ch : chModules) ch.setMaxBufferSize();
				mixProcessor.setMaxBufferSize();
			}
			// PARAM
			void setDepth(const float d) {
				depth = d;
				for (auto& ch : chModules)
					ch.setDepth(depth);
			}
			void setFreq(const float f) {
				freq = f;
				for (auto& ch : chModules)
					ch.setFreq(freq);
			}
			void setShape(const float s) {
				shape = std::pow(ShapeMax, s);
				for (auto& ch : chModules)
					ch.setShape(shape);
			}
			void setWidth(const float w) { widthProcessor.setWidth(w); }
			void setMix(const float m) {
				mix = m;
				mixProcessor.setMix(mix);
			}
			void setSplineMix(const float s) {
				splineMix = s;
			}
			// PROCESS
			void processBlock(juce::AudioBuffer<float>& buffer) {
				if (mixProcessor.saveDryBuffer(buffer)) return;
				wHead.synthesizeBlock();
				chModules[0].synthesizeLFO();
				chModules[0].filterDelay();
				chModules[0].saveLFOValue(lfoValues[0]);
				widthProcessor(data, chModules, lfoValues[1]);
				for (auto ch = 0; ch < utils.numChannels; ++ch) {
					chModules[ch].clampDelay();
					chModules[ch].processBlock(buffer, wHead.data.data(), interpolator, ch);
				}
				mixProcessor(buffer, data);
			}
			void processBlockBypassed(juce::AudioBuffer<float>& buffer) {
				setDepth(0.f); setFreq(1.f); setShape(0.f);
				return processBlock(buffer);
				/* // these must be a better way to deal with bypass, lol
				wHead.synthesizeBlock();
				chModules[0].synthesizeLFOBypassed();
				for (auto ch = 1; ch < utils.numChannels; ++ch)
					chModules[ch].copyLFO(chModules[0].data.data());
				for (auto ch = 0; ch < utils.numChannels; ++ch)
					chModules[ch].processBlock(buffer, wHead.data.data(), ch);
				*/
			}
			// GET
			std::array<std::atomic<float>, 2>& getLFOValues() { return lfoValues; }
			std::vector<float> data;
		private:
			interpolation::Lanczos interpolator;
			std::array<std::atomic<float>, 2> lfoValues;
			MixProcessor mixProcessor;
			std::vector<MultiChannelModules> chModules;
			certainty::Generator certainty;
			WriteHead wHead;
			WidthProcessor widthProcessor;
			Utils& utils;

			// PARAM
			float depth, freq, shape, mix, splineMix;
		};
	}

	struct StereoMode {
		StereoMode(const Utils& u) :
			utils(u),
			crossfade(u),
			isMS(true)
		{}
		// SET
		void setSampleRate() {
			crossfade.setAttackInMs(17);
			crossfade.setReleaseInMs(17);
		}
		void setMaxBufferSize() { crossfade.setMaxBufferSize(); }
		// PARAM
		void setIsMS(bool x) { isMS = x; }
		// PROCESS
		bool encode(juce::AudioBuffer<float>& audioBuffer) {
			if (utils.numChannels != 2) return false;
			if (isMS) {
				auto samples = audioBuffer.getArrayOfWritePointers();
				if (crossfade[0] != 1.f) {
					crossfade.processBlock(isMS);
					for (auto s = 0; s < utils.numSamples; ++s) {
						const auto mid = (samples[0][s] + samples[1][s]) * .5f;
						const auto side = (samples[0][s] - samples[1][s]) * .5f;
						samples[0][s] += crossfade[s] * (mid - samples[0][s]);
						samples[1][s] += crossfade[s] * (side - samples[1][s]);
					}
				}
				else {
					for (auto s = 0; s < utils.numSamples; ++s) {
						const auto mid = (samples[0][s] + samples[1][s]) * .5f;
						const auto side = (samples[0][s] - samples[1][s]) * .5f;
						samples[0][s] = mid;
						samples[1][s] = side;
					}
				}
			}
			else {
				if (crossfade[0] != 0.f) {
					crossfade.processBlock(isMS);
					auto samples = audioBuffer.getArrayOfWritePointers();
					for (auto s = 0; s < utils.numSamples; ++s) {
						const auto mid = (samples[0][s] + samples[1][s]) * .5f;
						const auto side = (samples[0][s] - samples[1][s]) * .5f;
						samples[0][s] += crossfade[s] * (mid - samples[0][s]);
						samples[1][s] += crossfade[s] * (side - samples[1][s]);
					}
				}
			}
			return true;
		}
		void decode(juce::AudioBuffer<float>& audioBuffer) {
			if (isMS) {
				auto samples = audioBuffer.getArrayOfWritePointers();
				if (crossfade[0] != 1.f) {
					for (auto s = 0; s < audioBuffer.getNumSamples(); ++s) {
						const auto left = samples[0][s] + samples[1][s];
						const auto right = samples[0][s] - samples[1][s];
						samples[0][s] += crossfade[s] * (left - samples[0][s]);
						samples[1][s] += crossfade[s] * (right - samples[1][s]);
					}
				}
				else {
					for (auto s = 0; s < audioBuffer.getNumSamples(); ++s) {
						const auto left = samples[0][s] + samples[1][s];
						const auto right = samples[0][s] - samples[1][s];
						samples[0][s] = left;
						samples[1][s] = right;
					}
				}
			}
			else {
				if (crossfade[0] != 0.f) {
					auto samples = audioBuffer.getArrayOfWritePointers();
					for (auto s = 0; s < audioBuffer.getNumSamples(); ++s) {
						const auto left = samples[0][s] + samples[1][s];
						const auto right = samples[0][s] - samples[1][s];
						samples[0][s] += crossfade[s] * (left - samples[0][s]);
						samples[1][s] += crossfade[s] * (right - samples[1][s]);
					}
				}
			}
		}
		
		const Utils& utils;
		ASREnvelope crossfade;
		bool isMS;
	};

	struct Nel19 {
		Nel19(juce::AudioProcessor* p) :
			depthMax(),
			utils(p),
			stereoMode(utils),
			vibrato(utils)
		{
		}
		// SET
		void prepareToPlay(const double sampleRate, const int maxBufferSize, const int channelCount, bool forcedUpdate = false) {
			if (utils.sampleRateChanged(sampleRate, forcedUpdate)) {
				vibrato.setSampleRate();
				stereoMode.setSampleRate();
				utils.setLatency(utils.delayCenter);
			}
			if (utils.maxBufferSizeChanged(maxBufferSize, forcedUpdate)) {
				vibrato.setMaxBufferSize();
				stereoMode.setMaxBufferSize();
			}
			if(utils.numChannelsChanged(channelCount, forcedUpdate))
				vibrato.setNumChannels();
		}
		// PARAM
		void setDepthMax(const float dm) {
			DBG(dm);
			if (utils.delaySizeChanged(dm))
				prepareToPlay(utils.sampleRate, utils.maxBufferSize, utils.numChannels, true);
		}
		void setDepth(const float depth) { vibrato.setDepth(depth); }
		void setFreq(const float freq) { vibrato.setFreq(freq); }
		void setShape(const float shape) { vibrato.setShape(shape); }
		void setLRMS(const float lrms) { stereoMode.setIsMS(lrms > .5f); }
		void setWidth(const float width) { vibrato.setWidth(width); }
		void setMix(const float mix) { vibrato.setMix(mix); }
		void setSplineMix(const float sMix) { vibrato.setSplineMix(sMix); }
		// PROCESS
		void processBlock(juce::AudioBuffer<float>& buffer) {
			utils.numSamples = buffer.getNumSamples();
			if (stereoMode.encode(buffer)) {
				vibrato.processBlock(buffer);
				return stereoMode.decode(buffer);
			}
			vibrato.processBlock(buffer);
		}
		void processBlockBypassed(juce::AudioBuffer<float>& buffer) {
			utils.numSamples = buffer.getNumSamples();
			vibrato.processBlockBypassed(buffer);
		}
		// GET
		std::array<std::atomic<float>, 2>& getLFOValues() { return vibrato.getLFOValues(); }
	private:
		std::vector<float> depthMax;
		Utils utils;
		StereoMode stereoMode;
		vibrato::Vibrato vibrato;
	};
}

/* TO DO

BUGS:
	FL Parameter Randomizer makes plugin freeze
		processBlock not called anymore
		parameters not shown above Randomize
	STUDIO ONE v5.1 (SB Dvs):
		jiterry output
		daw crashes when plugin removed
	STUDIO ONE
		no mouseCursor shown
	REAPER (F Mry):
		plugin doesn't show up (maybe missing dependencies)
	soupiraille
		plugin doesn't load with static linking ??

ADD FEATURES / IMPROVE:
	mac support
	rewrite so that param smoothing before go to processors
	lower cpu consumption, maybe on interpolation?
	poly vibrato? (unison)
	new midi learn (modulation stuff)
	temposync freq
	multiband
	monoizer for stereowidth-slider to flanger?
	oversampling?

TESTED:
	DAWS
		cubase      CHECK 9.5, 10
		fl          CHECK
		ableton		CHECK (thx julian)
		bitwig		NOT CHECKED YET
		studio one  CHECK

DAWS Debug:

D:\Pogramme\Cubase 9.5\Cubase9.5.exe
D:\Pogramme\FL Studio 20\FL64.exe
D:\Pogramme\Studio One 5\Studio One.exe

C:\Users\Eine Alte Oma\Documents\CPP\vst3sdk\out\build\x64-Debug (default)\bin\validator.exe
-e "C:/Program Files/Common Files/VST3/NEL-19.vst3"

DOWNLOAD C++14 AT:
https://support.microsoft.com/en-us/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0

*/
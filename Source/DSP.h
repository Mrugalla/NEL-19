#pragma once
#include <array>
#include <functional>

namespace nelDSP {
	// MATH CONST
	static constexpr float Tau = 6.28318530718f;
	static constexpr float Pi = 3.14159265359f;
	// INTERPOLATION CONST
	static constexpr int MaxInterpolationOrder = 7;
	static constexpr int SincResolution = 1 << 9;
	// CERTAINTY LFO & SMOOTH CONST
	static constexpr float DepthDefault = .1f;
	static constexpr float LFOFreqMin = .01f, LFOFreqMax = 30.f, LFOFreqDefault = 4.f, LFOFreqInterval = .5f;
	static constexpr float ShapeMax = 24;
	static constexpr int LowPassOrderDefault = 3;
	// DELAY CONST
	static constexpr float DelaySizeInMS = 5.f, DelaySizeInMSMin = 1, DelaySizeInMSMax = 1000;
	static constexpr float WowWidthDefault = 0.f;

	namespace util {
		static juce::NormalisableRange<float> PowX2Range(float min, float max) {
			return juce::NormalisableRange<float>(min, max,
				[](float start, float end, float normalised) {
					return start + std::pow(normalised, 2.f) * (end - start);
				},
				[](float start, float end, float value) {
					return std::sqrt((value - start) / (end - start));
					
				}
			);
		}
		static juce::NormalisableRange<float> LogRange(float min, float max) {
			return juce::NormalisableRange<float>(min, max,
				[](float start, float end, float normalised) {
					return start + (std::pow(2.f, normalised * 10) - 1.f) * (end - start) / 1023.f;
				},
				[](float start, float end, float value) {
					return (std::log(((value - start) * 1023.f / (end - start)) + 1.f) / std::log(2.f)) / 10.f;
				}
			);
		}
		static juce::NormalisableRange<float> LogRange(float min, float max, float interval) {
			return juce::NormalisableRange<float>(min, max,
				[](float start, float end, float normalised) {
					return start + (std::pow(2.f, normalised * 10) - 1.f) * (end - start) / 1023.f;
				},
				[](float start, float end, float value) {
					return (std::log(((value - start) * 1023.f / (end - start)) + 1.f) / std::log(2.f)) / 10.f;
				},
				[interval](float start, float end, float value) {
					return juce::jlimit(start, end, interval * std::rint(value / interval));
				}
			);
		}
		static juce::NormalisableRange<float> QuadraticBezierRange(float min, float max, float shape) {
			// 0 <= SHAPE < 1 && SHAPE && SHAPE != .5
			auto rangedShape = shape * (max - min) + min;
			return juce::NormalisableRange<float>(
				min, max,
				[rangedShape](float start, float end, float normalized) {
					auto lin0 = start + normalized * (rangedShape - start);
					auto lin1 = rangedShape + normalized * (end - rangedShape);
					auto lin2 = lin0 + normalized * (lin1 - lin0);
					return lin2;
				},
				[rangedShape](float start, float end, float value) {
					auto start2 = 2 * start;
					auto shape2 = 2 * rangedShape;
					auto t0 = start2 - shape2;
					auto t1 = std::sqrt(std::pow(shape2 - start2, 2.f) - 4.f * (start - value) * (start - shape2 + end));
					auto t2 = 2.f * (start - shape2 + end);
					auto y = (t0 + t1) / t2;
					return juce::jlimit(0.f, 1.f, y);
				}
			);
		}
		static juce::AudioBuffer<float> load(const char* data, const int size) {
			auto* stream = new juce::MemoryInputStream(data, size, false);
			juce::WavAudioFormat format;
			auto* reader = format.createReaderFor(stream, true);
			auto lengthInSamples = (int)reader->lengthInSamples;
			juce::AudioBuffer<float> buffer(1, lengthInSamples);
			if (reader) reader->read(buffer.getArrayOfWritePointers(), 1, 0, lengthInSamples);
			delete reader;
			return buffer;
		}
		static void shakerSort(std::vector<double>& data) {
			auto lenMax = (int)(data.size() - 2);
			bool swapped;
			do {
				swapped = false;
				for (auto i = 0; i < lenMax; ++i)
					if (data[i] > data[i + 1]) {
						std::swap(data[i], data[i + 1]);
						swapped = true;
					}
				if (!swapped) return;
				swapped = false;
				for (auto i = lenMax; i > 0; --i)
					if (data[i] > data[i + 1]) {
						std::swap(data[i], data[i + 1]);
						swapped = true;
					}
			} while (swapped);
		}
		static void shakerSort2(std::vector<float>& data) {
			auto upperLimit = (int)(data.size() - 2);
			auto lowerLimit = 0;
			auto tempLimit = 0;
			bool swapped = false;
			do {
				for (auto i0 = lowerLimit; i0 < upperLimit; ++i0) {
					auto i1 = i0 + 1;
					if (data[i0] > data[i1]) {
						tempLimit = i1;
						std::swap(data[i0], data[i1]);
						swapped = true;
					}
				}
				if (!swapped) return;
				upperLimit = tempLimit;
				swapped = false;
				for (auto i0 = upperLimit; i0 > lowerLimit; --i0) {
					auto i1 = i0 + 1;
					if (data[i0] > data[i1]) {
						tempLimit = i0;
						std::swap(data[i0], data[i1]);
						swapped = true;
					}
				}
				if (!swapped) return;
				lowerLimit = tempLimit - 1;
				swapped = false;
			} while (true);
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<float>& data, const int ch) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto s = 0; s < buffer.getNumSamples(); ++s)
				samples[ch][s] = data[s];
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<float>& l, const std::vector<float>& r) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto s = 0; s < buffer.getNumSamples(); ++s) {
				samples[0][s] = l[s];
				samples[1][s] = r[s];
			}
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<float>& data) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
				for (auto s = 0; s < buffer.getNumSamples(); ++s)
					samples[ch][s] = data[s];
		}
		static juce::StringArray makeChoicesArray(std::vector<juce::String> str) {
			juce::StringArray y;
			for (const auto& i : str)
				y.add(i);
			return y;
		}
	}

	struct Range {
		Range(const float s = 0, const float e = 1):
			start(s),
			end(e),
			distance(end - start)
		{}
		// SET
		void set(const float s, const float e) {
			start = s;
			end = e;
			distance = e - s;
		}
		// GET
		float start, end, distance;
		// DBG
		void dbg() const { DBG(end << " - " << start << " = " << distance); }
	};

	struct Rand {
		Rand() :
			gen(std::chrono::high_resolution_clock::now().time_since_epoch().count())
		{}
		float operator()() { return gen.nextFloat(); }
		int operator()(const int len) { return gen.nextInt(len); }
	private:
		juce::Random gen;
	};

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
			delayCenter = size;
			setLatency(delayCenter);
		}
		
		int delaySize, delayMax, delayCenter;
	};

	namespace param {
		enum class ID { DepthMax, Depth, Freq, Shape, LRMS, Width, Mix };

		static juce::String getName(ID i) {
			switch (i) {
			case ID::DepthMax: return "Depth Max";
			case ID::Depth: return "Depth";
			case ID::Freq: return "Freq";
			case ID::Shape: return "Shape";
			case ID::LRMS: return "LRMS";
			case ID::Width: return "Width";
			case ID::Mix: return "Mix";
			default: return "";
			}
		}
		static juce::String getName(int i) { getName(static_cast<ID>(i)); }
		static juce::String getID(const ID i) { return getName(i).toLowerCase().removeCharacters(" "); }
		static juce::String getID(const int i) { return getName(i).toLowerCase().removeCharacters(" "); }

		static std::unique_ptr<juce::AudioParameterBool> createPBool(ID i, bool defaultValue, std::function<juce::String(bool value, int maxLen)> func) {
			return std::make_unique<juce::AudioParameterBool>(
				getID(i), getName(i), defaultValue, getName(i), func
				);
		}
		static std::unique_ptr<juce::AudioParameterChoice> createPChoice(ID i, const juce::StringArray& choices, int defaultValue) {
			return std::make_unique<juce::AudioParameterChoice>(
				getID(i), getName(i), choices, defaultValue, getName(i)
			);
		}
		static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, const juce::NormalisableRange<float>& range, float defaultValue,
			std::function<juce::String(float value, int maxLen)> stringFromValue = nullptr) {
			return std::make_unique<juce::AudioParameterFloat>(
				getID(i), getName(i), range, defaultValue, getName(i), juce::AudioProcessorParameter::Category::genericParameter,
				stringFromValue
			);
		}

		static juce::AudioProcessorValueTreeState::ParameterLayout createParameters() {
			std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

			auto percentStr = [](float value, int) {
				if (value == 1.f) return juce::String("100 %");
				value *= 100.f;
				if(value > 9.f) return static_cast<juce::String>(value).substring(0, 2) + " %";
				return static_cast<juce::String>(value).substring(0, 1) + " %";
			};
			auto freqStr = [](float value, int) {
				if(value < 10.f)
					return static_cast<juce::String>(value).substring(0, 3) + " hz";
				return static_cast<juce::String>(value).substring(0, 2) + " hz";
			};
			auto mixStr = [](float value, int) {
				auto nV = static_cast<int>(std::rint((value + 1.f) * 5.f));
				switch (nV) {
				case 0: return juce::String("Dry");
				case 10: return juce::String("Wet");
				default: return static_cast<juce::String>(10 - nV) + " : " + static_cast<juce::String>(nV);
				}
			};
			auto lrMsStr = [](float value, int) {
				return value < .5f ? juce::String("L/R") : juce::String("M/S");
			};
			auto sinSqrStr = [](float val, int) {
				juce::juce_wchar s = 's', i = 'i', n = 'n', q = 'q', r = 'r';
				const auto l0 = juce::String::charToString(s);
				const auto l1 = juce::String::charToString(juce::juce_wchar(i + val * (q - i)));
				const auto l2 = juce::String::charToString(juce::juce_wchar(n + val * (r - n)));
				return l0 + l1 + l2;
			};
			const auto depthMaxChoices = util::makeChoicesArray({ "1","2","3","5","8","13","21","34","55","420" });

			parameters.push_back(createPChoice(ID::DepthMax, depthMaxChoices, 2));
			parameters.push_back(createParameter(
				ID::Depth, util::QuadraticBezierRange(0, 1, .51f), DepthDefault, percentStr));
			parameters.push_back(createParameter(
				ID::Freq, util::QuadraticBezierRange(LFOFreqMin, LFOFreqMax, .01f), LFOFreqDefault, freqStr));
			parameters.push_back(createParameter(
				ID::Shape, util::QuadraticBezierRange(0, 1, .01f), 1, sinSqrStr));
			parameters.push_back(createPBool(ID::LRMS, true, lrMsStr));
			parameters.push_back(createParameter(
				ID::Width, util::QuadraticBezierRange(0, 1, .3f), WowWidthDefault, percentStr));
			parameters.push_back(createParameter(
				ID::Mix, juce::NormalisableRange<float>(-1.f, 1.f), 1.f, mixStr));

			return { parameters.begin(), parameters.end() };
		}
	};

	struct Lanczos {
		static constexpr int Alpha = MaxInterpolationOrder;
		Lanczos() :
			sinc()
		{}
		struct SincLUT {
			SincLUT() :
				lut()
			{
				const auto size = SincResolution + 2;
				lut.reserve(size);
				max = Alpha * Pi;
				lut.emplace_back(1.f);
				for (auto i = 1; i < size; ++i) {
					const auto x = static_cast<float>(i) / SincResolution;
					const auto mapped = x * Alpha * Pi;
					lut.emplace_back(sinc(mapped));
				}
			}
			const float operator[](const float idx) const {
				const auto normal = std::abs(idx / max);
				const auto upscaled = normal * SincResolution;
				const auto mapF = static_cast<int>(upscaled);
				const auto mapC = mapF + 1;
				const auto x = upscaled - mapF;
				const auto s0 = lut[mapF];
				const auto s1 = lut[mapC];
				return s0 + x * (s1 - s0);
			}
		private:
			std::vector<float> lut;
			float max;

			float sinc(float xPi) { return std::sin(xPi) / xPi; }
		};
		const float operator()(const std::vector<float>& buffer, const float idx) const {
			const auto iFloor = static_cast<int>(idx);
			const auto x = idx - iFloor;
			const auto size = static_cast<int>(buffer.size());

			auto sum = 0.f;
			for (auto i = -Alpha + 1; i < Alpha; ++i) {
				auto iLegal = i + iFloor;
				if (iLegal < 0) iLegal += size;
				else if (iLegal >= size) iLegal -= size;

				auto xi = x - i;
				if (xi == 0.f) xi = 1.f;
				else if (x > -Alpha && x < Alpha) {
					auto xPi = xi * Pi;
					xi = sinc[xPi] * sinc[xPi / Alpha];
				}
				else xi = 0.f;

				sum += buffer[iLegal] * xi;
			}
			return sum;
		}
	private:
		SincLUT sinc;
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
			const float operator()(const Range& range) {
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
			Rand rand;
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
					Lanczos interpolator;
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
				Range r(min, max);
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

	namespace wt {
		struct Wavetable {
			Wavetable() :
				points(), tmp(),
				vt(),
				id("wavetable"),
				idPoint("point"),
				idX("x"),
				idY("y"),
				hasChanged(false),
				stillChanging(false)
			{}
			// SET
			void setState(juce::ValueTree& v) {
				vt = v.getChildWithName(id);
				if (vt.isValid()) {
					points.clear();
					const auto numChildren = vt.getNumChildren();
					for (auto i = 0; i < numChildren; ++i) {
						const auto child = vt.getChild(i);
						if (child.getType() == idPoint) {
							const auto x = static_cast<float>(child.getProperty(idX));
							const auto y = static_cast<float>(child.getProperty(idY));
							points.push_back({ x, y });
						}
					}
				}
				else {
					vt = juce::ValueTree(id);
					v.appendChild(vt, nullptr);
				}
			}
			void getState() {
				for (const auto& p : points) {
					juce::ValueTree child(idPoint);
					child.setProperty(idX, p.x, nullptr);
					child.setProperty(idY, p.y, nullptr);
					vt.appendChild(child, nullptr);
				}
			}
			void sendChange() { hasChanged.store(true); }
			// PROCESS
			void processBlock() {
				if (hasChanged.load()) {
					points = tmp;
					hasChanged.store(false);
				}
			}
		private:
			std::vector<juce::Point<float>> points, tmp;
			juce::ValueTree vt;
			juce::Identifier id, idPoint, idX, idY;
			std::atomic<bool> hasChanged, stillChanging;
		};
	}

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
				const auto centre = static_cast<float>(utils.delayCenter);
				juce::FloatVectorOperations::multiply(data, centre * .5f, utils.numSamples);
				juce::FloatVectorOperations::add(data, centre, utils.numSamples);
			}
			void synthesizeBlockBypassed(std::vector<float>& data) {
				auto centre = static_cast<float>(utils.delayCenter);
				for (auto& d : data) d = centre;
			}
			// DBG
			void playback(juce::AudioBuffer<float>& buffer, const std::vector<float>& data, const int ch) {
				const auto centre = static_cast<float>(utils.delayCenter);
				const auto centre2 = centre * 2.f;
				auto samples = buffer.getArrayOfWritePointers();
				for (auto s = 0; s < utils.numSamples; ++s)
					samples[ch][s] = data[s] / centre2 - .5f;
			}
		private:
			Phase phase;
			Rand rand;
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
					if (idx == utils.delaySize)
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
					if (data[s] < 0)
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
			void processBlock(juce::AudioBuffer<float>& b, const int* wHead, const std::vector<float>& rHead, const Lanczos& interpolator, const int ch) {
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
				for (auto s = 0; s < utils.numSamples; ++s)
					if (data[s] < 0) data[s] = 0;
					else if (data[s] > utils.delayMax) data[s] = static_cast<float>(utils.delayMax);
			}
			void processBlock(juce::AudioBuffer<float>& buffer, const int* wHead, const Lanczos& interpolator, const int ch) {
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
				width(0.f), dest(static_cast<float>(WowWidthDefault))
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

				depth(1.f), freq(static_cast<float>(LFOFreqDefault)), shape(1), mix(-1.f)
			{
			}
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
				wHead.synthesizeBlock();
				chModules[0].synthesizeLFOBypassed();
				for (auto ch = 1; ch < utils.numChannels; ++ch)
					chModules[ch].copyLFO(chModules[0].data.data());
				for (auto ch = 0; ch < utils.numChannels; ++ch)
					chModules[ch].processBlock(buffer, wHead.data.data(), ch);
			}
			// GET
			std::array<std::atomic<float>, 2>& getLFOValues() { return lfoValues; }
			std::vector<float> data;
		private:
			Lanczos interpolator;
			std::array<std::atomic<float>, 2> lfoValues;
			MixProcessor mixProcessor;
			std::vector<MultiChannelModules> chModules;
			certainty::Generator certainty;
			WriteHead wHead;
			WidthProcessor widthProcessor;
			Utils& utils;

			// PARAM
			float depth, freq, shape, mix;
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
			if (utils.delaySizeChanged(dm))
				prepareToPlay(utils.sampleRate, utils.maxBufferSize, utils.numChannels, true);
		}
		void setDepth(const float depth) { vibrato.setDepth(depth); }
		void setFreq(const float freq) { vibrato.setFreq(freq); }
		void setShape(const float shape) { vibrato.setShape(shape); }
		void setLRMS(const float lrms) { stereoMode.setIsMS(lrms > .5f); }
		void setWidth(const float width) { vibrato.setWidth(width); }
		void setMix(const float mix) { vibrato.setMix(mix); }
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
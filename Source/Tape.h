#pragma once
#include <array>

namespace tape {
	// MATH CONST
	static constexpr double Tau = 6.28318530718;
	static constexpr double Pi = 3.14159265359;
	// INTERPOLATION CONST
	static constexpr int SplineSize = 1 << 12;
	static constexpr int MaxInterpolationOrder = 4;
	static constexpr int InterpolationOrderHalf = MaxInterpolationOrder / 2;
	// CERTAINTY LFO & SMOOTH CONST
	static constexpr float DepthDefault = .1f;
	static constexpr float LFOFreqMin = .5f, LFOFreqMax = 13, LFOFreqDefault = 4.f, LFOFreqInterval = .5f;
	static constexpr int SmoothOrderDefault = 3;
	// DELAY CONST
	static constexpr int DelaySizeInMS = 18; // 25
	static constexpr float WowWidthDefault = 0.f;
	// SLEW CONST
	static constexpr int SlewFreqMin = 20, SlewFreqMax = 20000;
	static constexpr double SlewFreqMaxInv = 1. / SlewFreqMax;

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
			// 0 <= SHAPE < 1 && SHAPE != .5
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

		template<typename Float>
		static void shakerSort(std::vector<Float>& data) {
			auto lenMax = data.size() - 2;
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
		template<typename Float>
		static void shakerSort2(std::vector<Float>& data) {
			auto upperLimit = data.size() - 2;
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
		static double average(const std::vector<double>& data) {
			double x = 0;
			for (const auto& d : data)
				x += d;
			return x / data.size();
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<double>& data, const int ch) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto s = 0; s < buffer.getNumSamples(); ++s)
				samples[ch][s] = (float)data[s];
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<float>& data, const int ch) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto s = 0; s < buffer.getNumSamples(); ++s)
				samples[ch][s] = data[s];
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<double>& l, const std::vector<double>& r) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto s = 0; s < buffer.getNumSamples(); ++s) {
				samples[0][s] = (float)l[s];
				samples[1][s] = (float)r[s];
			}	
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<float>& l, const std::vector<float>& r) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto s = 0; s < buffer.getNumSamples(); ++s) {
				samples[0][s] = l[s];
				samples[1][s] = r[s];
			}
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<double>& data) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
				for (auto s = 0; s < buffer.getNumSamples(); ++s)
					samples[ch][s] = (float)data[s];
		}
		static void playback(juce::AudioBuffer<float>& buffer, const std::vector<float>& data) {
			auto samples = buffer.getArrayOfWritePointers();
			for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
				for (auto s = 0; s < buffer.getNumSamples(); ++s)
					samples[ch][s] = data[s];
		}
		static void dbg(const juce::AudioBuffer<float>& data, const int wordsPerLine = 17) {
			auto samples = data.getArrayOfReadPointers();
			juce::String header = "Samples:\n";
			auto idx = 0;
			for(auto s = 0; s < data.getNumSamples(); ++s) {
				header += juce::String(samples[0][s]) + ", ";
				++idx;
				if (idx == wordsPerLine) {
					header += "\n";
					idx = 0;
				}
			}
			DBG(header += "\n");
		}
		static void dbg(const std::vector<double>& data, juce::String header = "", const int wordsPerLine = 17) {
			if (header.length() != 0) header += ":\n";
			auto idx = 0;
			for (const auto& d : data) {
				header += juce::String(d) + ", ";
				++idx;
				if (idx == wordsPerLine) {
					header += "\n";
					idx = 0;
				}
			}
			DBG(header += "\n");
		}
		static void dbg(const std::vector<float>& data, juce::String header = "", const int wordsPerLine = 17) {
			if (header.length() != 0) header += ":\n";
			auto idx = 0;
			for (const auto& d : data) {
				header += juce::String(d) + ", ";
				++idx;
				if (idx == wordsPerLine) {
					header += "\n";
					idx = 0;
				}
			}
			DBG(header += "\n");
		}
		static void dbg(const std::vector<int>& data, juce::String header = "", const int wordsPerLine = 17) {
			if(header.length() != 0) header += ":\n";
			auto idx = 0;
			for (const auto& d : data) {
				header += juce::String(d) + ", ";
				++idx;
				if (idx == wordsPerLine) {
					header += "\n";
					idx = 0;
				}
			}
			DBG(header += "\n");
		}
	}

	template<typename Float>
	struct Range {
		Range(const Float s = 0, const Float e = 1):
			start(s),
			end(e),
			distance(end - start)
		{}
		// SET
		void set(const Float s, const Float e) {
			start = s;
			end = e;
			distance = e - s;
		}
		// GET
		Float start, end, distance;
		// DBG
		void dbg() const { DBG(end << " - " << start << " = " << distance); }
	};

	struct Rand {
		Rand() :
			gen(std::chrono::high_resolution_clock::now().time_since_epoch().count())
		{}
		const double operator()() { return gen.nextDouble(); }
		const double operator()(const int len) { return gen.nextInt(len); }
	private:
		juce::Random gen;
	};

	template<typename Float>
	struct Utils {
		Utils(juce::AudioProcessor* p) :
			processor(p)
		{}
		// SET
		bool numChannelsChanged(const int num) {
			if (numChannels != num) {
				numChannels = num;
				return true;
			}
			return false;
		}
		bool sampleRateChanged(const double sr) {
			if (sampleRate != sr) {
				sampleRate = sr;

				numChannels = numSamples = 0;

				Fs = (Float)sampleRate;
				FsInv = (Float)1 / Fs;
				Nyquist = Fs / 2;

				setDelaySize((int)ms2samples(DelaySizeInMS));
				return true;
			}
			return false;
		}
		bool maxBufferSizeChanged(const int b) {
			if (maxBufferSize != b) {
				maxBufferSize = b;
				return true;
			}
			return false;
		}
		void setLatency(const int i) { processor->setLatencySamples(i); }

		juce::AudioProcessor* processor;
		double sampleRate = 0;
		Float Fs, FsInv, Nyquist;
		int numChannels, numSamples, maxBufferSize;

		const Float ms2samples(const Float ms) const { return ms * Fs / 1000; }
		const Float hz2inc(const Float frequency) const { return frequency * FsInv; }

		void setDelaySize(const int size) {
			maxDelayTime = size;
			delaySize = 2 * size + MaxInterpolationOrder * 2;
			delayMax = delaySize - 1;
			delayCenter = delaySize / 2;
			delayLatency = delayCenter;
		}
		void refreshDelayLatency(const Float depth) { delayLatency = (int)((delayCenter * depth)) / 16; } // don't know why tho
		
		int maxDelayTime, delaySize, delayMax, delayCenter, delayLatency;
	};

	namespace param {
		enum ID { Lookahead, VibratoDepth, VibratoFreq, VibratoWidth, SlewCutoff };

		static juce::String getName(const int i) {
			switch (i) {
			case ID::Lookahead: return "Lookahead";
			case ID::VibratoDepth: return "Depth";
			case ID::VibratoFreq: return "Freq";
			case ID::VibratoWidth: return "Width";
			case ID::SlewCutoff: return "Slew Cutoff";
			}
			return "";
		}
		static juce::String getID(const int i) { return getName(i).toLowerCase().removeCharacters(" "); }

		static juce::AudioProcessorValueTreeState::ParameterLayout createParameters() {
			std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

			parameters.push_back(std::make_unique<juce::AudioParameterBool>(
				getID(Lookahead), getName(Lookahead), true));
			parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
				getID(VibratoDepth), getName(VibratoDepth), util::QuadraticBezierRange(0, 1, .3f), DepthDefault));
			parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
				getID(VibratoFreq), getName(VibratoFreq), util::QuadraticBezierRange(LFOFreqMin, LFOFreqMax, .55f), LFOFreqDefault));
			parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
				getID(VibratoWidth), getName(VibratoWidth), util::QuadraticBezierRange(0, 1, .4f), WowWidthDefault));
			//parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
			//	getID(SlewCutoff), getName(SlewCutoff), util::LogRange(SlewFreqMin, SlewFreqMax), SlewFreqMax));

			return { parameters.begin(), parameters.end() };
		}
	};

	namespace interpolation {
		template<typename Float>
		struct Interpolation {
			Interpolation(int dataSize, int numNeighbors = 0) :
				access(),
				numNeighbors(numNeighbors)
			{
				if (dataSize != 0) resize(dataSize);
			}
			void resize(int size) {
				auto accessSize = size + numNeighbors;
				access.resize(accessSize, 0);
				for (auto i = 0; i < accessSize; ++i)
					access[i] = i % size;
			}
			std::vector<int> access;
			const int numNeighbors;

			const Float at(const std::vector<Float>& data, const int idx) const { return data[access[idx]]; }
			const Float at(const std::vector<Float>& data, const Float idx) const { return at(data, (int)idx); }
		};

		template<typename Float>
		struct Int : public Interpolation<Float> {
			Int(int size = 0) :
				Interpolation<Float>(size, 0)
			{}
			Float operator()(std::vector<Float>& data, Float idx) { return at(data, idx); }
		};

		template<typename Float>
		struct Rint : public Interpolation<Float> {
			Rint(int size = 0) :
				Interpolation<Float>(size, 1)
			{}
			Float operator()(std::vector<Float>& data, Float idx) { return at(data, std::rint(idx)); }
		};

		template<typename Float>
		struct Linear : public Interpolation<Float> {
			Linear(int size = 0) :
				Interpolation<Float>(size, 1)
			{}
			Float operator()(std::vector<Float>& data, Float idx) {
				auto iFloor = (int)idx;
				auto mix = idx - iFloor;
				return at(data, iFloor) + mix * (at(data, iFloor + 1) - at(data, iFloor));
			}
		};

		template<typename Float>
		struct Cubic : public Interpolation<Float> {
			Cubic(int size = 0) :
				Interpolation<Float>(size, 4),
				spline()
			{
				if (size != 0) initSpecific(size);
			}

			const Float operator()(const std::vector<Float>& data, const Float idx) const {
				auto nIdx = (int)idx;
				auto mix = idx - nIdx;
				auto splIdx = int(SplineSize * mix);
				Float y = 0;
				for (auto n = 0; n < numNeighbors; ++n)
					y += at(data, nIdx + n) * spline[n][splIdx];
				return y;
			}
			void initSpecific(int size) {
				for (auto& spl : spline) spl.resize(SplineSize, 0);
				for (auto i = 0; i < SplineSize; ++i) {
					Float x = (Float)i / SplineSize;
					spline[0][i] = (-std::pow(x, 3) + 2 * std::pow(x, 2) - x) / 2;
					spline[1][i] = (3 * std::pow(x, 3) - 5 * std::pow(x, 2) + 2) / 2;
					spline[2][i] = (-3 * std::pow(x, 3) + 4 * std::pow(x, 2) + x) / 2;
					spline[3][i] = (std::pow(x, 3) - std::pow(x, 2)) / 2;
				}
			}
		private:
			std::array<std::vector<Float>, 4> spline;
		};

		template<typename Float>
		class Lagrange : public Interpolation<Float> {
			struct NeighborTables {
				std::vector<Float> subIJInv;
				std::vector<int> addFloorJ;
			};
		public:
			Lagrange(int size = 0) :
				Interpolation<Float>(size, MaxInterpolationOrder),
				nbt()
			{
				if (size != 0) initSpecific(size);
			}

			const Float operator()(const std::vector<Float>& data, const Float idx) const {
				const auto iFloor = (int)idx;
				auto p = at(data, iFloor);
				for (auto j = 1; j < numNeighbors; ++j)
					p *= (idx - nbt[j].addFloorJ[iFloor]) * nbt[0].subIJInv[j];
				Float yp = p;
				for (auto i = 1; i < numNeighbors; ++i) {
					p = at(data, iFloor + i);
					for (auto j = 0; j < numNeighbors; ++j)
						if (j != i)
							p *= (idx - nbt[j].addFloorJ[iFloor]) * nbt[i].subIJInv[j];
					yp += p;
				}
				return yp;
			}
			const Float operator()(const std::vector<Float>& data, const Float idx, bool saved) const {
				const auto iFloor = (int)((int)idx % data.size());
				auto p = at(data, iFloor);
				for (auto j = 1; j < numNeighbors; ++j)
					p *= (idx - nbt[j].addFloorJ[iFloor]) * nbt[0].subIJInv[j];
				Float yp = p;
				for (auto i = 1; i < numNeighbors; ++i) {
					p = at(data, iFloor + i);
					for (auto j = 0; j < numNeighbors; ++j)
						if (j != i)
							p *= (idx - nbt[j].addFloorJ[iFloor]) * nbt[i].subIJInv[j];
					yp += p;
				}
				return yp;
			}
			void initSpecific(int size) {
				nbt.resize(numNeighbors);
				for (auto n = 0; n < numNeighbors; ++n) {
					nbt[n].subIJInv.clear();
					nbt[n].subIJInv.reserve(numNeighbors);
					for (auto j = 0; j < numNeighbors; ++j)
						if (n != j) nbt[n].subIJInv.emplace_back((Float)1 / (n - j));
						else nbt[n].subIJInv.emplace_back(0);

					nbt[n].addFloorJ.clear();
					nbt[n].addFloorJ.reserve(size);
					for (auto s = 0; s < size; ++s)
						nbt[n].addFloorJ.emplace_back(n + s);
				}
			}
		private:
			std::vector<NeighborTables> nbt;
		};
	}

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

		template<typename Float>
		struct Generator {
			Generator() :
				certainties(),
				rand(),
				size()
			{
				auto rawData = juce::String::createStringFromData(
					BinaryData::likelyRawData_txt, BinaryData::likelyRawData_txtSize
				);

				prepareCertainties(rawData);
				if(ShouldSort) sortCertainties();
				if(ShouldDeleteOutliers) deleteOutliers();
				upscaleCertainties(CertaintiesOrder);
				if (ShouldLimit) limitCertainties();
				if (ShouldNormalize) normalize();
			}

			const Float operator()(const Term term) { return certainties[term][rand(size)]; }
			const Float operator()(const Term term, const Range<Float>& range) {
				return range.start + certainties[term][rand(size)] * range.distance;
			}
			const Float getAverage(const Term term) const {
				Float sum = 0;
				for (auto& c : certainties[term])
					sum += c;
				auto average = sum / size;
				return average;
			}
		private:
			std::array<std::vector<Float>, CertaintiesCount> certainties;
			Rand rand;
			int size;

			void prepareCertainties(const juce::String& rawData) {
				for (auto& c : certainties) c.clear();

				juce::String word;
				auto row = 0;
				for (auto i = 0; i < rawData.length(); ++i) {
					auto character = rawData[i];
					if (character == '\n') { // next line (row restart)
						addWordToCertainties(word, row);
						row = 0;
					}
					else if (character == ',') { // next argument (next row)
						addWordToCertainties(word, row);
						++row;
					}
					else word += character; // keep on writing word
				}
				addWordToCertainties(word, row);
			}
			void addWordToCertainties(juce::String& word, const int row) {
				auto certaintyInPercent = (Float)word.getIntValue();
				auto certaintyAsDecimal = certaintyInPercent / 100;
				certainties[row].push_back(certaintyAsDecimal);
				word.clear();
			}

			void sortCertainties() {
				for (auto& c : certainties)
					util::shakerSort2(c);
			}
			void deleteOutliers() {
				for (auto& c : certainties) {
					Float averageInc = 0;
					for (auto i = 1; i < CertaintiesPerCertainty; ++i)
						averageInc += c[i] - c[i - 1];
					averageInc /= CertaintiesPerCertainty;
					auto half = CertaintiesPerCertainty / 2;
					for (auto i = 0; i < half; ++i)
						if (c[i + 1] - c[i] > averageInc)
							c[i] = c[i + 1];
					for (auto i = half + 1; i < CertaintiesPerCertainty; ++i)
						if (c[i] - c[i - 1] > averageInc)
							c[i] = c[i - 1];
				}
			}
			void upscaleCertainties(const int order) {
				size = order * CertaintiesPerCertainty;
				if (order != 1) {
					interpolation::Lagrange<Float> interpolate(CertaintiesPerCertainty);
					std::array<std::vector<Float>, CertaintiesCount> newData;
					for (auto c = 0; c < CertaintiesCount; ++c) {
						newData[c].reserve(size);
						for (auto n = 0; n < size; ++n) {
							auto idx = Float(n) * CertaintiesPerCertainty / size;
							newData[c].emplace_back(interpolate(certainties[c], idx));
						}
					}
					certainties = newData;
				}
			}
			void limitCertainties() {
				for (auto& c : certainties)
					for (auto& i : c)
						if (i < 0) i = 0;
						else if (i >= 1) i = (Float)1 - std::numeric_limits<Float>::epsilon();
			}
			void normalize() {
				for (auto c = 0; c < CertaintiesCount; ++c) {
					auto min = (Float)1;
					auto max = (Float)0;
					for (auto& s : certainties[c]) {
						if (s < min) min = s;
						if (s > max) max = s;
					}
					Range<Float> r(min, max);
					for (auto& s : certainties[c])
						s = (s - r.start) / r.distance;
				}
			}
		};
	}

	namespace vibrato {
		template<typename Float>
		struct Phase {
			Phase(const Utils<Float>& utils) :
				phase(1),
				inc(0),
				utils(utils)
			{}
			void setFrequencyInHz(Float hz) { inc = utils.hz2inc(hz); }
			bool operator()() {
				phase += inc;
				if (phase >= 1) {
					--phase;
					return true;
				}
				return false;
			}
			Float phase;
		private:
			Float inc;
			const Utils<Float>& utils;
		};

		template<typename Float>
		struct Smooth {
			Smooth(const Utils<Float>& utils, Float start = 0) :
				utils(utils),
				value(start),
				inertia(1)
			{}
			// SET / PARAM
			void setInertiaInMs(Float ms) { inertia = (Float)1 / utils.ms2samples(ms); }
			void setInertiaInHz(Float hz) { inertia = utils.hz2inc(hz); }
			void setInertiaInSamples(Float smpls) { inertia = (Float)1 / smpls; }
			// PROCESS
			const Float operator()(const Float x) {
				value += inertia * (x - value);
				return value;
			}
		private:
			const Utils<Float>& utils;
			Float value, inertia;
		};

		template<typename Float>
		struct MultiOrderSmooth {
			MultiOrderSmooth(const Utils<Float>& utils, const int order = 1) :
				smooth(),
				utils(utils)
			{
				setOrder(order);
			}
			// SET / PARAM
			void setOrder(int o) { smooth.resize(o, { utils, 0 }); }
			void setInertiaInMs(Float ms) {
				for (auto& s : smooth)
					s.setInertiaInMs(ms);
			}
			void setInertiaInHz(Float hz) {
				for (auto& s : smooth)
					s.setInertiaInHz(hz);
			}
			void setInertiaInSamples(Float smpls) {
				for (auto& s : smooth)
					s.setInertiaInSamples(smpls);
			}
			// PROCESS
			void processBlock(std::vector<Float>& data) {
				for (auto d = 0; d < utils.numSamples; ++d)
					for (auto& s : smooth)
						data[d] = s(data[d]);
			}
		private:
			std::vector<Smooth<Float>> smooth;
			const Utils<Float>& utils;
		};

		template<typename Float>
		struct WIdx {
			WIdx(const Utils<Float>& utils) :
				data(),
				utils(utils),
				idx(0)
			{}
			// SET
			void setMaxBufferSize() {
				data.clear();
				data.resize(utils.maxBufferSize, 0);
				idx = 0;
			}
			// PROCESS
			void processBlock() {
				for (auto s = 0; s < utils.numSamples; ++s) {
					data[s] = idx;
					++idx;
					if (idx == utils.delaySize)
						idx = 0;
				}
			}
			// GET
			const int operator[](const int i) const { return data[i]; }
			std::vector<int> data;
		private:
			const Utils<Float>& utils;
			int idx;
		};

		template<typename Float>
		struct CertaintySequencer {
			CertaintySequencer(const Utils<Float>& utils, certainty::Generator<Float>& certainty) :
				phase(utils),
				rand(),
				smooth(utils, SmoothOrderDefault),
				utils(utils),
				certainty(certainty),
				curValue(0)
			{}
			// PARAM
			void setFrequencyInHz(Float hz) {
				phase.setFrequencyInHz(hz);
				smooth.setInertiaInHz(hz * SmoothOrderDefault);
			}
			void setDepth(Float d) { depth = d; }
			// PROCESS
			void synthesizeBlock(std::vector<Float>& data, const certainty::Term term) {
				synthesizeRandomValues(data, term);
			}
			void scaleForDelay(Float* data) {
				juce::FloatVectorOperations::multiply(data, utils.maxDelayTime, utils.numSamples);
				juce::FloatVectorOperations::add(data, utils.delayCenter, utils.numSamples);
			}
			void synthesizeBlockBypassed(std::vector<Float>& data) { for (auto& d : data) d = utils.delayCenter; }
			// DBG
			void playback(juce::AudioBuffer<float>& buffer, const std::vector<Float>& data, const int ch) {
				auto samples = buffer.getArrayOfWritePointers();
				for (auto s = 0; s < utils.numSamples; ++s) {
					samples[ch][s] = (float)((data[s] - utils.delayCenter) / utils.maxDelayTime);
				}
			}
		private:
			Phase<Float> phase;
			Rand rand;
			MultiOrderSmooth<Float> smooth;
			const Utils<Float>& utils;
			certainty::Generator<Float>& certainty;

			Float curValue, depth;

			/* synthesizes a smooth random curve (-1, 1) */
			void synthesizeRandomValues(std::vector<Float>& data, const certainty::Term term) {
				for (auto s = 0; s < utils.numSamples; ++s) {
					if (phase()) {
						curValue = certainty(term) * depth;
						if (rand() > .5)
							curValue *= -1;
					}
					data[s] = curValue;
				}
				smooth.processBlock(data);
			}
		};

		template<typename Float>
		struct RIdx {
			RIdx(const Utils<Float>& utils) :
				latencySmooth(utils, 2),
				utils(utils)
			{}
			// SET
			void setMaxBufferSize(const int maxBufferSize) {
				latencySmooth.setInertiaInHz(LFOFreqMax);
				latencyData.resize(maxBufferSize, 0);
			}
			// PROCESS
			void processBlock(std::vector<Float>& data, const int* wIdx) {
				for (auto s = 0; s < utils.numSamples; ++s) {
					data[s] = wIdx[s] - data[s];
					if (data[s] < 0)
						data[s] += utils.delaySize;
				}
			}
			void processBlockWithLatencyReduction(std::vector<Float>& data, const int* wIdx) {
				for (auto l = 0; l < utils.numSamples; ++l) latencyData[l] = utils.delayLatency;
				latencySmooth.processBlock(latencyData); // latency smoothing

				for (auto s = 0; s < utils.numSamples; ++s) {
					data[s] -= latencyData[s];
					data[s] = wIdx[s] - data[s];
					if (data[s] < 0)
						data[s] += utils.delaySize;
				}
			}
		private:
			std::vector<Float> latencyData;
			MultiOrderSmooth<Float> latencySmooth;
			const Utils<Float>& utils;
		};

		template<typename Float, class Interpolation>
		struct FFDelay {
			FFDelay(const Utils<Float>& utils) :
				buffer(),
				utils(utils)
			{
				buffer.resize(utils.delaySize, 0);
			}
			// PROCESS
			void processBlock(juce::AudioBuffer<float>& b, const int* wIdx, const std::vector<Float>& rIdx, const Interpolation& interpolate, const int ch) {
				auto samples = b.getWritePointer(ch, 0);
				for (auto s = 0; s < utils.numSamples; ++s) {
					buffer[wIdx[s]] = samples[s];
					samples[s] = interpolate(buffer, rIdx[s]);
				}
			}
			void processBlock(juce::AudioBuffer<float>& b, const int* wIdx, const std::vector<Float>& rIdx, const int ch) {
				auto samples = b.getWritePointer(ch, 0);
				for (auto s = 0; s < utils.numSamples; ++s) {
					buffer[wIdx[s]] = samples[s];
					samples[s] = buffer[(int)rIdx[s]];
				}
			}
		private:
			std::vector<Float> buffer;
			const Utils<Float>& utils;
		};

		template<typename Float, class Interpolation>
		struct MultiChannelModules {
			MultiChannelModules(Utils<Float>& utils, certainty::Generator<Float>& certainty) :
				data(),
				lfoValue(0),
				seq(utils, certainty),
				rIdx(utils),
				delay(utils),
				utils(utils)
			{
				data.resize(utils.maxBufferSize, 0);
				rIdx.setMaxBufferSize(utils.maxBufferSize);
			}
			// PARAM
			void setDepth(const Float d) { seq.setDepth(d); }
			void setFreq(const Float f) { seq.setFrequencyInHz(f); }
			// PROCESS
			void synthesizeLFO() { seq.synthesizeBlock(data, tape::certainty::HighlyUnlikely); }
			void synthesizeLFOBypassed() { seq.synthesizeBlockBypassed(data); }
			void copyLFO(Float* other) { juce::FloatVectorOperations::copy(data.data(), other, utils.numSamples); } // IF WIDTH == 0
			void mixLFO(Float* other, const std::vector<Float>& widthData) { // FOR ALL LFOS of channel > 0 && WIDTH != 0 WITH BUFFER
				for (auto s = 0; s < utils.numSamples; ++s)
					data[s] = other[s] + widthData[s] * (data[s] - other[s]);
			}
			void saveLFOValue() { lfoValue = data[utils.numSamples - 1]; }
			void upscaleLFO() { seq.scaleForDelay(data.data()); }
			void processBlock(juce::AudioBuffer<float>& buffer, const int* wIdx, const Interpolation& interpolate, const int ch) {
				//seq.playback(buffer, data, ch);
				if(utils.processor->getLatencySamples() != 0) rIdx.processBlock(data, wIdx); // LOOKAHEAD ENABLED
				else rIdx.processBlockWithLatencyReduction(data, wIdx); // LOOKAHEAD DISABLED
				delay.processBlock(buffer, wIdx, data, interpolate, ch);
			}
			void processBlock(juce::AudioBuffer<float>& buffer, const int* wIdx, const int ch) {
				rIdx.processBlock(data, wIdx);
				delay.processBlock(buffer, wIdx, data, ch);
			}
			// GET
			std::vector<Float> data;
			Float lfoValue;
		private:
			CertaintySequencer<Float> seq;
			RIdx<Float> rIdx;
			FFDelay<Float, Interpolation> delay;
			const Utils<Float>& utils;
		};

		template<typename Float, class Interpolation>
		struct WidthProcessor {
			WidthProcessor(const Utils<Float>& utils) :
				utils(utils),
				smooth(utils, 2),
				width(0), dest((Float)WowWidthDefault)
			{}
			// SET
			void init() { smooth.setInertiaInHz(20); }
			// PARAM
			void setWidth(Float w) { dest = w; }
			// PROCESS
			void operator()(std::vector<Float>& data, std::vector<MultiChannelModules<Float, Interpolation>>& chModules) {
				for (auto s = 0; s < utils.numSamples; ++s) data[s] = dest;
				smooth.processBlock(data); // smoothen width transitions

				if (utils.numChannels == 1)
					processWidthDisabled(chModules);
				else
					processWidthEnabled(chModules, data);
			}
		private:
			const Utils<Float>& utils;
			MultiOrderSmooth<Float> smooth;
			Float width, dest;

			void processWidthDisabled(std::vector<MultiChannelModules<Float, Interpolation>>& chModules) {
				chModules[0].upscaleLFO();
			}
			void processWidthEnabled(std::vector<MultiChannelModules<Float, Interpolation>>& chModules, const std::vector<Float>& widthData) {
				for (auto ch = 1; ch < utils.numChannels; ++ch) {
					chModules[ch].synthesizeLFO();
					chModules[ch].mixLFO(chModules[0].data.data(), widthData);
					chModules[ch].saveLFOValue();
					chModules[ch].upscaleLFO();
				}
				chModules[0].upscaleLFO();
			}
		};

		template<typename Float>
		struct Vibrato {
			Vibrato(Utils<Float>& utils) :
				chModules(),
				certainty(),
				interpolation(),
				wIdx(utils),
				widthProcessor(utils),
				utils(utils),

				depth(1), freq((Float)LFOFreqDefault),
				lookaheadEnabled(false)
			{}
			// SET
			void init() {
				widthProcessor.init();
				interpolation.resize(utils.delaySize);
				interpolation.initSpecific(utils.delaySize);
			}
			void setNumChannels() {
				chModules.resize(utils.numChannels, { utils, certainty });
				setDepth(depth);
				setFreq(freq);
			}
			void setMaxBufferSize() {
				data.resize(utils.maxBufferSize, 0);
				wIdx.setMaxBufferSize();
			}
			// PARAM
			void setDepth(const Float d) {
				depth = d;
				utils.refreshDelayLatency(depth);
				for (auto& ch : chModules)
					ch.setDepth(depth);
			}
			void setFreq(const Float f) {
				freq = f;
				for (auto& ch : chModules)
					ch.setFreq(freq);
			}
			void setWidth(const Float w) {
				widthProcessor.setWidth(w);
			}
			// PROCESS
			void processBlock(juce::AudioBuffer<float>& buffer) {
				wIdx.processBlock();
				chModules[0].synthesizeLFO();
				chModules[0].saveLFOValue();
				widthProcessor(data, chModules);
				for (auto ch = 0; ch < utils.numChannels; ++ch)
					chModules[ch].processBlock(buffer, wIdx.data.data(), interpolation, ch);
			}
			void processBlockBypassed(juce::AudioBuffer<float>& buffer) {
				wIdx.processBlock();
				chModules[0].synthesizeLFOBypassed();
				for (auto ch = 1; ch < utils.numChannels; ++ch)
					chModules[ch].copyLFO(chModules[0].data.data());
				for (auto ch = 0; ch < utils.numChannels; ++ch)
					chModules[ch].processBlock(buffer, wIdx.data.data(), ch);
			}
			// GET
			const Float* getLFOValue(const int ch) const { return &chModules[ch].lfoValue; }
			std::vector<Float> data;
		private:
			std::vector<MultiChannelModules<Float, interpolation::Cubic<Float>>> chModules;
			certainty::Generator<Float> certainty;
			interpolation::Cubic<Float> interpolation;
			WIdx<Float> wIdx;
			WidthProcessor<Float, interpolation::Cubic<Float>> widthProcessor;
			Utils<Float>& utils;

			// PARAM
			Float depth, freq;
			bool lookaheadEnabled;
		};
	}

	template<typename Float>
	struct Tape {
		Tape(juce::AudioProcessor* p) :
			utils(p),
			vibrato(utils),
			lookaheadEnabled(false)
		{}
		// SET
		void prepareToPlay(const double sampleRate, const int maxBufferSize, const int channelCount) {
			if (utils.sampleRateChanged(sampleRate)) {
				vibrato.init();
				setLookaheadEnabled(lookaheadEnabled);
			}
			if (utils.maxBufferSizeChanged(maxBufferSize))
				vibrato.setMaxBufferSize();

			if(utils.numChannelsChanged(channelCount))
				vibrato.setNumChannels();
		}
		// PARAM
		void setLookaheadEnabled(const bool enabled) {
			if (lookaheadEnabled != enabled) {
				lookaheadEnabled = enabled;
				if(lookaheadEnabled) utils.setLatency(utils.delayCenter);
				else utils.setLatency(0);
			}
		}
		void setWowDepth(const Float depth) { vibrato.setDepth(depth); }
		void setWowFreq(const Float freq) { vibrato.setFreq(freq); }
		void setWowWidth(const Float width) { vibrato.setWidth(width); }
		// PROCESS
		void processBlock(juce::AudioBuffer<float>& buffer) {
			utils.numSamples = buffer.getNumSamples();
			vibrato.processBlock(buffer);
		}
		void processBlockBypassed(juce::AudioBuffer<float>& buffer) {
			if (!lookaheadEnabled) return;
			utils.numSamples = buffer.getNumSamples();
			vibrato.processBlockBypassed(buffer);
		}
		// GET
		const Float* getLFOValue(const int ch) const { return vibrato.getLFOValue(ch); }
	private:
		Utils<Float> utils;
		vibrato::Vibrato<Float> vibrato;
		bool lookaheadEnabled;
	};
}

/* TO DO

BUGS:
	FL Randomizer makes plugin freeze
		processBlock not called anymore
		parameters not shown above Randomize
	STUDIO ONE v5.1 (S.B. Davis):
		jiterry output
		daw crashes when plugin is removed
	STUDIO ONE
		no mouseCursor shown

IMPROVEMENTS:
	add a manual (especially because of the ALT- and SHIFT-features)
	temposync
	multiband
	mix
	options menue, alternative design-parameter based
	parameter symbols unclear
	Installer
		because people might not know where vst3 is

TEST:
	LIVE
		gitarristen
		synths
	DAWS
		cubase      CHECK 9.5, 10
		fl          CHECK
		ableton		CHECK (thx julian)
		bitwig
		protools
		studio one  CHECK, but might not

DAWS Debug:

D:\Pogramme\Cubase 9.5\Cubase9.5.exe
D:\Pogramme\FL Studio 20\FL64.exe
D:\Pogramme\Studio One 5\Studio One.exe

*/
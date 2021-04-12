#pragma once

namespace util {
	// MATH CONST
	static constexpr float Pi = 3.14159265359f;
	static constexpr float Tau = 6.28318530718f;
	static constexpr float PiHalf = Pi * .5f;
	static constexpr float PiQuart = Pi * .25f;
	static constexpr float PiInv = 1.f / Pi;
	// GRAPHICS
	static constexpr float Thicc = 3.f;
	static constexpr float Thicc2 = Thicc * 2.f;
	static constexpr float Rounded = 4.f;
	static constexpr float DialTickCount = 16.f;
	static constexpr unsigned int ColBlack = 0xff171623;
	static constexpr unsigned int ColDarkGrey = 0xff595652;
	static constexpr unsigned int ColBeige = 0xff9c8980;
	static constexpr unsigned int ColGrey = 0xff696a6a;
	static constexpr unsigned int ColRed = 0xffac3232;
	static constexpr unsigned int ColGreen = 0xff37946e;
	static constexpr unsigned int ColYellow = 0xfffffa8f;
	static constexpr unsigned int ColGreenNeon = 0xff99c550;

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

	struct Point :
		public juce::Point<float>
	{
		Point(float xx = 0, float yy = 0) :
			juce::Point<float>(xx, yy)
		{}
		bool operator<(Point& p) { return this->x < p.x; }
		bool operator<(juce::Point<float>& p) { return this->x < p.x; }
	};

	struct Range {
		Range(const float s = 0, const float e = 1) :
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

	struct Counter {
		Counter() :
			now(std::chrono::high_resolution_clock::now())
		{}
		double operator()() { return get(); }
		~Counter() { DBG(get() << "ms"); }
	private:
		std::chrono::high_resolution_clock::time_point now;

		double get() {
			const auto then = std::chrono::high_resolution_clock::now();
			const auto dif = then - now;
			return static_cast<double>(dif.count()) * .000001;
		}
	};
}
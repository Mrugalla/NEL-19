#pragma once

namespace modSys2 {
	static constexpr float pi = 3.14159265359f;
	static constexpr float tau = 6.28318530718f;
	static float noteInHz(const float note) noexcept {
		return std::pow(2.f, (note - 69.f) * .08333333333f) * 440.f;
	}
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
		cpi.isPlaying = false;
		cpi.isRecording = false;
		cpi.ppqLoopEnd = 1;
		cpi.ppqLoopStart = 0;
		cpi.ppqPosition = -1;
		cpi.ppqPositionOfLastBarStart = 69;
		cpi.timeInSamples = -1;
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
}


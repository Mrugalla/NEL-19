#pragma once
#include <JuceHeader.h>

namespace param {
	static constexpr int PerlinMaxOctaves = 8;

	enum class ID {
		Macro0, Macro1, Macro2, Macro3, // general modulator params

		EnvFolGain0, EnvFolAtk0, EnvFolRls0, EnvFolBias0, EnvFolWidth0, // modulator 0 params
		LFOSync0, LFORate0, LFOWidth0, LFOWaveTable0, LFOPolarity0, LFOPhase0,
		RandSync0, RandRate0, RandBias0, RandWidth0, RandSmooth0,
		PerlinRate0, PerlinOctaves0, PerlinWidth0,

		EnvFolGain1, EnvFolAtk1, EnvFolRls1, EnvFolBias1, EnvFolWidth1, // modulator 1 params
		LFOSync1, LFORate1, LFOWidth1, LFOWaveTable1, LFOPolarity1, LFOPhase1,
		RandSync1, RandRate1, RandBias1, RandWidth1, RandSmooth1,
		PerlinRate1, PerlinOctaves1, PerlinWidth1,

		Depth, ModulatorsMix, DryWetMix, Voices, StereoConfig, // non modulator params (crossfb?)
		EnumSize
	};

	// PARAMETER ID STUFF
	static juce::String getName(ID i) {
		switch (i) {
		case ID::Macro0: return "Macro 0";
		case ID::Macro1: return "Macro 1";
		case ID::Macro2: return "Macro 2";
		case ID::Macro3: return "Macro 3";
		
		case ID::EnvFolGain0: return "EnvFolGain 0";
		case ID::EnvFolAtk0: return "EnvFolAtk 0";
		case ID::EnvFolRls0: return "EnvFolRls 0";
		case ID::EnvFolBias0: return "EnvFolBias 0";
		case ID::EnvFolWidth0: return "EnvFolWidth 0";
		case ID::LFOSync0: return "LFOSync 0";
		case ID::LFORate0: return "LFORate 0";
		case ID::LFOWidth0: return "LFOWidth 0";
		case ID::LFOWaveTable0: return "LFOWaveTable 0";
		case ID::LFOPolarity0: return "LFOPolarity 0";
		case ID::LFOPhase0: return "LFOPhase 0";
		case ID::RandSync0: return "RandSync 0";
		case ID::RandRate0: return "RandRate 0";
		case ID::RandBias0: return "RandBias 0";
		case ID::RandWidth0: return "RandWidth 0";
		case ID::RandSmooth0: return "RandSmooth 0";
		case ID::PerlinRate0: return "PerlinRate 0";
		case ID::PerlinOctaves0: return "PerlinOctaves 0";
		case ID::PerlinWidth0: return "PerlinWidth 0";

		case ID::EnvFolGain1: return "EnvFolGain 1";
		case ID::EnvFolAtk1: return "EnvFolAtk 1";
		case ID::EnvFolRls1: return "EnvFolRls 1";
		case ID::EnvFolBias1: return "EnvFolBias 1";
		case ID::EnvFolWidth1: return "EnvFolWidth 1";
		case ID::LFOSync1: return "LFOSync 1";
		case ID::LFORate1: return "LFORate 1";
		case ID::LFOWidth1: return "LFOWidth 1";
		case ID::LFOWaveTable1: return "LFOWaveTable 1";
		case ID::LFOPolarity1: return "LFOPolarity 1";
		case ID::LFOPhase1: return "LFOPhase 1";
		case ID::RandSync1: return "RandSync 1";
		case ID::RandRate1: return "RandRate 1";
		case ID::RandBias1: return "RandBias 1";
		case ID::RandWidth1: return "RandWidth 1";
		case ID::RandSmooth1: return "RandSmooth 1";
		case ID::PerlinRate1: return "PerlinRate 1";
		case ID::PerlinOctaves1: return "PerlinOctaves 1";
		case ID::PerlinWidth1: return "PerlinWidth 1";

		case ID::Depth: return "Depth"; // put depth max somewhere else because buffer realloc
		case ID::ModulatorsMix: return "Mix Mods";
		case ID::DryWetMix: return "Mix DryWet";
		case ID::Voices: return "Voices";
		case ID::StereoConfig: return "StereoConfig";

		default: return "";
		}
	}
	static juce::String getName(int i) { getName(static_cast<ID>(i)); }
	static juce::String getID(const ID i) { return getName(i).toLowerCase().removeCharacters(" "); }
	static juce::String getID(const int i) { return getName(i).toLowerCase().removeCharacters(" "); }

	// NORMALISABLE RANGE STUFF
	static juce::NormalisableRange<float> getBiasedRange(float min, float max, float bias) noexcept {
		bias = 1.f - bias;
		const auto biasInv = 1.f / bias;
		const auto range = max - min;
		const auto rangeInv = 1.f / range;
		return juce::NormalisableRange<float>(min, max,
			[b = bias, bInv = biasInv, r = range](float start, float end, float normalised) {
				return start + r * std::pow(normalised, bInv);
			},
			[b = bias, rInv = rangeInv](float start, float end, float denormalized) {
				return std::pow((denormalized - start) * rInv, b);
			}
			);
	}

	// PARAMETER CREATION STUFF
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
	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, float defaultValue,
		std::function<juce::String(float value, int maxLen)> stringFromValue,
		const juce::NormalisableRange<float>& range) {
		return std::make_unique<juce::AudioParameterFloat>(
			getID(i), getName(i), range, defaultValue, getName(i), juce::AudioProcessorParameter::Category::genericParameter,
			stringFromValue
			);
	}
	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, float defaultValue = 1.f,
		std::function<juce::String(float value, int maxLen)> stringFromValue = nullptr,
		const float min = 0.f, const float max = 1.f, const float interval = -1.f) {
		if(interval != -1.f)
			return createParameter(i, defaultValue, stringFromValue, juce::NormalisableRange<float>(min, max, interval));
		else
			return createParameter(i, defaultValue, stringFromValue, juce::NormalisableRange<float>(min, max));
	}

	// TEMPOSYNC VS FREE STUFF
	struct MultiRange {
		struct Range
		{
			Range(const juce::String& rID, const juce::NormalisableRange<float>& r) :
				id(rID),
				range(r)
			{}
			bool operator==(const juce::Identifier& rID) const { return id == rID; }
			const juce::NormalisableRange<float>& operator()() const { return range; }
			const juce::Identifier& getID() const { return id; }
		protected:
			const juce::Identifier id;
			const juce::NormalisableRange<float> range;
		};
		MultiRange() :
			ranges()
		{}
		void add(const juce::String& rID, const juce::NormalisableRange<float>&& r) { ranges.push_back({ rID, r }); }
		void add(const juce::String& rID, const juce::NormalisableRange<float>& r) { ranges.push_back({ rID, r }); }
		const juce::NormalisableRange<float>& operator()(const juce::Identifier& rID) const {
			for (const auto& r : ranges)
				if (r == rID)
					return r();
			return ranges[0]();
		}
		const juce::Identifier& getID(const juce::String&& idStr) const {
			for (const auto& r : ranges)
				if (r.getID().toString() == idStr)
					return r.getID();
			return ranges[0].getID();
		}
	private:
		std::vector<Range> ranges;
	};

	static std::vector<float> getTempoSyncValues(int range) {
		/*
		* range = 6 => 2^6=64 => [1, 64] =>
		* 1, 1., 1t, 1/2, 1/2., 1/2t, 1/4, 1/4., 1/4t, ..., 1/64
		*/
		const auto numRates = range * 3 + 1;
		std::vector<float> rates;
		rates.reserve(numRates);
		for (auto i = 0; i < range; ++i) {
			const auto denominator = 1 << i;
			const auto beat = 1.f / static_cast<float>(denominator);
			const auto euclid = beat * 3.f / 4.f;
			const auto triplet = beat * 2.f / 3.f;
			rates.emplace_back(beat);
			rates.emplace_back(euclid);
			rates.emplace_back(triplet);
		}
		const auto denominator = 1 << range;
		const auto beat = 1.f / static_cast<float>(denominator);
		rates.emplace_back(beat);
		return rates;
	}
	static std::vector<juce::String> getTempoSyncStrings(int range) {
		/*
		* range = 6 => 2^6=64 => [1, 64] =>
		* 1, 1., 1t, 1/2, 1/2., 1/2t, 1/4, 1/4., 1/4t, ..., 1/64
		*/
		const auto numRates = range * 3 + 1;
		std::vector<juce::String> rates;
		rates.reserve(numRates);
		for (auto i = 0; i < range; ++i) {
			const auto denominator = 1 << i;
			juce::String beat("1/" + juce::String(denominator));
			rates.emplace_back(beat);
			rates.emplace_back(beat + ".");
			rates.emplace_back(beat + "t");
		}
		const auto denominator = 1 << range;
		juce::String beat("1/" + juce::String(denominator));
		rates.emplace_back(beat);
		return rates;
	}
	static juce::NormalisableRange<float> getTempoSyncRange(const std::vector<float>& rates) {
		return juce::NormalisableRange<float>(
			0.f, static_cast<float>(rates.size()),
			[rates](float, float end, float normalized) {
				const auto idx = static_cast<int>(normalized * end);
				if (idx >= end) return rates[idx - 1];
				return rates[idx];
			},
			[rates](float, float end, float mapped) {
				for (auto r = 0; r < end; ++r)
					if (rates[r] == mapped)
						return static_cast<float>(r) / end;
				return 0.f;
			}
			);
	}
	static std::function<juce::String(float, int)> getRateStr(
		const juce::AudioProcessorValueTreeState& apvts,
		const ID syncID,
		const juce::NormalisableRange<float>& freeRange,
		const std::vector<juce::String>& syncStrings)
	{
		return [&apvts, syncID, freeRange, syncStrings](float value, int) {
			const auto syncP = apvts.getRawParameterValue(getID(syncID));
			const auto sync = syncP->load();
			if (sync < .5f) {
				value = freeRange.convertFrom0to1(value);
				if (value < 10) {
					value = std::rint(value * 100.f) * .01f;
					const juce::String valueStr(value);
					if(valueStr.length() < 3)
						return valueStr + ".00 hz";
					else if(valueStr.length() < 4)
						return valueStr + "0 hz";
					else
						return valueStr + " hz";
				}
				else
					return static_cast<juce::String>(std::rint(value)) + " hz";
			}
			else {
				const auto idx = static_cast<int>(value * syncStrings.size());
				return value < 1.f ? syncStrings[idx] : syncStrings[idx - 1];
			}
		};
	}


	static juce::AudioProcessorValueTreeState::ParameterLayout createParameters(const juce::AudioProcessorValueTreeState& apvts, MultiRange& modRateRanges) {
		std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

		// string lambdas
		// [0, 1]:
		auto percentStr = [](float value, int) {
			if (value == 1.f) return juce::String("100 %");
			value *= 100.f;
			if (value > 9.f) return static_cast<juce::String>(value).substring(0, 2) + " %";
			return static_cast<juce::String>(value).substring(0, 1) + " %";
		};
		// [-1, 1]:
		const auto percentStrM1 = [](float value, int) {
			if (value == 1.f) return juce::String("100 %");
			value = (value + 1.f) * 50.f;
			if (value > 9.f) return static_cast<juce::String>(value).substring(0, 2) + " %";
			return static_cast<juce::String>(value).substring(0, 1) + " %";
		};
		// [0, 1]:
		const auto normBiasStr = [](float value, int) {
			value = std::floor(value * 100.f);
			return juce::String(value) + " %";

		};
		const auto freqStr = [](float value, int) {
			if (value < 10.f)
				return static_cast<juce::String>(value).substring(0, 3) + " hz";
			return static_cast<juce::String>(value).substring(0, 2) + " hz";
		};
		// [-1, 1]:
		const auto mixStr = [](float value, int) {
			auto nV = static_cast<int>(std::rint((value + 1.f) * 5.f));
			switch (nV) {
			case 0: return juce::String("Dry");
			case 10: return juce::String("Wet");
			default: return static_cast<juce::String>(10 - nV) + " : " + static_cast<juce::String>(nV);
			}
		};
		// [0, 1]
		const auto modsMixStr = [](float value, int) {
			// [0,1] >> [25:75] etc
			const auto mapped = value * 100.f;
			const auto beautified = std::floor(mapped);
			return static_cast<juce::String>(100.f - beautified) + " : " + static_cast<juce::String>(beautified);
		};
		const auto lrMsStr = [](float value, int) {
			return value < .5f ? juce::String("L/R") : juce::String("M/S");
		};
		const auto dbStr = [](float value, int) {
			return static_cast<juce::String>(std::rint(value)).substring(0, 5) + " db";
		};
		const auto msStr = [](float value, int) {
			return static_cast<juce::String>(std::rint(value)).substring(0, 5) + " ms";
		};
		const auto syncStr = [](bool value, int) {
			return value ? static_cast<juce::String>("Sync") : static_cast<juce::String>("Free");
		};
		const auto wtStr = [](float value, int) {
			return value < 1 ?
				juce::String("SIN") : value < 2 ?
				juce::String("TRI") : value < 3 ?
				juce::String("SQR") :
				juce::String("SAW");
		};
		const auto octStr = [](float value, int) {
			return juce::String(static_cast<int>(value)) + " oct";
		};
		const auto voicesStr = [](float value, int) {
			return juce::String(static_cast<int>(value)) + " voices";
		};
		const auto polarityStr = [](float value, int) {
			return value < .5f ?
				juce::String("Polarity Off") :
				juce::String("Polarity On");
		};
		// [-1, 1]
		const auto phaseStr = [](float value, int) {
			return juce::String(std::floor(value * 180.f)) + " dgr";
		};

		parameters.push_back(createParameter(ID::Macro0, 0.f, percentStr));
		parameters.push_back(createParameter(ID::Macro1, 0.f, percentStr));
		parameters.push_back(createParameter(ID::Macro2, 0.f, percentStr));
		parameters.push_back(createParameter(ID::Macro3, 0.f, percentStr));

		// general constants of the env fol parameter creation
		static constexpr float maxEnvFolAtk = 10000.f;
		static constexpr float maxEnvFolRls = 10000.f;
		static constexpr float EnvFolAtkRlsBias = .75f;
		const auto envFolAtkRlsRange = getBiasedRange(1.f, maxEnvFolAtk, EnvFolAtkRlsBias);

		// general constants of the lfo rand perlin's freq parameter creation
		static constexpr float lfoRandPerlinMinFreq = .1f;
		static constexpr float lfoRandPerlinMaxFreq = 24.f;
		static constexpr float lfoRandPerlinFreqBias = .5f;
		const auto lfoRandPerlinFreqRange = getBiasedRange(lfoRandPerlinMinFreq, lfoRandPerlinMaxFreq, lfoRandPerlinFreqBias);
		static constexpr int maxSyncTime = 6; // 64th notes
		const auto tsValues = getTempoSyncValues(maxSyncTime);
		modRateRanges.add("free", lfoRandPerlinFreqRange);
		modRateRanges.add("sync", getTempoSyncRange(tsValues));
		const auto tsStrings = getTempoSyncStrings(maxSyncTime);
		auto rateStr = getRateStr(apvts, ID::LFOSync0, modRateRanges("free"), tsStrings);

		// mod's parameters 0:

		parameters.push_back(createParameter(ID::EnvFolGain0, 0.f, dbStr, 0.f, 24.f));
		parameters.push_back(createParameter(ID::EnvFolAtk0, 1.f, msStr, envFolAtkRlsRange));
		parameters.push_back(createParameter(ID::EnvFolRls0, 1.f, msStr, envFolAtkRlsRange));
		parameters.push_back(createParameter(ID::EnvFolBias0, .5f, normBiasStr));
		parameters.push_back(createParameter(ID::EnvFolWidth0, 0.f, percentStr));

		parameters.push_back(createPBool(ID::LFOSync0, false, syncStr));
		parameters.push_back(createParameter(ID::LFORate0, .1f, rateStr));
		parameters.push_back(createParameter(ID::LFOWidth0, 0.f, percentStrM1));
		parameters.push_back(createParameter(ID::LFOWaveTable0, 0.f, wtStr, 0.f, 3.f));
		parameters.push_back(createPBool(ID::LFOPolarity0, false, polarityStr));
		parameters.push_back(createParameter(ID::LFOPhase0, 0.f, phaseStr, -1.f, 1.f));

		rateStr = getRateStr(apvts, ID::RandSync0, modRateRanges("free"), tsStrings);

		parameters.push_back(createPBool(ID::RandSync0, false, syncStr));
		parameters.push_back(createParameter(ID::RandRate0, .5f, rateStr));
		parameters.push_back(createParameter(ID::RandBias0, .5f, normBiasStr));
		parameters.push_back(createParameter(ID::RandWidth0, 0.f, percentStr));
		parameters.push_back(createParameter(ID::RandSmooth0, 1.f, percentStr));

		parameters.push_back(createParameter(ID::PerlinRate0, .5f, rateStr));
		parameters.push_back(createParameter(ID::PerlinOctaves0, 1.f, octStr, 1.f, PerlinMaxOctaves, 1.f));
		parameters.push_back(createParameter(ID::PerlinWidth0, 0.f, percentStr));

		// mod's parameters 1:

		parameters.push_back(createParameter(ID::EnvFolGain1, 0.f, dbStr, 0.f, 24.f));
		parameters.push_back(createParameter(ID::EnvFolAtk1, 1.f, msStr, envFolAtkRlsRange));
		parameters.push_back(createParameter(ID::EnvFolRls1, 1.f, msStr, envFolAtkRlsRange));
		parameters.push_back(createParameter(ID::EnvFolBias1, .5f, normBiasStr));
		parameters.push_back(createParameter(ID::EnvFolWidth1, 0.f, percentStr));

		rateStr = getRateStr(apvts, ID::LFOSync1, modRateRanges("free"), tsStrings);

		parameters.push_back(createPBool(ID::LFOSync1, false, syncStr));
		parameters.push_back(createParameter(ID::LFORate1, .1f, rateStr));
		parameters.push_back(createParameter(ID::LFOWidth1, 0.f, percentStrM1));
		parameters.push_back(createParameter(ID::LFOWaveTable1, 0.f, wtStr, 0.f, 3.f));
		parameters.push_back(createPBool(ID::LFOPolarity1, false, polarityStr));
		parameters.push_back(createParameter(ID::LFOPhase1, 0.f, phaseStr, -1.f, 1.f));

		rateStr = getRateStr(apvts, ID::RandSync1, modRateRanges("free"), tsStrings);

		parameters.push_back(createPBool(ID::RandSync1, false, syncStr));
		parameters.push_back(createParameter(ID::RandRate1, .5f, rateStr));
		parameters.push_back(createParameter(ID::RandBias1, .5f, normBiasStr));
		parameters.push_back(createParameter(ID::RandWidth1, 0.f, percentStr));
		parameters.push_back(createParameter(ID::RandSmooth1, 1.f, percentStr));

		parameters.push_back(createParameter(ID::PerlinRate1, .5f, rateStr));
		parameters.push_back(createParameter(ID::PerlinOctaves1, 1.f, octStr, 1.f, PerlinMaxOctaves, 1.f));
		parameters.push_back(createParameter(ID::PerlinWidth1, 0.f, percentStr));

		// non modulators' parameters:
		parameters.push_back(createParameter(ID::Depth, 1.f, percentStr));
		parameters.push_back(createParameter(ID::ModulatorsMix, .5f, modsMixStr));
		parameters.push_back(createParameter(ID::DryWetMix, 1.f, percentStr));
		parameters.push_back(createParameter(ID::Voices, 1.f, voicesStr, 1.f, 8.f, 1.f));
		parameters.push_back(createPBool(ID::StereoConfig, true, lrMsStr));

		return { parameters.begin(), parameters.end() };
	}
};

/*

seperate free from temposync rate again
	no one's ever going to automate that crap anyway

depth parameter string on 9% draws 9.% but should draw 9%

*/
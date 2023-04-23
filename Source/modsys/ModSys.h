#pragma once
#include <JuceHeader.h>

namespace modSys6
{
	inline float nextLowestPowTwoX(float x) noexcept
	{
		return std::pow(2.f, std::floor(std::log2(x)));
	}

	inline bool stringNegates(const juce::String& t)
	{
		return t == "off"
			|| t == "false"
			|| t == "no"
			|| t == "0"
			|| t == "disabled"
			|| t == "none"
			|| t == "null"
			|| t == "nil"
			|| t == "nada"
			|| t == "nix"
			|| t == "nichts"
			|| t == "niente"
			|| t == "nope"
			|| t == "nay"
			|| t == "nein"
			|| t == "njet"
			|| t == "nicht"
			|| t == "nichts";
	}

	static constexpr float tau = 6.28318530718f;
	static constexpr float pi = 3.14159265359f;
	static constexpr float piHalf = 1.57079632679f;
	static constexpr float piQuart = .785398163397f;
	static constexpr float piInv = 1.f / pi;

	inline juce::String toID(const juce::String& name)
	{
		return name.toLowerCase().removeCharacters(" ");
	}

	enum class PID
	{
		MSMacro0, MSMacro1, MSMacro2, MSMacro3,
		
		Perlin0RateHz, Perlin0RateBeats, Perlin0Octaves, Perlin0Width, Perlin0RateType, Perlin0Phase, Perlin0Shape, Perlin0RandType,
		AudioRate0Oct, AudioRate0Semi, AudioRate0Fine, AudioRate0Width, AudioRate0RetuneSpeed, AudioRate0Atk, AudioRate0Dcy, AudioRate0Sus, AudioRate0Rls,
		Dropout0Decay, Dropout0Spin, Dropout0Chance, Dropout0Smooth, Dropout0Width,
		EnvFol0Attack, EnvFol0Release, EnvFol0Gain, EnvFol0Width,
		Macro0,
		Pitchbend0Smooth,
		LFO0FreeSync, LFO0RateFree, LFO0RateSync, LFO0Waveform, LFO0Phase, LFO0Width,

		Perlin1RateHz, Perlin1RateBeats, Perlin1Octaves, Perlin1Width, Perlin1RateType, Perlin1Phase, Perlin1Shape, Perlin1RandType,
		AudioRate1Oct, AudioRate1Semi, AudioRate1Fine, AudioRate1Width, AudioRate1RetuneSpeed, AudioRate1Atk, AudioRate1Dcy, AudioRate1Sus, AudioRate1Rls,
		Dropout1Decay, Dropout1Spin, Dropout1Chance, Dropout1Smooth, Dropout1Width,
		EnvFol1Attack, EnvFol1Release, EnvFol1Gain, EnvFol1Width,
		Macro1,
		Pitchbend1Smooth,
		LFO1FreeSync, LFO1RateFree, LFO1RateSync, LFO1Waveform, LFO1Phase, LFO1Width,

		Depth, ModsMix, DryWetMix, WetGain, StereoConfig, Feedback, Damp, HQ, Lookahead,

		NumParams
	};
	
	static constexpr int NumMSParams = static_cast<int>(PID::MSMacro3) + 1;
	static constexpr int NumParamsPerMod = static_cast<int>(PID::Perlin1RateHz) - NumMSParams;
	static constexpr int NumParams = static_cast<int>(PID::NumParams);
	
	inline juce::String toString(PID pID)
	{
		switch (pID)
		{
		case PID::MSMacro0: return "MS Macro 0";
		case PID::MSMacro1: return "MS Macro 1";
		case PID::MSMacro2: return "MS Macro 2";
		case PID::MSMacro3: return "MS Macro 3";

		case PID::Perlin0RateHz: return "Perlin 0 Rate Hz";
		case PID::Perlin0RateBeats: return "Perlin 0 Rate Beats";
		case PID::Perlin0Octaves: return "Perlin 0 Octaves";
		case PID::Perlin0Width: return "Perlin 0 Width";
		case PID::Perlin0RateType: return "Perlin 0 Rate Type";
		case PID::Perlin0Phase: return "Perlin 0 Phase";
		case PID::Perlin0Shape: return "Perlin 0 Shape";
		case PID::Perlin0RandType: return "Perlin 0 Rand Type";
		case PID::AudioRate0Oct: return "AudioRate 0 Oct";
		case PID::AudioRate0Semi: return "AudioRate 0 Semi";
		case PID::AudioRate0Fine: return "AudioRate 0 Fine";
		case PID::AudioRate0Width: return "AudioRate 0 Width";
		case PID::AudioRate0RetuneSpeed: return "AudioRate 0 RetuneSpeed";
		case PID::AudioRate0Atk: return "AudioRate 0 Attack";
		case PID::AudioRate0Dcy: return "AudioRate 0 Decay";
		case PID::AudioRate0Sus: return "AudioRate 0 Sustain";
		case PID::AudioRate0Rls: return "AudioRate 0 Release";
		case PID::Dropout0Decay: return "Dropout 0 Decay";
		case PID::Dropout0Spin: return "Dropout 0 Spin";
		case PID::Dropout0Chance: return "Dropout 0 Chance";
		case PID::Dropout0Smooth: return "Dropout 0 Smooth";
		case PID::Dropout0Width: return "Dropout 0 Width";
		case PID::EnvFol0Attack: return "EnvFol 0 Attack";
		case PID::EnvFol0Release: return "EnvFol 0 Release";
		case PID::EnvFol0Gain: return "EnvFol 0 Gain";
		case PID::EnvFol0Width: return "EnvFol 0 Width";
		case PID::Macro0: return "Macro 0";
		case PID::Pitchbend0Smooth: return "Pitchbend 0 Smooth";
		case PID::LFO0FreeSync: return "LFO 0 FreeSync";
		case PID::LFO0RateFree: return "LFO 0 Rate Free";
		case PID::LFO0RateSync: return "LFO 0 Rate Sync";
		case PID::LFO0Waveform: return "LFO 0 Waveform";
		case PID::LFO0Phase: return "LFO 0 Phase";
		case PID::LFO0Width: return "LFO 0 Width";

		case PID::Perlin1RateHz: return "Perlin 1 Rate Hz";
		case PID::Perlin1RateBeats: return "Perlin 1 Rate Beats";
		case PID::Perlin1Octaves: return "Perlin 1 Octaves";
		case PID::Perlin1Width: return "Perlin 1 Width";
		case PID::Perlin1RateType: return "Perlin 1 Rate Type";
		case PID::Perlin1Phase: return "Perlin 1 Phase";
		case PID::Perlin1Shape: return "Perlin 1 Shape";
		case PID::Perlin1RandType: return "Perlin 1 Rand Type";
		case PID::AudioRate1Oct: return "AudioRate 1 Oct";
		case PID::AudioRate1Semi: return "AudioRate 1 Semi";
		case PID::AudioRate1Fine: return "AudioRate 1 Fine";
		case PID::AudioRate1Width: return "AudioRate 1 Width";
		case PID::AudioRate1RetuneSpeed: return "AudioRate 1 RetuneSpeed";
		case PID::AudioRate1Atk: return "AudioRate 1 Attack";
		case PID::AudioRate1Dcy: return "AudioRate 1 Decay";
		case PID::AudioRate1Sus: return "AudioRate 1 Sustain";
		case PID::AudioRate1Rls: return "AudioRate 1 Release";
		case PID::Dropout1Decay: return "Dropout 1 Decay";
		case PID::Dropout1Spin: return "Dropout 1 Spin";
		case PID::Dropout1Chance: return "Dropout 1 Chance";
		case PID::Dropout1Smooth: return "Dropout 1 Smooth";
		case PID::Dropout1Width: return "Dropout 1 Width";
		case PID::EnvFol1Attack: return "EnvFol 1 Attack";
		case PID::EnvFol1Release: return "EnvFol 1 Release";
		case PID::EnvFol1Gain: return "EnvFol 1 Gain";
		case PID::EnvFol1Width: return "EnvFol 1 Width";
		case PID::Macro1: return "Macro 1";
		case PID::Pitchbend1Smooth: return "Pitchbend 1 Smooth";
		case PID::LFO1FreeSync: return "LFO 1 FreeSync";
		case PID::LFO1RateFree: return "LFO 1 Rate Free";
		case PID::LFO1RateSync: return "LFO 1 Rate Sync";
		case PID::LFO1Waveform: return "LFO 1 Waveform";
		case PID::LFO1Phase: return "LFO 1 Phase";
		case PID::LFO1Width: return "LFO 1 Width";

		case PID::Depth: return "Depth";
		case PID::ModsMix: return "Mods Mix";
		case PID::DryWetMix: return "DryWet Mix";
		case PID::WetGain: return "Gain Wet";
		case PID::StereoConfig: return "Stereo Config";
		case PID::Feedback: return "Feedback";
		case PID::Damp: return "Damp";
		case PID::HQ: return "HQ";
		case PID::Lookahead: return "Lookahead";

		default: return "";
		}
	}
	
	inline int withOffset(PID p, int o) noexcept
	{
		return static_cast<int>(p) + o;
	}

	enum class Unit { Percent, Hz, Beats, Degree, Octaves, Semi, Fine, Ms, Decibel, Power, PerlinShape, PerlinRandType, NumUnits };
	
	inline juce::String toString(Unit pID)
	{
		switch (pID)
		{
		case Unit::Percent: return "%";
		case Unit::Hz: return "hz";
		case Unit::Beats: return "x";
		case Unit::Degree: return juce::CharPointer_UTF8("\xc2\xb0");
		case Unit::Octaves: return "oct";
		case Unit::Semi: return "semi";
		case Unit::Fine: return "fine";
		case Unit::Ms: return "ms";
		case Unit::Decibel: return "db";
		default: return "";
		}
	}

	enum class ModType { None, Macro, Perlin, LFO, NumTypes };
	
	static constexpr int NumModTypes = static_cast<int>(ModType::NumTypes);
	
	inline juce::String toString(ModType t)
	{
		switch (t)
		{
		case ModType::Macro: return "Macro";
		case ModType::Perlin: return "Perlin";
		case ModType::LFO: return "LFO";
		default: return "";
		}
	}
	
	struct ModTypeContext
	{
		ModTypeContext(ModType t = ModType::None, int _idx = -1) :
			type(t),
			idx(_idx)
		{}

		ModType type; int idx;

		bool valid() const noexcept { return type != ModType::None; }
		
		bool operator==(ModTypeContext o) const noexcept { return type == o.type && idx == o.idx; }
		
		bool operator!=(ModTypeContext o) const noexcept { return !this->operator==(o); }
		
	};
	
	inline juce::String toString(ModTypeContext mtc)
	{
		return toString(mtc.type) + juce::String(mtc.idx);
	}

	using ValToStrFunc = std::function<juce::String(float)>;
	using StrToValFunc = std::function<float(const juce::String&)>;

	enum class BeatType { Whole, Triplet, Dotted, NumTypes };
	
	inline juce::String toString(BeatType t)
	{
		switch (t)
		{
		case BeatType::Whole: return "";
		case BeatType::Triplet: return "t";
		case BeatType::Dotted: return ".";
		default: return "";
		}
	}
	
	inline juce::String toString(float whole, BeatType t)
	{
		if(whole >= 1.f)
			return "1/" + juce::String(whole) + toString(t);
		else
			return juce::String(1.f / whole) + "/1" + toString(t);
	}

	enum class WaveForm { Sine, Triangle, Saw, Square, Noise, NumWaveForms };
	
	inline juce::String toString(WaveForm wf)
	{
		switch (wf)
		{
		case WaveForm::Sine: return "Sine";
		case WaveForm::Triangle: return "Triangle";
		case WaveForm::Saw: return "Saw";
		case WaveForm::Square: return "Square";
		case WaveForm::Noise: return "Noise";
		default: return "";
		}
	}

	struct StateIDs
	{
		juce::Identifier state{ "state" };
		juce::Identifier params{ "params" };
		juce::Identifier param{ "param" };
		juce::Identifier name{ "name" };
		juce::Identifier value{ "value" };
		juce::Identifier locked{ "locked" };

		juce::Identifier connex{ "connex" };
		juce::Identifier connec{ "connec" };
		juce::Identifier mod{ "mod" };
	};

	namespace beats
	{
		struct Beat
		{
			Beat(float v, const juce::String& s) :
				val(1.f / v),
				str(s)
			{}
			const float val;
			const juce::String str;
		};
		
		inline void create(std::vector<Beat>& beats, const float min, const float max)
		{
			auto c = 1.f / min;
			beats.push_back({ c, toString(c, BeatType::Whole) });
			while (c > 1.f / max) {
				c *= .5f;
				beats.push_back({ c * 7.f / 4.f, toString(c, BeatType::Dotted) });
				beats.push_back({ c * 5.f / 3.f, toString(c, BeatType::Triplet) });
				beats.push_back({ c, toString(c, BeatType::Whole) });
			}
		}
	}

	using BeatsData = std::vector<modSys6::beats::Beat>;

	namespace makeRange
	{
		inline juce::NormalisableRange<float> bias(float start, float end, float bias) noexcept
		{
			if (bias > 0.f)
				return
			{
					start, end,
					[b = 1.f - bias, range = end - start](float min, float, float normalized)
					{
						return min + range * std::pow(normalized, b);
					},
					[b = 1.f / (1.f - bias), rangeInv = 1.f / (end - start)](float min, float, float denormalized)
					{
						return std::pow((denormalized - min) * rangeInv, b);
					},
					nullptr
			};
			else if (bias < 0.f)
				return
			{
					start, end,
					[b = 1.f / (bias + 1.f), range = end - start](float min, float, float normalized)
					{
						return min + range * std::pow(normalized, b);
					},
					[b = bias + 1, rangeInv = 1.f / (end - start)](float min, float, float denormalized)
					{
						return std::pow((denormalized - min) * rangeInv, b);
					},
					nullptr
			};
			else return { start, end };
		}

		inline juce::NormalisableRange<float> biasXL(float start, float end, float bias) noexcept
		{
			// https://www.desmos.com/calculator/ps8q8gftcr
			const auto a = bias * .5f + .5f;
			const auto a2 = 2.f * a;
			const auto aM = 1.f - a;

			const auto r = end - start;
			const auto aR = r * a;
			if (bias != 0.f)
				return
			{
					start, end,
					[a2, aM, aR](float min, float, float x)
					{
						const auto denom = aM - x + a2 * x;
						if (denom == 0.f)
							return min;
						return min + aR * x / denom;
					},
					[a2, aM, aR](float min, float, float x)
					{
						const auto denom = a2 * min + aR - a2 * x - min + x;
						if (denom == 0.f)
							return 0.f;
						auto val = aM * (x - min) / denom;
						return val > 1.f ? 1.f : val;
					},
					[](float min, float max, float x)
					{
						return x < min ? min : x > max ? max : x;
					}
			};
			else return { start, end };
		}

		inline juce::NormalisableRange<float> withCentre(float start, float end, float centre) noexcept
		{
			const auto r = end - start;
			const auto v = (centre - start) / r;

			return biasXL(start, end, 2.f * v - 1.f);
		}

		inline juce::NormalisableRange<float> quad(float min, float max, int numSteps) noexcept
		{
			return
			{
				min, max,
				[numSteps, range = max - min](float start, float, float x)
				{
					for (auto i = 0; i < numSteps; ++i)
						x *= x;
					return start + x * range;
				},
				[numSteps, rangeInv = 1.f / (max - min)](float start, float, float x)
				{
					x = (x - start) * rangeInv;
					for (auto i = 0; i < numSteps; ++i)
						x = std::sqrt(x);
					return x;
				},
				[](float start, float end, float x)
				{
					return juce::jlimit(start, end, x);
				}
			};
		}

		inline juce::NormalisableRange<float> stepped(float start, float end, float steps = 1.f) noexcept
		{
			return { start, end, steps };
		}

		inline juce::NormalisableRange<float> temposync(int numSteps)
		{
			return stepped(0.f, static_cast<float>(numSteps));
		}

		inline juce::NormalisableRange<float> beats(float minDenominator, float maxDenominator, bool withZero)
		{
			std::vector<float> table;

			const auto minV = std::log2(minDenominator);
			const auto maxV = std::log2(maxDenominator);

			const auto numWholeBeatsF = static_cast<float>(minV - maxV);
			const auto numWholeBeatsInv = 1.f / numWholeBeatsF;

			const auto numWholeBeats = static_cast<int>(numWholeBeatsF);
			const auto numValues = numWholeBeats * 3 + 1 + (withZero ? 1 : 0);
			table.reserve(numValues);
			if (withZero)
				table.emplace_back(0.f);

			for (auto i = 0; i < numWholeBeats; ++i)
			{
				const auto iF = static_cast<float>(i);
				const auto x = iF * numWholeBeatsInv;

				const auto curV = minV - x * numWholeBeatsF;
				const auto baseVal = std::pow(2.f, curV);

				const auto valWhole = 1.f / baseVal;
				const auto valTriplet = valWhole * 1.666666666667f;
				const auto valDotted = valWhole * 1.75f;

				table.emplace_back(valWhole);
				table.emplace_back(valTriplet);
				table.emplace_back(valDotted);
			}
			table.emplace_back(1.f / maxDenominator);

			static constexpr float Eps = 1.f - std::numeric_limits<float>::epsilon();
			static constexpr float EpsInv = 1.f / Eps;

			const auto numValuesF = static_cast<float>(numValues);
			const auto numValuesInv = 1.f / numValuesF;
			const auto numValsX = numValuesInv * EpsInv;
			const auto normValsY = numValuesF * Eps;

			juce::NormalisableRange<float> range
			{
				table.front(), table.back(),
				[table, normValsY](float, float, float normalized)
				{
					const auto valueIdx = normalized * normValsY;
					return table[static_cast<int>(valueIdx)];
				},
				[table, numValsX](float, float, float denormalized)
				{
					for (auto i = 0; i < table.size(); ++i)
						if (denormalized <= table[i])
							return static_cast<float>(i) * numValsX;
					return 0.f;
				},
				[table, numValsX](float start, float end, float denormalized)
				{
					auto closest = table.front();
					for (auto i = 0; i < table.size(); ++i)
					{
						const auto diff = std::abs(table[i] - denormalized);
						if (diff < std::abs(closest - denormalized))
							closest = table[i];
					}
					return juce::jlimit(start, end, closest);
				}
			};

			return range;
		}

		inline juce::NormalisableRange<float> toggle() noexcept
		{
			return stepped(0.f, 1.f);
		}

		inline juce::NormalisableRange<float> lin(float start, float end) noexcept
		{
			const auto range = end - start;

			return
			{
				start,
				end,
				[range](float min, float, float normalized)
				{
					return min + normalized * range;
				},
				[inv = 1.f / range](float min, float, float denormalized)
				{
					return (denormalized - min) * inv;
				},
				[](float min, float max, float x)
				{
					return juce::jlimit(min, max, x);
				}
			};
		}
	}

	struct Param :
		public juce::AudioProcessorParameter
	{
		Param(const PID pID, const juce::NormalisableRange<float>& _range, const float _valDenormDefault,
			const ValToStrFunc& _valToStr, const StrToValFunc& _strToVal,
			const Unit _unit = Unit::NumUnits, const ModTypeContext _attachedMod = ModTypeContext(ModType::None, -1)) :

			juce::AudioProcessorParameter(),
			id(pID),
			range(_range),
			attachedMod(_attachedMod),
			valDenormDefault(_valDenormDefault),
			valNorm(range.convertTo0to1(_valDenormDefault)),
			valToStr(_valToStr),
			strToVal(_strToVal),
			unit(_unit),

			valNormMod(0.f), valNormSum(0.f)
		{
		}
		
		Param(const int pID, const juce::NormalisableRange<float>& _range, const float _valDenormDefault,
			const ValToStrFunc& _valToStr, const StrToValFunc& _strToVal,
			const Unit _unit, const ModTypeContext _attachedMod = ModTypeContext(ModType::None, -1)) :

			juce::AudioProcessorParameter(),
			id(static_cast<PID>(pID)),
			range(_range),
			attachedMod(_attachedMod),
			valDenormDefault(_valDenormDefault),
			valNorm(range.convertTo0to1(_valDenormDefault)),
			valToStr(_valToStr),
			strToVal(_strToVal),
			unit(_unit),

			valNormMod(0.f), valNormSum(0.f)
		{
		}

		//called by host, normalized, thread-safe
		float getValue() const override { return valNorm.load(); }
		float denormalized() const noexcept { return range.convertFrom0to1(valNorm.load()); }

		// called by host, normalized, avoid locks, not used by editor
		// use setValueNotifyingHost() from the editor
		void setValue(float normalized) override
		{
			valNorm.store(normalized);
		}
		void setValueWithGesture(float norm)
		{
			beginChangeGesture();
			setValueNotifyingHost(norm);
			endChangeGesture();
		}
		void beginGesture() { beginChangeGesture(); }
		void endGesture() { endChangeGesture(); }

		float getDefaultValue() const override { return range.convertTo0to1(valDenormDefault); }

		juce::String getName(int) const override { return toString(id); }

		// units of param (hz, % etc.)
		juce::String getLabel() const override { return toString(unit); }

		// string of norm val
		juce::String getText(float norm, int) const override
		{
			return valToStr(range.snapToLegalValue(range.convertFrom0to1(norm)));
		}

		// string to norm val
		float getValueForText(const juce::String& text) const override
		{
			const auto val = juce::jlimit(range.start, range.end, strToVal(text));
			return range.convertTo0to1(val);
		}
		// string to denorm val
		float getValForTextDenorm(const juce::String& text) const { return strToVal(text); }

		void processBlockInit() noexcept
		{
			valNormMod = valNorm.load();
		}
		void processBlockModulate(float m) noexcept
		{
			valNormMod += m;
		}
		void processBlockFinish()
		{
			valNormSum = juce::jlimit(0.f, 1.f, valNormMod);
		}

		float getValueSum() const noexcept { return valNormSum; }
		float getValSumDenorm() const noexcept
		{
			return range.snapToLegalValue(range.convertFrom0to1(valNormSum));
		}

		juce::String _toString()
		{
			auto v = getValue();
			return getName(10) + ": " + juce::String(v) + "; " + getText(v, 10) + "; attached to " + toString(attachedMod);
		}
		void dbg()
		{
			DBG(_toString());
		}

		const PID id;
		const juce::NormalisableRange<float> range;
		const ModTypeContext attachedMod;
	protected:
		const float valDenormDefault;
		std::atomic<float> valNorm;
		ValToStrFunc valToStr;
		StrToValFunc strToVal;
		Unit unit;

		float valNormMod, valNormSum;
	};

	struct Params
	{
		int getDigitFromString(const juce::String& txt) noexcept
		{
			for (auto t = 0; t < txt.length(); ++t)
			{
				const auto chr = txt[t];
				if (!(chr < '0' || chr > '9'))
					return static_cast<int>(chr - '0');
			}
			return -1;
		}

		Params(juce::AudioProcessor& audioProcessor, BeatsData& beatsData) :
			params()
		{
			beats::create(beatsData, 1.f / 64.f, 8.f);

			const auto strToValDivision = [](const juce::String& txt, const float altVal)
			{
				if (txt.contains(":") || txt.contains("/"))
				{
					for (auto i = 0; i < txt.length(); ++i)
					{
						if (txt[i] == ':' || txt[i] == '/')
						{
							const auto a = txt.substring(0, i).getFloatValue();
							const auto b = txt.substring(i + 1).getFloatValue();
							if(b != 0.f)
								return a / b;
						}
					}
				}
				return altVal;
			};

			const ValToStrFunc valToStrPercent = [](float v) { return juce::String(std::floor(v * 100.f)) + " " + toString(Unit::Percent); };
			const ValToStrFunc valToStrHz = [](float v)
			{
				if (v >= 10000.f)
					return juce::String(v * .001).substring(0, 4) + " khz";
				else if (v >= 1000.f)
					return juce::String(v * .001).substring(0, 3) + " khz";
				else if (v >= 1.f)
					return juce::String(v).substring(0, 5) + " hz";
				else
				{
					v *= 1000.f;

					if (v >= 100.f)
						return juce::String(v).substring(0, 3) + " mhz";
					else if (v >= 10.f)
						return juce::String(v).substring(0, 2) + " mhz";
					else
						return juce::String(v).substring(0, 1) + " mhz";
				}
			};
			const ValToStrFunc valToStrBeats = [&bts = beatsData](float v) { return bts[static_cast<int>(v)].str; };
			const ValToStrFunc valToStrPhase = [](float v) { return juce::String(std::floor(v * 180.f)) + " " + toString(Unit::Degree); };
			const ValToStrFunc valToStrPhase360 = [](float v) { return juce::String(std::floor(v * 360.f)) + " " + toString(Unit::Degree); };
			const ValToStrFunc valToStrOct = [](float v) { return juce::String(std::round(v)) + " " + toString(Unit::Octaves); };
			const ValToStrFunc valToStrOct2 = [](float v) { return juce::String(std::floor(v / 12.f)) + " " + toString(Unit::Octaves); };
			const ValToStrFunc valToStrSemi = [](float v) { return juce::String(std::floor(v)) + " " + toString(Unit::Semi); };
			const ValToStrFunc valToStrFine = [](float v) { return juce::String(std::floor(v * 100.f)) + " " + toString(Unit::Fine); };
			const ValToStrFunc valToStrRatio = [](float v)
			{
				const auto y = static_cast<int>(std::floor(v * 100.f));
				return juce::String(100 - y) + " : " + juce::String(y);
			};
			const ValToStrFunc valToStrLRMS = [](float v) { return v > .5f ? juce::String("m/s") : juce::String("l/r"); };
			const ValToStrFunc valToStrFreeSync = [](float v) { return v > .5f ? juce::String("sync") : juce::String("free"); };
			const ValToStrFunc valToStrPolarity = [](float v) { return v > .5f ? juce::String("on") : juce::String("off"); };
			const ValToStrFunc valToStrMs = [](float v) { return juce::String(std::floor(v * 10.f) * .1f) + " " + toString(Unit::Ms); };
			const ValToStrFunc valToStrDb = [](float v) { return juce::String(std::floor(v * 100.f) * .01f) + " " + toString(Unit::Decibel); };
			const ValToStrFunc valToStrEmpty = [](float) { return juce::String(""); };
			const ValToStrFunc valToStrSeed = [](float v)
			{
				if (v == 0.f)
					return juce::String("off");

				juce::String str("abcde");
				juce::Random rnd;
				for(auto i = 0; i < str.length(); ++i)
					str = str.replaceCharacter('a' + i, static_cast<juce::juce_wchar>(rnd.nextInt(26) + 'a'));
				return str;
			};
			const ValToStrFunc valToStrBeats2 = [](float v)
			{
				enum Mode { Whole, Triplet, Dotted, NumModes };
					
				if (v == 0.f)
					return juce::String("0");

				const auto denormFloor = nextLowestPowTwoX(v);
				const auto denormFrac = v - denormFloor;
				const auto modeVal = denormFrac / denormFloor;
				const auto mode = modeVal < .66f ? Mode::Whole :
					modeVal < .75f ? Mode::Triplet :
					Mode::Dotted;
				const auto modeStr = mode == Mode::Whole ? juce::String("") :
					mode == Mode::Triplet ? juce::String("t") :
					juce::String(".");

				auto denominator = 1.f / denormFloor;
				auto numerator = 1.f;
				if (denominator < 1.f)
				{
					numerator = denormFloor;
					denominator = 1.f;
				}

				return juce::String(numerator) + " / " + juce::String(denominator) + modeStr;
			};
			const ValToStrFunc valToStrPower = [](float v)
			{
				return juce::String((v > .5f ? "Enabled" : "Disabled"));
			};

			const StrToValFunc strToValPercent = [strToValDivision](const juce::String& txt)
			{
				const auto val = strToValDivision(txt, 0.f);
				if (val != 0.f)
					return val;
				return txt.trimCharactersAtEnd(toString(Unit::Percent)).getFloatValue() * .01f;
			};
			const StrToValFunc strToValHz = [](const juce::String& txt)
			{
				auto text = txt.trimCharactersAtEnd(toString(Unit::Hz));
				auto multiplier = 1.f;
				if (text.getLastCharacter() == 'k')
				{
					multiplier = 1000.f;
					text = text.dropLastCharacters(1);
				}
				else if (text.getLastCharacter() == 'm')
				{
					multiplier = .001f;
					text = text.dropLastCharacters(1);
				}
				const auto val = text.getFloatValue();
				const auto val2 = val * multiplier;

				return val2;
			};
			const StrToValFunc strToValBeats = [&bts = beatsData](const juce::String& txt)
			{
				for (auto b = 0; b < bts.size(); ++b)
					if (bts[b].str == txt)
						return static_cast<float>(b);
				return 0.f;
			};
			const StrToValFunc strToValPhase = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Degree)).getFloatValue() / 180.f; };
			const StrToValFunc strToValPhase360 = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Degree)).getFloatValue() / 360.f; };
			const StrToValFunc strToValOct = [](const juce::String& txt) { return std::round(txt.trimCharactersAtEnd(toString(Unit::Octaves)).getFloatValue()); };
			const StrToValFunc strToValOct2 = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Octaves)).getFloatValue() / 12.f; };
			const StrToValFunc strToValSemi = [](const juce::String& txt) { return std::round(txt.trimCharactersAtEnd(toString(Unit::Semi)).getFloatValue()); };
			const StrToValFunc strToValFine = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Fine)).getFloatValue() * .01f; };
			const StrToValFunc strToValRatio = [strToValDivision](const juce::String& txt)
			{
				const auto val = strToValDivision(txt, -1.f);
				if (val != -1.f)
					return val;
				return juce::jlimit(0.f, 1.f, txt.getFloatValue() * .01f);
			};
			const StrToValFunc strToValLRMS = [](const juce::String& txt) { return txt[0] == 'l' ? 0.f : 1.f; };
			const StrToValFunc strToValFreeSync = [](const juce::String& txt) { return txt[0] == 'f' ? 0.f : 1.f; };
			const StrToValFunc strToValPolarity = [](const juce::String& txt) { return txt[0] == '0' ? 0.f : 1.f; };
			const StrToValFunc strToValMs = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Ms)).getFloatValue(); };
			const StrToValFunc strToValDb = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Decibel)).getFloatValue(); };
			const StrToValFunc strToValSeed = [](const juce::String& str) { return str.getFloatValue(); };
			const StrToValFunc strToValBeats2 = [](const juce::String& txt)
			{
				enum Mode { Beats, Triplet, Dotted, NumModes };
				const auto lastChr = txt[txt.length() - 1];
				const auto mode = lastChr == 't' ? Mode::Triplet : lastChr == '.' ? Mode::Dotted : Mode::Beats;

				const auto text = mode == Mode::Beats ? txt : txt.substring(0, txt.length() - 1);
				auto val = txt.getFloatValue();
				if (mode == Mode::Triplet)
					val *= 1.666666666667f;
				else if (mode == Mode::Dotted)
					val *= 1.75f;
				return val;
			};
			const StrToValFunc strToValPower = [](const juce::String& txt)
			{
				const auto text = txt.trimCharactersAtEnd(toString(Unit::Power));
				if (stringNegates(text))
					return 0.f;
				const auto val = text.getFloatValue();
				return val > .5f ? 1.f : 0.f;
			};

			ValToStrFunc valToStrShape = [](float v)
			{
				return v < .5f ? juce::String("Steppy") :
					v < 1.5f ? juce::String("Lerp") :
					juce::String("Round");
			};
			StrToValFunc strToValShape = [](const juce::String& str)
			{
				const auto text = str.toLowerCase();
				if (text == "steppy" || text == "step")
					return 0.f;
				else if (text == "lerp" || text == "linear")
					return 1.f;
				else if (text == "round" || text == "smooth")
					return 2.f;
				
				return str.getFloatValue();
			};

			ValToStrFunc valToStrRandType = [](float v)
			{
				return v < .5f ? juce::String("Random") :
					juce::String("Procedural");
			};
			StrToValFunc strToValRandType = [](const juce::String& str)
			{
				const auto text = str.toLowerCase();
				if (text == "rand" || text == "random" || text == "randomise" || text == "randomize")
					return 0.f;
				else if (text == "procedural" || text == "proc" || text == "proceduralise" || text == "proceduralize")
					return 1.f;
				
				return str.getFloatValue();
			};

			ValToStrFunc valToStrHQ = [](float v)
			{
				return v < .5f ? juce::String("1x") :
					juce::String("4x");
			};
			StrToValFunc strToValHQ = [](const juce::String& str)
			{
				const auto text = str.toLowerCase();
				if (text == "1x" || text == "1" || text == "low" || text == "lo" || text == "off" || text == "false")
					return 0.f;
				else if (text == "4x" || text == "4" || text == "high" || text == "hi" || text == "420")
					return 1.f;

				return 1.f;
			};
			
			ValToStrFunc valToStrLookahead = [](float v)
			{
				return v < .5f ? juce::String("Off") :
					juce::String("On");
			};
			StrToValFunc strToValLookahead = [](const juce::String& str)
			{
				const auto text = str.toLowerCase();
				if (text == "off" || text == "false" || text == "0" || text == "Nopezies")
					return 0.f;
				else if (text == "on" || text == "true" || text == "1" || text == "Fuck Yeah")
					return 1.f;

				return 1.f;
			};

			for (auto p = 0; p < NumMSParams; ++p)
			{
				const PID pID = static_cast<PID>(p);
				const auto name = toString(pID);

				juce::NormalisableRange<float> range(0.f, 1.f);
				float valDenormDefault = 0.f;
				ValToStrFunc valToStr = [](float v)               { return juce::String(v); };
				StrToValFunc strToVal = [](const juce::String& t) { return t.getFloatValue(); };
				Unit unit = Unit::NumUnits;
				ModTypeContext attachedMod = { ModType::None, getDigitFromString(name) };

				if (name.substring(0, 2) == "MS")
				{
					if (name.contains("Macro"))
					{
						range = juce::NormalisableRange<float>(0.f, 1.f);
						valDenormDefault = 0.f;
						valToStr = valToStrPercent;
						strToVal = strToValPercent;
						unit = Unit::Percent;
						attachedMod.type = ModType::Macro;
					}
					else if (name.contains("LFO"))
					{
						if (name.contains("Freq Hz"))
						{
							range = makeRange::biasXL(.1f, 40.f, -.7f);
							valDenormDefault = 1.f;
							valToStr = valToStrHz;
							strToVal = strToValHz;
							unit = Unit::Hz;
							attachedMod.type = ModType::LFO;
						}
						else if (name.contains("Freq Sync"))
						{
							range = juce::NormalisableRange<float>(0.f, static_cast<float>(beatsData.size() - 1));
							valDenormDefault = 12.f;
							valToStr = valToStrBeats;
							strToVal = strToValBeats;
							unit = Unit::Beats;
							attachedMod.type = ModType::LFO;
						}
						else if (name.contains("Phase"))
						{
							range = juce::NormalisableRange<float>(-1.f, 1.f);
							valDenormDefault = 0.f;
							valToStr = valToStrPhase;
							strToVal = strToValPhase;
							unit = Unit::Degree;
							attachedMod.type = ModType::LFO;
						}
					}
					else if (name.contains("Perlin"))
					{
						if (name.contains("Freq Hz"))
						{
							range = makeRange::biasXL(.1f, 40.f, -.7f);
							valDenormDefault = 1.f;
							valToStr = valToStrHz;
							strToVal = strToValHz;
							unit = Unit::Hz;
							attachedMod.type = ModType::Perlin;
						}
						else if (name.contains("Octaves"))
						{
							range = makeRange::biasXL(1, 8, 0);
							valDenormDefault = 4.f;
							valToStr = valToStrOct;
							strToVal = strToValOct;
							unit = Unit::Octaves;
							attachedMod.type = ModType::Perlin;
						}
						else if (name.contains("Width"))
						{
							range = juce::NormalisableRange<float>(0.f, 1.f);
							valDenormDefault = 0.f;
							valToStr = valToStrPercent;
							strToVal = strToValPercent;
							unit = Unit::Percent;
							attachedMod.type = ModType::Perlin;
						}
					}
				}
				params.push_back(new Param(pID, range, valDenormDefault, valToStr, strToVal, unit, attachedMod));
			}

			// ADD NON MODSYS PARAMETERS HERE
			for (auto m = 0; m < 2; ++m)
			{
				const auto offset = m * NumParamsPerMod;
				params.push_back(new Param(withOffset(PID::Perlin0RateHz, offset), makeRange::withCentre(1.f / 1000.f, 40.f, 2.f), .420f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::Perlin0RateBeats, offset), makeRange::beats(32.f, .5f, false), .25f, valToStrBeats2, strToValBeats2, Unit::Beats));
				params.push_back(new Param(withOffset(PID::Perlin0Octaves, offset), makeRange::lin(1.f, 7.f), 3.f, valToStrOct, strToValOct, Unit::Octaves));
				params.push_back(new Param(withOffset(PID::Perlin0Width, offset), makeRange::quad(0.f, 2.f, 1), 0.f, valToStrPercent, strToValPercent, Unit::Percent));
				params.push_back(new Param(withOffset(PID::Perlin0RateType, offset), makeRange::toggle(), 0.f, valToStrPower, strToValPower, Unit::Power));
				params.push_back(new Param(withOffset(PID::Perlin0Phase, offset), makeRange::quad(0.f, 2.f, 1), 0.f, valToStrPhase360, strToValPhase360, Unit::Degree));
				params.push_back(new Param(withOffset(PID::Perlin0Shape, offset), makeRange::stepped(0.f, 2.f), 2.f, valToStrShape, strToValShape, Unit::PerlinShape));
				params.push_back(new Param(withOffset(PID::Perlin0RandType, offset), makeRange::toggle(), 1.f, valToStrRandType, strToValRandType, Unit::PerlinRandType));

				params.push_back(new Param(withOffset(PID::AudioRate0Oct, offset), makeRange::stepped(-3.f * 12.f, 3.f * 12.f, 12.f), 0.f, valToStrOct2, strToValOct2, Unit::Octaves));
				params.push_back(new Param(withOffset(PID::AudioRate0Semi, offset), makeRange::stepped(-12.f, 12.f, 1.f), 0.f, valToStrSemi, strToValSemi, Unit::Semi));
				params.push_back(new Param(withOffset(PID::AudioRate0Fine, offset), makeRange::biasXL(-1.f, 1.f, 0.f), 0.f, valToStrFine, strToValFine, Unit::Fine));
				params.push_back(new Param(withOffset(PID::AudioRate0Width, offset), makeRange::biasXL(-1.f, 1.f, 0.f), 0.f, valToStrPhase, strToValPhase, Unit::Degree));
				params.push_back(new Param(withOffset(PID::AudioRate0RetuneSpeed, offset), makeRange::biasXL(1.f, 2000.f, -.97f), 1.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::AudioRate0Atk, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 20.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::AudioRate0Dcy, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 20.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::AudioRate0Sus, offset), makeRange::biasXL(0.f, 1.f, 0.f), 1.f, valToStrPercent, strToValPercent, Unit::Percent));
				params.push_back(new Param(withOffset(PID::AudioRate0Rls, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 20.f, valToStrMs, strToValMs, Unit::Ms));

				params.push_back(new Param(withOffset(PID::Dropout0Decay, offset), makeRange::quad(10.f, 10000.f, 3), 1000.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::Dropout0Spin, offset), makeRange::biasXL(.1f, 40.f, -.6f), 4.f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::Dropout0Chance, offset), makeRange::quad(10.f, 10000.f, 2), 1000.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::Dropout0Smooth, offset), makeRange::withCentre(.01f, 20.f, 2.f), 4.f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::Dropout0Width, offset), makeRange::biasXL(0.f, 1.f, 0.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));

				params.push_back(new Param(withOffset(PID::EnvFol0Attack, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 80.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::EnvFol0Release, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 250.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::EnvFol0Gain, offset), makeRange::biasXL(-20.f, 80.f, 0.f), 0.f, valToStrDb, strToValDb, Unit::Decibel));
				params.push_back(new Param(withOffset(PID::EnvFol0Width, offset), makeRange::biasXL(0.f, 1.f, 0.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));

				params.push_back(new Param(withOffset(PID::Macro0, offset), makeRange::biasXL(-1.f, 1.f, 0.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));
				params.push_back(new Param(withOffset(PID::Pitchbend0Smooth, offset), makeRange::biasXL(1.f, 1000.f, -.97f), 30.f, valToStrMs, strToValMs, Unit::Ms));

				static constexpr float LFOPhaseStep = 5.f / 360.f;

				params.push_back(new Param(withOffset(PID::LFO0FreeSync, offset), makeRange::toggle(), 1.f, valToStrFreeSync, strToValFreeSync, Unit::NumUnits));
				params.push_back(new Param(withOffset(PID::LFO0RateFree, offset), makeRange::quad(.2f, 40.f, 2), 4.f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::LFO0RateSync, offset), makeRange::beats(32.f, .5f, false), .25f, valToStrBeats2, strToValBeats2, Unit::Beats));
				params.push_back(new Param(withOffset(PID::LFO0Waveform, offset), makeRange::lin(0.f, 1.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));
				params.push_back(new Param(withOffset(PID::LFO0Phase, offset), makeRange::stepped(-.5f, .5f, LFOPhaseStep), 0.f, valToStrPhase360, strToValPhase, Unit::Degree));
				params.push_back(new Param(withOffset(PID::LFO0Width, offset), makeRange::stepped(0.f, .5f, LFOPhaseStep), 0.f, valToStrPhase360, strToValPhase, Unit::Degree));
			}

			params.push_back(new Param(PID::Depth, makeRange::lin(0.f, 1.f), 1.f, valToStrPercent, strToValPercent, Unit::Percent));
			params.push_back(new Param(PID::ModsMix, makeRange::biasXL(0.f, 1.f, 0.f), 0.f, valToStrRatio, strToValRatio));
			params.push_back(new Param(PID::DryWetMix, makeRange::biasXL(0.f, 1.f, 0.f), 1.f, valToStrRatio, strToValRatio));
			params.push_back(new Param(PID::WetGain, makeRange::biasXL(-120.f, 4.5f, .9f), 0.f, valToStrDb, strToValDb));
			params.push_back(new Param(PID::StereoConfig, makeRange::toggle(), 1.f, valToStrLRMS, strToValLRMS));
			params.push_back(new Param(PID::Feedback, makeRange::lin(0.f, 1.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));
			params.push_back(new Param(PID::Damp, makeRange::quad(40.f, 8000.f, 2), 180.f, valToStrHz, strToValHz, Unit::Hz));
			params.push_back(new Param(PID::HQ, makeRange::toggle(), 1.f, valToStrHQ, strToValHQ, Unit::Power));
			params.push_back(new Param(PID::Lookahead, makeRange::toggle(), 1.f, valToStrLookahead, strToValLookahead, Unit::Power));

			for (auto param : params)
				audioProcessor.addParameter(param);
		}
		
		void loadPatch(juce::ValueTree state)
		{
			const StateIDs ids;

			auto childParams = state.getChildWithName(ids.params);
			if (!childParams.isValid())
				return;

			for (auto c = 0; c < childParams.getNumChildren(); ++c)
			{
				const auto childParam = childParams.getChild(c);
				if (childParam.hasType(ids.param))
				{
					const auto pName = childParam.getProperty(ids.name).toString();
					const auto pIdx = getParamIdx(pName);
					if (pIdx != -1)
					{
						const auto pVal = static_cast<float>(childParam.getProperty(ids.value));
						params[pIdx]->setValueNotifyingHost(pVal);
					}
				}
			}
		}
		
		void savePatch(juce::ValueTree state)
		{
			const StateIDs ids;

			auto childParams = state.getChildWithName(ids.params);
			if (!childParams.isValid())
			{
				childParams = juce::ValueTree(ids.params);
				state.appendChild(childParams, nullptr);
			}
			
			for (auto param : params)
			{
				const auto paramID = toID(toString(param->id));
				auto childParam = childParams.getChildWithProperty(ids.name, paramID);
				if (!childParam.isValid())
				{
					childParam = juce::ValueTree(ids.param);
					childParam.setProperty(ids.name, paramID, nullptr);
					childParams.appendChild(childParam, nullptr);
				}
				childParam.setProperty(ids.value, param->getValue(), nullptr);
			}
		}

		void processBlockInit() noexcept
		{
			for (auto p : params)
				p->processBlockInit();
		}
		
		void processBlockFinish()
		{
			for (auto p : params)
				p->processBlockFinish();
		}

		int getParamIdx(const juce::String& name) const noexcept
		{
			for (auto p = 0; p < params.size(); ++p)
			{
				const auto str = toString(params[p]->id);
				if (name == str || name == toID(str))
					return p;
			}	
			return -1;
		}

		size_t numParams() const noexcept { return params.size(); }

		Param* operator[](int i) noexcept { return params[i]; }
		
		const Param* operator[](int i) const noexcept { return params[i]; }
		
	protected:
		std::vector<Param*> params;
	};

	struct Mod
	{
		Mod(ModTypeContext _mtc) :
			mtc(_mtc),
			params(),
			val(0.f)
		{}
		
		bool operator==(ModTypeContext other) const noexcept
		{
			return other.type == mtc.type && other.idx == mtc.idx;
		}
		
		void dbg()
		{
			juce::String txt(toString(mtc) + "\n");
			for (auto p = 0; p < params.size() - 1; ++p)
				txt += "    " + params[p]->_toString() + "\n";
			txt += "    " + params.back()->_toString();
			DBG(txt);
		}

		bool hasParam(Param* param)
		{
			for (auto p = 0; p < params.size(); ++p)
				if (params[p]->id == param->id)
					return true;
			return false;
		}

		void processBlock(int /*numSamples*/) noexcept
		{
			if(mtc.type == ModType::Macro)
				val = params[0]->getValueSum();
			else if(mtc.type == ModType::LFO)
			{
				
			}
		}

		ModTypeContext mtc;
		std::vector<Param*> params;
		float val;
	};

	struct Mods
	{
		Mods(Params& params) :
			mods()
		{
			for (auto p = 0; p < NumParams; ++p)
			{
				auto param = params[p];
				const auto attachedMod = param->attachedMod;
				if (attachedMod.idx == -1)
					break;
				bool foundMod = false;
				for(size_t m = 0; m < mods.size(); ++m)
					if (mods[m] == attachedMod)
					{
						mods[m].params.push_back(param);
						foundMod = true;
						m = mods.size();
					}
				if (!foundMod)
				{
					mods.push_back(attachedMod);
					mods.back().params.push_back(param);
				}
			}
		}
		
		void dbg()
		{
			for (auto& m : mods)
				m.dbg();
		}

		void processBlock(int numSamples) noexcept
		{
			for (auto& m : mods)
				m.processBlock(numSamples);
		}

		const Mod& operator[](int i) const noexcept { return mods[i]; }
		Mod& operator[](int i) noexcept { return mods[i]; }

		size_t numMods() const noexcept { return mods.size(); }
	protected:
		std::vector<Mod> mods;
	};

	struct Connec
	{
		Connec() :
			depth(0.f), enabled(0.f),
			pIdx(0), mIdx(0)
		{
		}
		
		void disable() noexcept { enabled = 0.f; }
		
		void enable(int _mIdx, int _pIdx, float _depth) noexcept
		{
			disable();
			pIdx = _pIdx;
			mIdx = _mIdx;
			depth = _depth;
			enabled = 1.f;
		}
		
		void setDepth(float d) noexcept { depth = d; }
		
		float getDepth() const noexcept { return depth; }
		
		int getPIdx() const noexcept { return pIdx; }
		
		int getMIdx() const noexcept { return mIdx; }
		
		void processBlock(Params& params, const Mods& mods) noexcept
		{
			const auto val = mods[mIdx].val * depth * enabled;
			params[pIdx]->processBlockModulate(val);
		}
		
		bool has(int _mIdx, int _pIdx) const noexcept
		{
			return isEnabled() && pIdx == _pIdx && mIdx == _mIdx;
		}
		
		bool isEnabled() const noexcept { return enabled > 0.f; }
		
		juce::String toString() const
		{
			if (enabled != 1.f) return "disabled";
			return "m: " + juce::String(mIdx) + "; p: " + juce::String(pIdx);
		}

		void savePatch(juce::ValueTree& connexState, const StateIDs& ids)
		{
			juce::ValueTree state(ids.connec);
			state.setProperty(ids.mod, mIdx, nullptr);
			state.setProperty(ids.param, pIdx, nullptr);
			state.setProperty(ids.value, depth, nullptr);
			connexState.appendChild(state, nullptr);
		}
		
	protected:
		float depth, enabled;
		int pIdx, mIdx;
	};
	
	struct Connex
	{
		Connex() :
			connex()
		{}
		
		bool enableConnection(int mIdx, int pIdx, float depth) noexcept
		{
			for (auto c = 0; c < connex.size(); ++c)
				if (!connex[c].isEnabled())
				{
					connex[c].enable(mIdx, pIdx, depth);
					return true;
				}
			return false;
		}
		
		int getConnecIdxWith(int mIdx, int pIdx) const noexcept
		{
			for (auto c = 0; c < connex.size(); ++c)
				if (connex[c].has(mIdx, pIdx))
					return c;
			return -1;
		}
		
		void processBlock(Params& params, const Mods& mods) noexcept
		{
			for (auto& connec : connex)
				connec.processBlock(params, mods);
		}

		Connec& operator[](int c) noexcept { return connex[c]; }
		
		const Connec& operator[](int c) const noexcept { return connex[c]; }

		void loadPatch(juce::ValueTree& state)
		{
			StateIDs ids;
			const auto connexState = state.getChildWithName(ids.connex);
			if (!connexState.isValid()) return;
			auto c = 0;
			for (; c < connexState.getNumChildren(); ++c)
			{
				const auto connecState = connexState.getChild(c);
				const auto pIdx = static_cast<int>(connecState.getProperty(ids.param));
				const auto mIdx = static_cast<int>(connecState.getProperty(ids.mod));
				const auto depth = static_cast<float>(connecState.getProperty(ids.value));
				connex[c].enable(mIdx, pIdx, depth);
			}
			for (; c < connex.size(); ++c)
				connex[c].disable();
		}
		
		void savePatch(juce::ValueTree& state)
		{
			StateIDs ids;
			auto connexState = state.getChildWithName(ids.connex);
			if (!connexState.isValid())
			{
				connexState = juce::ValueTree(ids.connex);
				state.appendChild(connexState, nullptr);
			}
			else
				connexState.removeAllChildren(nullptr);
			
			for (auto c = 0; c < connex.size(); ++c)
				if (connex[c].isEnabled())
					connex[c].savePatch(connexState, ids);
				else
					return;
		}

		std::array<Connec, 32> connex;

		juce::String toString() const
		{
			juce::String str("Connex:\n");
			for (auto c = 0; c < connex.size(); ++c)
				if (!connex[c].isEnabled())
					return str;
				else
					str += "c: " + juce::String(c) + "; " + connex[c].toString() + "\n";
			return str;
		}
	};

	struct ModSys
	{
		ModSys(juce::AudioProcessor& audioProcessor, std::function<void()>&& _updatePatch) :
			state(),
			beatsData(),
			params(audioProcessor, beatsData),
			mods(params),
			connex(),
			hasPlayHead(true),
			updatePatchFunc(_updatePatch)
		{
			{
				StateIDs ids;
				state = juce::ValueTree(ids.state);
			}
		}

		void updatePatch(const juce::String& xmlString)
		{
			state = state.fromXml(xmlString);
			updatePatchFunc();
		}
		
		void updatePatch(const juce::ValueTree& newState)
		{
			state = newState;
			updatePatchFunc();
		}

		void loadPatch()
		{
			connex.loadPatch(state);
			params.loadPatch(state);
		}
		
		void savePatch()
		{
			params.savePatch(state);
			connex.savePatch(state);
		}

		void processBlock(int numSamples, const juce::AudioPlayHead* playHead) noexcept
		{
			hasPlayHead.store(playHead != nullptr);
			params.processBlockInit();
			mods.processBlock(numSamples);
			connex.processBlock(params, mods);
			params.processBlockFinish();
		}

		const Param* getParam(PID p) const noexcept { return params[static_cast<int>(p)]; }
		Param* getParam(PID p) noexcept { return params[static_cast<int>(p)]; }
		const Param* getParam(int p) const noexcept { return params[p]; }
		Param* getParam(int p) noexcept { return params[p]; }
		const Params& getParams() const noexcept { return params; }
		Params& getParams() noexcept { return params; }
		int getParamIdx(PID pID) const noexcept {
			for (auto p = 0; p < NumParams; ++p)
				if (params[p]->id == pID)
					return p;
			return -1;
		}

		const BeatsData& getBeatsData() const noexcept
		{
			return beatsData;
		}

		int getModIdx(ModTypeContext mtc) const noexcept
		{
			for (auto m = 0; m < mods.numMods(); ++m)
				if (mods[m].mtc == mtc)
					return m;
			return -1;
		}
		
		int getConnecIdxWith(int mIdx, PID pID) const noexcept
		{
			return connex.getConnecIdxWith(mIdx, getParamIdx(pID));
		}
		
		int getConnecIdxWith(int mIdx, int pIdx) const noexcept
		{
			return connex.getConnecIdxWith(mIdx, pIdx);
		}

		bool enableConnection(int mIdx, int pIdx, float depth)
		{
			const auto cIdx = connex.getConnecIdxWith(mIdx, pIdx);
			const bool connectionExistsAlready = cIdx != -1;
			if (connectionExistsAlready)
				return false;

			const bool modDoesntExist = mIdx >= mods.numMods();
			if (modDoesntExist)
				return false;
			
			auto param = params[pIdx];
			const bool paramDoesntExist = param == nullptr;
			if (paramDoesntExist)
				return false;

			const auto& mod = mods[mIdx];
			for (auto p = 0; p < mod.params.size(); ++p)
			{
				const bool modTrysToModulateParamOfSameMod = mod.params[p]->id == param->id;
				if (modTrysToModulateParamOfSameMod)
					return false;
			}

			const auto paramMTC = param->attachedMod;
			const auto paramModIdx = getModIdx(paramMTC);
			if (paramModIdx != -1)
				for (auto p = 0; p < mod.params.size(); ++p)
				{
					const auto xModIdx = connex.getConnecIdxWith(paramModIdx, getParamIdx(mod.params[p]->id));
					const bool paramModTrysToModulateParamOfMod = xModIdx != -1;
					if (paramModTrysToModulateParamOfMod)
						return false;
				}

			return connex.enableConnection(mIdx, pIdx, depth);
		}
		
		bool disableConnection(int cIdx) noexcept
		{
			//const auto param = params[connex[cIdx].getPIdx()];
			connex[cIdx].disable();
			return true;	
		}

		float getConnecDepth(int cIdx) const noexcept
		{
			return connex[cIdx].getDepth();
		}
		
		bool setConnecDepth(int cIdx, float depth) noexcept
		{
			//const auto param = params[connex[cIdx].getPIdx()];
			connex[cIdx].setDepth(depth);
			return true;
		}

		bool getHasPlayHead() const noexcept
		{
			return hasPlayHead.load();
		}

		void dbgConnex()
		{
			DBG(connex.toString());
		}

		juce::ValueTree state;
	protected:
		BeatsData beatsData;
		Params params;
		Mods mods;
		Connex connex;
		std::atomic<bool> hasPlayHead;
		std::function<void()> updatePatchFunc;
	};
}

/*

stringToVal functions sometimes make no sense
	stringToVal functions need ability to return current value if input invalid
		how to determine if input is invalid without causing edgecases?

*/
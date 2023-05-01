#pragma once
#include "../FormulaParser.h"
#include <JuceHeader.h>

namespace modSys6
{
	using String = juce::String;
	using Identifier = juce::Identifier;
	using ValueTree = juce::ValueTree;

	inline float nextLowestPowTwoX(float x) noexcept
	{
		return std::pow(2.f, std::floor(std::log2(x)));
	}

	inline bool stringNegates(const String& t)
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

	inline String toID(const String& name)
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

		Depth, ModsMix, DryWetMix, WetGain, StereoConfig, Feedback, Damp, HQ, Lookahead, BufferSize,

		NumParams
	};
	
	static constexpr int NumMacros = static_cast<int>(PID::MSMacro3) + 1;
	static constexpr int NumParamsPerMod = static_cast<int>(PID::Perlin1RateHz) - NumMacros;
	static constexpr int NumParams = static_cast<int>(PID::NumParams);
	
	inline String toString(PID pID)
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
		case PID::BufferSize: return "BufferSize";

		default: return "";
		}
	}
	
	inline PID withOffset(PID p, int o) noexcept
	{
		return static_cast<PID>(static_cast<int>(p) + o);
	}

	enum class Unit
	{
		Percent,
		Hz,
		Beats,
		Degree,
		Octaves,
		Semi,
		Fine,
		Ms,
		Decibel,
		Power,
		PerlinShape,
		PerlinRandType,
		NumUnits
	};
	
	inline String toString(Unit pID)
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

	using ValToStrFunc = std::function<String(float)>;
	using StrToValFunc = std::function<float(const String&)>;

	namespace stateID
	{
		inline Identifier state() { return "state"; };
		inline Identifier params() { return "params"; };
		inline Identifier value() { return "value"; };
		inline Identifier modDepth(int m) { return "moddepth" + String(m); };
		inline Identifier modBias(int m) { return "modbias" + String(m); };
	};

	using Range = juce::NormalisableRange<float>;

	namespace makeRange
	{
		inline Range bias(float start, float end, float bias) noexcept
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

		inline Range biasXL(float start, float end, float bias) noexcept
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

		inline Range withCentre(float start, float end, float centre) noexcept
		{
			const auto r = end - start;
			const auto v = (centre - start) / r;

			return biasXL(start, end, 2.f * v - 1.f);
		}

		inline Range quad(float min, float max, int numSteps) noexcept
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

		inline Range stepped(float start, float end, float steps = 1.f) noexcept
		{
			return { start, end, steps };
		}

		inline Range temposync(int numSteps)
		{
			return stepped(0.f, static_cast<float>(numSteps));
		}

		inline Range beats(double minDenominator, double maxDenominator, bool withZero)
		{
			std::vector<float> table;

			const auto minV = std::log2(minDenominator);
			const auto maxV = std::log2(maxDenominator);

			const auto numWholeBeatsF = static_cast<double>(minV - maxV);
			const auto numWholeBeatsInv = 1. / numWholeBeatsF;

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
				const auto baseVal = std::pow(2., curV);

				const auto valWhole = 1. / baseVal;
				const auto valTriplet = valWhole * 1.66666666666666666667;
				const auto valDotted = valWhole * 1.75;

				table.emplace_back(static_cast<float>(valWhole));
				table.emplace_back(static_cast<float>(valTriplet));
				table.emplace_back(static_cast<float>(valDotted));
			}
			table.emplace_back(static_cast<float>(1. / maxDenominator));

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

		inline Range beatsSlowToFast(double maxDenominator, double minDenominator, bool withZero)
		{
			std::vector<float> table;

			const auto minV = std::log2(minDenominator);
			const auto maxV = std::log2(maxDenominator);

			const auto numWholeBeatsF = static_cast<double>(minV - maxV);
			const auto numWholeBeatsInv = 1. / numWholeBeatsF;

			const auto numWholeBeats = static_cast<int>(numWholeBeatsF);
			const auto numValues = numWholeBeats * 3 + 1 + (withZero ? 1 : 0);
			table.reserve(numValues);
			
			if (withZero)
				table.emplace_back(0.f);
			for (auto i = 0; i < numWholeBeats; ++i)
			{
				const auto iF = static_cast<double>(i);
				const auto x = iF * numWholeBeatsInv;

				const auto curV = maxV + x * numWholeBeatsF;
				const auto baseVal = std::pow(2., curV);

				const auto valWhole = 1. / baseVal;
				const auto valDotted = valWhole * .75;
				const auto valTriplet = valWhole * 2. / 3.;
				
				table.emplace_back(static_cast<float>(valWhole));
				table.emplace_back(static_cast<float>(valDotted));
				table.emplace_back(static_cast<float>(valTriplet));
			}
			table.emplace_back(static_cast<float>(1. / minDenominator));

			for (auto i = (withZero ? 1 : 0); i < table.size(); ++i)
				table[i] = 1.f / table[i];

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

		inline Range toggle() noexcept
		{
			return stepped(0.f, 1.f);
		}

		inline Range lin(float start, float end) noexcept
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

		inline Range bufferSizes(const std::vector<float>& bufferSizes)
		{
			return
			{
					bufferSizes.front(), bufferSizes.back(),
					[bufferSizes](float, float, float normalized)
					{
						const auto idx = static_cast<int>(std::round(normalized * static_cast<float>(bufferSizes.size() - 1)));
						return bufferSizes[idx];
					},
					[bufferSizes](float, float, float denormalized)
					{
						for (auto i = 0; i < bufferSizes.size(); ++i)
							if (denormalized <= bufferSizes[i])
								{
									return static_cast<float>(i) / static_cast<float>(bufferSizes.size() - 1);
								}
						return bufferSizes.front();
					},
					nullptr
			};
		}
	}

	struct Param :
		public juce::AudioProcessorParameter
	{
		static constexpr float BiasEps = .000001f;
		
		Param(const PID pID, const Range& _range, const float _valDenormDefault,
			const ValToStrFunc& _valToStr, const StrToValFunc& _strToVal,
			const Unit _unit = Unit::NumUnits, int _attachedMod = -1) :

			juce::AudioProcessorParameter(),
			id(pID),
			range(_range),
			attachedMod(_attachedMod),
			valDenormDefault(_valDenormDefault),
			valNorm(range.convertTo0to1(_valDenormDefault)),
			modDepth{ 0.f, 0.f, 0.f, 0.f },
			modBias{ .5f, .5f, .5f, .5f },
			valToStr(_valToStr),
			strToVal(_strToVal),
			unit(_unit),
			valNormSum(0.f),
			locked(false),
			valMod(0.f)
		{
		}
		
		void savePatch(ValueTree& state) const
		{
			const auto nVal = denormalized();
			state.setProperty(stateID::value(), nVal, nullptr);
			for (auto i = 0; i < NumMacros; ++i)
			{
				const auto md = modDepth[i].load();
				state.setProperty(stateID::modDepth(i), md, nullptr);

				const auto mb = modBias[i].load();
				state.setProperty(stateID::modBias(i), mb, nullptr);
			}
		}

		void loadPatch(ValueTree& state)
		{
			if (locked.load())
				return;

			auto nVal = static_cast<float>(state.getProperty(stateID::value(), valDenormDefault));
			nVal = range.convertTo0to1(range.snapToLegalValue(nVal));
			setValueNotifyingHost(nVal);

			for (auto i = 0; i < NumMacros; ++i)
			{
				const auto md = static_cast<float>(state.getProperty(stateID::modDepth(i), 0.f));
				modDepth[i].store(md);

				const auto mb = static_cast<float>(state.getProperty(stateID::modBias(i), .5f));
				modBias[i].store(mb);
			}
		}

		float getValue() const override
		{
			return valNorm.load();
		}
		
		float denormalized() const noexcept
		{
			return range.convertFrom0to1(valNorm.load());
		}

		void setValue(float normalized) override
		{
			if (locked.load())
				return;
			
			valNorm.store(normalized);
		}
		
		void setValueWithGesture(float norm)
		{
			beginChangeGesture();
			setValueNotifyingHost(norm);
			endChangeGesture();
		}
		
		void beginGesture()
		{
			beginChangeGesture();
		}
		
		void endGesture()
		{
			endChangeGesture();
		}

		float getDefaultValue() const override
		{
			return range.convertTo0to1(valDenormDefault);
		}

		String getName(int) const override
		{
			return toString(id);
		}

		String getID() const
		{
			return toID(getName(0));
		}

		// units of param (hz, % etc.)
		String getLabel() const override
		{
			return toString(unit);
		}

		// string of norm val
		String getText(float norm, int) const override
		{
			return valToStr(range.snapToLegalValue(range.convertFrom0to1(norm)));
		}

		// string to norm val
		float getValueForText(const String& text) const override
		{
			const auto val = juce::jlimit(range.start, range.end, strToVal(text));
			return range.convertTo0to1(val);
		}
		
		// string to denorm val
		float getValForTextDenorm(const String& text) const
		{
			return strToVal(text);
		}

		void modulateInit() noexcept
		{
			const auto val = valNorm.load();
			valMod = val;
		}

		void modulate(float depth, int mIdx) noexcept
		{
			valMod = calcValMod(valMod, depth, mIdx);
		}

		void modulateEnd() noexcept
		{
			valNormSum.store(juce::jlimit(0.f, 1.f, valMod));
		}

		float getValueSum() const noexcept
		{
			return valNormSum.load();
		}
		
		float getValSumDenorm() const noexcept
		{
			return range.snapToLegalValue(range.convertFrom0to1(valNormSum.load()));
		}

		String getDescription()
		{
			auto v = getValue();
			return getName(10) + ": " + String(v) + "; " + getText(v, 10) + (attachedMod == -1 ? "" : "; mod: " + String(attachedMod));
		}

		void setModBias(float b, int mIdx) noexcept
		{
			if (locked.load())
				return;

			if (modDepth[mIdx] == 0.f)
				return;

			b = juce::jlimit(BiasEps, 1.f - BiasEps, b);
			modBias[mIdx].store(b);
		}

		/* start, end, bias[0,1], x */
		float biased(float start, float end, float bias, float depth) const noexcept
		{
			const auto r = end - start;
			if (r == 0.f)
				return 0.f;
			const auto a2 = 2.f * bias;
			const auto aM = 1.f - bias;
			const auto aR = r * bias;
			return start + aR * depth / (aM - depth + a2 * depth);
		}

		/* modVal, depth[0,1], mIdx[0,3] */
		float calcValMod(float modVal, float depth, int mIdx) const noexcept
		{
			const auto mmd = modDepth[mIdx].load();
			const auto bias = modBias[mIdx].load();
			if (bias == .5f)
				return modVal + mmd * depth;
			
			const auto pol = mmd > 0.f ? 1.f : -1.f;
			const auto md = mmd * pol;
			const auto mdSkew = biased(0.f, md, bias, depth);
			const auto mod = mdSkew * pol;

			return modVal + mod;
		}

		const PID id;
		const Range range;
		const int attachedMod;
		const float valDenormDefault;
		std::atomic<float> valNorm;
		std::array<std::atomic<float>, NumMacros> modDepth, modBias;
		ValToStrFunc valToStr;
		StrToValFunc strToVal;
		Unit unit;
		std::atomic<float> valNormSum;
		std::atomic<bool> locked;
		float valMod;
	};

	struct Params
	{
		int getDigitFromString(const String& txt) noexcept
		{
			for (auto t = 0; t < txt.length(); ++t)
			{
				const auto chr = txt[t];
				if (!(chr < '0' || chr > '9'))
					return static_cast<int>(chr - '0');
			}
			return -1;
		}

		Params(juce::AudioProcessor& audioProcessor) :
			state("state"),
			params()
		{
			const ValToStrFunc valToStrPercent = [](float v)
			{
				v = std::round(v * 100.f);
				return String(v) + " " + toString(Unit::Percent);
			};
			
			const ValToStrFunc valToStrHz = [](float v)
			{
				if (v >= 10000.f)
					return String(v * .001).substring(0, 4) + " khz";
				else if (v >= 1000.f)
					return String(v * .001).substring(0, 3) + " khz";
				else if (v >= 1.f)
					return String(v).substring(0, 5) + " hz";
				else
				{
					v *= 1000.f;

					if (v >= 100.f)
						return String(v).substring(0, 3) + " mhz";
					else if (v >= 10.f)
						return String(v).substring(0, 2) + " mhz";
					else
						return String(v).substring(0, 1) + " mhz";
				}
			};
			
			const ValToStrFunc valToStrPhase = [](float v)
			{
				return String(std::round(v * 180.f)) + " " + toString(Unit::Degree);
			};
			
			const ValToStrFunc valToStrPhase360 = [](float v)
			{
				return String(std::round(v * 360.f)) + " " + toString(Unit::Degree);
			};
			
			const ValToStrFunc valToStrOct = [](float v)
			{
				return juce::String(std::round(v)) + " " + toString(Unit::Octaves);
			};
			
			const ValToStrFunc valToStrOct2 = [](float v)
			{
				return juce::String(std::round(v / 12.f)) + " " + toString(Unit::Octaves);
			};
			
			const ValToStrFunc valToStrSemi = [](float v)
			{
				return String(std::round(v)) + " " + toString(Unit::Semi);
			};
			
			const ValToStrFunc valToStrFine = [](float v)
			{
				return juce::String(std::round(v * 100.f)) + " " + toString(Unit::Fine);
			};
			
			const ValToStrFunc valToStrRatio = [](float v)
			{
				const auto y = static_cast<int>(std::round(v * 100.f));
				return String(100 - y) + " : " + String(y);
			};
			
			const ValToStrFunc valToStrLRMS = [](float v)
			{
				return v > .5f ? String("m/s") : String("l/r");
			};
			
			const ValToStrFunc valToStrFreeSync = [](float v)
			{
				return v > .5f ? String("sync") : String("free");
			};
			
			const ValToStrFunc valToStrPolarity = [](float v)
			{
				return v > .5f ? String("on") : String("off");
			};
			
			const ValToStrFunc valToStrMs = [](float v)
			{
				return String(std::round(v * 10.f) * .1f) + " " + toString(Unit::Ms);
			};
			
			const ValToStrFunc valToStrDb = [](float v)
			{
				v = std::round(v * 100.f) * .01f;
				if(v > -120.f)
					return String(v) + " " + toString(Unit::Decibel);
				else
					return String("-inf ") + toString(Unit::Decibel);;
			};
			
			const ValToStrFunc valToStrEmpty = [](float)
			{
				return String("");
			};
			
			const ValToStrFunc valToStrSeed = [](float v)
			{
				if (v == 0.f)
					return String("off");

				String str("abcde");
				juce::Random rnd;
				for(auto i = 0; i < str.length(); ++i)
					str = str.replaceCharacter('a' + i, static_cast<juce::juce_wchar>(rnd.nextInt(26) + 'a'));
				return str;
			};
			
			const ValToStrFunc valToStrBeats2 = [](float v)
			{
				enum Mode { Whole, Triplet, Dotted, NumModes };
					
				if (v == 0.f)
					return String("0");

				const auto denormFloor = nextLowestPowTwoX(v);
				const auto denormFrac = v - denormFloor;
				const auto modeVal = denormFrac / denormFloor;
				const auto mode = modeVal < .66f ? Mode::Whole :
					modeVal < .75f ? Mode::Triplet :
					Mode::Dotted;
				const auto modeStr = mode == Mode::Whole ? String("") :
					mode == Mode::Triplet ? String("t") :
					String(".");

				auto denominator = 1.f / denormFloor;
				auto numerator = 1.f;
				if (denominator < 1.f)
				{
					numerator = denormFloor;
					denominator = 1.f;
				}

				return String(numerator) + " / " + String(denominator) + modeStr;
			};
			
			const ValToStrFunc valToStrBeatsSlowToFast = [](float v)
			{
				enum Mode { Whole, Triplet, Dotted, NumModes };

				if (v == 0.f)
					return String("0");

				v = 1.f / v;

				const auto denormFloor = nextLowestPowTwoX(v);
				const auto denormFrac = v - denormFloor;
				const auto modeVal = denormFrac / denormFloor;
				const auto mode = modeVal >= .5f ? Mode::Dotted :
					modeVal >= .333f ? Mode::Triplet :
					Mode::Whole;
				const auto modeStr = mode == Mode::Whole ? String("") :
					mode == Mode::Triplet ? String("t") :
					String(".");

				auto denominator = 1.f / denormFloor;
				auto numerator = 1.f;
				if (denominator < 1.f)
				{
					numerator = denormFloor;
					denominator = 1.f;
				}

				return String(numerator) + " / " + String(denominator) + modeStr;
			};

			const ValToStrFunc valToStrPower = [](float v)
			{
				return String((v > .5f ? "Enabled" : "Disabled"));
			};

			const auto parse = [](const String& str, float defaultVal)
			{
				fx::Parser parse;
				if (parse(str))
					return parse();
				return defaultVal;
			};

			const StrToValFunc strToValPercent = [parse](const String& txt)
			{
				return parse(txt.trimCharactersAtEnd(toString(Unit::Percent)), 0.f) * .01f;
			};
			
			const StrToValFunc strToValHz = [parse](const String& txt)
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
				const auto val = parse(text, 0.f);
				const auto val2 = val * multiplier;

				return val2;
			};
			
			const StrToValFunc strToValPhase = [parse](const String& txt)
			{
				return parse(txt.trimCharactersAtEnd(toString(Unit::Degree)), 0.f) / 180.f;
			};
			
			const StrToValFunc strToValPhase360 = [parse](const String& txt)
			{
				return parse(txt.trimCharactersAtEnd(toString(Unit::Degree)), 0.f) / 360.f;
			};
			
			const StrToValFunc strToValOct = [parse](const String& txt)
			{
				return std::round(parse(txt.trimCharactersAtEnd(toString(Unit::Octaves)), 0.f));
			};
			
			const StrToValFunc strToValOct2 = [parse](const String& txt)
			{
				return parse(txt.trimCharactersAtEnd(toString(Unit::Octaves)), 0.f) / 12.f;
			};
			
			const StrToValFunc strToValSemi = [parse](const String& txt)
			{
				return std::round(parse(txt.trimCharactersAtEnd(toString(Unit::Semi)), 0.f));
			};
			
			const StrToValFunc strToValFine = [parse](const String& txt)
			{
				return parse(txt.trimCharactersAtEnd(toString(Unit::Fine)), 0.f) * .01f;
			};
			
			const StrToValFunc strToValRatio = [parse](const String& txt)
			{
				return parse(txt, 0.f) * .01f;
			};
			
			const StrToValFunc strToValLRMS = [parse](const String& txt)
			{
				return txt[0] == 'l' ? 0.f : 1.f;
			};
			
			const StrToValFunc strToValFreeSync = [parse](const String& txt)
			{
				return txt[0] == 'f' ? 0.f : 1.f;
			};
			
			const StrToValFunc strToValPolarity = [parse](const String& txt)
			{
				return txt[0] == '0' ? 0.f : 1.f;
			};
			
			const StrToValFunc strToValMs = [parse](const String& txt)
			{
				return parse(txt.trimCharactersAtEnd(toString(Unit::Ms)), 0.f);
			};
			
			const StrToValFunc strToValDb = [parse](const String& txt)
			{
				if (txt == "inf" || txt == "-inf")
					return -120.f;
				return parse(txt.trimCharactersAtEnd(toString(Unit::Decibel)), 0.f);
			};
			
			const StrToValFunc strToValSeed = [parse](const String& str)
			{
				return parse(str, 0.f);
			};
			
			const StrToValFunc strToValBeats2 = [parse](const String& txt)
			{
				enum Mode { Beats, Triplet, Dotted, NumModes };
				const auto lastChr = txt[txt.length() - 1];
				const auto mode = lastChr == 't' ? Mode::Triplet : lastChr == '.' ? Mode::Dotted : Mode::Beats;

				const auto text = mode == Mode::Beats ? txt : txt.substring(0, txt.length() - 1);
				auto val = parse(txt, 0.f);
				if (mode == Mode::Triplet)
					val *= 1.666666666667f;
				else if (mode == Mode::Dotted)
					val *= 1.75f;
				return val;
			};
			
			const StrToValFunc strToValBeatsSlowToFast = [parse](const String& txt)
			{
				enum Mode { Beats, Triplet, Dotted, NumModes };
				const auto lastChr = txt[txt.length() - 1];
				const auto mode = lastChr == 't' ? Mode::Triplet : lastChr == '.' ? Mode::Dotted : Mode::Beats;

				const auto text = mode == Mode::Beats ? txt : txt.substring(0, txt.length() - 1);
				auto val = parse(txt, 1.f);
				if (mode == Mode::Triplet)
					val *= 1.666666666667f;
				else if (mode == Mode::Dotted)
					val *= 1.75f;
				return 1.f / val;
			};
			
			const StrToValFunc strToValPower = [parse](const String& txt)
			{
				const auto text = txt.trimCharactersAtEnd(toString(Unit::Power));
				if (stringNegates(text))
					return 0.f;
				return parse(text, 0.f);
			};

			ValToStrFunc valToStrShape = [](float v)
			{
				return v < .5f ? juce::String("Steppy") :
					v < 1.5f ? juce::String("Lerp") :
					juce::String("Round");
			};
			StrToValFunc strToValShape = [parse](const String& str)
			{
				const auto text = str.toLowerCase();
				if (text == "steppy" || text == "step")
					return 0.f;
				else if (text == "lerp" || text == "linear")
					return 1.f;
				else if (text == "round" || text == "smooth")
					return 2.f;
				
				return parse(str, 0.f);
			};

			ValToStrFunc valToStrRandType = [](float v)
			{
				return v < .5f ? juce::String("Random") :
					juce::String("Procedural");
			};
			StrToValFunc strToValRandType = [parse](const String& str)
			{
				const auto text = str.toLowerCase();
				if (text == "rand" || text == "random" || text == "randomise" || text == "randomize")
					return 0.f;
				else if (text == "procedural" || text == "proc" || text == "proceduralise" || text == "proceduralize")
					return 1.f;
				
				return parse(str, 1.f);
			};

			ValToStrFunc valToStrHQ = [](float v)
			{
				return v < .5f ? juce::String("1x") :
					juce::String("4x");
			};
			StrToValFunc strToValHQ = [parse](const String& str)
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
			StrToValFunc strToValLookahead = [parse](const String& str)
			{
				const auto text = str.toLowerCase();
				if (text == "off" || text == "false" || text == "0" || text == "Nopezies")
					return 0.f;
				else if (text == "on" || text == "true" || text == "1" || text == "Fuck Yeah")
					return 1.f;

				return 1.f;
			};

			ValToStrFunc valToStrBufferSize = [](float v)
			{
				if(v < 1000.f)
					return String(std::floor(v)) + " ms";
				v /= 1000.f;
				return String(std::floor(v)) + "k ms";
			};
			StrToValFunc strToValBufferSize = [parse](const String& str)
			{
				auto nStr = str.removeCharacters("ms").removeCharacters(" ");
				if (nStr[nStr.length() - 1] == 'k')
				{
					auto val = parse(nStr.removeCharacters("k"), .001f);
					return val * 1000.f;
				}
				return parse(str, 1.f);
			};

			// ADD MACRO PARAMETERS
			for (auto p = 0; p < NumMacros; ++p)
			{
				const PID pID = static_cast<PID>(p);
				params.push_back(new Param(pID, Range(0.f, 1.f), 1.f, valToStrPercent, strToValPercent, Unit::Percent, p));
			}

			// ADD VIBRATO MODSYS PARAMETERS
			for (auto m = 0; m < 2; ++m)
			{
				const auto offset = m * NumParamsPerMod;
				params.push_back(new Param(withOffset(PID::Perlin0RateHz, offset), makeRange::withCentre(1.f / 1000.f, 40.f, 2.f), .420f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::Perlin0RateBeats, offset), makeRange::beatsSlowToFast(.25, 64., false), 4.f, valToStrBeatsSlowToFast, strToValBeatsSlowToFast, Unit::Beats));
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

				params.push_back(new Param(withOffset(PID::LFO0FreeSync, offset), makeRange::toggle(), 0.f, valToStrFreeSync, strToValFreeSync, Unit::NumUnits));
				params.push_back(new Param(withOffset(PID::LFO0RateFree, offset), makeRange::quad(.2f, 40.f, 2), 4.f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::LFO0RateSync, offset), makeRange::beatsSlowToFast(.25, 64., false), 8.f, valToStrBeatsSlowToFast, strToValBeatsSlowToFast, Unit::Beats));
				params.push_back(new Param(withOffset(PID::LFO0Waveform, offset), makeRange::lin(0.f, 1.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));
				params.push_back(new Param(withOffset(PID::LFO0Phase, offset), makeRange::stepped(-.5f, .5f, LFOPhaseStep), 0.f, valToStrPhase360, strToValPhase, Unit::Degree));
				params.push_back(new Param(withOffset(PID::LFO0Width, offset), makeRange::stepped(0.f, .5f, LFOPhaseStep), 0.f, valToStrPhase360, strToValPhase, Unit::Degree));
			}

			// ADD GLOBAL PARAMETERS
			params.push_back(new Param(PID::Depth, makeRange::lin(0.f, 1.f), 1.f, valToStrPercent, strToValPercent, Unit::Percent));
			params.push_back(new Param(PID::ModsMix, makeRange::biasXL(0.f, 1.f, 0.f), 0.f, valToStrRatio, strToValRatio));
			params.push_back(new Param(PID::DryWetMix, makeRange::biasXL(0.f, 1.f, 0.f), 1.f, valToStrRatio, strToValRatio));
			params.push_back(new Param(PID::WetGain, makeRange::biasXL(-120.f, 4.5f, .9f), 0.f, valToStrDb, strToValDb));
			params.push_back(new Param(PID::StereoConfig, makeRange::toggle(), 1.f, valToStrLRMS, strToValLRMS));
			params.push_back(new Param(PID::Feedback, makeRange::lin(0.f, 1.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));
			params.push_back(new Param(PID::Damp, makeRange::quad(40.f, 8000.f, 2), 180.f, valToStrHz, strToValHz, Unit::Hz));
			params.push_back(new Param(PID::HQ, makeRange::toggle(), 1.f, valToStrHQ, strToValHQ, Unit::Power));
			params.push_back(new Param(PID::Lookahead, makeRange::toggle(), 1.f, valToStrLookahead, strToValLookahead, Unit::Power));
			params.push_back(new Param(PID::BufferSize, makeRange::bufferSizes({1.f, 4.f, 12.f, 24.f, 69.f, 420.f, 2000.f}), 0, valToStrBufferSize, strToValBufferSize));

			for (auto param : params)
				audioProcessor.addParameter(param);
		}
		
		void loadPatch()
		{
			auto childParams = state.getChildWithName(stateID::params());
			if (!childParams.isValid())
				return;

			for (auto param : params)
			{
				const auto id = param->getID();
				auto childParam = childParams.getChildWithName(id);
				if(childParam.isValid())
					param->loadPatch(childParam);
			}
		}
		
		void savePatch()
		{
			auto childParams = state.getChildWithName(stateID::params());
			if (!childParams.isValid())
			{
				childParams = ValueTree(stateID::params());
				state.appendChild(childParams, nullptr);
			}
			
			for (auto param : params)
			{
				const auto id = param->getID();
				auto childParam = childParams.getChildWithName(id);
				if (!childParam.isValid())
				{
					childParam = ValueTree(id);
					childParams.appendChild(childParam, nullptr);
				}
				param->savePatch(childParam);
			}
		}

		void setModDepth(PID pID, float md, int mIdx) noexcept
		{
			auto pIDInt = static_cast<int>(pID);

			auto& paramDest = *params[pIDInt];

			if (paramDest.locked.load())
				return;

			const bool sameMacro = pID == static_cast<PID>(mIdx);
			if (sameMacro)
				return;

			const auto& paramSrc = *params[mIdx];
			const auto depthFromDest = paramSrc.modDepth[pIDInt].load();
			const bool isDestParamAlreadyModulatingSelectedModulator = depthFromDest != 0.f;
			if (isDestParamAlreadyModulatingSelectedModulator)
				return;

			paramDest.modDepth[mIdx].store(md);
		}

		void updatePatch(const ValueTree& other)
		{
			state = other;
			loadPatch();
		}

		void processMacros() noexcept
		{
			for (auto param : params)
				param->modulateInit();

			for (auto i = 0; i < NumMacros; ++i)
			{
				auto& macro = *params[i];
				auto depth = macro.getValueSum();

				for (auto j = 0; j < NumParams; ++j)
				{
					if (i != j)
					{
						auto& param = *params[j];
						param.modulate(depth, i);
					}
				}
			}

			for (auto param : params)
				param->modulateEnd();
		}

		int getParamIdx(const String& name) const noexcept
		{
			for (auto p = 0; p < params.size(); ++p)
			{
				const auto str = toString(params[p]->id);
				if (name == str || name == toID(str))
					return p;
			}	
			return -1;
		}

		size_t numParams() const noexcept
		{
			return params.size();
		}

		Param* operator[](int i) noexcept
		{
			return params[i];
		}
		
		const Param* operator[](int i) const noexcept
		{
			return params[i];
		}
		
		Param& operator()(int i) noexcept
		{
			return *params[i];
		}

		const Param& operator()(int i) const noexcept
		{
			return *params[i];
		}

		Param& operator()(PID pID) noexcept
		{
			return *params[static_cast<int>(pID)];
		}

		const Param& operator()(PID pID) const noexcept
		{
			return *params[static_cast<int>(pID)];
		}

		ValueTree state;
	protected:
		std::vector<Param*> params;
	};
}
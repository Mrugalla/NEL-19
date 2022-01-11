#pragma once
#include <JuceHeader.h>

namespace modSys6
{
	static constexpr float tau = 6.28318530718f;
	static constexpr float pi = 3.14159265359f;
	static constexpr float piHalf = 1.57079632679f;
	static constexpr float piQuart = .785398163397f;
	static constexpr float piInv = 1.f / pi;

	inline juce::String toID(const juce::String& name) { return name.toLowerCase().removeCharacters(" "); }

	enum class PID
	{
		MSMacro0, MSMacro1, MSMacro2, MSMacro3,
		
		Perlin0FreqHz, Perlin0Octaves, Perlin0Width,
		AudioRate0Oct, AudioRate0Semi, AudioRate0Fine, AudioRate0Width, AudioRate0RetuneSpeed, AudioRate0Atk, AudioRate0Dcy, AudioRate0Sus, AudioRate0Rls,
		Dropout0Decay, Dropout0Spin, Dropout0Chance, Dropout0Smooth, Dropout0Width,
		EnvFol0Attack, EnvFol0Release, EnvFol0Gain, EnvFol0Width,
		Macro0,
		Pitchbend0Smooth,
		LFO0FreeSync, LFO0RateFree, LFO0RateSync, LFO0Waveform, LFO0Phase, LFO0Width,

		Perlin1FreqHz, Perlin1Octaves, Perlin1Width,
		AudioRate1Oct, AudioRate1Semi, AudioRate1Fine, AudioRate1Width, AudioRate1RetuneSpeed, AudioRate1Atk, AudioRate1Dcy, AudioRate1Sus, AudioRate1Rls,
		Dropout1Decay, Dropout1Spin, Dropout1Chance, Dropout1Smooth, Dropout1Width,
		EnvFol1Attack, EnvFol1Release, EnvFol1Gain, EnvFol1Width,
		Macro1,
		Pitchbend1Smooth,
		LFO1FreeSync, LFO1RateFree, LFO1RateSync, LFO1Waveform, LFO1Phase, LFO1Width,

		Depth, ModsMix, DryWetMix, WetGain, StereoConfig,

		NumParams
	};
	static constexpr int NumMSParams = static_cast<int>(PID::MSMacro3) + 1;
	static constexpr int NumParamsPerMod = static_cast<int>(PID::Perlin1FreqHz) - NumMSParams;
	static constexpr int NumParams = static_cast<int>(PID::NumParams);
	inline juce::String toString(PID pID)
	{
		switch (pID)
		{
		case PID::MSMacro0: return "MS Macro 0";
		case PID::MSMacro1: return "MS Macro 1";
		case PID::MSMacro2: return "MS Macro 2";
		case PID::MSMacro3: return "MS Macro 3";

		case PID::Perlin0FreqHz: return "Perlin 0 Freq Hz";
		case PID::Perlin0Octaves: return "Perlin 0 Octaves";
		case PID::Perlin0Width: return "Perlin 0 Width";
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

		case PID::Perlin1FreqHz: return "Perlin 1 Freq Hz";
		case PID::Perlin1Octaves: return "Perlin 1 Octaves";
		case PID::Perlin1Width: return "Perlin 1 Width";
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

		default: return "";
		}
	}
	inline int withOffset(PID p, int o) noexcept { return static_cast<int>(p) + o; }

	enum class Unit { Percent, Hz, Beats, Degree, Octaves, Semi, Fine, Ms, Decibel, NumUnits };
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

	struct Smooth
	{
		static void makeFromDecayInSamples(Smooth& s, float d) noexcept
		{
			const auto e = 2.71828182846f;
			const auto x = std::pow(e, -1.f / d);
			s.setX(x);
		}
		static void makeFromDecayInSecs(Smooth& s, float d, float Fs) noexcept
		{
			makeFromDecayInSamples(s, d * Fs);
		}
		static void makeFromDecayInFc(Smooth& s, float fc) noexcept
		{
			static constexpr float e = 2.71828182846f;
			const auto x = std::pow(e, -tau * fc);
			s.setX(x);
		}
		static void makeFromDecayInHz(Smooth& s, float d, float Fs) noexcept
		{
			makeFromDecayInFc(s, d / Fs);
		}
		static void makeFromDecayInMs(Smooth& s, float d, float Fs) noexcept
		{
			makeFromDecayInSamples(s, d * Fs * .001f);
		}

		Smooth(const bool _snap = true) :
			a0(1.f),
			b1(0.f),
			y1(0.f),
			eps(0.f),
			snap(_snap)
		{}
		void reset()
		{
			a0 = 1.f;
			b1 = 0.f;
			y1 = 0.f;
			eps = 0.f;
		}
		void setX(float x) noexcept
		{
			a0 = 1.f - x;
			b1 = x;
			eps = a0 * 1.5f;
		}
		void operator()(float* buffer, float val, int numSamples) noexcept
		{
			if (buffer[0] == val)
				return juce::FloatVectorOperations::fill(buffer, val, numSamples);
			for (auto s = 0; s < numSamples; ++s)
				buffer[s] = processSample(val);
		}
		void operator()(float* buffer, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				buffer[s] = processSample(buffer[s]);
		}
		float operator()(float sample) noexcept
		{
			return processSample(sample);
		}
	protected:
		float a0, b1, y1, eps;
		const bool snap;

		float processSample(float x0) noexcept
		{
			if (snap && std::abs(y1 - x0) < eps)
				y1 = x0;
			else
				y1 = x0 * a0 + y1 * b1;
			return y1;
		}
	};

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
						return min + aR * x / (aM - x + a2 * x);
					},
					[a2, aM, aR](float min, float, float x)
					{
						return aM * (x - min) / (a2 * min + aR - a2 * x - min + x);
					},
					nullptr
			};
			else return { start, end };
		}

		inline juce::NormalisableRange<float> toggle()
		{
			return { 0.f, 1.f, 1.f };
		}

		inline juce::NormalisableRange<float> stepped(float start, float end, float steps = 1.f)
		{
			return
			{
					start, end,
					[range = end - start](float min, float, float normalized)
					{
						return min + normalized * range;
					},
					[rangeInv = 1.f / (end - start)](float min, float, float denormalized)
					{
						return (denormalized - min) * rangeInv;
					},
					[steps, stepsInv = 1.f / steps](float, float, float val)
					{
						return std::rint(val * stepsInv) * steps;
					}
			};
		}

		inline juce::NormalisableRange<float> temposync(int numSteps)
		{
			return stepped(0.f, static_cast<float>(numSteps));
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
			beats::create(beatsData, 1.f / 64.f, 2.f);

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

			const auto valToStrPercent = [](float v) { return juce::String(std::floor(v * 100.f)) + " " + toString(Unit::Percent); };
			const auto valToStrHz = [](float v) { return juce::String(v).substring(0, 4) + " " + toString(Unit::Hz); };
			const auto valToStrBeats = [&bts = beatsData](float v) { return bts[static_cast<int>(v)].str; };
			const auto valToStrPhase = [](float v) { return juce::String(std::floor(v * 180.f)) + " " + toString(Unit::Degree); };
			const auto valToStrPhase360 = [](float v) { return juce::String(std::floor(v * 360.f)) + " " + toString(Unit::Degree); };
			const auto valToStrOct = [](float v) { return juce::String(std::floor(v)) + " " + toString(Unit::Octaves); };
			const auto valToStrOct2 = [](float v) { return juce::String(std::floor(v / 12.f)) + " " + toString(Unit::Octaves); };
			const auto valToStrSemi = [](float v) { return juce::String(std::floor(v)) + " " + toString(Unit::Semi); };
			const auto valToStrFine = [](float v) { return juce::String(std::floor(v * 100.f)) + " " + toString(Unit::Fine); };
			const auto valToStrRatio = [](float v)
			{
				const auto y = static_cast<int>(std::floor(v * 100.f));
				return juce::String(100 - y) + " : " + juce::String(y);
			};
			const auto valToStrLRMS = [](float v) { return v > .5f ? juce::String("m/s") : juce::String("l/r"); };
			const auto valToStrFreeSync = [](float v) { return v > .5f ? juce::String("sync") : juce::String("free"); };
			const auto valToStrPolarity = [](float v) { return v > .5f ? juce::String("on") : juce::String("off"); };
			const auto valToStrMs = [](float v) { return juce::String(std::floor(v * 10.f) * .1f) + " " + toString(Unit::Ms); };
			const auto valToStrDb = [](float v) { return juce::String(std::floor(v * 100.f) * .01f) + " " + toString(Unit::Decibel); };
			const auto valToStrEmpty = [](float) { return juce::String(""); };

			const auto strToValPercent = [strToValDivision](const juce::String& txt)
			{
				const auto val = strToValDivision(txt, 0.f);
				if (val != 0.f)
					return val;
				return txt.trimCharactersAtEnd(toString(Unit::Percent)).getFloatValue() * .01f;
			};
			const auto strToValHz = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Hz)).getFloatValue(); };
			const auto strToValBeats = [&bts = beatsData](const juce::String& txt)
			{
				for (auto b = 0; b < bts.size(); ++b)
					if (bts[b].str == txt)
						return static_cast<float>(b);
				return 0.f;
			};
			const auto strToValPhase = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Degree)).getFloatValue(); };
			const auto strToValOct = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Octaves)).getIntValue(); };
			const auto strToValOct2 = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Octaves)).getFloatValue() / 12.f; };
			const auto strToValSemi = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Semi)).getIntValue(); };
			const auto strToValFine = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Fine)).getFloatValue() * .01f; };
			const auto strToValRatio = [strToValDivision](const juce::String& txt)
			{
				const auto val = strToValDivision(txt, -1.f);
				if (val != -1.f)
					return val;
				return juce::jlimit(0.f, 1.f, txt.getFloatValue() * .01f);
			};
			const auto strToValLRMS = [](const juce::String& txt) { return txt[0] == 'l' ? 0.f : 1.f; };
			const auto strToValFreeSync = [](const juce::String& txt) { return txt[0] == 'f' ? 0.f : 1.f; };
			const auto strToValPolarity = [](const juce::String& txt) { return txt[0] == '0' ? 0.f : 1.f; };
			const auto strToValMs = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Ms)).getFloatValue(); };
			const auto strToValDb = [](const juce::String& txt) { return txt.trimCharactersAtEnd(toString(Unit::Decibel)).getFloatValue(); };

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
				params.push_back(new Param(withOffset(PID::Perlin0FreqHz, offset), makeRange::biasXL(.2f, 20.f, -.8f), 6.f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::Perlin0Octaves, offset), makeRange::stepped(1.f, 8.f, 1.f), 4.f, valToStrOct, strToValOct, Unit::Octaves));
				params.push_back(new Param(withOffset(PID::Perlin0Width, offset), makeRange::biasXL(0.f, 1.f, 0), 1.f, valToStrPercent, strToValPercent, Unit::Percent));

				params.push_back(new Param(withOffset(PID::AudioRate0Oct, offset), makeRange::stepped(-3.f * 12.f, 3.f * 12.f, 12.f), 0.f, valToStrOct2, strToValOct2, Unit::Octaves));
				params.push_back(new Param(withOffset(PID::AudioRate0Semi, offset), makeRange::stepped(-12.f, 12.f, 1.f), 0.f, valToStrSemi, strToValSemi, Unit::Semi));
				params.push_back(new Param(withOffset(PID::AudioRate0Fine, offset), makeRange::biasXL(-1.f, 1.f, 0.f), 0.f, valToStrFine, strToValFine, Unit::Fine));
				params.push_back(new Param(withOffset(PID::AudioRate0Width, offset), makeRange::biasXL(-1.f, 1.f, 0.f), 0.f, valToStrPhase, strToValPhase, Unit::Degree));
				params.push_back(new Param(withOffset(PID::AudioRate0RetuneSpeed, offset), makeRange::biasXL(1.f, 2000.f, -.97f), 1.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::AudioRate0Atk, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 20.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::AudioRate0Dcy, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 20.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::AudioRate0Sus, offset), makeRange::biasXL(0.f, 1.f, 0.f), 1.f, valToStrPercent, strToValPercent, Unit::Percent));
				params.push_back(new Param(withOffset(PID::AudioRate0Rls, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 20.f, valToStrMs, strToValMs, Unit::Ms));

				params.push_back(new Param(withOffset(PID::Dropout0Decay, offset), makeRange::biasXL(10.f, 8000.f, -.95f), 1000.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::Dropout0Spin, offset), makeRange::biasXL(.1f, 40.f, -.6f), 4.f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::Dropout0Chance, offset), makeRange::biasXL(10.f, 8000.f, -.95f), 1000.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::Dropout0Smooth, offset), makeRange::biasXL(.01f, 20.f, -.7f), 4.f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::Dropout0Width, offset), makeRange::biasXL(0.f, 1.f, 0.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));

				params.push_back(new Param(withOffset(PID::EnvFol0Attack, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 80.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::EnvFol0Release, offset), makeRange::biasXL(1.f, 2000.f, -.9f), 250.f, valToStrMs, strToValMs, Unit::Ms));
				params.push_back(new Param(withOffset(PID::EnvFol0Gain, offset), makeRange::biasXL(-20.f, 80.f, 0.f), 0.f, valToStrDb, strToValDb, Unit::Decibel));
				params.push_back(new Param(withOffset(PID::EnvFol0Width, offset), makeRange::biasXL(0.f, 1.f, 0.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));

				params.push_back(new Param(withOffset(PID::Macro0, offset), makeRange::biasXL(-1.f, 1.f, 0.f), 0.f, valToStrPercent, strToValPercent, Unit::Percent));
				params.push_back(new Param(withOffset(PID::Pitchbend0Smooth, offset), makeRange::biasXL(1.f, 1000.f, -.97f), 30.f, valToStrMs, strToValMs, Unit::Ms));

				static constexpr float LFOPhaseStep = 5.f / 360.f;

				params.push_back(new Param(withOffset(PID::LFO0FreeSync, offset), makeRange::toggle(), 1.f, valToStrFreeSync, strToValFreeSync, Unit::NumUnits));
				params.push_back(new Param(withOffset(PID::LFO0RateFree, offset), makeRange::biasXL(0.f, 20.f, -.8f), 4.f, valToStrHz, strToValHz, Unit::Hz));
				params.push_back(new Param(withOffset(PID::LFO0RateSync, offset), makeRange::temposync(static_cast<int>(beatsData.size()) - 1), 9.f, valToStrBeats, strToValBeats, Unit::Beats));
				params.push_back(new Param(withOffset(PID::LFO0Waveform, offset), makeRange::biasXL(0.f, 1.f, 0.f), 0.f, valToStrEmpty, strToValPercent, Unit::NumUnits));
				params.push_back(new Param(withOffset(PID::LFO0Phase, offset), makeRange::stepped(-.5f, .5f, LFOPhaseStep), 0.f, valToStrPhase360, strToValPhase, Unit::Degree));
				params.push_back(new Param(withOffset(PID::LFO0Width, offset), makeRange::stepped(0.f, .5f, LFOPhaseStep), 0.f, valToStrPhase360, strToValPhase, Unit::Degree));
			}

			params.push_back(new Param(PID::Depth, makeRange::biasXL(0.f, 1.f, 0.f), .95f, valToStrPercent, strToValPercent));
			params.push_back(new Param(PID::ModsMix, makeRange::biasXL(0.f, 1.f, 0.f), 0.f, valToStrRatio, strToValRatio));
			params.push_back(new Param(PID::DryWetMix, makeRange::biasXL(0.f, 1.f, 0.f), 1.f, valToStrRatio, strToValRatio));
			params.push_back(new Param(PID::WetGain, makeRange::biasXL(-120.f, 4.5f, .9f), 0.f, valToStrDb, strToValDb));
			params.push_back(new Param(PID::StereoConfig, makeRange::toggle(), 1.f, valToStrLRMS, strToValLRMS));

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
				const auto pIdx = connecState.getProperty(ids.param);
				const auto mIdx = connecState.getProperty(ids.mod);
				const auto depth = connecState.getProperty(ids.value);
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

		std::array<Connec, 256> connex;

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
			wannaUpdatePatch(false),
			updatePatch(_updatePatch)
		{
			{
				StateIDs ids;
				state = juce::ValueTree(ids.state);
			}
		}

		void triggerUpdatePatch(const juce::String& xmlString)
		{
			state = state.fromXml(xmlString);
			wannaUpdatePatch.store(true);
		}
		void triggerUpdatePatch(const juce::ValueTree& newState)
		{
			state = newState;
			wannaUpdatePatch.store(true);
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

		// returns false if patch needs to be updated
		bool processBlock(const float**, int numSamples, const juce::AudioPlayHead* playHead)
		{
			if (wannaUpdatePatch.load())
			{
				updatePatch();
				wannaUpdatePatch.store(false);
				return false;
			}
			hasPlayHead.store(playHead != nullptr);
			params.processBlockInit();
			mods.processBlock(numSamples);
			connex.processBlock(params, mods);
			params.processBlockFinish();
			return true;
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

		const BeatsData& getBeatsData() const noexcept { return beatsData; }

		int getModIdx(ModTypeContext mtc) const noexcept {
			for (auto m = 0; m < mods.numMods(); ++m)
				if (mods[m].mtc == mtc)
					return m;
			return -1;
		}
		int getConnecIdxWith(int mIdx, PID pID) const noexcept {
			return connex.getConnecIdxWith(mIdx, getParamIdx(pID));
		}
		int getConnecIdxWith(int mIdx, int pIdx) const noexcept {
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
			const auto param = params[connex[cIdx].getPIdx()];
			connex[cIdx].disable();
			return true;	
		}

		float getConnecDepth(int cIdx) const noexcept { return connex[cIdx].getDepth(); }
		bool setConnecDepth(int cIdx, float depth) noexcept
		{
			const auto param = params[connex[cIdx].getPIdx()];
			connex[cIdx].setDepth(depth);
			return true;
		}

		bool getHasPlayHead() const noexcept { return hasPlayHead.load(); }

		void dbgConnex() { DBG(connex.toString()); }

		juce::ValueTree state;
	protected:
		BeatsData beatsData;
		Params params;
		Mods mods;
		Connex connex;
		std::atomic<bool> hasPlayHead, wannaUpdatePatch;
		std::function<void()> updatePatch;
	};
}

/*

stringToVal functions sometimes make no sense
	stringToVal functions need ability to return current value if input invalid
		how to determine if input is invalid without causing edgecases?

*/
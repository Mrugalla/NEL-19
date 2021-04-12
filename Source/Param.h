#pragma once
#include <JuceHeader.h>

namespace param {
	enum class ID { DepthMax, Depth, Freq, Smooth, LRMS, Width, Mix, SplineMix };

	static juce::String getName(ID i) {
		switch (i) {
		case ID::DepthMax: return "Depth Max";
		case ID::Depth: return "Depth";
		case ID::Freq: return "Freq";
		case ID::Smooth: return "Smooth";
		case ID::LRMS: return "LRMS";
		case ID::Width: return "Width";
		case ID::Mix: return "Mix";
		case ID::SplineMix: return "Spline Mix";
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
			if (value > 9.f) return static_cast<juce::String>(value).substring(0, 2) + " %";
			return static_cast<juce::String>(value).substring(0, 1) + " %";
		};
		auto freqStr = [](float value, int) {
			if (value < 10.f)
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

		const float DepthDefault = .1f;
		const float LFOFreqMin = .01f, LFOFreqMax = 30.f, LFOFreqDefault = 4.f;
		const float WowWidthDefault = 0.f;

		parameters.push_back(createPChoice(ID::DepthMax, depthMaxChoices, 2));
		parameters.push_back(createParameter(
			ID::Depth, util::QuadraticBezierRange(0, 1, .51f), DepthDefault, percentStr));
		parameters.push_back(createParameter(
			ID::Freq, util::QuadraticBezierRange(LFOFreqMin, LFOFreqMax, .01f), LFOFreqDefault, freqStr));
		parameters.push_back(createParameter(
			ID::Smooth, util::QuadraticBezierRange(0, 1, .99f), 1, percentStr));
		parameters.push_back(createPBool(ID::LRMS, true, lrMsStr));
		parameters.push_back(createParameter(
			ID::Width, util::QuadraticBezierRange(0, 1, .3f), WowWidthDefault, percentStr));
		parameters.push_back(createParameter(
			ID::Mix, juce::NormalisableRange<float>(-1.f, 1.f), 1.f, mixStr));
		parameters.push_back(createParameter(
			ID::SplineMix, util::QuadraticBezierRange(0, 1, .51f), .5f, percentStr));

		return { parameters.begin(), parameters.end() };
	}
};
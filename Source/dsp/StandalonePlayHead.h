#pragma once
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_events/juce_events.h"

#define DebugLoop false
#define DebugIsPlaying false
#define DebugBPMChanges false

namespace dsp
{
	using PlayHead = juce::AudioPlayHead;
	using PosInfo = PlayHead::CurrentPositionInfo;
	using Int64 = juce::int64;
	static constexpr double SixtyInv = 1. / 60.;
	
	inline void movePlayHead(PosInfo& posInfo, double sampleRateInv, int numSamples) noexcept
	{
		posInfo.timeInSamples += numSamples;
		auto timeSecs = static_cast<double>(posInfo.timeInSamples) * sampleRateInv;
		posInfo.timeInSeconds = timeSecs;
		posInfo.ppqPosition += static_cast<double>(numSamples) * sampleRateInv * posInfo.bpm * SixtyInv;
	}

	inline void setPlayHead(PosInfo& posInfo, double bpm, double sampleRateInv,
		Int64 timeSamples, bool isPlaying) noexcept
	{
		posInfo.bpm = bpm;
		posInfo.timeInSamples = timeSamples;
		posInfo.timeInSeconds = static_cast<double>(timeSamples) * sampleRateInv;
		posInfo.ppqPosition = posInfo.timeInSeconds * bpm * SixtyInv;
		posInfo.isPlaying = isPlaying;
	}

	inline void copyPlayHead(PosInfo& dest, const PosInfo& src) noexcept
	{
		dest.bpm = src.bpm;
		dest.timeInSamples = src.timeInSamples;
		dest.timeInSeconds = src.timeInSeconds;
		dest.ppqPosition = src.ppqPosition;
		dest.isPlaying = src.isPlaying;
	}

	struct StandalonePlayHead
	{
		static constexpr double DefaultBPM = 90.;
		static constexpr double LoopStart = 1.;
		static constexpr double LoopEnd = 5.;
		static constexpr double LoopRange = LoopEnd - LoopStart;
		static constexpr double IsPlayingLength = 3.5;
		static constexpr bool IsPlayingDefault = true;
		static constexpr double BPMMin = 80.;
		static constexpr double BPMMax = 140.;
		static constexpr double BPMLength = 3.5;
		
		StandalonePlayHead() :
			posInfo(),
			sampleRate(1.),
			sampleRateInv(1.)
#if DebugIsPlaying
			,isPlayingPhase(0.)
#endif
#if DebugBPMChanges
			,bpmPhase(0.)
#endif
		{
			posInfo.bpm = DefaultBPM;
			posInfo.timeSigNumerator = 4;
			posInfo.timeSigDenominator = 4;
			posInfo.timeInSamples = 0;
			posInfo.timeInSeconds = 0.;
			posInfo.ppqPosition = 0.;
			posInfo.isPlaying = IsPlayingDefault;
			posInfo.isLooping = false;
			posInfo.isRecording = false;
		}

		void prepare(double _sampleRate)
		{
			sampleRate = _sampleRate;
			sampleRateInv = 1. / sampleRate;
		}

		void operator()(int numSamples) noexcept
		{
#if DebugIsPlaying
			isPlayingPhase += static_cast<double>(numSamples) * sampleRateInv * posInfo.bpm * SixtyInv;
			auto state = static_cast<int>(std::floor(isPlayingPhase / IsPlayingLength));
			state += IsPlayingDefault ? 1 : 0;
			if (state % 2 == 0)
			{
				posInfo.isPlaying = posInfo.isLooping = posInfo.isRecording = false;
			}
			else
			{
				posInfo.isPlaying = true;
				movePlayHead(posInfo, sampleRateInv, numSamples);
#endif
			movePlayHead(posInfo, sampleRateInv, numSamples);
#if DebugBPMChanges
			bpmPhase += static_cast<double>(numSamples) * sampleRateInv * posInfo.bpm * SixtyInv / BPMLength;
			if(bpmPhase >= 1.)
			{
				--bpmPhase;
				juce::Random rand;
				const auto newBPM = BPMMin + (BPMMax - BPMMin) * rand.nextDouble();
				setPlayHead(posInfo, newBPM, sampleRateInv, posInfo.timeInSamples, posInfo.isPlaying);
			}
#endif
#if DebugIsPlaying
			}
#endif	
#if DebugLoop
			while (posInfo.ppqPosition >= LoopEnd)
			{
				posInfo.ppqPosition -= LoopRange;
				posInfo.timeInSamples = static_cast<Int64>(posInfo.ppqPosition * 60. / posInfo.bpm * sampleRate);
				posInfo.timeInSeconds = static_cast<double>(posInfo.timeInSamples) * sampleRateInv;
			}
#endif
		}

		PosInfo posInfo;
	protected:
		double sampleRate, sampleRateInv;
#if DebugIsPlaying
		double isPlayingPhase;
#endif
#if DebugBPMChanges
		double bpmPhase;
#endif
	};

	inline void synthesizeTransport(PosInfo& transport,
		const juce::AudioPlayHead::PositionInfo& phx) noexcept
	{
		const auto isPlaying = phx.getIsPlaying();
		const auto bpm = phx.getBpm();
		const auto timeInSamples = phx.getTimeInSamples();
		const auto timeSecs = phx.getTimeInSeconds();
		const auto ppqPosition = phx.getPpqPosition();

		transport.isPlaying = isPlaying;
		transport.bpm = bpm.hasValue() ? *bpm : dsp::StandalonePlayHead::DefaultBPM;
		transport.timeInSamples = timeInSamples.hasValue() ? *timeInSamples : 0;
		transport.timeInSeconds = timeSecs.hasValue() ? *timeSecs : 0;
		transport.ppqPosition = ppqPosition.hasValue() ? *ppqPosition : 0;
	}

	inline void synthesizeTransport(PlayHead* playHead,
		StandalonePlayHead& standalonePlayHead, int numSamples) noexcept
	{
		if (juce::JUCEApplicationBase::isStandaloneApp() || playHead == nullptr)
			standalonePlayHead(numSamples);
		else
		{
			const auto phx = playHead->getPosition();
			if (phx.hasValue())
				dsp::synthesizeTransport(standalonePlayHead.posInfo, *phx);
			else
				standalonePlayHead(numSamples);
		}
	}
}

#undef DebugLoop
#undef DebugIsPlaying
#undef DebugBPMChanges
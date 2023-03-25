#pragma once
#include "juce_audio_basics/juce_audio_basics.h"

namespace dsp
{
	struct StandalonePlayHead
	{
		using PlayHead = juce::AudioPlayHead;
		using PosInfo = PlayHead::CurrentPositionInfo;
		
		static constexpr double BPM = 80.;
		static constexpr double sixtyInv = 1. / 60.;

		StandalonePlayHead() :
			posInfo(),
			Fs(1.),
			fsInv(1.)
		{
			posInfo.bpm = BPM;
			posInfo.timeSigNumerator = 4;
			posInfo.timeSigDenominator = 4;
			posInfo.timeInSamples = 0;
			posInfo.timeInSeconds = 0.0;
			posInfo.ppqPosition = 0.0;
			posInfo.isPlaying = true;
			posInfo.isLooping = false;
			posInfo.isRecording = false;
		}

		void prepare(double _Fs)
		{
			Fs = _Fs;
			fsInv = 1. / Fs;
		}

		void operator()(int numSamples) noexcept
		{
			posInfo.timeInSamples += numSamples;
			posInfo.timeInSeconds = posInfo.timeInSamples * fsInv;
			posInfo.ppqPosition += numSamples * fsInv * posInfo.bpm * sixtyInv;
		}

		PosInfo posInfo;
	protected:
		double Fs, fsInv;
	};
}

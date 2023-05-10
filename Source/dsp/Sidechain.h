#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace dsp
{
	struct Sidechain
	{
		using AudioBufferD = juce::AudioBuffer<double>;
		using AudioProcessor = juce::AudioProcessor;
		using Bus = AudioProcessor::Bus;
		using ChannelSet = juce::AudioChannelSet;

		Sidechain() :
			busMain(nullptr),
			busSC(nullptr),
			bufferMain(),
			bufferSC(),
			bufferUpsampled(nullptr),
			bufferMainUpsampled(),
			bufferSCUpsampled(),
			samplesMain(nullptr),
			samplesMainRead(nullptr),
			samplesSC(nullptr),
			samplesSCRead(nullptr),
			samplesMainUpsampled(nullptr),
			samplesMainReadUpsampled(nullptr),
			samplesSCUpsampled(nullptr),
			samplesSCReadUpsampled(nullptr),
			numChannels(0),
			numChannelsSC(0),
			enabled(false)
		{}

		void updateBuffers(AudioProcessor& p, AudioBufferD& buffer, bool standalone) noexcept
		{
			busMain = p.getBus(true, 0);
			bufferMain = busMain->getBusBuffer(buffer);
			numChannels = bufferMain.getNumChannels();
			samplesMain = bufferMain.getArrayOfWritePointers();
			samplesMainRead = bufferMain.getArrayOfReadPointers();
			if (!standalone)
			{
				busSC = p.getBus(true, 1);
				if (busSC != nullptr)
				{
					if (busSC->isEnabled())
					{
						bufferSC = busSC->getBusBuffer(buffer);
						samplesSC = bufferSC.getArrayOfWritePointers();
						samplesSCRead = bufferSC.getArrayOfReadPointers();
						numChannelsSC = bufferSC.getNumChannels();
						enabled = true;
						return;
					}
				}
			}
			bufferSC.setDataToReferTo(samplesMain, numChannels, bufferMain.getNumSamples());
			samplesSC = samplesMain;
			samplesSCRead = samplesMainRead;
			numChannelsSC = numChannels;
			enabled = false;
		}

		void setBufferUpsampled(AudioBufferD* _bufferUpsampled) noexcept
		{
			bufferUpsampled = _bufferUpsampled;
			bufferMainUpsampled = busMain->getBusBuffer(*bufferUpsampled);
			samplesMainUpsampled = bufferMainUpsampled.getArrayOfWritePointers();
			samplesMainReadUpsampled = bufferMainUpsampled.getArrayOfReadPointers();
			if (enabled)
			{
				bufferSCUpsampled = busSC->getBusBuffer(*bufferUpsampled);
				samplesSCUpsampled = bufferSCUpsampled.getArrayOfWritePointers();
				samplesSCReadUpsampled = bufferSCUpsampled.getArrayOfReadPointers();
				return;
			}
			bufferSCUpsampled = bufferMainUpsampled;
			samplesSCUpsampled = samplesMainUpsampled;
			samplesSCReadUpsampled = samplesMainReadUpsampled;
		}

		Bus *busMain, *busSC;
		AudioBufferD bufferMain, bufferSC, *bufferUpsampled, bufferMainUpsampled, bufferSCUpsampled;
		double* const* samplesMain;
		const double* const* samplesMainRead;
		double* const* samplesSC;
		const double* const* samplesSCRead;
		double* const* samplesMainUpsampled;
		const double* const* samplesMainReadUpsampled;
		double* const* samplesSCUpsampled;
		const double* const* samplesSCReadUpsampled;
		int numChannels, numChannelsSC;
		bool enabled;
	};
}
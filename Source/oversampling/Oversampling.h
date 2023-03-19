#pragma once
#include <array>
#include "juce_audio_basics/juce_audio_basics.h"
#include "Filter.h"
#include "ConvolutionFilter.h"
#include "IIRFilter.h"

namespace oversampling
{
	constexpr size_t MaxNumStages = 2;
	static constexpr size_t MaxOrder = 1 << MaxNumStages;
	static constexpr int NumChannels = 2;
	
	using String = juce::String;
	using AudioBuffer = juce::AudioBuffer<float>;

	inline String getID()
	{
		return "oversamplingOrder";
	}

	inline void zeroStuff(float* const* samplesDest, const float* const* samplesSrc, int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto up = samplesDest[ch];
			const auto in = samplesSrc[ch];

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto s2 = s * 2;
				up[s2] = in[s];
				up[s2 + 1] = 0.f;
			}
		}
	}

	inline void decimate(float* const* samples, int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
			for (auto s = 0; s < numSamples; ++s)
				samples[ch][s] = samples[ch][s * 2];
	}

	struct Processor
	{
		Processor() :
			buffer(),
			//
			filterUp4(NumChannels, 176400.f, 22050.f, 44100.f, true), //  17 samples
			filterDown4(NumChannels, 176400.f, 22050.f, 44100.f),
			filterUp2(NumChannels),
			filterDown2(NumChannels),
			//
			FsUp(0.),
			blockSizeUp(0),
			//
			numSamples1x(0), numSamples2x(0), numSamples4x(0),
			enabled(true)
		{
		}

		Processor(Processor& p) :
			buffer(p.buffer),
			filterUp2(p.filterUp2), filterUp4(p.filterUp4),
			filterDown4(p.filterDown4), filterDown2(p.filterDown2),
			FsUp(p.FsUp), blockSizeUp(p.blockSizeUp),
			numSamples1x(0), numSamples2x(0), numSamples4x(0),
			enabled(p.enabled)
		{
		}
		// prepare & params
		void prepareToPlay(const double Fs, const int blockSize, bool _enabled)
		{
			enabled = _enabled;
			if (enabled)
			{
				FsUp = Fs * static_cast<double>(MaxOrder);
				blockSizeUp = blockSize * MaxOrder;
			}
			else
			{
				FsUp = Fs;
				blockSizeUp = blockSize;
			}
			buffer.setSize(NumChannels, blockSize * MaxOrder, false, false, false);
		}
		/* processing methods */
		AudioBuffer& upsample(AudioBuffer& input) noexcept
		{
			if (enabled)
			{
				const auto numChannels = input.getNumChannels();
				numSamples1x = input.getNumSamples();
				numSamples2x = numSamples1x * 2;
				numSamples4x = numSamples1x * 4;

				buffer.setSize(numChannels, numSamples4x, true, false, true);
				const auto samplesIn = input.getArrayOfReadPointers();
				auto samplesUp = buffer.getArrayOfWritePointers();
				
				// 2x
				zeroStuff(samplesUp, samplesIn, numChannels, numSamples1x);
				filterUp2.processBlock(samplesUp, numSamples2x);
				// 4x
				zeroStuff(samplesUp, samplesUp, numChannels, numSamples2x);
				filterUp4.processBlockUp(samplesUp, numSamples4x);
				
				return buffer;
			}
			return input;
		}
		
		void downsample(AudioBuffer& outBuf) noexcept
		{
			auto samplesUp = buffer.getArrayOfWritePointers();
			auto samplesOut = outBuf.getArrayOfWritePointers();
			const auto numChannels = outBuf.getNumChannels();
			// 4x
			filterDown4.processBlockDown(samplesUp, numSamples4x);
			decimate(samplesUp, numChannels, numSamples2x);
			// 2x
			filterDown2.processBlock(samplesUp, numSamples2x);
			decimate(samplesUp, numChannels, numSamples1x);
		}
		
		////////////////////////////////////////
		const double getSampleRateUpsampled() const noexcept
		{
			return FsUp;
		}
		
		const int getBlockSizeUp() const noexcept
		{
			return blockSizeUp;
		}
		
		bool isEnabled() const noexcept
		{
			return enabled;
		}
		
		int getLatency() const noexcept
		{
			const auto mult = enabled ? 1 : 0;
			return mult * (filterUp2.getLatency() + filterDown2.getLatency() + filterUp4.getLatency() + filterDown4.getLatency());
		}
		
	protected:
		AudioBuffer buffer;

		ConvolutionFilter filterUp4, filterDown4;
		LowkeyChebyshevFilter<float> filterUp2, filterDown2;

		double FsUp;
		int blockSizeUp;

		int numSamples1x, numSamples2x, numSamples4x;
		bool enabled;
	};

	struct OversamplerWithShelf
	{
		OversamplerWithShelf() :
			filters(),
			coefficients(),
			cutoff(1.f),
			q(1.f),
			gain(0.f)
		{}

		void prepareToPlay(const double sampleRate, const int _blockSize, bool enabled)
		{
			processor.prepareToPlay(sampleRate, _blockSize, enabled);

			juce::dsp::ProcessSpec spec;
			spec.sampleRate = sampleRate;
			spec.maximumBlockSize = _blockSize;
			spec.numChannels = 2;

			cutoff = 20000.f;
			gain = juce::Decibels::decibelsToGain(14.436f * .5f);
			q = .229f;
			coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, cutoff, q, gain);

			for (auto& filter : filters)
			{
				filter.reset();
				filter.prepare(spec);
				filter.coefficients = coefficients;
			}
		}
		
		/* processing methods */
		AudioBuffer& upsample(juce::AudioBuffer<float>& input) noexcept
		{
			return processor.upsample(input);
		}
		
		void downsample(juce::AudioBuffer<float>& outBuf) noexcept
		{
			processor.downsample(outBuf);
			
			for (auto ch = 0; ch < outBuf.getNumChannels(); ++ch)
			{
				auto& filter = filters[ch];
				float* samples[] = { outBuf.getWritePointer(ch) };
				juce::dsp::AudioBlock<float> block(samples, 1, outBuf.getNumSamples());
				juce::dsp::ProcessContextReplacing<float> context(block);
				filter.process(context);
			}
		}
		
		////////////////////////////////////////
		const double getSampleRateUpsampled() const noexcept { return processor.getSampleRateUpsampled(); }
		const int getBlockSizeUp() const noexcept { return processor.getBlockSizeUp(); }
		
		bool isEnabled() const noexcept { return processor.isEnabled(); }
		
		int getLatency() const noexcept
		{
			return processor.getLatency();
		}
		
		Processor processor;

		std::array<juce::dsp::IIR::Filter<float>, 2> filters;
		juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coefficients;
		float cutoff, q, gain;
	};
}

/*
try other filter types:
	polyphase IIR
	Halfband-Polyphase IIR
	butterworth low pass filter
automatic reaction to different sampleRates

optimisations:
convolution filter simd

*/
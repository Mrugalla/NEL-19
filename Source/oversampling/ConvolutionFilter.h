#pragma once
#include "Filter.h"

namespace oversampling
{
	// http://www.dspguide.com/ch16/1.htm

	struct ImpulseResponse
	{
		ImpulseResponse() :
			data(),
			latency(0)
		{
			data.resize(1, 1.f);
		}
		
		ImpulseResponse(const std::vector<float>& _data) :
			data(_data),
			latency(static_cast<int>(data.size()) / 2)
		{
		}
		
		float operator[](int i) const noexcept { return data[i]; }
		const size_t size() const noexcept { return data.size(); }

		std::vector<float> data;
		int latency;
	};

	/*
	* fc < Nyquist && bw < Nyquist && fc + bw < Nyquist
	*/
	inline ImpulseResponse makeSincFilter2(float Fs, float fc, float bw, bool upsampling)
	{
		const auto nyquist = Fs * .5f;
		if (fc > nyquist || bw > nyquist || fc + bw > nyquist)
		{ // invalid arguments
			std::vector<float> ir;
			ir.resize(1, 1.f);
			return ir;
		}
		fc /= Fs;
		bw /= Fs;
		int M = static_cast<int>(4.f / bw);
		if (M % 2 != 0) M += 1; // M is even number
		const auto MHalf = static_cast<float>(M) * .5f;
		const float MInv = 1.f / static_cast<float>(M);
		const int N = M + 1;
		
		const auto h = [&](float i) // sinc
		{
			i -= MHalf;
			if (i != 0.f)
				return std::sin(tau * fc * i) / i;
			return tau * fc;
		};
		const auto w = [&](float i) // blackman window
		{
			i *= MInv;
			return .42f - .5f * std::cos(tau * i) + .08f * std::cos(tau2 * i);
		};

		std::vector<float> ir;
		ir.reserve(N);
		for (auto n = 0; n < N; ++n)
		{
			auto nF = static_cast<float>(n);
			ir.emplace_back(h(nF) * w(nF));
		}	

		const auto targetGain = upsampling ? 2.f : 1.f;
		auto sum = 0.f; // normalize
		for (const auto n : ir)
			sum += n;
		const auto sumInv = targetGain / sum;
		for (auto& n : ir)
			n *= sumInv;

		return ir;
	}

	struct Convolution
	{
		Convolution(const ImpulseResponse& ir) :
			buffer(),
			wIdx(0)
		{
			buffer.resize(ir.size(), 0.f);
		}

		void processBlock(float* audioBuffer, const ImpulseResponse& ir, const int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				++wIdx;
				if (wIdx == ir.size())
					wIdx = 0;
				buffer[wIdx] = audioBuffer[s];

				auto y = 0.f;
				auto rIdx = wIdx;
				for (auto i = 0; i < ir.size(); ++i)
				{
					y += buffer[rIdx] * ir[i];
					--rIdx;
					if (rIdx == -1)
						rIdx = static_cast<int>(ir.size()) - 1;
				}
				audioBuffer[s] = y;
			}
		}
		
		void processBlockUp(float* audioBuffer, const ImpulseResponse& ir, const int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; s += 2)
			{
				audioBuffer[s] = processSampleUpEven(audioBuffer[s], ir);
				audioBuffer[s + 1] = processSampleUpOdd(ir);
			}
		}
		
		float processSampleUpEven(const float sample, const ImpulseResponse& ir) noexcept
		{
			const auto irSize = static_cast<int>(ir.size());
			buffer[wIdx] = sample;
			auto y = 0.f;
			auto rIdx = wIdx;
			for (auto i = 0; i < irSize; i += 2)
			{
				y += buffer[rIdx] * ir[i];
				rIdx -= 2;
				if (rIdx < 0)
					rIdx += irSize;
			}
			++wIdx;
			if (wIdx == irSize)
				wIdx = 0;
			return y;
		}
		
		float processSampleUpOdd(const ImpulseResponse& ir) noexcept
		{
			const auto irSize = static_cast<int>(ir.size());
			auto y = 0.f;
			auto rIdx = wIdx - 1;
			if (rIdx == -1)
				rIdx = irSize - 1;
			buffer[wIdx] = 0.f;
			for (auto i = 1; i < irSize; i += 2)
			{
				y += buffer[rIdx] * ir[i];
				rIdx -= 2;
				if (rIdx < 0)
					rIdx += irSize;
			}
			++wIdx;
			if (wIdx == irSize)
				wIdx = 0;
			return y;
		}
	protected:
		std::vector<float> buffer;
		int wIdx;
	};

	struct ConvolutionFilter
	{
		ConvolutionFilter(float _Fs = 1.f, float _cutoff = .25f, float _bandwidth = .25f, bool upsampling = false) :
			ir(makeSincFilter2(_Fs, _cutoff, _bandwidth, upsampling)),
			filters{ ir, ir }
		{
		}
		
		int getLatency() const noexcept
		{
			return ir.latency;
		}
		
		void processBlockDown(float* const* audioBuffer, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				filters[ch].processBlock(audioBuffer[ch], ir, numSamples);
		}
		
		void processBlockUp(float* const* audioBuffer, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				filters[ch].processBlockUp(audioBuffer[ch], ir, numSamples);
		}
		
		float processSampleUpEven(const float sample, const int ch) noexcept
		{
			return filters[ch].processSampleUpEven(sample, ir);
		}
		
		float processSampleUpOdd(const int ch) noexcept 
		{
			return filters[ch].processSampleUpOdd(ir);
		}
		
	protected:
		ImpulseResponse ir;
		std::array<Convolution, 2> filters;
	};
}
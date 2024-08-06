#pragma once
#include "Filter.h"

namespace oversampling
{
	// http://www.dspguide.com/ch16/1.htm

	template<typename Float>
	struct ImpulseResponse
	{
		ImpulseResponse() :
			data(),
			latency(0)
		{
			data.resize(1, static_cast<Float>(1));
		}
		
		ImpulseResponse(const std::vector<Float>& _data) :
			data(_data),
			latency(static_cast<int>(data.size()) / 2)
		{
		}
		
		Float operator[](int i) const noexcept { return data[i]; }
		const size_t size() const noexcept { return data.size(); }

		std::vector<Float> data;
		int latency;
	};

	/*
	* fc < Nyquist && bw < Nyquist && fc + bw < Nyquist
	*/
	template<typename Float>
	inline ImpulseResponse<Float> makeSincFilter2(Float Fs, Float fc, Float bw, bool upsampling)
	{
		const auto nyquist = Fs * static_cast<Float>(.5);
		if (fc > nyquist || bw > nyquist || fc + bw > nyquist)
		{ // invalid arguments
			return {};
		}
		fc /= Fs;
		bw /= Fs;
		int M = static_cast<int>(static_cast<Float>(4.) / bw);
		if (M % 2 != 0) M += 1; // M is even number
		const auto MHalf = static_cast<Float>(M) * static_cast<Float>(.5);
		const auto MInv = static_cast<Float>(1) / static_cast<Float>(M);
		const int N = M + 1;
		
		const auto h = [&](Float i) // sinc
		{
			i -= MHalf;
			if (i != 0.f)
				return std::sin(static_cast<Float>(tau) * fc * i) / i;
			return static_cast<Float>(tau) * fc;
		};
		const auto w = [&](Float i) // blackman window
		{
			i *= MInv;
			return static_cast<Float>(.42) -
				static_cast<Float>(.5) * std::cos(static_cast<Float>(tau) * i) +
				static_cast<Float>(.08) * std::cos(static_cast<Float>(tau2) * i);
		};

		std::vector<Float> ir;
		ir.reserve(N);
		for (auto n = 0; n < N; ++n)
		{
			auto nF = static_cast<Float>(n);
			ir.emplace_back(h(nF) * w(nF));
		}	

		const auto targetGain = upsampling ? static_cast<Float>(2) : static_cast<Float>(1);
		auto sum = static_cast<Float>(0); // normalize
		for (const auto n : ir)
			sum += n;
		const auto sumInv = targetGain / sum;
		for (auto& n : ir)
			n *= sumInv;

		return ir;
	}

	template<typename Float>
	struct Convolution
	{
		using IR = ImpulseResponse<Float>;

		Convolution(const IR& ir) :
			buffer(),
			wIdx(0)
		{
			buffer.resize(ir.size(), static_cast<Float>(0));
		}

		void processBlock(Float* audioBuffer, const IR& ir, const int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				++wIdx;
				if (wIdx == ir.size())
					wIdx = 0;
				buffer[wIdx] = audioBuffer[s];

				auto y = 0.;
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
		
		void processBlockUp(Float* audioBuffer, const IR& ir, const int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; s += 2)
			{
				audioBuffer[s] = processSampleUpEven(audioBuffer[s], ir);
				audioBuffer[s + 1] = processSampleUpOdd(ir);
			}
		}
		
		Float processSampleUpEven(const Float sample, const IR& ir) noexcept
		{
			const auto irSize = static_cast<int>(ir.size());
			buffer[wIdx] = sample;
			auto y = 0.;
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
		
		Float processSampleUpOdd(const IR& ir) noexcept
		{
			const auto irSize = static_cast<int>(ir.size());
			auto y = 0.;
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
		std::vector<Float> buffer;
		int wIdx;
	};

	template<typename Float>
	struct ConvolutionFilter
	{
		using Convolver = Convolution<Float>;
        using IR = typename Convolver::IR;
		
		ConvolutionFilter(Float _Fs = static_cast<Float>(1),
			Float _cutoff = static_cast<Float>(.25), Float _bandwidth = static_cast<Float>(.25),
				bool upsampling = false) :
			ir(makeSincFilter2(_Fs, _cutoff, _bandwidth, upsampling)),
			filters{ ir, ir, ir, ir }
		{
		}
		
		int getLatency() const noexcept
		{
			return ir.latency;
		}
		
		void processBlockDown(Float* const* audioBuffer, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				filters[ch].processBlock(audioBuffer[ch], ir, numSamples);
		}
		
		void processBlockUp(Float* const* audioBuffer, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				filters[ch].processBlockUp(audioBuffer[ch], ir, numSamples);
		}
		
		Float processSampleUpEven(const Float sample, const int ch) noexcept
		{
			return filters[ch].processSampleUpEven(sample, ir);
		}
		
		Float processSampleUpOdd(const int ch) noexcept
		{
			return filters[ch].processSampleUpOdd(ir);
		}
		
	protected:
		IR ir;
		std::array<Convolver, 4> filters;
	};
}

#pragma once

namespace midSide
{
	template<typename Float>
	inline void encode(Float* const* samples, int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
		{
			const auto mid = (samples[0][s] + samples[1][s]) * static_cast<Float>(.5);
			const auto side = (samples[0][s] - samples[1][s]) * static_cast<Float>(.5);
			samples[0][s] = mid;
			samples[1][s] = side;
		}
	}
	
	template<typename Float>
	inline void decode(Float* const* samples, int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
		{
			const auto left = samples[0][s] + samples[1][s];
			const auto right = samples[0][s] - samples[1][s];
			samples[0][s] = left;
			samples[1][s] = right;
		}
	}
}

/*

midside encoder before mod matrix or after?

*/
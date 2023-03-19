#pragma once
#include <vector>

namespace dsp
{
	struct WHead
	{
		WHead() :
			buf(),
			wHead(0),
			delaySize(1)
		{}

		void prepare(int blockSize, int _delaySize)
		{
			delaySize = _delaySize;
			if (delaySize != 0)
			{
				wHead = wHead % delaySize;
				buf.resize(blockSize);
			}
		}

		void operator()(int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s, wHead = (wHead + 1) % delaySize)
				buf[s] = wHead;
		}

		int operator[](int i) const noexcept
		{
			return buf[i];
		}

		const int* data() const noexcept
		{
			return buf.data();
		}

		int* data() noexcept
		{
			return buf.data();
		}

		void shift(int shift, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				buf[s] = buf[s] + shift;
				if (buf[s] > delaySize)
					buf[s] -= delaySize;
				else if (buf[s] < 0)
					buf[s] += delaySize;
			}
			wHead = buf[numSamples - 1];
		}
		
		std::vector<int> buf;
		int wHead, delaySize;
	};
}
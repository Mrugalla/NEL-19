#pragma once

namespace oversampling {
	static constexpr float tau = 6.28318530718f;
	static constexpr float pi = tau * .5f;
	static constexpr float tau2 = tau * 2.f;

	struct Filter
	{
		Filter(int _numChannels) :
			numChannels(_numChannels)
		{}
		virtual const int getLatency() const noexcept { return 0; }
		virtual void processBlockDown(float**, const int) = 0;
		virtual void processBlockUp(float**, const int) = 0;
		virtual float processSampleUpEven(const float, const int) = 0;
		virtual float processSampleUpOdd(const int) = 0;
		virtual void addStuff(std::vector<void*>&, int) {}
		int numChannels;
	};
}
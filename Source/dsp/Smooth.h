#pragma once

namespace smooth
{
	// a block-based parameter smoother.
	template<typename Float>
	struct Block
	{
		/* startVal */
		Block(Float = static_cast<Float>(0));

		/* bufferOut, bufferIn, numSamples */
		void operator()(Float*, Float*, int) noexcept;

		/* buffer, val, numSamples */
		void operator()(Float*, Float, int) noexcept;

		/* buffer, numSamples */
		void operator()(Float*, int) noexcept;

		Float curVal;
	};

	template<typename Float>
	struct Lowpass
	{
		static constexpr Float Pi = static_cast<Float>(3.14159265359);
		static constexpr Float Tau = Pi * static_cast<Float>(2);

		/* decay */
		static Float getXFromFc(Float) noexcept;
		/* decay, Fs */
		static Float getXFromHz(Float, Float) noexcept;

		/* decay */
		void makeFromDecayInSamples(Float) noexcept;
		/* decay, Fs */
		void makeFromDecayInSecs(Float, Float) noexcept;
		/* decay */
		void makeFromDecayInFc(Float) noexcept;
		/* decay, Fs */
		void makeFromDecayInHz(Float, Float) noexcept;
		/* decay, Fs */
		void makeFromDecayInMs(Float, Float) noexcept;

		void copyCutoffFrom(const Lowpass<Float>&) noexcept;

		/* startVal */
		Lowpass(const Float = static_cast<Float>(0));

		void reset();

		/* buffer, val, numSamples */
		void operator()(Float*, Float, int) noexcept;
		/* buffer, numSamples */
		void operator()(Float*, int/*numSamples*/) noexcept;
		/* val */
		Float operator()(Float) noexcept;

		void setX(Float) noexcept;

		Float a0, b1, y1, startVal;

		Float processSample(Float) noexcept;
	};

	template<typename Float>
	struct Smooth
	{
		/* smoothLenMs, Fs */
		void makeFromDecayInMs(Float, Float) noexcept;

		/* freqHz, Fs */
		void makeFromFreqInHz(Float, Float) noexcept;

		Smooth(Float /*startVal*/ = static_cast<Float>(0));

		void operator=(Smooth<Float>& other) noexcept
		{
			block.curVal = other.block.curVal;
			lowpass.copyCutoffFrom(other.lowpass);
			cur = other.cur;
			dest = other.dest;
			smoothing = other.smoothing;
		}

		/* bufferOut, bufferIn, numSamples */
		void operator()(Float*, Float*, int) noexcept;

		/* buffer, val, numSamples */
		bool operator()(Float*, Float, int) noexcept;

		/* buffer, numSamples */
		bool operator()(Float*, int) noexcept;

		/* value (this method is not for parameters!) */
		Float operator()(Float) noexcept;

	protected:
		Block<Float> block;
		Lowpass<Float> lowpass;
		Float cur, dest;
		bool smoothing;
	};
}
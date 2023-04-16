#pragma once

namespace dsp
{
	template<typename Float>
	struct PhaseInfo
	{
		PhaseInfo(Float _phase = static_cast<Float>(0), bool _retrig = static_cast<Float>(false)) :
			phase(_phase),
			retrig(_retrig)
		{}

		Float phase;
		bool retrig;
	};

	template<typename Float>
	struct Phasor
	{
		using Phase = PhaseInfo<Float>;

		void setFrequencyHz(Float hz) noexcept
		{
			inc = hz * fsInv;
		}

		/* phase, inc */
		Phasor(Float _phase = static_cast<Float>(0), Float _inc = static_cast<Float>(0)) :
			phase(_phase, false),
			inc(_inc),
			fsInv(static_cast<Float>(1))
		{

		}

		/* fsInv */
		void prepare(Float _fsInv) noexcept
		{
			fsInv = _fsInv;
		}

		void reset(Float p = static_cast<Float>(0)) noexcept
		{
			phase.phase = p;
		}

		Phase operator()() noexcept
		{
			phase.phase += inc;
			if (phase.phase >= static_cast<Float>(1))
			{
				--phase.phase;
				phase.retrig = true;
				return phase;
			}
			phase.retrig = false;
			return phase;
		}

		Phase phase;
		Float inc, fsInv;
	};
}
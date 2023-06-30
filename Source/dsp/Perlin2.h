#pragma once
#include <array>
#include <random>
#include "../Interpolation.h"
#include "PRM.h"
#include "Phasor.h"
#include "XFade.h"
#include "../Approx.h"

#include <juce_audio_basics/juce_audio_basics.h>

#define oopsie(x) jassert(!(x))

namespace perlin2
{
	static constexpr double Pi = 3.1415926535897932384626433832795;
	using PRMInfo = dsp::PRMInfoD;
	using PRM = dsp::PRMD;
	using PhasorD = dsp::Phasor<double>;

	template<typename Float>
	inline Float msInSamples(Float ms, Float Fs) noexcept
	{
		return ms * Fs * static_cast<Float>(0.001);
	}

	template<typename Float>
	inline Float msInInc(Float ms, Float Fs) noexcept
	{
		return static_cast<Float>(1) / msInSamples(ms, Fs);
	}

	inline double applyBias(double x, double bias) noexcept
	{
		if (bias == 0.)
			return x;
		const auto X = 2. * x * x * x * x * x;
		const auto Y = X * X * X;
		return x + bias * (std::tanh(Y) - x);
	}

	inline void applyBias(double* smpls, double bias, int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
			smpls[s] = applyBias(smpls[s], bias);
	}

	inline void applyBias(double* const* samples, double bias, int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
			applyBias(samples[ch], bias, numSamples);
	}

	inline void applyBias(double* smpls, const double* bias, int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
			smpls[s] = applyBias(smpls[s], bias[s]);
	}

	inline void applyBias(double* const* samples, const double* bias, int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
			applyBias(samples[ch], bias, numSamples);
	}

	inline void generateProceduralNoise(double* noise, int size, unsigned int seed)
	{
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_real_distribution<float> dist(-.8f, .8f); // compensate spline overshoot

		for (auto s = 0; s < size; ++s, ++seed)
		{
			mt.seed(seed);
			noise[s] = dist(mt);
		}
	}

	inline double getInterpolatedNN(const double* noise, double phase) noexcept
	{
		return noise[static_cast<int>(std::round(phase)) + 1];
	}

	inline double getInterpolatedLerp(const double* noise, double phase) noexcept
	{
		return interpolation::lerp(noise, phase + 1.5);
	}

	inline double getInterpolatedSpline(const double* noise, double phase) noexcept
	{
		return interpolation::cubicHermiteSpline(noise, phase);
	}

	using PlayHeadPos = juce::AudioPlayHead::CurrentPositionInfo;
	using InterpolationFunc = double(*)(const double*, double) noexcept;
	using InterpolationFuncs = std::array<InterpolationFunc, 3>;
	using SIMD = juce::FloatVectorOperations;

	struct Perlin
	{
		enum class Shape
		{
			NN, Lerp, Spline, NumShapes
		};

		static constexpr int NumOctaves = 7;
		static constexpr int NoiseOvershoot = 4;

		static constexpr int NoiseSize = 1 << NumOctaves;
		static constexpr int NoiseSizeMax = NoiseSize - 1;

		using NoiseArray = std::array<double, NoiseSize + NoiseOvershoot>;
		using GainBuffer = std::array<double, NumOctaves + 2>;

		Perlin() :
			// misc
			interpolationFuncs{ &getInterpolatedNN, &getInterpolatedLerp, &getInterpolatedSpline },
			sampleRateInv(1), sampleRate(1.),
			// phase
			phasor(),
			phaseBuffer(),
			noiseIdx(0)
		{
		}

		/* sampleRate, blockSize */
		void prepare(double _sampleRate, int blockSize)
		{
			sampleRate = _sampleRate;
			sampleRateInv = 1. / sampleRate;
			phaseBuffer.resize(blockSize);
		}

		/* newPhase */
		void updatePosition(double newPhase) noexcept
		{
			const auto newPhaseFloor = std::floor(newPhase);
			noiseIdx = static_cast<int>(newPhaseFloor) & NoiseSizeMax;
			phasor.phase.phase = newPhase - newPhaseFloor;
		}
		
		/* rateHzInv */
		void updateSpeed(double rateHzInv) noexcept
		{
			phasor.inc = rateHzInv;
		}

		/* samples, noise, gainBuffer,
		octavesInfo, phsInfo, widthInfo,
		shape, numChannels, numSamples */
		void operator()(double* const* samples, const double* noise, const double* gainBuffer,
			const PRMInfo& octavesInfo, const PRMInfo& phsInfo, const PRMInfo& widthInfo,
			Shape shape, int numChannels, int numSamples) noexcept
		{
			synthesizePhasor(phsInfo, numSamples);

			processOctaves(samples[0], octavesInfo, noise, gainBuffer, shape, numSamples);

			if (numChannels == 2)
				processWidth(samples, octavesInfo, widthInfo, noise, gainBuffer, shape, numSamples);
		}

		// misc
		InterpolationFuncs interpolationFuncs;
		double sampleRateInv, sampleRate;

		// phase
		PhasorD phasor;
		std::vector<double> phaseBuffer;
		int noiseIdx;

	protected:
		/* phsInfo, numSamples */
		void synthesizePhasor(const PRMInfo& phsInfo, int numSamples) noexcept
		{
			if (!phsInfo.smoothing)
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto phaseInfo = phasor();
					if (phaseInfo.retrig)
						noiseIdx = (noiseIdx + 1) & NoiseSizeMax;

					phaseBuffer[s] = phaseInfo.phase + phsInfo.val + static_cast<double>(noiseIdx);
				}
			else
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto phaseInfo = phasor();
					if (phaseInfo.retrig)
						noiseIdx = (noiseIdx + 1) & NoiseSizeMax;

					phaseBuffer[s] = phaseInfo.phase + phsInfo[s] + static_cast<double>(noiseIdx);
				}
		}

		double getInterpolatedSample(const double* noise,
			double phase, Shape shape) const noexcept
		{
			const auto smpl0 = interpolationFuncs[static_cast<int>(shape)](noise, phase);
			return smpl0;
		}

		/* smpls, octavesInfo, noise, gainBuffer, shape, numSamples */
		void processOctaves(double* smpls, const PRMInfo& octavesInfo,
			const double* noise, const double* gainBuffer, Shape shape, int numSamples) noexcept
		{
			if (!octavesInfo.smoothing)
				processOctavesNotSmoothing(smpls, noise, gainBuffer, octavesInfo.val, shape, numSamples);
			else
				processOctavesSmoothing(smpls, octavesInfo.buf, noise, gainBuffer, shape, numSamples);
		}

		/* smpls, noise, gainBuffer, octaves, shape, numSamples */
		void processOctavesNotSmoothing(double* smpls, const double* noise, const double* gainBuffer, double octaves,
			Shape shape, int numSamples) noexcept
		{
			const auto octFloor = std::floor(octaves);

			for (auto s = 0; s < numSamples; ++s)
			{
				auto sample = 0.;
				for (auto o = 0; o < octFloor; ++o)
				{
					const auto phase = getPhaseOctaved(phaseBuffer[s], o);
					const auto smpl = getInterpolatedSample(noise, phase, shape);
					sample += smpl * gainBuffer[o];
				}

				smpls[s] = sample;
			}

			auto gain = 0.;
			for (auto o = 0; o < octFloor; ++o)
				gain += gainBuffer[o];

			const auto octFrac = octaves - octFloor;
			if (octFrac != 0.)
			{
				const auto octFloorInt = static_cast<int>(octFloor);

				for (auto s = 0; s < numSamples; ++s)
				{
					const auto phase = getPhaseOctaved(phaseBuffer[s], octFloorInt);
					const auto smpl = getInterpolatedSample(noise, phase, shape);
					smpls[s] += octFrac * smpl * gainBuffer[octFloorInt];;
				}

				gain += octFrac * gainBuffer[octFloorInt];
			}

			SIMD::multiply(smpls, 1. / std::sqrt(gain), numSamples);
		}

		/* smpls, octavesBuf, noise, gainBuffer, shape, numSamples */
		void processOctavesSmoothing(double* smpls, const double* octavesBuf,
			const double* noise, const double* gainBuffer,
			Shape shape, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto octFloor = std::floor(octavesBuf[s]);

				auto sample = 0.;
				for (auto o = 0; o < octFloor; ++o)
				{
					const auto phase = getPhaseOctaved(phaseBuffer[s], o);
					const auto smpl = getInterpolatedSample(noise, phase, shape);
					sample += smpl * gainBuffer[o];
				}

				smpls[s] = sample;

				auto gain = 0.;
				for (auto o = 0; o < octFloor; ++o)
					gain += gainBuffer[o];

				const auto octFrac = octavesBuf[s] - octFloor;
				if (octFrac != 0.)
				{
					const auto octFloorInt = static_cast<int>(octFloor);

					const auto phase = getPhaseOctaved(phaseBuffer[s], octFloorInt);
					const auto smpl = getInterpolatedSample(noise, phase, shape);
					smpls[s] += octFrac * smpl * gainBuffer[octFloorInt];

					gain += octFrac * gainBuffer[octFloorInt];
				}

				smpls[s] /= std::sqrt(gain);
			}
		}

		/* samples, octavesInfo, widthInfo, noise, gainBuffer, shape, numSamples */
		void processWidth(double* const* samples, const PRMInfo& octavesInfo,
			const PRMInfo& widthInfo, const double* noise, const double* gainBuffer,
			Shape shape, int numSamples) noexcept
		{
			if (!widthInfo.smoothing)
				if (widthInfo.val == 0.)
					return SIMD::copy(samples[1], samples[0], numSamples);
				else
					SIMD::add(phaseBuffer.data(), widthInfo.val, numSamples);
			else
				SIMD::add(phaseBuffer.data(), widthInfo.buf, numSamples);

			processOctaves(samples[1], octavesInfo, noise, gainBuffer, shape, numSamples);
		}

		double getPhaseOctaved(double phaseInfo, int o) const noexcept
		{
			const auto ox2 = 1 << o;
			const auto oPhase = phaseInfo * static_cast<double>(ox2);
			const auto oPhaseFloor = std::floor(oPhase);
			const auto oPhaseInt = static_cast<int>(oPhaseFloor) & NoiseSizeMax;
			return oPhase - oPhaseFloor + static_cast<double>(oPhaseInt);
		}

		// debug:
#if JUCE_DEBUG
		void discontinuityJassert(double* smpls, int numSamples, double threshold = .1)
		{
			auto lastSample = smpls[0];
			for (auto s = 1; s < numSamples; ++s)
			{
				auto curSample = smpls[s];
				oopsie(abs(curSample - lastSample) > threshold);
				lastSample = curSample;
			}
		}

		void controlRange(double* const* samples, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
				{
					oopsie(std::abs(samples[ch][s]) >= 1.);
					oopsie(std::abs(samples[ch][s]) <= -1.);
				}
		}
#endif
	};

	using AudioBuffer = juce::AudioBuffer<double>;
	using Shape = Perlin::Shape;
	
	static constexpr double XFadeLengthMs = 200.;
	static constexpr int NumPerlins = 3;
	using Mixer = dsp::XFadeMixer<NumPerlins, true>;
	using Perlins = std::array<Perlin, NumPerlins>;
	using Int64 = juce::int64;

	struct Perlin2
	{
		using Int64 = juce::int64;

		Perlin2() :
			mixer(),
			// misc
			sampleRateInv(1.),
			// perlin / noise
			noise(),
			gainBuffer(),
			perlins(),
			// parameters
			octavesPRM(1.),
			widthPRM(0.),
			phsPRM(0.),
			rateBeats(-1.),
			rateHz(-1.),
			inc(1.),
			bpm(1.), bps(1.),
			// noise seed
			seed(),
			// project position
			posEstimate(-1),
			oversamplingFactor(1),
			latency(0)
		{
			juce::Random rand;
			setSeed(rand.nextInt());

			for (auto s = 0; s < Perlin::NoiseOvershoot; ++s)
				noise[Perlin::NoiseSize + s] = noise[s];

			for (auto o = 0; o < gainBuffer.size(); ++o)
				gainBuffer[o] = 1. / static_cast<double>(1 << o);
		}

		void setSeed(int _seed)
		{
			seed.store(_seed);
			generateProceduralNoise(noise.data(), Perlin::NoiseSize, static_cast<unsigned int>(_seed));
		}

		void prepare(double fs, int blockSize, int _latency, int _oversamplingFactor)
		{
			latency = _latency;

			oversamplingFactor = _oversamplingFactor;
			sampleRateInv = 1. / fs;

			mixer.prepare(fs, XFadeLengthMs, blockSize);
			for (auto& perlin : perlins)
				perlin.prepare(fs, blockSize);
			octavesPRM.prepare(fs, blockSize, 10.);
			widthPRM.prepare(fs, blockSize, 20.);
			phsPRM.prepare(fs, blockSize, 20.);
		}

		/* samples, numChannels, numSamples, playHeadPos,
		rateHz, rateBeats, octaves, width, phs, bias[0,1]
		shape, temposync */
		void operator()(double* const* samples, int numChannels, int numSamples,
			const PlayHeadPos& transport,
			double _rateHz, double _rateBeats,
			double octaves, double width, double phs, double bias,
			Shape shape, bool temposync) noexcept
		{
			const auto octavesInfo = octavesPRM(octaves, numSamples);
			const auto phsInfo = phsPRM(phs, numSamples);
			const auto widthInfo = widthPRM(width, numSamples);
			
			updatePerlin(transport, _rateBeats, _rateHz, numSamples, temposync);
			
			{
				auto& track = mixer[0];

				if (track.isEnabled())
				{
					auto xSamples = mixer.getSamples(0);
					track.synthesizeGainValues(xSamples[2], numSamples);

					perlins[0]
					(
						samples,
						noise.data(),
						gainBuffer.data(),
						octavesInfo,
						phsInfo,
						widthInfo,
						shape,
						numChannels,
						numSamples
					);

					track.copy(samples, xSamples, numChannels, numSamples);
				}
				else
					for (auto ch = 0; ch < numChannels; ++ch)
						SIMD::clear(samples[ch], numSamples);
				
			}
			
			for (auto i = 1; i < NumPerlins; ++i)
			{
				auto& track = mixer[i];

				if (track.isEnabled())
				{
					auto xSamples = mixer.getSamples(i);
					track.synthesizeGainValues(xSamples[2], numSamples);

					perlins[i]
					(
						samples,
						noise.data(),
						gainBuffer.data(),
						octavesInfo,
						phsInfo,
						widthInfo,
						shape,
						numChannels,
						numSamples
					);

					track.add(samples, xSamples, numChannels, numSamples);
				}
			}
			
			processBias(samples, bias, numChannels, numSamples);
		}

		Mixer mixer;
		// misc
		double sampleRateInv;
		// noise
		Perlin::NoiseArray noise;
		Perlin::GainBuffer gainBuffer;
		Perlins perlins;
		// parameters
		PRM octavesPRM, widthPRM, phsPRM;
		double rateBeats, rateHz;
		double inc, bpm, bps, rateInv;
		// seed
		std::atomic<int> seed;
		// project position
		Int64 posEstimate;
		int oversamplingFactor, latency;

		void updatePerlin(const PlayHeadPos& transport,
			double _rateBeats, double _rateHz, int numSamples, bool temposync) noexcept
		{
			updateSpeed(transport.bpm, _rateHz, _rateBeats, transport.timeInSamples, temposync);

			if (transport.isPlaying)
			{
				//updatePosition(perlins[mixer.idx], transport.ppqPosition, transport.timeInSeconds, temposync);
				posEstimate = transport.timeInSamples + numSamples / oversamplingFactor;
			}
			else
				posEstimate = transport.timeInSamples;
		}

		void updateSpeed(double nBpm, double _rateHz, double _rateBeats, Int64 timeInSamples, bool temposync) noexcept
		{
			double nBps = nBpm / 60.;
			const auto nRateInv = .25 / _rateBeats;

			double nInc = 0.;
			if (temposync)
			{
				const auto bpSamples = nBps * sampleRateInv;
				nInc = nRateInv * bpSamples;
			}
			else
				nInc = _rateHz * sampleRateInv;

			if (isLooping(timeInSamples) || (changesSpeed(nBpm, nInc) && !mixer.stillFading()))
				initXFade(nInc, nBpm, nBps, nRateInv, _rateHz, _rateBeats);
		}

		void updatePosition(Perlin& perlin, double ppqPosition, double timeInSecs, bool temposync) noexcept
		{
			if (temposync)
			{
				const auto latencyInPPQ = latency * bps * sampleRateInv;
				const auto ppq = ppqPosition - latencyInPPQ;
				const auto nPhase = ppq * rateInv + .5;
				perlin.updatePosition(nPhase);
			}
			else
			{
				const auto nPhase = timeInSecs * rateHz;
				perlin.updatePosition(nPhase);
			}
		}

		// CROSSFADE FUNCS
		bool isLooping(Int64 timeInSamples) noexcept
		{
			const auto error = std::abs(timeInSamples - posEstimate);
			return error > 1;
		}

		const bool keepsSpeed(double nBpm, double nInc) const noexcept
		{
			return nInc == inc && bpm == nBpm;
		}

		const bool changesSpeed(double nBpm, double nInc) const noexcept
		{
			return !keepsSpeed(nBpm, nInc);
		}

		void initXFade(double nInc, double nBpm, double nBps,
			double nRateInv, double _rateHz, double _rateBeats) noexcept
		{
			inc = nInc;
			bpm = nBpm;
			bps = nBps;
			rateInv = nRateInv;
			rateHz = _rateHz;
			rateBeats = _rateBeats;
			mixer.init();
			perlins[mixer.idx].updateSpeed(inc);
		}

		void processBias(double* const* samples, double bias,
			int numChannels, int numSamples) noexcept
		{
			if (bias != 0.)
				for (auto ch = 0; ch < numChannels; ++ch)
					for (auto s = 0; s < numSamples; ++s)
					{
						auto x = samples[ch][s];
						auto y = 1.;
						if (x != 0.)
						{
							auto X = 8. * x * Pi;
							auto A = 1. - abs(x);
							y = approx::sin(X) * approx::sin(X) / X;
							y = 1.5 * A * A * y;
						}

						samples[ch][s] = x + bias * (y - x);
					}
		}
	};
}
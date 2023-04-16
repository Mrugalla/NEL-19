#pragma once
#include <array>
#include <random>
#include "../Interpolation.h"
#include "PRM.h"
#include "Phasor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#define oopsie(x) jassert(!(x))

namespace perlin
{
	static constexpr float Pi = 3.14159265359f;
	using PRMInfo = dsp::PRMInfo;
	using PRM = dsp::PRM;
	using PhasorF = dsp::Phasor<float>;
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

	inline void generateProceduralNoise(float* noise, int size, unsigned int seed)
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

	inline float getInterpolatedNN(const float* noise, float phase) noexcept
	{
		return noise[static_cast<int>(std::round(phase)) + 1];
	}

	inline float getInterpolatedLerp(const float* noise, float phase) noexcept
	{
		return interpolation::lerp(noise, phase + 1.5f);
	}

	inline float getInterpolatedSpline(const float* noise, float phase) noexcept
	{
		return interpolation::cubicHermiteSpline(noise, phase);
	}

	using PlayHeadPos = juce::AudioPlayHead::CurrentPositionInfo;
	using InterpolationFunc = float(*)(const float*, float) noexcept;
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

		using NoiseArray = std::array<float, NoiseSize + NoiseOvershoot>;
		using GainBuffer = std::array<float, NumOctaves + 2>;

		Perlin() :
			// misc
			interpolationFuncs{ &getInterpolatedNN, &getInterpolatedLerp, &getInterpolatedSpline },
			sampleRateInv(1.),
			fs(1.f),
			// phase
			phasor(),
			phaseBuffer(),
			noiseIdx(0)
		{
		}

		/* sampleRate, blockSize */
		void prepare(float _sampleRate, int blockSize)
		{
			fs = _sampleRate;
			const auto fsInv = 1.f / fs;
			sampleRateInv = static_cast<double>(fsInv);
			phaseBuffer.resize(blockSize);
		}

		/* newPhase */
		void updatePosition(double newPhase) noexcept
		{
			const auto newPhaseFloor = std::floor(newPhase);

			noiseIdx = static_cast<int>(newPhaseFloor) & NoiseSizeMax;
			phasor.phase.phase = newPhase - newPhaseFloor;
		}

		/* playHeadPos, rateBeatsInv */
		void updatePositionSyncProcedural(double ppq, double rateBeatsInv) noexcept
		{
			ppq = ppq * rateBeatsInv + .5;
			updatePosition(ppq);
		}

		/* rateHzInv */
		void updateSpeed(double rateHzInv) noexcept
		{
			phasor.inc = rateHzInv;
		}

		/*  playHeadPos, rateHz */
		void updatePosition(const PlayHeadPos& playHeadPos, double rateHz) noexcept
		{
			const auto timeSecs = playHeadPos.timeInSeconds;
			const auto timeHz = timeSecs * rateHz;
			updatePosition(timeHz);
		}
		
		/* samples, noise, gainBuffer,
		octavesInfo, phsInfo, widthInfo, shape,
		numChannels, numSamples */
		void operator()(float* const* samples, const float* noise, const float* gainBuffer,
			const PRMInfo& octavesInfo, const PRMInfo& phsInfo, const PRMInfo& widthInfo, Shape shape,
			int numChannels, int numSamples) noexcept
		{
			synthesizePhasor(phsInfo, numSamples);

			processOctaves(samples[0], octavesInfo, noise, gainBuffer, shape, numSamples);

			if (numChannels == 2)
				processWidth(samples, octavesInfo, widthInfo, noise, gainBuffer, shape, numSamples);
		}

		// misc
		InterpolationFuncs interpolationFuncs;
		double sampleRateInv;
		float fs;

		// phase
		PhasorD phasor;
		std::vector<float> phaseBuffer;
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

					phaseBuffer[s] = static_cast<float>(phaseInfo.phase) + phsInfo.val + static_cast<float>(noiseIdx);
				}
			else
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto phaseInfo = phasor();
					if (phaseInfo.retrig)
						noiseIdx = (noiseIdx + 1) & NoiseSizeMax;

					phaseBuffer[s] = static_cast<float>(phaseInfo.phase) + phsInfo[s] + static_cast<float>(noiseIdx);
				}
		}

		/* smpls, octavesInfo, noise, gainBuffer, shape, numSamples */
		void processOctaves(float* smpls, const PRMInfo& octavesInfo,
			const float* noise, const float* gainBuffer, Shape shape, int numSamples) noexcept
		{
			if (!octavesInfo.smoothing)
				processOctavesNotSmoothing(smpls, noise, gainBuffer, octavesInfo.val, shape, numSamples);
			else
				processOctavesSmoothing(smpls, octavesInfo.buf, noise, gainBuffer, shape, numSamples);
		}

		float getInterpolatedSample(const float* noise, float phase, Shape shape) noexcept
		{
			return interpolationFuncs[static_cast<int>(shape)](noise, phase);
		}

		/* smpls, noise, gainBuffer, octaves, shape, numSamples */
		void processOctavesNotSmoothing(float* smpls, const float* noise,
			const float* gainBuffer, float octaves, Shape shape, int numSamples) noexcept
		{
			const auto octFloor = std::floor(octaves);

			for (auto s = 0; s < numSamples; ++s)
			{
				auto sample = 0.f;
				for (auto o = 0; o < octFloor; ++o)
				{
					const auto phase = getPhaseOctaved(phaseBuffer[s], o);
					const auto smpl = getInterpolatedSample(noise, phase, shape);
					sample += smpl * gainBuffer[o];
				}

				smpls[s] = sample;
			}

			auto gain = 0.f;
			for (auto o = 0; o < octFloor; ++o)
				gain += gainBuffer[o];

			const auto octFrac = octaves - octFloor;
			if (octFrac != 0.f)
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

			SIMD::multiply(smpls, 1.f / std::sqrt(gain), numSamples);
		}

		/* smpls, octavesBuf, noise, gainBuffer, shape, numSamples */
		void processOctavesSmoothing(float* smpls, const float* octavesBuf,
			const float* noise, const float* gainBuffer, Shape shape, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto octFloor = std::floor(octavesBuf[s]);

				auto sample = 0.f;
				for (auto o = 0; o < octFloor; ++o)
				{
					const auto phase = getPhaseOctaved(phaseBuffer[s], o);
					const auto smpl = getInterpolatedSample(noise, phase, shape);
					sample += smpl * gainBuffer[o];
				}

				smpls[s] = sample;

				auto gain = 0.f;
				for (auto o = 0; o < octFloor; ++o)
					gain += gainBuffer[o];

				const auto octFrac = octavesBuf[s] - octFloor;
				if (octFrac != 0.f)
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
		void processWidth(float* const* samples, const PRMInfo& octavesInfo,
			const PRMInfo& widthInfo, const float* noise, const float* gainBuffer,
			Shape shape, int numSamples) noexcept
		{
			if (!widthInfo.smoothing)
				if (widthInfo.val == 0.f)
					return SIMD::copy(samples[1], samples[0], numSamples);
				else
					SIMD::add(phaseBuffer.data(), widthInfo.val, numSamples);
			else
				SIMD::add(phaseBuffer.data(), widthInfo.buf, numSamples);

			processOctaves(samples[1], octavesInfo, noise, gainBuffer, shape, numSamples);
		}

		float getPhaseOctaved(float phaseInfo, int o) const noexcept
		{
			const auto ox2 = 1 << o;
			const auto oPhase = phaseInfo * static_cast<float>(ox2);
			const auto oPhaseFloor = std::floor(oPhase);
			const auto oPhaseInt = static_cast<int>(oPhaseFloor) & NoiseSizeMax;
			return oPhase - oPhaseFloor + static_cast<float>(oPhaseInt);
		}

		// debug:
#if JUCE_DEBUG
		void discontinuityJassert(float* smpls, int numSamples, float threshold = .1f)
		{
			auto lastSample = smpls[0];
			for (auto s = 1; s < numSamples; ++s)
			{
				auto curSample = smpls[s];
				oopsie(abs(curSample - lastSample) > threshold);
				lastSample = curSample;
			}
		}

		void controlRange(float* const* samples, int numChannels, int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				for (auto s = 0; s < numSamples; ++s)
				{
					oopsie(std::abs(samples[ch][s]) >= 1.f);
					oopsie(std::abs(samples[ch][s]) <= -1.f);
				}
		}
#endif
	};

	using AudioBuffer = juce::AudioBuffer<float>;
	using Shape = Perlin::Shape;

	struct Perlin2
	{
		Perlin2() :
			// misc
			sampleRateInv(1.),
			// noise
			noise(),
			gainBuffer(),
			// perlin
			prevBuffer(),
			perlins(),
			perlinIndex(0),
			// parameters
			octavesPRM(1.f),
			widthPRM(0.f),
			phsPRM(0.f),
			rateBeats(-1.),
			rateHz(-1.),
			rateInv(0.),
			// crossfade
			xFadeBuffer(),
			xPhase(0.f),
			xInc(0.f),
			crossfading(false),
			lastBlockWasTemposync(false),
			seed(),
			// project position
			curPosEstimate(-1),
			curPosInSamples(0),
			latency(0)
		{
			setSeed(69420);

			for (auto s = 0; s < Perlin::NoiseOvershoot; ++s)
				noise[Perlin::NoiseSize + s] = noise[s];

			for (auto o = 0; o < gainBuffer.size(); ++o)
				gainBuffer[o] = 1.f / static_cast<float>(1 << o);
		}

		void setSeed(int _seed)
		{
			seed.store(_seed);
			generateProceduralNoise(noise.data(), Perlin::NoiseSize, static_cast<unsigned int>(_seed));
		}

		void prepare(float fs, int blockSize, int _latency)
		{
			latency = _latency;

			sampleRateInv = 1. / static_cast<double>(fs);

			prevBuffer.setSize(2, blockSize, false, false, false);
			for (auto& perlin : perlins)
				perlin.prepare(fs, blockSize);
			xInc = msInInc(420.f, fs);
			xFadeBuffer.resize(blockSize);
			octavesPRM.prepare(fs, blockSize, 10.f);
			widthPRM.prepare(fs, blockSize, 20.f);
			phsPRM.prepare(fs, blockSize, 20.f);
		}

		/* samples, numChannels, numSamples, playHeadPos,
		rateHz, rateBeats, octaves, width, phs,
		shape, temposync, procedural */
		void operator()(float* const* samples, int numChannels, int numSamples,
			const PlayHeadPos& playHeadPos,
			double _rateHz, double _rateBeats,
			float octaves, float width, float phs,
			Shape shape, bool temposync, bool procedural) noexcept
		{
			if (temposync)
				processSync(playHeadPos, numSamples, _rateBeats, procedural);
			else
				processFree(playHeadPos, numSamples, _rateHz, procedural);
			
			lastBlockWasTemposync = temposync;

			const auto octavesInfo = octavesPRM(octaves, numSamples);
			const auto phsInfo = phsPRM(phs, numSamples);
			const auto widthInfo = widthPRM(width, numSamples);

			perlins[perlinIndex]
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

			processCrossfade
			(
				samples,
				octavesInfo,
				phsInfo,
				widthInfo,
				shape,
				numChannels,
				numSamples
			);
		}

		// misc
		double sampleRateInv;
		// noise
		Perlin::NoiseArray noise;
		Perlin::GainBuffer gainBuffer;
		// perlin
		AudioBuffer prevBuffer;
		std::array<Perlin, 2> perlins;
		int perlinIndex;
		// parameters
		PRM octavesPRM, widthPRM, phsPRM;
		double rateBeats, rateHz;
		double rateInv;
		// crossfade
		std::vector<float> xFadeBuffer;
		float xPhase, xInc;
		bool crossfading, lastBlockWasTemposync;
		// seed
		std::atomic<int> seed;
		// project position
		__int64 curPosEstimate, curPosInSamples;
		int latency;

		// PROCESS FREE
		void processFree(const PlayHeadPos& playHeadPos, int numSamples, double _rateHz, bool procedural) noexcept
		{
			if (procedural && playHeadPos.isPlaying)
				processFreeProcedural(playHeadPos, _rateHz, numSamples);
			else
				processFreeRandom(_rateHz);
		}

		void processFreeRandom(double _rateHz) noexcept
		{
			rateHz = _rateHz;
			rateInv = _rateHz * sampleRateInv;

			perlins[perlinIndex].updateSpeed(rateInv);
		}

		void processFreeProcedural(const PlayHeadPos& playHeadPos, double _rateHz, int numSamples) noexcept
		{
			curPosInSamples = playHeadPos.timeInSamples - latency;

			if (!crossfading)
			{
				if (playHeadJumps() || rateHz != _rateHz || lastBlockWasTemposync)
				{
					rateHz = _rateHz;
					rateInv = _rateHz * sampleRateInv;
					
					initCrossfade();
					perlins[perlinIndex].updateSpeed(rateInv);
				}
			}

			perlins[perlinIndex].updatePosition(playHeadPos, rateHz);

			processCurPosEstimate(numSamples);
		}

		// PROCESS TEMPOSYNC
		void processSync(const PlayHeadPos& playHeadPos, int numSamples, double _rateBeats, bool procedural) noexcept
		{
			if (procedural && playHeadPos.isPlaying)
				processSyncProcedural(playHeadPos, _rateBeats, numSamples);
			else
				processSyncRandom(playHeadPos, _rateBeats);
		}

		void processSyncUpdateSpeed(double bps) noexcept
		{
			const auto bpSamples = bps * sampleRateInv;
			const auto speed = rateInv * bpSamples;

			perlins[perlinIndex].updateSpeed(speed);
		}

		void processSyncRandom(const PlayHeadPos& playHeadPos, double _rateBeats) noexcept
		{
			rateBeats = _rateBeats;
			rateInv = .25 / rateBeats;

			const auto bpm = playHeadPos.bpm;
			const auto bps = bpm / 60.;

			processSyncUpdateSpeed(bps);
		}

		void processSyncProcedural(const PlayHeadPos& playHeadPos, double _rateBeats, int numSamples) noexcept
		{
			curPosInSamples = playHeadPos.timeInSamples;

			const auto bpm = playHeadPos.bpm;
			const auto bps = bpm / 60.;

			if (!crossfading)
			{
				if (playHeadJumps() || rateBeats != _rateBeats || !lastBlockWasTemposync)
				{
					rateBeats = _rateBeats;
					rateInv = .25 / rateBeats;
					
					initCrossfade();
					processSyncUpdateSpeed(bps);
				}
			}
			
			const auto latencyInPPQ = latency * bps * sampleRateInv;
			const auto ppq = playHeadPos.ppqPosition - latencyInPPQ;
			
			perlins[perlinIndex].updatePositionSyncProcedural(ppq, rateInv);
			

			processCurPosEstimate(numSamples);
		}

		// CROSSFADE FUNCS
		bool playHeadJumps() noexcept
		{
			const auto distance = std::abs(curPosInSamples - curPosEstimate);
			return distance > 2;
		}

		void processCurPosEstimate(int numSamples) noexcept
		{
			curPosEstimate = curPosInSamples + numSamples;
		}

		void initCrossfade() noexcept
		{
			xPhase = 0.f;
			crossfading = true;
			perlinIndex = 1 - perlinIndex;
		}

		/* samples, octavesBuf, phsBuf, widthBuf,
		octaves, width, phs, numChannels, numSamples */
		void processCrossfade(float* const* samples, const PRMInfo& octavesInfo,
			const PRMInfo& phsInfo, const PRMInfo& widthInfo,
			Shape shape, int numChannels, int numSamples) noexcept
		{
			if (crossfading)
			{
				auto prevSamples = prevBuffer.getArrayOfWritePointers();
				perlins[1 - perlinIndex]
				(
					prevSamples,
					noise.data(),
					gainBuffer.data(),
					octavesInfo,
					phsInfo,
					widthInfo,
					shape,
					numChannels,
					numSamples
				);

				for (auto s = 0; s < numSamples; ++s)
				{
					xFadeBuffer[s] = xPhase;
					xPhase += xInc;
					if (xPhase > 1.f)
					{
						crossfading = false;
						xPhase = 1.f;
					}
				}

				for (auto ch = 0; ch < numChannels; ++ch)
				{
					auto smpls = samples[ch];
					const auto prevSmpls = prevSamples[ch];

					for (auto s = 0; s < numSamples; ++s)
					{
						const auto prev = prevSmpls[s];
						const auto cur = smpls[s];

						const auto xFade = xFadeBuffer[s];
						const auto xPi = xFade * Pi;
						const auto xPrev = std::cos(xPi) + 1.f;
						const auto xCur = std::cos(xPi + Pi) + 1.f;

						smpls[s] = (prev * xPrev + cur * xCur) * .5f;
					}
				}
			}
		}
	};
}

/*

*/
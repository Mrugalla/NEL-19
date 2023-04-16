#pragma once
#include "Phasor.h"
#include "Wavetable.h"
#include "PRM.h"
#include <array>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>

#define DebugPhasor true

namespace dsp
{
	static constexpr float Pi = 3.14159265359f;
    
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
    
    struct LFO
    {
        using Wavetables = dsp::LFOTables;
        
        LFO() :
            phasor(0., 0.)
        {
        }

		void prepare(double fsInv)
		{
			phasor.prepare(fsInv);
		}

        /* phasorPhase[0,1] */
        void updatePhase(double phase) noexcept
        {
            phasor.phase.phase = phase;
        }

        bool speedChanged(double inc) noexcept
        {
			return phasor.inc != inc;
        }

        /* phasorInc[0,1] */
        void updateSpeed(double inc) noexcept
        {
            phasor.inc = inc;
        }

        /* samples, numChannels, numSamples, wavetables,
        phase[-.5, .5], width[0, .5], wtPos[0,1] */
        void operator()(float* const* samples, int numChannels, int numSamples,
            const Wavetables& wavetables,
            const PRMInfo& phaseInfo, const PRMInfo& widthInfo, const PRMInfo& wtPosInfo) noexcept
        {
            synthesizePhasor
            (
                samples,
                phaseInfo, widthInfo,
                numChannels, numSamples
            );

#if !DebugPhasor
            processWavetables
            (
                wavetables,
                samples,
                wtPosInfo,
                numChannels,
                numSamples
            );
#endif
        }
        
        Phasor<double> phasor;
    protected:
        
        void synthesizePhasor(float* const* samples,
            const PRMInfo& phaseInfo, const PRMInfo& widthInfo, int numChannels, int numSamples) noexcept
        {
			auto smplsL = samples[0];

			if(phaseInfo.smoothing)
			    for (auto s = 0; s < numSamples; ++s)
                {
                    const auto phase = phasor();
                    smplsL[s] = static_cast<float>(phase.phase) + phaseInfo[s];
                }
            else
                for (auto s = 0; s < numSamples; ++s)
                {
                    const auto phase = phasor();
                    smplsL[s] = static_cast<float>(phase.phase) + phaseInfo.val;
                }
            
            for (auto s = 0; s < numSamples; ++s)
                if (smplsL[s] < 0.f)
                    ++smplsL[s];

            wrapPhasor(smplsL, numSamples);

            if (numChannels != 2)
                return;

			auto smplsR = samples[1];

            if (widthInfo.smoothing)
                juce::FloatVectorOperations::add(smplsR, smplsL, widthInfo.buf, numSamples);
            else
                juce::FloatVectorOperations::add(smplsR, smplsL, widthInfo.val, numSamples);

            wrapPhasor(smplsR, numSamples);
        }

        void wrapPhasor(float* smpls, int numSamples) noexcept
        {
            for (auto s = 0; s < numSamples; ++s)
                if (smpls[s] > 1.f)
                    --smpls[s];
        }

        void processWavetables(const Wavetables& wavetables,
            float* const* samples, const PRMInfo& wtPosInfo,
            int numChannels, int numSamples) noexcept
        {
            if (wtPosInfo.smoothing)
                for (auto ch = 0; ch < numChannels; ++ch)
                {
                    auto smpls = samples[ch];
                    for (auto s = 0; s < numSamples; ++s)
                    {
                        smpls[s] = wavetables(wtPosInfo[s], smpls[s]);
                    }
                }
            else
                for (auto ch = 0; ch < numChannels; ++ch)
                {
                    auto smpls = samples[ch];
                    for (auto s = 0; s < numSamples; ++s)
                    {
                        smpls[s] = wavetables(wtPosInfo.val, smpls[s]);
                    }
                }
        }
    };

    struct LFO2
    {
        using AudioBuffer = juce::AudioBuffer<float>;
        using Wavetables = LFO::Wavetables;
		using LFOs = std::array<LFO, 2>;
		using PosInfo = juce::AudioPlayHead::CurrentPositionInfo;
        
        LFO2(const Wavetables& _wavetables) :
            wavetables(_wavetables),
            sampleRate(1.), sampleRateInv(1.),
            latency(0.),
            lfos(),
            currentLFOIndex(0),
            phasePRM(0.f),
            widthPRM(0.f),
            wtPosPRM(0.f),
			phasorInc(0.), phasorRate(1.),
            xAudioBuffer(),
            xPhase(0.), xInc(0.),
            xFading(false),
            timeEstimate(0)
        {}

        void prepare(float fs, int blockSize, double _latency)
        {
            sampleRate = static_cast<double>(fs);
			sampleRateInv = 1. / sampleRate;
			for (auto& lfo : lfos)
				lfo.prepare(sampleRateInv);
			latency = _latency;
			xInc = msInInc(420., sampleRate);
            phasePRM.prepare(fs, blockSize, 8.f);
            widthPRM.prepare(fs, blockSize, 8.f);
            wtPosPRM.prepare(fs, blockSize, 8.f);
            xAudioBuffer.setSize(3, blockSize, false, false, false);
        }
        
        /* samples, numChannels, numSamples, transport,
        rateHz, rateSync, phase[-.5,.5], width[0,.5], wtPos[0,1], temposync*/
        void operator()(float* const* samples, int numChannels, int numSamples,
            const PosInfo& transport, double _rateHz, double _rateSync,
            float phase, float width, float wtPos,
            bool temposync) noexcept
        {
            const auto bpm = transport.bpm;
            const auto bps = bpm / 60.;
			const auto quarterNoteLengthInSamples = sampleRate / bps;
            
            if (!xFading)
            {
                bool shallXFade = false;
                
                //if (playHeadJumped(transport.timeInSamples))
                //    shallXFade = true;
                if (updatePhasorSpeed(quarterNoteLengthInSamples, _rateHz, _rateSync, temposync))
                    shallXFade = false;

                if (shallXFade)
                    initXFade();
            }
            
            if (transport.isPlaying)
                updatePhasorPosition
                (
                    lfos[currentLFOIndex],
                    transport.ppqPosition,
                    bps,
                    quarterNoteLengthInSamples,
                    temposync
                );
                
			const auto phaseInfo = phasePRM(phase, numSamples);
            const auto widthInfo = widthPRM(width, numSamples);
			const auto wtPosInfo = wtPosPRM(wtPos, numSamples);
            
            lfos[currentLFOIndex]
            (
                samples,
                numChannels,
                numSamples,
                wavetables,
                phaseInfo,
                widthInfo,
                wtPosInfo
            );

			if (xFading)
				processXFade
                (
                    samples,
                    numChannels,
                    numSamples,
                    phaseInfo,
                    widthInfo,
                    wtPosInfo
                );

			timeEstimate = transport.timeInSamples + numSamples;
        }

    protected:
        const Wavetables& wavetables;
        LFOs lfos;
        double sampleRate, sampleRateInv, latency;
		int currentLFOIndex;
        // parameters
        PRM phasePRM, widthPRM, wtPosPRM;
        // lfo
        double phasorInc, phasorRate;
        // crossfade
        AudioBuffer xAudioBuffer;
        double xPhase, xInc;
        bool xFading;
        // project position
        __int64 timeEstimate;
        
        // true if xFade initiated
        bool updatePhasorSpeed(double quarterNoteLengthInSamples, double _rateHz, double _rateSync, bool temposync) noexcept
        {
            double inc = 0.;
            if (temposync)
            {
                const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
                inc = 1. / (barLengthInSamples * _rateSync);
            }
            else
                inc = _rateHz * sampleRateInv;
            
            bool shallXFade = lfos[currentLFOIndex].speedChanged(inc);

            if (shallXFade)
            {
				initXFade();
                phasorInc = inc;
				phasorRate = temposync ? _rateSync : _rateHz;
                lfos[currentLFOIndex].updateSpeed(phasorInc);
            }
            
            return shallXFade;
        }

        void updatePhasorPosition(LFO& lfo, double ppqPosition, double bps,
            double quarterNoteLengthInSamples, bool temposync) noexcept
        {
            if (temposync)
                updatePhasorTemposync(lfo, ppqPosition, quarterNoteLengthInSamples);
            else
                updatePhasorHz(lfo, ppqPosition, bps);
        }

        void updatePhasorTemposync(LFO& lfo, double ppqPosition, double quarterNoteLengthInSamples) noexcept
        {
            const auto latencyLengthInQuarterNotes = latency / quarterNoteLengthInSamples;
            auto ppq = (ppqPosition - latencyLengthInQuarterNotes) * .25;
            while (ppq < 0.f)
                ++ppq;
            const auto ppqCh = ppq / phasorRate;
            lfo.updatePhase(ppqCh - std::floor(ppqCh));
        }

        void updatePhasorHz(LFO& lfo, double ppqPosition, double bps) noexcept
        {
            const auto phase = ppqPosition / bps * phasorRate;

            lfo.updatePhase(phase - std::floor(phase));
        }

        // crossfade funcs

        bool playHeadJumped(__int64 timeInSamples) noexcept
        {
			const auto distance = std::abs(timeInSamples - timeEstimate);
            return distance > 2;
        }
        
        void initXFade() noexcept
        {
            xPhase = 0.;
			currentLFOIndex = 1 - currentLFOIndex;
			xFading = true;
        }

        void processXFade(float* const* samples, int numChannels, int numSamples,
            const PRMInfo& phaseInfo, const PRMInfo& widthInfo, const PRMInfo& wtPosInfo) noexcept
        {
            auto& lfo = lfos[1 - currentLFOIndex];

			auto xSamples = xAudioBuffer.getArrayOfWritePointers();

            lfo
            (
                xSamples,
                numChannels,
                numSamples,
                wavetables,
                phaseInfo,
                widthInfo,
                wtPosInfo
			);

            auto xBuffer = xSamples[2];
            
            synthesizeXFadeBuffer(xBuffer, numSamples);
            
            for (auto ch = 0; ch < numChannels; ++ch)
            {
                auto smpls = samples[ch];
                const auto xSmpls = xSamples[ch];

                for (auto s = 0; s < numSamples; ++s)
                {
                    const auto xSmpl = xSmpls[s];
                    const auto smpl = smpls[s];

                    const auto xFade = xBuffer[s];
                    const auto xPi = xFade * Pi;
                    const auto xPrev = std::cos(xPi) + 1.f;
                    const auto xCur = std::cos(xPi + Pi) + 1.f;

                    smpls[s] = (xSmpl * xPrev + smpl * xCur) * .5f;
                }
            }
        }
        
        void synthesizeXFadeBuffer(float* xBuffer, int numSamples) noexcept
        {
            for (auto s = 0; s < numSamples; ++s)
            {
                xBuffer[s] = static_cast<float>(xPhase);
                xPhase += xInc;
                if (xPhase > 1.)
                {
                    xFading = false;
                    xPhase = 1.;
                }
            }
        }
    };
}

#undef DebugPhasor
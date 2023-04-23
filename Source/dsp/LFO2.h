#pragma once
#include "Phasor.h"
#include "Wavetable.h"
#include "PRM.h"
#include "StandalonePlayHead.h"
#include "XFade.h"

#define DebugPhasor false

namespace dsp
{
	struct LFO
    {
        using Wavetables = dsp::LFOTables;

        LFO() :
            phasor(0., 0.)
        {
        }

        void prepare(double fsInv) noexcept
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

        /* samples, wavetables,
        phase[-.5, .5], width[0, .5], wtPos[0,1],
        numChannels, numSamples */
        void operator()(double* const* samples, const Wavetables& wavetables,
            const PRMInfoD& phaseInfo, const PRMInfoD& widthInfo, const PRMInfoD& wtPosInfo,
            int numChannels, int numSamples) noexcept
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
    
    protected:
        Phasor<double> phasor;

        void synthesizePhasor(double* const* samples,
            const PRMInfoD& phaseInfo, const PRMInfoD& widthInfo, int numChannels, int numSamples) noexcept
        {
            auto smplsL = samples[0];

            if (phaseInfo.smoothing)
                for (auto s = 0; s < numSamples; ++s)
                {
                    const auto phase = phasor();
                    smplsL[s] = phase.phase + phaseInfo[s];
                }
            else
                for (auto s = 0; s < numSamples; ++s)
                {
                    const auto phase = phasor();
                    smplsL[s] = phase.phase + phaseInfo.val;
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

        void wrapPhasor(double* smpls, int numSamples) noexcept
        {
            for (auto s = 0; s < numSamples; ++s)
                if (smpls[s] > 1.)
                    --smpls[s];
        }

        void processWavetables(const Wavetables& wavetables,
            double* const* samples, const PRMInfoD& wtPosInfo,
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

    struct LFO_Procedural
    {
        static constexpr double XFadeLengthMs = 200.;
        static constexpr int NumLFOs = 3;
        using Mixer = XFadeMixer<NumLFOs, true>;
        using Wavetables = LFO::Wavetables;
        using LFOs = std::array<LFO, NumLFOs>;
        using Int64 = juce::int64;

        LFO_Procedural(const Wavetables& _wavetables) :
            mixer(),
            wavetables(_wavetables),
            lfos(),
            phasePRM(0.f), widthPRM(0.f), wtPosPRM(0.f),
            latency(0.), sampleRate(1.), sampleRateInv(1.),
            quarterNoteLength(0.), bps(1.),
            rateHz(0.), rateSync(0.), bpm(0.), inc(0.),
            posEstimate(0), oversamplingFactor(1)
        {}

        void prepare(double _sampleRate, int blockSize, double _latency, int _oversamplingFactor)
        {
            inc = 0.;
            latency = _latency;
			sampleRate = _sampleRate;
            sampleRateInv = 1. / sampleRate;
			oversamplingFactor = _oversamplingFactor;

            mixer.prepare(sampleRate, XFadeLengthMs, blockSize);
            
            phasePRM.prepare(sampleRate, blockSize, 20.);
            widthPRM.prepare(sampleRate, blockSize, 20.);
            wtPosPRM.prepare(sampleRate, blockSize, 14.);

            for(auto& lfo: lfos)
                lfo.prepare(sampleRateInv);
        }

        void operator()(double* const* samples,
            int numChannels, int numSamples,
            PosInfo& transport, double _rateHz, double _rateSync,
            double phase, double width, double wtPos,
            bool temposync) noexcept
        {
            const auto phaseInfo = phasePRM(phase, numSamples);
            const auto widthInfo = widthPRM(width, numSamples);
            const auto wtPosInfo = wtPosPRM(wtPos, numSamples);
            
			updateLFO(transport, _rateHz, _rateSync, numSamples, temposync);
            
            {
                auto& track = mixer[0];
                auto xSamples = mixer.getSamples(0);

                if (track.isEnabled())
                {
                    track.synthesizeGainValues(xSamples[2], numSamples);

                    lfos[0]
                    (
                        xSamples,
                        wavetables,
                        phaseInfo,
                        widthInfo,
                        wtPosInfo,
                        numChannels,
                        numSamples
                    );

                    track.copy(samples, xSamples, numChannels, numSamples);
                }
                else
                    for(auto ch = 0; ch < numChannels; ++ch)
						SIMD::clear(samples[ch], numSamples);
            }

            for (auto i = 1; i < NumLFOs; ++i)
            {
                auto& track = mixer[i];
                
                if (track.isEnabled())
                {
                    auto xSamples = mixer.getSamples(i);
                    track.synthesizeGainValues(xSamples[2], numSamples);

                    lfos[i]
                    (
                        xSamples,
                        wavetables,
                        phaseInfo,
                        widthInfo,
                        wtPosInfo,
                        numChannels,
                        numSamples
                    );
                    
                    track.add(samples, xSamples, numChannels, numSamples);
                }
            }

            for(auto ch = 0; ch < numChannels; ++ch)
				SIMD::multiply(samples[ch], .999, numSamples);
        }

    protected:
        Mixer mixer;
        const Wavetables& wavetables;
        LFOs lfos;
        PRMD phasePRM, widthPRM, wtPosPRM;
        double latency, sampleRate, sampleRateInv, quarterNoteLength, bps;
        double rateHz, rateSync, bpm, inc;
        Int64 posEstimate;
        int oversamplingFactor;
        
        const bool isLooping(Int64 timeInSamples) const noexcept
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

        void updateLFO(const PosInfo& transport, double _rateHz, double _rateSync, 
            int numSamples, bool temposync) noexcept
        {
			updateSpeed(transport.bpm, _rateHz, _rateSync, transport.timeInSamples, temposync);
            
            if (transport.isPlaying)
            {
                updatePosition(lfos[mixer.idx], transport.ppqPosition, temposync);
                posEstimate = transport.timeInSamples + numSamples / oversamplingFactor;
            }
            else
				posEstimate = transport.timeInSamples;
        }

        void updateSpeed(double nBpm, double _rateHz, double _rateSync,
            Int64 timeInSamples, bool temposync) noexcept
        {
            const auto nBps = nBpm / 60.;
            const auto nQuarterNoteLength = sampleRate / nBps;

            auto nInc = 0.;
            if (temposync)
            {
                const auto barLength = nQuarterNoteLength * 4.;
                nInc = 1. / (barLength * _rateSync);
            }
            else
                nInc = _rateHz * sampleRateInv;

            if (isLooping(timeInSamples) || (changesSpeed(nBpm, nInc) && !mixer.stillFading()))
			{
                inc = nInc;
                bpm = nBpm;
                bps = nBps;
                quarterNoteLength = nQuarterNoteLength;
                rateSync = _rateSync;
                rateHz = _rateHz;
                
                mixer.init();
                lfos[mixer.idx].updateSpeed(inc);           
			}
        }

        void updatePosition(LFO& lfo, double ppqPosition, bool temposync) noexcept
        {
            auto lfoPhase = 0.;
            if (temposync)
            {
                const auto latencyLengthInQuarterNotes = latency / quarterNoteLength;
                const auto ppq = (ppqPosition - latencyLengthInQuarterNotes) * .25;
                lfoPhase = ppq / rateSync;
            }
            else
                lfoPhase = ppqPosition / bps * rateHz;

            lfo.updatePhase(lfoPhase - std::floor(lfoPhase));
        }
    };
}

#undef DebugPhasor
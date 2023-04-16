#pragma once
#include "Phasor.h"
#include "Wavetable.h"
#include "PRM.h"
#include "StandalonePlayHead.h"

#define DebugPhasor false

namespace dsp
{
	static constexpr float Pi = 3.141592653589f;

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
    
    protected:
        Phasor<double> phasor;

        void synthesizePhasor(float* const* samples,
            const PRMInfo& phaseInfo, const PRMInfo& widthInfo, int numChannels, int numSamples) noexcept
        {
            auto smplsL = samples[0];

            if (phaseInfo.smoothing)
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

    struct LFO_Procedural
    {
        using Wavetables = LFO::Wavetables;

        LFO_Procedural() :
            lfo(),
            //
            sampleRate(1.), sampleRateInv(1.), latency(0.),
            rateSync(1.), rateHz(1.)
        {
        }

        void prepare(double _sampleRate, int, double _latency)
        {
            sampleRate = _sampleRate;
            sampleRateInv = 1. / sampleRate;
            latency = _latency;

            lfo.prepare(sampleRateInv);
        }

        void operator()(const Wavetables& wavetables, float* const* samples,
            int numChannels, int numSamples,
            const PosInfo& transport, double _rateHz, double _rateSync,
            const PRMInfo& phaseInfo, const PRMInfo& widthInfo, const PRMInfo& wtPosInfo,
            bool temposync) noexcept
        {
            const auto bpm = transport.bpm;
            const auto bps = bpm / 60.;
            const auto quarterNoteLength = sampleRate / bps;

            auto nInc = 0.;
            if (temposync)
            {
                const auto barLength = quarterNoteLength * 4.;
                nInc = 1. / (barLength * _rateSync);
            }
            else
            {
                nInc = _rateHz * sampleRateInv;
            }

            rateSync = _rateSync;
            rateHz = _rateHz;
            lfo.updateSpeed(nInc);

            if (transport.isPlaying)
            {
                if (temposync)
                {
                    const auto latencyLengthInQuarterNotes = latency / quarterNoteLength;
                    auto ppq = (transport.ppqPosition - latencyLengthInQuarterNotes) * .25;
                    while (ppq < 0.f)
                        ++ppq;
                    const auto ppqCh = ppq / rateSync;
                    lfo.updatePhase(ppqCh - std::floor(ppqCh));
                }
                else
                {
                    const auto lfoPhase = transport.ppqPosition / bps * rateHz;
                    lfo.updatePhase(lfoPhase - std::floor(lfoPhase));
                }
            }

            lfo
            (
                samples,
                numChannels,
                numSamples,
                wavetables,
                phaseInfo,
                widthInfo,
                wtPosInfo
            );
        }

    protected:
        LFO lfo;
        //
        double sampleRate, sampleRateInv, latency;
        double rateSync, rateHz;
    };

    struct LFOFinal
    {
        using Wavetables = LFO::Wavetables;

        LFOFinal(const Wavetables& _wavetables) :
            wavetables(_wavetables),
            lfo(),
            phasePRM(0.f), widthPRM(0.f), wtPosPRM(0.f)
        {}

        void prepare(double sampleRate, int blockSize, double latency)
        {
            const auto fs = static_cast<float>(sampleRate);
            phasePRM.prepare(fs, blockSize, 14.f);
            widthPRM.prepare(fs, blockSize, 14.f);
            wtPosPRM.prepare(fs, blockSize, 14.f);

            lfo.prepare(sampleRate, blockSize, latency);
        }

        void operator()(float* const* samples,
            int numChannels, int numSamples,
            PosInfo& transport, double rateHz, double rateSync,
            float phase, float width, float wtPos,
            bool temposync) noexcept
        {
            const auto phaseInfo = phasePRM(phase, numSamples);
            const auto widthInfo = widthPRM(width, numSamples);
            const auto wtPosInfo = wtPosPRM(wtPos, numSamples);

            lfo
            (
                wavetables,
                samples,
                numChannels,
                numSamples,
                transport,
                rateHz,
                rateSync,
                phaseInfo,
                widthInfo,
                wtPosInfo,
                temposync
            );
        }

    protected:
        const Wavetables& wavetables;
        LFO_Procedural lfo;
        PRM phasePRM, widthPRM, wtPosPRM;
    };
}

#undef DebugPhasor


/*

todo:

xFade when
    bpm changed

*/
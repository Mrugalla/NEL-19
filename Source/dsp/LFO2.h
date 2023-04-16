#pragma once
#include "Phasor.h"
#include "Wavetable.h"
#include "PRM.h"
#include <array>
#include <vector>
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

    struct XFade
    {
        using AudioBuffer = juce::AudioBuffer<float>;

        XFade() :
            buffer(),
            phase(0.),
            inc(0.),
            idx(0),
            fading(false)
        {}

		void prepare(double sampleRate, double lengthMs, int blockSize)
		{
            buffer.setSize(3, blockSize, false, true, false);
			inc = msInInc(lengthMs, sampleRate);
		}

        void init() noexcept
        {
            idx = 1 - idx;
            phase = 0.;
			fading = true;
        }

        float* const* getSamples() noexcept
        {
			return buffer.getArrayOfWritePointers();
        }

        void synthesizePhase(int numSamples) noexcept
        {
            auto xSamples = getSamples();
            auto xBuf = xSamples[2];

            for (auto s = 0; s < numSamples; ++s)
            {
                xBuf[s] = static_cast<float>(phase);
                phase += inc;
                if (phase > 1.)
                {
                    fading = false;
                    phase = 1.;
                    for (; s < numSamples; ++s)
                        xBuf[s] = 1.f;
                    return;
                }
            }
        }

        void operator()(float* const* samples, int numChannels, int numSamples) noexcept
        {
            synthesizePhase(numSamples);

            auto xSamples = getSamples();
            auto xBuf = xSamples[2];

            for (auto ch = 0; ch < numChannels; ++ch)
            {
                auto smpls = samples[ch];
                const auto xSmpls = xSamples[ch];

                for (auto s = 0; s < numSamples; ++s)
                {
                    const auto smpl = smpls[s];
                    const auto xSmpl = xSmpls[s];

                    const auto xFade = xBuf[s];
                    const auto xPi = xFade * Pi;
                    const auto frac = std::cos(xPi + Pi) + 1.f;
                    const auto xFrac = std::cos(xPi) + 1.f;

                    smpls[s] = (smpl * frac + xSmpl * xFrac) * .5f;
                }
            }
        }

        AudioBuffer buffer;
        double phase, inc;
        int idx;
        bool fading;
    };
    
    struct LFO_XFadesSpeed
    {
        using Wavetables = LFO::Wavetables;
        using LFOs = std::array<LFO, 2>;

        LFO_XFadesSpeed() :
            lfos(),
            xFade(),
            //
            xTransport(),
            sampleRate(1.), sampleRateInv(1.), latency(0.),
            rateSync(1.), rateHz(1.)
        {
            setPlayHead(xTransport, 120., sampleRateInv, 0, false);
        }
        
        void prepare(double _sampleRate, int blockSize, double _latency)
        {
			sampleRate = _sampleRate;
			sampleRateInv = 1. / sampleRate;
			latency = _latency;
            
            xFade.prepare(sampleRate, 120., blockSize);
            for (auto& lfo : lfos)
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
            
            if(!xFade.fading)
                updateSpeed(quarterNoteLength, _rateSync, _rateHz, temposync);

            if (transport.isPlaying)
            {
                updatePosition
                (
                    transport.ppqPosition,
                    quarterNoteLength,
                    bps,
                    temposync
                );
            }

			auto& lfo = lfos[xFade.idx];
            
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

            if (!xFade.fading)
                return;

            lfos[1 - xFade.idx]
            (
                xFade.getSamples(),
                numChannels,
                numSamples,
                wavetables,
                phaseInfo,
                widthInfo,
                wtPosInfo
            );
            
            xFade(samples, numChannels, numSamples);
        }

    protected:
        LFOs lfos;
        XFade xFade;
        //
        PosInfo xTransport;
        double sampleRate, sampleRateInv, latency;
        double rateSync, rateHz;
        
        void updateSpeed(double quarterNoteLength,
            double _rateSync, double _rateHz, bool temposync) noexcept
        {
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

            if (lfos[xFade.idx].speedChanged(nInc))
            {
                xFade.init();
				rateSync = _rateSync;
				rateHz = _rateHz;
                lfos[xFade.idx].updateSpeed(nInc);
            }
        }

        void updatePosition(double ppqPosition, double quarterNoteLength, 
            double bps, bool temposync) noexcept
        {
			auto& lfo = lfos[xFade.idx];

            if (temposync)
            {
                const auto latencyLengthInQuarterNotes = latency / quarterNoteLength;
                auto ppq = (ppqPosition - latencyLengthInQuarterNotes) * .25;
                while (ppq < 0.f)
                    ++ppq;
                const auto ppqCh = ppq / rateSync;
                lfo.updatePhase(ppqCh - std::floor(ppqCh));
            }
            else
            {
                const auto lfoPhase = ppqPosition / bps * rateHz;
                lfo.updatePhase(lfoPhase - std::floor(lfoPhase));
            }
        }
    };
    
    struct LFO_XFadesPlayHeadJump
    {
        using Wavetables = LFO::Wavetables;
        using LFOs = std::array<LFO_XFadesSpeed, 2>;
        
        LFO_XFadesPlayHeadJump() :
            lfos(),
            xFade(),
            //
            xTransport(),
            sampleRateInv(1.),
            posEstimate(0)
        {
        }

        void prepare(double sampleRate, int blockSize, double latency)
        {
			sampleRateInv = 1. / sampleRate;
            xFade.prepare(sampleRate, 120., blockSize);

            for (auto& lfo : lfos)
                lfo.prepare(sampleRate, blockSize, latency);
        }

        void operator()(const Wavetables& wavetables, float* const* samples,
            int numChannels, int numSamples,
            const PosInfo& transport, double rateHz, double rateSync,
            const PRMInfo& phaseInfo, const PRMInfo& widthInfo, const PRMInfo& wtPosInfo,
            bool temposync) noexcept
        {
            Int64 posInc = transport.isPlaying ? numSamples : 0;
            if (!xFade.fading)
            {
                const auto error = std::abs(transport.timeInSamples - posEstimate);
                if (error > 1)
                {
                    xFade.init();
                    setPlayHead(xTransport, transport.bpm, sampleRateInv, posEstimate, transport.isPlaying);
                }
            }

            lfos[xFade.idx]
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

            posEstimate = transport.timeInSamples + posInc;

            if (!xFade.fading)
                return;
            
			lfos[1 - xFade.idx]
			(
				wavetables,
				xFade.getSamples(),
				numChannels,
				numSamples,
				xTransport,
				rateHz,
				rateSync,
				phaseInfo,
				widthInfo,
				wtPosInfo,
				temposync
			);

            xFade(samples, numChannels, numSamples);

            movePlayHead(xTransport, sampleRateInv, numSamples);
        }

    protected:
        LFOs lfos;
        XFade xFade;
        //
        PosInfo xTransport;
        double sampleRateInv;
        Int64 posEstimate;
    };

    struct LFO_XFadesPlayBack
    {
        using Wavetables = LFO::Wavetables;
        using LFOs = std::array<LFO_XFadesPlayHeadJump, 2>;

        LFO_XFadesPlayBack() :
            lfos(),
            xFade(),
            //
            xTransport(),
            sampleRateInv(1.),
            wasPlaying(false)
        {
            xTransport.isPlaying = false;
        }

        void prepare(double sampleRate, int blockSize, double latency)
        {
            sampleRateInv = 1. / sampleRate;
            xFade.prepare(sampleRate, 120., blockSize);
            for (auto& lfo : lfos)
                lfo.prepare(sampleRate, blockSize, latency);
        }

        void operator()(const Wavetables& wavetables, float* const* samples,
            int numChannels, int numSamples,
            const PosInfo& transport, double rateHz, double rateSync,
            const PRMInfo& phaseInfo, const PRMInfo& widthInfo, const PRMInfo& wtPosInfo,
            bool temposync) noexcept
        {
            if (transport.isPlaying)
            {
                if (!xFade.fading && !wasPlaying)
                {
                    xFade.init();
                    xTransport.bpm = transport.bpm;
					xTransport.timeInSamples = transport.timeInSamples;
					xTransport.timeInSeconds = transport.timeInSeconds;
					xTransport.ppqPosition = transport.ppqPosition;
                }

                wasPlaying = true;
            }
            else
                wasPlaying = false;

            lfos[xFade.idx]
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

			if (!xFade.fading)
				return;

			lfos[1 - xFade.idx]
			(
				wavetables,
				xFade.getSamples(),
				numChannels,
				numSamples,
				xTransport,
				rateHz,
				rateSync,
				phaseInfo,
				widthInfo,
				wtPosInfo,
				temposync
			);

			xFade(samples, numChannels, numSamples);

            movePlayHead(xTransport, sampleRateInv, numSamples);
		}

    protected:
        LFOs lfos;
        XFade xFade;
        //
        PosInfo xTransport;
        double sampleRateInv;
        bool wasPlaying;
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
        LFO_XFadesPlayBack lfo;
        PRM phasePRM, widthPRM, wtPosPRM;
    };
}

#undef DebugPhasor


/*

todo:

xFade when
    bpm changed

*/
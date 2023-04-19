#pragma once
#include <JuceHeader.h>

namespace outtake
{
    /*
    extremely expensive method just to check what it would look
    like to draw a cool pseudo-3d grid over a component, lmao
    */
	static void drawRandGrid(juce::Graphics& g, juce::Rectangle<int> bounds, int numVerticalLines = 32, int numHorizontalLines = 16, juce::Colour c = juce::Colours::white, float maxHillSize = .1f) {
        const auto width = static_cast<float>(bounds.getWidth());
        const auto height = static_cast<float>(bounds.getHeight());
        const auto mhsHeight = maxHillSize * height;

        juce::Random rand;

        g.setColour(c);
        std::vector<std::vector<juce::Point<float>>> points;
        points.resize(numVerticalLines);
        for (auto& p : points)
            p.resize(numHorizontalLines);
        for (auto x = 0; x < numVerticalLines; ++x) {
            auto xRatio = float(x) / (numVerticalLines - 1.f);
            auto xStart = xRatio * width;
            auto xEnd = xRatio * width * 2 - width * .5f;
            for (auto y = 0; y < numHorizontalLines; ++y) {
                auto yRatio = (1.f * y) / (numHorizontalLines);
                auto yStart = height * (yRatio * (1.f + 2.f * maxHillSize) * numHorizontalLines) / ((numHorizontalLines - 1.f) - maxHillSize);

                auto xE = xStart + yRatio * (xEnd - xStart);
                auto randVal = rand.nextFloat();
                auto yE = yStart - mhsHeight * randVal * randVal;
                points[x][y] = { xE, yE };
            }
        }
        for (auto x = 0; x < numVerticalLines - 1; ++x) {
            const auto xp1 = x + 1;
            for (auto y = 0; y < numHorizontalLines - 1; ++y) {
                const auto yp1 = y + 1;
                juce::Line<float> horizontalLine(points[x][y], points[xp1][y]);
                juce::Line<float> verticalLine(points[x][y], points[x][yp1]);
                g.drawLine(horizontalLine);
                g.drawLine(verticalLine);
            }
        }
	}
}

// OLD LFO CODE:
/*
class LFO
		{
			template<typename Float>
			struct PhaseSyncronizer
			{
				PhaseSyncronizer() :
					inc(static_cast<Float>(1))
				{}

				void prepare(Float Fs, Float timeInMs) noexcept
				{
					inc = static_cast<Float>(1) / (Fs * timeInMs * static_cast<Float>(.001));
				}

				Float operator()(Float curPhase, Float destPhase) const noexcept
				{
					const auto dist = destPhase - curPhase;
					curPhase += inc * dist;
					return curPhase;
				}

			protected:
				Float inc;
			};

			struct TempoSync
			{
				TempoSync(const BeatsData& _beatsData) :
					syncer(),
					phaseSmooth(0.f),
					beatsData(_beatsData),
					fs(1.), extLatency(0.),
					phasor(0.), inc(0.)
				{}

				void prepare(float sampleRate, int latency)
				{
					fs = static_cast<double>(sampleRate);
					extLatency = static_cast<double>(latency);
					phaseSmooth.makeFromDecayInMs(20.f, sampleRate);
					syncer.prepare(fs, 420.f);
				}

				void processTempoSyncStuff(float* buffer, float rateSync, float phase, int numSamples, const PosInfo& transport)
				{
					if (transport.isPlaying)
					{
						const auto rateSyncV = static_cast<double>(beatsData[static_cast<int>(rateSync)].val);
						const auto rateSyncInv = 1. / rateSyncV;

						const auto bpm = transport.bpm;
						const auto bps = bpm / 60.;
						const auto quarterNoteLengthInSamples = fs / bps;
						const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
						inc = 1. / (barLengthInSamples * rateSyncV);

						const auto latencyLengthInQuarterNotes = extLatency / quarterNoteLengthInSamples;
						auto ppq = (transport.ppqPosition - latencyLengthInQuarterNotes) * .25;
						while (ppq < 0.f)
							++ppq;
						const auto ppqCh = ppq * rateSyncInv;

						auto newPhasor = ppqCh - std::floor(ppqCh);
						if (newPhasor < phasor)
							++newPhasor;

						auto phaseVal = 0.;

						for (auto s = 0; s < numSamples; ++s)
						{
							phasor += inc;
							phasor = syncer(phasor, newPhasor);
							newPhasor += inc;

							phaseVal = static_cast<double>(phaseSmooth(phase));
							auto shifted = phasor + phaseVal;
							while (shifted < 0.)
								++shifted;
							while (shifted >= 1.)
								--shifted;
							buffer[s] = static_cast<float>(shifted);
						}

						const auto p = buffer[numSamples - 1] - phaseVal;
						phasor = p < 0.f ? p + 1.f : p >= 1.f ? p - 1.f : p;
					}
					else
					{
						for (auto s = 0; s < numSamples; ++s)
						{
							phasor += inc;
							if (phasor >= 1.f)
								--phasor;
							const auto phaseVal = static_cast<double>(phaseSmooth(phase));
							auto shifted = phasor + phaseVal;
							if (shifted < 0.)
								++shifted;
							else if (shifted >= 1.)
								--shifted;
							buffer[s] = static_cast<float>(shifted);
						}
					}
				}

				PhaseSyncronizer<double> syncer;
				SmoothF phaseSmooth;
			protected:
				const BeatsData& beatsData;
				double fs, extLatency, phasor, inc;
			};

		public:
			LFO(const Tables& _tables, const BeatsData& _beatsData) :
				tables(_tables),
				tempoSync(_beatsData),

				waveformSmooth(0.f),
				widthSmooth(0.f), rateSmooth(0.f),
				widthBuf(), rateBuf(), waveformBuf(),

				phasor(),
				fsInv(1.f),

				rateFree(-1.f),
				rateSync(0.f),
				isSync(false),

				waveformV(0.f),
				phaseV(0.f),
				widthV(0.f)
			{}

			void prepare(float sampleRate, int blockSize, int latency)
			{
				const auto fs = sampleRate;
				fsInv = 1.f / fs;
				tempoSync.prepare(sampleRate, latency);
				waveformSmooth.makeFromDecayInMs(20.f, fs);
				widthSmooth.makeFromDecayInMs(20.f, fs);
				rateSmooth.makeFromDecayInMs(12.f, fs);
				widthBuf.resize(blockSize);
				rateBuf.resize(blockSize);
				waveformBuf.resize(blockSize);
			}

			void setParameters(bool _isSync, float _rateFree, float _rateSync,
				float _waveform, float _phase, float _width) noexcept
			{
				isSync = _isSync;
				rateSync = _rateSync;
				rateFree = _rateFree;
				waveformV = _waveform;
				phaseV = _phase;
				widthV = _width;
			}

			void operator()(Buffer& buffer, int numChannels, int numSamples,
				const PosInfo& transport) noexcept
			{
				{ // SYNTHESIZE PHASOR
					auto buf = buffer[0].data();

					if (isSync)
						tempoSync.processTempoSyncStuff(buf, rateSync, phaseV, numSamples, transport);
					else
					{
						if (phaseV != 0.f)
						{
							const auto ppq = transport.ppqPosition;
							const auto bpm = transport.bpm;
							const auto bps = bpm / 60.;

							const auto inc = rateFree * fsInv;
							const auto phase = ppq / bps * rateFree;

							phasor.inc = inc;
							phasor.phase = phase - std::floor(phase);

							for (auto s = 0; s < numSamples; ++s)
							{
								buf[s] = static_cast<float>(phasor.phase) + tempoSync.phaseSmooth(phaseV);
								if (buf[s] < 0.f)
									++buf[s];
								else if (buf[s] >= 1.f)
									--buf[s];
								phasor();
							}
						}
						else
						{
							auto rateSmoothing = rateSmooth(rateBuf.data(), rateFree * fsInv, numSamples);

							if (rateSmoothing)
								for (auto s = 0; s < numSamples; ++s)
								{
									phasor.inc = rateBuf[s];
									buf[s] = static_cast<float>(phasor.process());
								}
							else
							{
								phasor.inc = rateFree * fsInv;
								for (auto s = 0; s < numSamples; ++s)
									buf[s] = static_cast<float>(phasor.process());
							}
						}
					}
				}

				{ // PROCESS WIDTH
					if (numChannels != 2)
						return;

					const auto buf0 = buffer[0].data();
					auto buf1 = buffer[1].data();
					juce::FloatVectorOperations::copy(buf1, buf0, numSamples);

					auto widthSmoothing = widthSmooth(widthBuf.data(), widthV, numSamples);

					if (widthSmoothing)
						juce::FloatVectorOperations::add(buf1, buf0, widthBuf.data(), numSamples);
					else
						juce::FloatVectorOperations::add(buf1, buf0, widthV, numSamples);

					for (auto s = 0; s < numSamples; ++s)
					{
						if (buf1[s] >= 1.f)
							--buf1[s];
					}
				}

				{ // PROCESS WAVEFORM
					auto waveformSmoothing = waveformSmooth(waveformBuf.data(), waveformV, numSamples);
					if(waveformSmoothing)
						for (auto ch = 0; ch < numChannels; ++ch)
						{
							auto buf = buffer[ch].data();
							for (auto s = 0; s < numSamples; ++s)
							{
								const auto tablesPhase = waveformBuf[s];
								const auto tablePhase = buf[s];
								buf[s] = tables(tablesPhase, tablePhase) * SafetyCoeff;
							}
						}
					else
						for (auto ch = 0; ch < numChannels; ++ch)
						{
							auto buf = buffer[ch].data();
							for (auto s = 0; s < numSamples; ++s)
								buf[s] = tables(waveformV, buf[s]) * SafetyCoeff;
						}
				}
			}

		protected:
			const Tables& tables;
			TempoSync tempoSync;
			SmoothF waveformSmooth;
			SmoothF widthSmooth, rateSmooth;
			std::vector<float> widthBuf, rateBuf, waveformBuf;

			Phasor<double> phasor;

			float fsInv;

			float rateFree, rateSync;
			bool isSync;

			float waveformV, phaseV, widthV;
		};

*/

// NEW LFO CODE WITH CROSSFADES HIERARCHY:
/*
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
        
void updatePhase(double phase) noexcept
{
    phasor.phase.phase = phase;
}

bool speedChanged(double inc) noexcept
{
    return phasor.inc != inc;
}

void updateSpeed(double inc) noexcept
{
    phasor.inc = inc;
}

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

            if (!xFade.fading)
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

            auto& lfo = lfos[xFade.idx];

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
            sampleRateInv(1.), bpm(1.),
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
                bool shallXFade = false;

                if (bpm != transport.bpm)
                {
                    bpm = transport.bpm;
                    shallXFade = true;
                }

                const auto error = std::abs(transport.timeInSamples - posEstimate);

                shallXFade = shallXFade && error > 1;

                if (shallXFade)
                {
                    xFade.init();
                    setPlayHead(xTransport, bpm, sampleRateInv, posEstimate, transport.isPlaying);
                    bpm = transport.bpm;
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
        double sampleRateInv, bpm;
        Int64 posEstimate;
    };

    // not used rn
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
        LFO_XFadesPlayHeadJump lfo;
        PRM phasePRM, widthPRM, wtPosPRM;
    };
}

#undef DebugPhasor

*/

// NEW LFO CODE WITHOUT ANYTHING
/*
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
        
void updatePhase(double phase) noexcept
{
    phasor.phase.phase = phase;
}

bool speedChanged(double inc) noexcept
{
    return phasor.inc != inc;
}

void updateSpeed(double inc) noexcept
{
    phasor.inc = inc;
}

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


*/


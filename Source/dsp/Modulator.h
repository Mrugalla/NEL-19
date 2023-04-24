#pragma once
#include "../Interpolation.h"
#include "../modsys/ModSys.h"
#include "../Approx.h"
#include <random>
#include "Smooth.h"
#include "Perlin.h"
#include "Wavetable.h"
#include "LFO2.h"

#define DebugAudioRateEnv false

namespace vibrato
{
	static constexpr double Tau = 6.283185307179586476925286766559;
	static constexpr double Pi = 3.1415926535897932384626433832795;
	static constexpr double PiHalf = Pi * .5;

	using SmoothD = smooth::Smooth<double>;
	using String = juce::String;

	enum class ObjType
	{
		ModType,
		InterpolationType,
		DelaySize,
		Wavetable,
		NumTypes
	};
	
	inline String toString(ObjType t)
	{
		switch (t)
		{
		case ObjType::ModType: return "ModType";
		case ObjType::InterpolationType: return "InterpolationType";
		case ObjType::DelaySize: return "DelaySize";
		case ObjType::Wavetable: return "Wavetable";
		default: return "";
		}
	}
	
	inline String with(ObjType t, int i)
	{
		return toString(t) + juce::String(i);
	}

	enum class ModType
	{
		Perlin,
		AudioRate,
		Dropout,
		EnvFol,
		Macro,
		Pitchwheel,
		LFO,
		//Rand, Trigger, Spline, Orbit
		NumMods
	};
	
	inline String toString(ModType t)
	{
		switch (t)
		{
		case ModType::Perlin: return "Perlin";
		case ModType::AudioRate: return "AudioRate";
		case ModType::Dropout: return "Dropout";
		case ModType::EnvFol: return "EnvFol";
		case ModType::Macro: return "Macro";
		case ModType::Pitchwheel: return "Pitchwheel";
		case ModType::LFO: return "LFO";
		//case ModType::Rand: return "Rand";
		default: return "";
		}
	}
	
	inline String with(ModType t, int i)
	{
		return toString(t) + juce::String(i);
	}
	
	inline ModType getModType(const juce::String& str)
	{
		for (auto i = 0; i < static_cast<int>(ModType::NumMods); ++i)
		{
			const auto type = static_cast<ModType>(i);
			if (str == toString(static_cast<ModType>(i)))
				return type;
		}
		return ModType::Perlin;
	}

	template<typename Float>
	struct Phasor
	{
		Phasor() :
			phase(static_cast<Float>(0)),
			inc(static_cast<Float>(0)),

			fs(static_cast<Float>(1)),
			fsInv(static_cast<Float>(1))
		{}

		void prepare(Float sampleRate) noexcept
		{
			fs = sampleRate;
			fsInv = static_cast<Float>(1) / fs;
		}
		
		void setFrequencyMs(Float f) noexcept
		{
			inc = static_cast<Float>(1000) / (fs * f);
		}
		
		void setFrequencySecs(Float f) noexcept
		{
			inc = static_cast<Float>(1) / (fs * f);
		}
		
		void setFrequencyHz(Float f) noexcept
		{
			inc = fsInv * f;
		}
		
		bool operator()() noexcept
		{
			phase += inc;
			if (phase >= static_cast<Float>(1))
			{
				--phase;
				return true;
			}
			return false;
		}
		
		Float process() noexcept
		{
			phase += inc;
			if (phase >= static_cast<Float>(1))
				--phase;
			return phase;
		}

		Float phase, inc;
	protected:
		Float fs, fsInv;
	};

	struct EnvGen
	{
		enum class State { A, D, R };

		EnvGen() :
			attack(1.), decay(1.), sustain(1.), release(1.),

			Fs(1.), env(0.),
			state(State::R),
			noteOn(false),

			smooth(0.)
		{}
		
		void prepare(double sampleRate)
		{
			Fs = sampleRate;
			switch (state)
			{
			case State::A:
				smooth.makeFromDecayInMs(attack, sampleRate);
				break;
			case State::D:
				smooth.makeFromDecayInMs(decay, sampleRate);
				break;
			case State::R:
				smooth.makeFromDecayInMs(release, sampleRate);
				break;
			}
		}
		
		double operator()(bool n) noexcept
		{
			noteOn = n;
			return processSample();
		}

		double operator()() noexcept
		{
			return processSample();
		}

		void retrig() noexcept
		{
			state = State::A;
			smooth.makeFromDecayInMs(attack, Fs);
		}

		double attack, decay, sustain, release;
		double Fs, env;
		State state;
		bool noteOn;
	protected:
		SmoothD smooth;

		double processSample() noexcept
		{
			switch (state)
			{
			case State::A:
				if (noteOn)
					if (env < .999)
					{
						env = smooth(1.);
					}
					else
					{
						state = State::D;
						smooth.makeFromDecayInMs(decay, Fs);
					}
				else
				{
					state = State::R;
					smooth.makeFromDecayInMs(release, Fs);
				}
				break;
			case State::D:
				if (noteOn)
					env = smooth(sustain);
				else
				{
					state = State::R;
					smooth.makeFromDecayInMs(release, Fs);
				}
				break;
			case State::R:
				if (!noteOn)
					env = smooth(0.);
				else
				{
					state = State::A;
					smooth.makeFromDecayInMs(attack, Fs);
				}
				break;
			}
			return env;
		}
	};

	// creates a modulator curve mapped to [-1, 1]
	// of some ModType (like perlin, audiorate, dropout etc.)
	class Modulator
	{
		using Buffer = std::array<std::vector<double>, 4>;
		using Tables = dsp::LFOTables;

		using PlayHead = juce::AudioPlayHead;
		using PosInfo = PlayHead::CurrentPositionInfo;

		struct Perlin
		{
			using PlayHeadPos = perlin::PlayHeadPos;
			using Shape = perlin::Shape;

			Perlin() :
				perlin(),
				rateHz(1.), rateBeats(1.),
				octaves(1.), width(0.), phs(0.),
				shape(Shape::Spline),
				temposync(false), procedural(true)
			{
			}

			void prepare(double sampleRate, int blockSize, int latency)
			{
				perlin.prepare(sampleRate, blockSize, latency);
			}

			void setParameters(double _rateHz, double _rateBeats,
				double _octaves, double _width, double _phs,
				Shape _shape, bool _temposync, bool _procedural) noexcept
			{
				rateHz = _rateHz;
				rateBeats = _rateBeats;
				octaves = _octaves;
				width = _width;
				phs = _phs;
				shape = _shape;
				temposync = _temposync;
				procedural = _procedural;
			}

			void operator()(Buffer& buffer, int numChannels, int numSamples,
				const PosInfo& transport) noexcept
			{
				double* samples[2] = { buffer[0].data(), buffer[1].data() };
				
				perlin
				(
					samples,
					numChannels,
					numSamples,
					transport,
					rateHz,
					rateBeats,
					octaves,
					width,
					phs,
					shape,
					temposync,
					procedural
				);
			}
		
		protected:
			perlin::Perlin2 perlin;
			double rateHz, rateBeats;
			double octaves, width, phs;
			Shape shape;
			bool temposync, procedural;
		};

		class AudioRate
		{
			static constexpr double PBGain = 2. / static_cast<double>(0x3fff);

			struct Osc
			{
				Osc() :
					phasor()
				{}
				
				void prepare(double sampleRate) noexcept
				{
					phasor.prepare(sampleRate);
				}
				
				void setFrequencyHz(double f) noexcept
				{
					phasor.setFrequencyHz(f);
				}
				
				double operator()() noexcept
				{
					phasor();
					return std::cos(Tau * phasor.phase);
				}
				
				double withPhaseOffset(Osc& other, double offset) const noexcept
				{
					const auto phase = other.phasor.phase + offset;
					return std::cos(Tau * phase);
				}

				Phasor<double> phasor;
			};
			
		public:
			AudioRate() :
				retuneSpeedSmooth(0.),
				widthSmooth(0.),
				widthBuf(),
				
				osc(),
				env(),

				noteValue(0.), pitchbendValue(0.),

				noteOffset(0.), width(0.), retuneSpeed(0.),
				attack(1.), decay(1.), sustain(1.), release(1.),

				Fs(1.)
			{
				osc.resize(2);
			}
			
			void prepare(double sampleRate, int blockSize)
			{
				Fs = sampleRate;
				for(auto& o: osc)
					o.prepare(sampleRate);
				env.prepare(Fs);
				retuneSpeedSmooth.makeFromDecayInMs(retuneSpeed, Fs);
				widthSmooth.makeFromDecayInMs(10., Fs);
				widthBuf.resize(blockSize);
			}
			
			void setParameters(double _noteOffset, double _width, double _retuneSpeed,
				double _attack, double _decay, double _sustain, double _release) noexcept
			{
				noteOffset = _noteOffset;
				width = _width * .5;
				if (retuneSpeed != _retuneSpeed)
				{
					retuneSpeed = _retuneSpeed;
					retuneSpeedSmooth.makeFromDecayInMs(retuneSpeed, Fs);
				}
				attack = _attack;
				env.attack = attack;
				decay = _decay;
				env.decay = decay;
				release = _release;
				env.release = release;
				sustain = _sustain;
				env.sustain = sustain;
			}

			void operator()(Buffer& buffer, const juce::MidiBuffer& midi,
				int numChannels, int numSamples) noexcept
			{
				auto bufEnv = buffer[2].data();

				{ // SYNTHESIZE MIDI NOTE VALUES (0-127), PITCHBEND AND ENVELOPE
					auto bufNotes = buffer[1].data();
					auto currentValue = noteValue + pitchbendValue;
					if (midi.isEmpty())
					{
						for (auto s = 0; s < numSamples; ++s)
						{
							bufNotes[s] = currentValue;
							bufEnv[s] = env();
						}
					}	
					else
					{
						auto evt = midi.begin();
						auto ref = *evt;
						auto ts = ref.samplePosition;
						for (auto s = 0; s < numSamples; ++s)
						{
							if (ts > s)
							{
								bufNotes[s] = currentValue;
								bufEnv[s] = env();
							}
							else
							{
								bool noteOn = env.noteOn;
								while (ts == s)
								{
									auto msg = ref.getMessage();
									if (msg.isNoteOn())
									{
										noteValue = static_cast<double>(msg.getNoteNumber());
										currentValue = noteValue + pitchbendValue;
										noteOn = true;
										env.retrig();
									}
									else if (msg.isNoteOff())
									{
										if(static_cast<int>(noteValue) == msg.getNoteNumber())
											noteOn = false;
									}
									else if (msg.isPitchWheel())
									{
										const auto pwv = msg.getPitchWheelValue();
										pitchbendValue = static_cast<double>(pwv) * PBGain - 1.;
										currentValue = noteValue + pitchbendValue;
									}
									++evt;
									if (evt == midi.end())
 										ts = numSamples;
									else
									{
										ref = *evt;
										ts = ref.samplePosition;
									}
								}
								bufNotes[s] = currentValue;
								bufEnv[s] = env(noteOn);
							}
						}
					}
				}
				{ // ADD OCT+SEMI+FINE SHIFT TO MIDI NOTE VALUE
					for (auto s = 0; s < numSamples; ++s)
						buffer[1][s] += noteOffset;
				}
				{ // CONVERT MIDI NOTE VALUES TO FREQUENCIES HZ
					for (auto s = 0; s < numSamples; ++s)
					{
						const auto midiN = buffer[1][s];
						const auto freq = 440. * std::pow(2., (midiN - 69.) * .083333333333);
						buffer[1][s] = juce::jlimit(1., 22049., freq);
					}
				}
				// PROCESS RETUNE SPEED OF OSC (FILTER CUTOFF)
				auto retuningNow = retuneSpeedSmooth(buffer[1].data(), numSamples);
#if DebugAudioRateEnv
				{ // COPY ENVELOPE ONLY
					for(auto ch = 0; ch < numChannelsOut; ++ch)
						for (auto s = 0; s < numSamples; ++s)
							buffer[ch][s] = bufEnv[s];
				}
#else
				{ // SYNTHESIZE OSCILLATOR
					if(numChannels == 1)
					{ // channel 0
						auto& osci = osc[0];
						auto buf = buffer[0].data();

						if(retuningNow)
							for (auto s = 0; s < numSamples; ++s)
							{
								const auto freq = buffer[1][s];
								osci.setFrequencyHz(freq);
								buf[s] = osci() * bufEnv[s];
							}
						else
						{
							const auto freq = buffer[1][0];
							osci.setFrequencyHz(freq);
							for (auto s = 0; s < numSamples; ++s)
								buf[s] = osci() * bufEnv[s];
						}
					}
					else
					{ // PROCESS STEREO WIDTH
						auto& osciL = osc[0];
						auto& osciR = osc[1];
						auto bufL = buffer[0].data();
						auto bufR = buffer[1].data();
						
						auto smoothingWidth = widthSmooth(widthBuf.data(), width, numSamples);

						if (retuningNow)
							if(smoothingWidth)
								for (auto s = 0; s < numSamples; ++s)
								{
									const auto freq = bufR[s];
									osciL.setFrequencyHz(freq);
									bufL[s] = osciL() * bufEnv[s];
									bufR[s] = osciR.withPhaseOffset(osciL, widthBuf[s] * bufEnv[s]);
								}
							else
								for (auto s = 0; s < numSamples; ++s)
								{
									const auto freq = bufR[s];
									osciL.setFrequencyHz(freq);
									bufL[s] = osciL() * bufEnv[s];
									bufR[s] = osciR.withPhaseOffset(osciL, width * bufEnv[s]);
								}
						else
						{
							const auto freq = bufR[0];
							osciL.setFrequencyHz(freq);
							if (smoothingWidth)
								for (auto s = 0; s < numSamples; ++s)
								{
									bufL[s] = osciL() * bufEnv[s];
									bufR[s] = osciR.withPhaseOffset(osciL, widthSmooth(width)) * bufEnv[s];
								}
							else
								for (auto s = 0; s < numSamples; ++s)
								{
									bufL[s] = osciL() * bufEnv[s];
									bufR[s] = osciR.withPhaseOffset(osciL, width) * bufEnv[s];
								}
						}
					}
				}
#endif
			}
			
		protected:
			SmoothD retuneSpeedSmooth, widthSmooth;
			std::vector<double> widthBuf;
			
			std::vector<Osc> osc;
			EnvGen env;
			double noteValue, pitchbendValue;

			double noteOffset, width, retuneSpeed, attack, decay, sustain, release;
			double Fs;
		};

		class Dropout
		{
			static constexpr double FreqCoeff = Pi * 10. * 10. * 10. * 10. * 10.;
		
		public:
			Dropout() :
				widthSmooth(0.),
				widthBuf(),
				
				phasor(),
				decay(1.), spin(1.), freqChance(0.), freqSmooth(0.), width(0.),
				rand(),

				accel{ 0., 0. },
				speed{ 0., 0. },
				dest{ 0., 0. },
				env{ 0., 0. },
				smooth{ 0., 0. },
				fs(44100.), dcy(1.), spinV(420.)
			{}
			
			void prepare(double sampleRate, int blockSize)
			{
				fs = sampleRate;

				for (auto& p : phasor)
				{
					p.prepare(fs);
					p.setFrequencyMs(freqChance);
				}
				
				dcy = 1. - 1. / (decay * fs * .001);
				for (auto& p : phasor)
					p.setFrequencyMs(freqChance);
				
				for (auto& s : smooth)
					s.makeFromFreqInHz(freqSmooth, fs);
				
				spinV = 1. / (fs * FreqCoeff / (spin * spin));
				
				widthSmooth.makeFromDecayInMs(10., fs);
				widthBuf.resize(blockSize);
			}
			
			void setParameters(double _decay, double _spin,
				double _freqChance, double _freqSmooth, double _width) noexcept
			{
				if (decay != _decay)
				{
					decay = _decay;
					dcy = 1. - 1. / (decay * fs * .001);
				}
				if (spin != _spin)
				{
					spin = _spin;
					const auto spin2 = spin * spin;
					spinV = 1. / (fs * FreqCoeff / spin2);
				}
				if (freqChance != _freqChance)
				{
					freqChance = _freqChance;
					for (auto& p : phasor)
						p.setFrequencyMs(freqChance);

				}
				freqSmooth = _freqSmooth;
				width = _width;
			}
			
			void operator()(Buffer& buffer, int numChannels, int numSamples) noexcept
			{
				{ // FILL BUFFERS WITH IMPULSES
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto& phasr = phasor[ch];
						auto impulseBuf = buffer[ch].data();
						juce::FloatVectorOperations::fill(impulseBuf, 0., numSamples);
						for (auto s = 0; s < numSamples; ++s)
							if (phasr())
								impulseBuf[s] = 2. * (rand.nextDouble() - .5);
					}
				}
				
				{ // SYNTHESIZE MOD
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto buf = buffer[ch].data();
						
						{
							auto& ac = accel[ch];
							auto& velo = speed[ch];
							auto& dst = dest[ch];
							auto& en = env[ch];

							for (auto s = 0; s < numSamples; ++s)
							{
								auto& smpl = buf[s];
								if (smpl != 0.)
								{
									en = smpl;
									dst = 0.;
									ac = 0.;
									velo = 0.;
								}
								const auto dist = dst - en;
								const auto direc = dist < 0. ? -1. : 1.;

								ac = (velo * direc + dist) * spinV;
								velo += ac;
								en = (en + velo) * dcy;
								smpl = en;
							}
						}
						
						for (auto s = 0; s < numSamples; ++s)
						{
							auto& smpl = buf[s];
							smpl = approx::tanh(PiHalf * smpl * smpl * smpl);
						}

						smooth[ch].makeFromFreqInHz(freqSmooth, fs);
						for (auto s = 0; s < numSamples; ++s)
						{
							auto& smpl = buf[s];
							smpl = static_cast<float>(smooth[ch](smpl));
						}	
					}
				}
				
				if (numChannels == 2)
				{
					const auto widthSmoothing = widthSmooth(widthBuf.data(), width, numSamples);
					
					if(widthSmoothing)
						for (auto s = 0; s < numSamples; ++s)
							buffer[1][s] = buffer[0][s] + widthBuf[s] * (buffer[1][s] - buffer[0][s]);
					else
						for (auto s = 0; s < numSamples; ++s)
							buffer[1][s] = buffer[0][s] + width * (buffer[1][s] - buffer[0][s]);
				}
			}
			
		protected:
			SmoothD widthSmooth;
			std::vector<double> widthBuf;

			std::array<Phasor<double>, 2> phasor;
			double decay, spin, freqChance, freqSmooth, width;
			juce::Random rand;

			std::array<double, 2> accel, speed, dest, env;
			std::array<SmoothD, 2> smooth;
			double fs, dcy, spinV;
		};

		struct EnvFol
		{
			
			EnvFol() :
				gainSmooth(0.), widthSmooth(0.),
				widthBuf(),

				envelope{ 0., 0. },
				envSmooth(),
				Fs(1.),

				attackInMs(-1.), releaseInMs(-1.), gain(-420.),

				attackV(1.), releaseV(1.), gainV(1.), widthV(0.),
				autogainV(1.)
			{}
			
			void prepare(double sampleRate, int blockSize)
			{
				Fs = sampleRate;
				gainSmooth.makeFromDecayInMs(10., Fs);
				widthSmooth.makeFromDecayInMs(10., Fs);
				widthBuf.resize(blockSize);
				for (auto ch = 0; ch < 2; ++ch)
					envSmooth[ch].makeFromDecayInMs(20., Fs);
				{
					const auto inSamples = attackInMs * Fs * .001;
					attackV = 1. / inSamples;
					updateAutogainV();
				}
				{
					const auto inSamples = releaseInMs * Fs * .001;
					releaseV = 1. / inSamples;
					updateAutogainV();
				}
				updateAutogainV();
			}
			
			void setParameters(double _attackInMs, double _releaseInMs, double _gain, double _width) noexcept
			{
				if (attackInMs != _attackInMs)
				{
					attackInMs = _attackInMs;
					const auto inSamples = attackInMs * Fs * .001;
					attackV = 1. / inSamples;
					updateAutogainV();
				}
				if (releaseInMs != _releaseInMs)
				{
					releaseInMs = _releaseInMs;
					const auto inSamples = releaseInMs * Fs * .001;
					releaseV = 1. / inSamples;
					updateAutogainV();
				}
				if (gain != _gain)
				{
					gain = _gain;
					gainV = juce::Decibels::decibelsToGain(gain);
				}
				widthV = _width;
			}
			
			void operator()(Buffer& buffer, const double* const* samples,
				int numChannels, int numSamples) noexcept
			{
				auto gainBuf = buffer[2].data();
				{ // PROCESS GAIN SMOOTH
					const auto gVal = gainV * autogainV;
					const auto gainSmoothing = gainSmooth(gainBuf, gVal, numSamples);
					if(!gainSmoothing)
						juce::FloatVectorOperations::fill(gainBuf, gVal, numSamples);
				}
				{ // SYNTHESIZE ENVELOPE FROM SAMPLES
					{
						auto& env = envelope[0];
						const auto smpls = samples[0];
						auto buf = buffer[0].data();

						for (auto s = 0; s < numSamples; ++s)
						{
							const auto smpl = gainBuf[s] * smpls[s] * smpls[s];
							if (env < smpl)
								env += attackV * (smpl - env);
							else
								env += releaseV * (smpl - env);
							buf[s] = env;
						}
					}
					if (numChannels == 2)
					{
						auto& env = envelope[1];
						const auto smpls = samples[1];
						auto buf = buffer[1].data();

						for (auto s = 0; s < numSamples; ++s)
						{
							const auto smpl = gainBuf[s] * smpls[s] * smpls[s];
							if (env < smpl)
								env += attackV * (smpl - env);
							else
								env += releaseV * (smpl - env);
							buf[s] = env;
						}

						{ // PROCESS WIDTH
							if (numChannels != 2)
								return;
							
							auto widthSmoothing = widthSmooth(widthBuf.data(), widthV, numSamples);

							if (widthSmoothing)
									for (auto s = 0; s < numSamples; ++s)
										buffer[1][s] = buffer[0][s] + widthBuf[s] * (buffer[1][s] - buffer[0][s]);
							else
								for (auto s = 0; s < numSamples; ++s)
									buffer[1][s] = buffer[0][s] + widthV * (buffer[1][s] - buffer[0][s]);
						}
					}
				}
				{ // PROCESS ANTI-CLIPPING
					for (auto ch = 0; ch < numChannels; ++ch)
					{
						auto buf = buffer[ch].data();
						auto& smooth = envSmooth[ch];

						for (auto s = 0; s < numSamples; ++s)
							buf[s] = smooth(buf[s] < 1. ? buf[s] : 1.);
					}
				}
			}
			
		protected:
			SmoothD gainSmooth, widthSmooth;
			std::vector<double> widthBuf;

			std::array<double, 2> envelope;
			std::array<SmoothD, 2> envSmooth;
			double Fs;

			double attackInMs, releaseInMs, gain;

			double attackV, releaseV, gainV, widthV;
			double autogainV;

			void updateAutogainV() noexcept
			{
				autogainV = attackV != 0. ? 1. + std::sqrt(releaseV / attackV) : 1.;
			}
		};

		struct Macro
		{
			Macro() :
				smooth(0.),
				macro(0.)
			{}
			
			void prepare(double sampleRate) noexcept
			{
				smooth.makeFromDecayInMs(8., sampleRate);
			}
			
			void setParameters(double _macro) noexcept
			{
				macro = _macro;
			}
			
			void operator()(Buffer& buffer, int numChannels, int numSamples) noexcept
			{
				const auto smoothing = smooth(buffer[0].data(), macro, numSamples);
				if(!smoothing)
					juce::FloatVectorOperations::fill(buffer[0].data(), macro, numSamples);

				if (numChannels != 2)
					return;
				
				juce::FloatVectorOperations::copy(buffer[1].data(), buffer[0].data(), numSamples);
			}
			
		protected:
			SmoothD smooth;
			double macro;
		};

		struct Pitchbend
		{
			Pitchbend() :
				smooth(0.),
				fs(1.),
				bendV(0.),
				smoothRate(0.),
				pitchbend(0)
			{}
			
			void prepare(double sampleRate) noexcept
			{
				fs = sampleRate;
			}
			
			void setParameters(double _smoothRate) noexcept
			{
				smoothRate = _smoothRate;
			}
			
			void operator()(Buffer& buffer, int numChannels, int numSamples,
				const juce::MidiBuffer& midiBuffer) noexcept
			{
				{ // UPDATE MIDI DATA
					auto s = 0;
					for (auto midi : midiBuffer)
					{
						auto msg = midi.getMessage();
						if (msg.isPitchWheel())
						{
							auto ts = midi.samplePosition;
							while (s < ts)
							{
								buffer[0][s] = bendV;
								++s;
							}
							const auto pb = msg.getPitchWheelValue();
							if (pitchbend != pb)
							{
								pitchbend = pb;
								static constexpr double PBCoeff = 2. / static_cast<double>(0x3fff);
								bendV = static_cast<double>(pitchbend) * PBCoeff - 1.;
							}
						}
					}
					while (s < numSamples)
					{
						buffer[0][s] = bendV;
						++s;
					}
				}
				{ // SMOOTHING
					smooth.makeFromDecayInMs(smoothRate, fs);
					for (auto s = 0; s < numSamples; ++s)
						buffer[0][s] = smooth(buffer[0][s]);
				}
				if (numChannels == 2)
					juce::FloatVectorOperations::copy(buffer[1].data(), buffer[0].data(), numSamples);
			}
		
		protected:
			SmoothD smooth;
			
			double fs;

			double bendV, smoothRate;
			int pitchbend;
		};
		
		struct LFO
		{
			LFO(const Tables& _tables) :
				lfo(_tables),
				rateHz(0.),
				rateSync(0.),
				phase(0.f),
				width(0.f),
				wtPos(0.f),
				temposync(false)
			{}
			
			/* fs, blockSize, latency, oversamplingFactor */
			void prepare(double fs, int blockSize, double latency, int oversamplingFactor)
			{
				lfo.prepare
				(
					fs,
					blockSize,
					latency,
					oversamplingFactor
				);
			}
			
			/* temposync, rateHz, rateSync, wtPos[0,1], phase[0,.5], width[0,.5] */
			void setParameters(bool _temposync, double _rateHz, double _rateSync,
				double _wtPos, double _phase, double _width) noexcept
			{
				temposync = _temposync;
				rateHz = _rateHz;
				rateSync = 1. / _rateSync;
				wtPos = _wtPos;
				phase = _phase;
				width = _width;
			}
			
			void operator()(Buffer& buffer, int numChannels, int numSamples,
				PosInfo& transport) noexcept
			{
				double* samples[] = { buffer[0].data(), buffer[1].data() };

				lfo
				(
					samples,
					numChannels,
					numSamples,
					transport,
					rateHz,
					rateSync,
					phase,
					width,
					wtPos,
					temposync
				);
			}
		
		protected:
			dsp::LFO_Procedural lfo;
			double rateHz, rateSync;
			double phase, width, wtPos;
			bool temposync;
		};

	public:
		Modulator() :
			buffer(),
			standalonePlayHead(),
			tables(),
			perlin(),
			audioRate(),
			dropout(),
			envFol(),
			macro(),
			pitchbend(),
			lfo(tables),
			
			type(ModType::Perlin)
		{
			tables.makeTablesWeierstrass();
		}

		void loadPatch(juce::ValueTree& state, int mIdx)
		{
			const juce::Identifier id(toString(ObjType::Wavetable) + juce::String(mIdx));
			const auto child = state.getChildWithName(id);
			if (child.isValid())
			{
				const auto tableType = child.getProperty(id).toString();
				if (tableType == toString(dsp::TableType::Weierstrass))
					tables.makeTablesWeierstrass();
				else if (tableType == toString(dsp::TableType::Tri))
					tables.makeTablesTriangles();
				else if (tableType == toString(dsp::TableType::Sinc))
					tables.makeTablesSinc();
			}
		}

		void savePatch(juce::ValueTree& state, int mIdx)
		{
			const juce::Identifier id(toString(ObjType::Wavetable) + juce::String(mIdx));
			auto child = state.getChildWithName(id);
			if (!child.isValid())
			{
				child = juce::ValueTree(id);
				state.appendChild(child, nullptr);
			}
			child.setProperty(id, tables.name, nullptr);
		}

		void setType(ModType t) noexcept { type = t; }
		
		void prepare(double sampleRate, int maxBlockSize, int latency, int oversamplingFactor)
		{
			standalonePlayHead.prepare(sampleRate);
			for(auto& b: buffer)
				b.resize(maxBlockSize + 4, 0.f); // compensate for potential spline interpolation
			perlin.prepare(sampleRate, maxBlockSize, latency);
			audioRate.prepare(sampleRate, maxBlockSize);
			dropout.prepare(sampleRate, maxBlockSize);
			envFol.prepare(sampleRate, maxBlockSize);
			macro.prepare(sampleRate);
			pitchbend.prepare(sampleRate);
			lfo.prepare(sampleRate, maxBlockSize, static_cast<double>(latency), oversamplingFactor);
		}

		// parameters
		void setParametersPerlin(double _rateHz, double _rateBeats,
			double _octaves, double _width, double _phs,
			perlin::Shape _shape, bool _temposync, bool _procedural) noexcept
		{
			perlin.setParameters(_rateHz, _rateBeats, _octaves, _width, _phs, _shape, _temposync, _procedural);
		}
		
		void setParametersAudioRate(double oct, double semi, double fine, double width, double retuneSpeed,
			double attack, double decay, double sustain, double release) noexcept
		{
			const auto noteOffset = oct + semi + fine;
			audioRate.setParameters(noteOffset, width, retuneSpeed, attack, decay, sustain, release);
		}
		
		void setParametersDropout(double decay, double spin, double freqChance, double freqSmooth, double width) noexcept
		{
			dropout.setParameters(decay, spin, freqChance, freqSmooth, width);
		}
		
		void setParametersEnvFol(double atk, double rls, double gain, double width) noexcept
		{
			envFol.setParameters(atk, rls, gain, width);
		}
		
		void setParametersMacro(double m) noexcept
		{
			macro.setParameters(m);
		}
		
		void setParametersPitchbend(double rate) noexcept
		{
			pitchbend.setParameters(rate);
		}
		
		void setParametersLFO(bool isSync, double rateFree, double rateSync, double waveform, double phase, double width) noexcept
		{
			lfo.setParameters(isSync, rateFree, rateSync, waveform, phase, width);
		}

		void processBlock(const double* const* samples, const juce::MidiBuffer& midi,
			juce::AudioPlayHead* playHead, int numChannels, int numSamples) noexcept
		{
			if (juce::JUCEApplicationBase::isStandaloneApp())
			{
				standalonePlayHead(numSamples);
			}
			else
			{
				if (playHead)
					playHead->getCurrentPosition(standalonePlayHead.posInfo);
			}

			switch (type)
			{
			case ModType::Perlin: return perlin(buffer, numChannels, numSamples, standalonePlayHead.posInfo);
			case ModType::AudioRate: return audioRate(buffer, midi, numChannels, numSamples);
			case ModType::Dropout: return dropout(buffer, numChannels, numSamples);
			case ModType::EnvFol: return envFol(buffer, samples, numChannels, numSamples);
			case ModType::Macro: return macro(buffer, numChannels, numSamples);
			case ModType::Pitchwheel: return pitchbend(buffer, numChannels, numSamples, midi);
			case ModType::LFO: return lfo(buffer, numChannels, numSamples, standalonePlayHead.posInfo);
			}
		}
		
		Tables& getTables() noexcept
		{
			return tables;
		}
		
		const Tables& getTables() const noexcept
		{
			return tables;
		}

		Buffer buffer;
	protected:
		dsp::StandalonePlayHead standalonePlayHead;
		Tables tables;

		Perlin perlin;
		AudioRate audioRate;
		Dropout dropout;
		EnvFol envFol;
		Macro macro;
		Pitchbend pitchbend;
		LFO lfo;

		ModType type;
	};
}

/*

pitchbend range define globally or so?
	useful for audiorate mod

dropout
	another dropout mode (biased smoothed random values)
	temposync?

lfo mod
	improve wavetable browser
		to make room for more wavetables
		import wavetables
		wavetable editor

make trigger mod
	trigger type (midi note, automation(&button), onset envelope)
	waveform (sinc, tapestop, tapestart, dropout)

envFol
	lookahead?

make konami mod

*/

#undef DebugAudioRateEnv
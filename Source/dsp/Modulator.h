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
	static constexpr float Tau = 6.283185307179586476925286766559f;

	using SmoothF = smooth::Smooth<float>;
	using SmoothD = smooth::Smooth<double>;

	enum class ObjType
	{
		ModType,
		InterpolationType,
		DelaySize,
		Wavetable,
		NumTypes
	};
	
	inline juce::String toString(ObjType t)
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
	
	inline juce::String with(ObjType t, int i)
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
	
	inline juce::String toString(ModType t)
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
	
	inline juce::String with(ModType t, int i)
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
			attack(1.f), decay(1.f), sustain(1.f), release(1.f),

			Fs(1.f), env(0.f),
			state(State::R),
			noteOn(false),

			smooth(0.f)
		{}
		
		void prepare(float sampleRate)
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
		
		float operator()(bool n) noexcept
		{
			noteOn = n;
			return processSample();
		}

		float operator()() noexcept
		{
			return processSample();
		}

		void retrig() noexcept
		{
			state = State::A;
			smooth.makeFromDecayInMs(attack, Fs);
		}

		float attack, decay, sustain, release;

		float Fs, env;
		State state;
		bool noteOn;
	protected:
		SmoothF smooth;

		float processSample() noexcept
		{
			switch (state)
			{
			case State::A:
				if (noteOn)
					if (env < .999f)
					{
						env = smooth(1.f);
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
					env = smooth(0.f);
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
		static constexpr float SafetyCoeff = .99f;
		using Buffer = std::array<std::vector<float>, 4>;
		using BeatsData = modSys6::BeatsData;
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
				octaves(1.f), width(0.f), phs(0.f),
				shape(Shape::Spline),
				temposync(false), procedural(true)
			{
			}

			void prepare(float sampleRate, int blockSize, int latency)
			{
				perlin.prepare(sampleRate, blockSize, latency);
			}

			void setParameters(double _rateHz, double _rateBeats,
				float _octaves, float _width, float _phs,
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

			void operator()(Buffer& buffer, int numChannelsOut, int numSamples,
				const PosInfo& transport) noexcept
			{
				float* samples[2] = { buffer[0].data(), buffer[1].data() };
				
				perlin
				(
					samples,
					numChannelsOut,
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
			float octaves, width, phs;
			Shape shape;
			bool temposync, procedural;
		};

		class AudioRate
		{
			static constexpr float PBGain = 2.f / static_cast<float>(0x3fff);

			struct Osc
			{
				Osc() :
					phasor()
				{}
				
				void prepare(float sampleRate) noexcept
				{
					phasor.prepare(sampleRate);
				}
				
				void setFrequencyHz(float f) noexcept
				{
					phasor.setFrequencyHz(f);
				}
				
				float operator()() noexcept
				{
					phasor();
					return std::cos(Tau * phasor.phase);
				}
				
				inline float withPhaseOffset(Osc& other, float offset) noexcept
				{
					const auto phase = other.phasor.phase + offset;
					return std::cos(Tau * phase);
				}

				Phasor<float> phasor;
			};
			
		public:
			AudioRate() :
				retuneSpeedSmooth(0.f),
				widthSmooth(0.f),
				widthBuf(),
				
				osc(),
				env(),

				noteValue(0.f), pitchbendValue(0.f),

				noteOffset(0.f), width(0.f), retuneSpeed(0.f),
				attack(1.f), decay(1.f), sustain(1.f), release(1.f),

				Fs(1.f)
			{
				osc.resize(2);
			}
			
			void prepare(float sampleRate, int blockSize)
			{
				Fs = sampleRate;
				for(auto& o: osc)
					o.prepare(sampleRate);
				env.prepare(Fs);
				retuneSpeedSmooth.makeFromDecayInMs(retuneSpeed, Fs);
				widthSmooth.makeFromDecayInMs(10.f, Fs);
				widthBuf.resize(blockSize);
			}
			
			void setParameters(float _noteOffset, float _width, float _retuneSpeed,
				float _attack, float _decay, float _sustain, float _release) noexcept
			{
				noteOffset = _noteOffset;
				width = _width * .5f;
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

			void operator()(Buffer& buffer, const juce::MidiBuffer& midi, int numChannels, int numSamples) noexcept
			{
				auto& bufEnv = buffer[2];

				{ // SYNTHESIZE MIDI NOTE VALUES (0-127), PITCHBEND AND ENVELOPE
					auto& bufNotes = buffer[1];
					auto currentValue = noteValue + pitchbendValue;
					if (midi.isEmpty())
					{
						for (auto s = 0; s < numSamples; ++s)
						{
							bufNotes[s] = currentValue;
							bufEnv[s] = env() * SafetyCoeff;
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
								bufEnv[s] = env() * SafetyCoeff;
							}
							else
							{
								bool noteOn = env.noteOn;
								while (ts == s)
								{
									auto msg = ref.getMessage();
									if (msg.isNoteOn())
									{
										noteValue = static_cast<float>(msg.getNoteNumber());
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
										pitchbendValue = static_cast<float>(pwv) * PBGain - 1.f;
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
								bufEnv[s] = env(noteOn) * SafetyCoeff;
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
						const auto freq = 440.f * std::pow(2.f, (midiN - 69.f) * .083333333333f);
						buffer[1][s] = juce::jlimit(1.f, 22049.f, freq);
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
			SmoothF retuneSpeedSmooth, widthSmooth;
			std::vector<float> widthBuf;
			
			std::vector<Osc> osc;
			EnvGen env;
			float noteValue, pitchbendValue;

			float noteOffset, width, retuneSpeed, attack, decay, sustain, release;
			float Fs;
		};

		class Dropout
		{
			static constexpr float pi = 3.14159265359f;
			static constexpr float piHalf = pi * .5f;
			static constexpr float FreqCoeff = pi * 10.f * 10.f * 10.f * 10.f * 10.f;
		
		public:
			Dropout() :
				widthSmooth(0.f),
				widthBuf(),
				
				phasor(),
				decay(1.f), spin(1.f), freqChance(0.f), freqSmooth(0.f), width(0.f),
				rand(),

				accel{ 0.f, 0.f },
				speed{ 0.f, 0.f },
				dest{ 0.f, 0.f },
				env{ 0.f, 0.f },
				smooth{ 0.f, 0.f },
				fs(44100.f), dcy(1.f), spinV(420.f)
			{}
			
			void prepare(float sampleRate, int blockSize)
			{
				fs = sampleRate;

				for (auto& p : phasor)
				{
					p.prepare(fs);
					p.setFrequencyMs(freqChance);
				}
				
				dcy = 1.f - 1.f / (decay * fs * .001f);
				for (auto& p : phasor)
					p.setFrequencyMs(freqChance);
				
				for (auto& s : smooth)
					s.makeFromFreqInHz(freqSmooth, fs);
				
				spinV = 1.f / (fs * FreqCoeff / (spin * spin));
				
				widthSmooth.makeFromDecayInMs(10.f, fs);
				widthBuf.resize(blockSize);
			}
			
			void setParameters(float _decay, float _spin,
				float _freqChance, float _freqSmooth, float _width) noexcept
			{
				if (decay != _decay)
				{
					decay = _decay;
					dcy = 1.f - 1.f / (decay * fs * .001f);
				}
				if (spin != _spin)
				{
					spin = _spin;
					const auto spin2 = spin * spin;
					spinV = 1.f / (fs * FreqCoeff / spin2);
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
						juce::FloatVectorOperations::fill(impulseBuf, 0.f, numSamples);
						for (auto s = 0; s < numSamples; ++s)
							if (phasr())
								impulseBuf[s] = 2.f * (rand.nextFloat() - .5f);
					}
				}
				
				{ // SYNTHESIZE MOD
					const auto freqSmoothD = static_cast<double>(freqSmooth);
					const auto fsD = static_cast<double>(fs);

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
								if (smpl != 0.f)
								{
									en = smpl;
									dst = 0.f;
									ac = 0.f;
									velo = 0.f;
								}
								const auto dist = dst - en;
								const auto direc = dist < 0.f ? -1.f : 1.f;

								ac = (velo * direc + dist) * spinV;
								velo += ac;
								en = (en + velo) * dcy;
								smpl = en;
							}
						}
						
						for (auto s = 0; s < numSamples; ++s)
						{
							auto& smpl = buf[s];
							smpl = approx::tanh(piHalf * smpl * smpl * smpl);
						}

						smooth[ch].makeFromFreqInHz(freqSmoothD, fsD);
						for (auto s = 0; s < numSamples; ++s)
						{
							auto& smpl = buf[s];
							smpl = static_cast<float>(smooth[ch](static_cast<double>(smpl)));
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
			SmoothF widthSmooth;
			std::vector<float> widthBuf;

			std::array<Phasor<float>, 2> phasor;
			float decay, spin, freqChance, freqSmooth, width;
			juce::Random rand;

			std::array<float, 2> accel, speed, dest, env;
			std::array<SmoothD, 2> smooth;
			float fs, dcy, spinV;
		};

		struct EnvFol
		{
			
			EnvFol() :
				gainSmooth(0.f), widthSmooth(0.f),
				widthBuf(),

				envelope{ 0.f, 0.f },
				envSmooth(),
				Fs(1.f),

				attackInMs(-1.f), releaseInMs(-1.f), gain(-420.f),

				attackV(1.f), releaseV(1.f), gainV(1.f), widthV(0.f),
				autogainV(1.f)
			{}
			
			void prepare(float sampleRate, int blockSize)
			{
				Fs = sampleRate;
				gainSmooth.makeFromDecayInMs(10.f, Fs);
				widthSmooth.makeFromDecayInMs(10.f, Fs);
				widthBuf.resize(blockSize);
				for (auto ch = 0; ch < 2; ++ch)
					envSmooth[ch].makeFromDecayInMs(20.f, Fs);
				{
					const auto inSamples = attackInMs * Fs * .001f;
					attackV = 1.f / inSamples;
					updateAutogainV();
				}
				{
					const auto inSamples = releaseInMs * Fs * .001f;
					releaseV = 1.f / inSamples;
					updateAutogainV();
				}
				updateAutogainV();
			}
			
			void setParameters(float _attackInMs, float _releaseInMs, float _gain, float _width) noexcept
			{
				if (attackInMs != _attackInMs)
				{
					attackInMs = _attackInMs;
					const auto inSamples = attackInMs * Fs * .001f;
					attackV = 1.f / inSamples;
					updateAutogainV();
				}
				if (releaseInMs != _releaseInMs)
				{
					releaseInMs = _releaseInMs;
					const auto inSamples = releaseInMs * Fs * .001f;
					releaseV = 1.f / inSamples;
					updateAutogainV();
				}
				if (gain != _gain)
				{
					gain = _gain;
					gainV = juce::Decibels::decibelsToGain(gain);
				}
				widthV = _width;
			}
			
			void operator()(Buffer& buffer, const float* const* samples, int numChannels, int numSamples) noexcept
			{
				auto gainBuf = buffer[2].data();
				{ // PROCESS GAIN SMOOTH
					auto gainSmoothing = gainSmooth(gainBuf, gainV * autogainV, numSamples);
					
					if(!gainSmoothing)
						juce::FloatVectorOperations::fill(gainBuf, gainV * autogainV, numSamples);
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
							buf[s] = smooth(buf[s] < 1.f ? buf[s] : 1.f);
					}
				}
			}
			
		protected:
			SmoothF gainSmooth, widthSmooth;
			std::vector<float> widthBuf;

			std::array<float, 2> envelope;
			std::array<SmoothF, 2> envSmooth;
			float Fs;

			float attackInMs, releaseInMs, gain;

			float attackV, releaseV, gainV, widthV;
			float autogainV;

			void updateAutogainV() noexcept
			{
				autogainV = attackV != 0.f ? 1.f + std::sqrt(releaseV / attackV) : 1.f;
			}
		};

		struct Macro
		{
			Macro() :
				smooth(0.f),
				macro(0.f)
			{}
			
			void prepare(float sampleRate) noexcept
			{
				smooth.makeFromDecayInMs(8.f, sampleRate);
			}
			
			void setParameters(float _macro) noexcept
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
			SmoothF smooth;
			float macro;
		};

		struct Pitchbend
		{
			Pitchbend() :
				smooth(0.f),
				fs(1.f),
				bendV(0.f),
				smoothRate(0.f),
				pitchbend(0)
			{}
			
			void prepare(float sampleRate) noexcept
			{
				fs = sampleRate;
			}
			
			void setParameters(float _smoothRate) noexcept
			{
				smoothRate = _smoothRate;
			}
			
			void operator()(Buffer& buffer, int numChannels, int numSamples, const juce::MidiBuffer& midiBuffer) noexcept
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
								static constexpr float PBCoeff = 2.f / static_cast<float>(0x3fff);
								bendV = static_cast<float>(pitchbend) * PBCoeff - 1.f;
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
			SmoothF smooth;
			
			float fs;

			float bendV, smoothRate;
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
			
			/* fs, blockSize, latency */
			void prepare(float fs, int blockSize, int latency)
			{
				lfo.prepare(fs, blockSize, static_cast<double>(latency));
			}
			
			/* temposync, rateHz, rateSync, wtPos[0,1], phase[0,.5], width[0,.5] */
			void setParameters(bool _temposync, float _rateHz, float _rateSync,
				float _wtPos, float _phase, float _width) noexcept
			{
				temposync = _temposync;
				rateHz = _rateHz;
				rateSync = _rateSync;
				wtPos = _wtPos;
				phase = _phase;
				width = _width;
			}
			
			void operator()(Buffer& buffer, int numChannels, int numSamples,
				PosInfo& transport) noexcept
			{
				float* samples[] = { buffer[0].data(), buffer[1].data() };

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
			float phase, width, wtPos;
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
			tables.makeTablesWeierstrasz();
		}

		void loadPatch(juce::ValueTree& state, int mIdx)
		{
			const juce::Identifier id(toString(ObjType::Wavetable) + juce::String(mIdx));
			const auto child = state.getChildWithName(id);
			if (child.isValid())
			{
				const auto tableType = child.getProperty(id).toString();
				if (tableType == toString(dsp::TableType::Weierstrasz))
					tables.makeTablesWeierstrasz();
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
		
		void prepare(float sampleRate, int maxBlockSize, int latency)
		{
			standalonePlayHead.prepare(static_cast<double>(sampleRate));
			for(auto& b: buffer)
				b.resize(maxBlockSize + 4, 0.f); // compensate for potential spline interpolation
			perlin.prepare(sampleRate, maxBlockSize, latency);
			audioRate.prepare(sampleRate, maxBlockSize);
			dropout.prepare(sampleRate, maxBlockSize);
			envFol.prepare(sampleRate, maxBlockSize);
			macro.prepare(sampleRate);
			pitchbend.prepare(sampleRate);
			lfo.prepare(sampleRate, maxBlockSize, latency);
		}

		// parameters
		void setParametersPerlin(double _rateHz, double _rateBeats,
			float _octaves, float _width, float _phs,
			perlin::Shape _shape, bool _temposync, bool _procedural) noexcept
		{
			perlin.setParameters(_rateHz, _rateBeats, _octaves, _width, _phs, _shape, _temposync, _procedural);
		}
		
		void setParametersAudioRate(float oct, float semi, float fine, float width, float retuneSpeed,
			float attack, float decay, float sustain, float release) noexcept
		{
			const auto noteOffset = oct + semi + fine;
			audioRate.setParameters(noteOffset, width, retuneSpeed, attack, decay, sustain, release);
		}
		
		void setParametersDropout(float decay, float spin, float freqChance, float freqSmooth, float width) noexcept
		{
			dropout.setParameters(decay, spin, freqChance, freqSmooth, width);
		}
		
		void setParametersEnvFol(float atk, float rls, float gain, float width) noexcept
		{
			envFol.setParameters(atk, rls, gain, width);
		}
		
		void setParametersMacro(float m) noexcept
		{
			macro.setParameters(m);
		}
		
		void setParametersPitchbend(float rate) noexcept
		{
			pitchbend.setParameters(rate);
		}
		
		void setParametersLFO(bool isSync, float rateFree, float rateSync, float waveform, float phase, float width) noexcept
		{
			lfo.setParameters(isSync, rateFree, rateSync, waveform, phase, width);
		}

		void processBlock(const float* const* samples, const juce::MidiBuffer& midi,
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
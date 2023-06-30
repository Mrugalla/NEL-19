#pragma once
#include "../Interpolation.h"
#include "../modsys/ModSys.h"
#include "../Approx.h"
#include <random>
#include "Smooth.h"
#include "Perlin2.h"
#include "Wavetable.h"
#include "LFO2.h"
#include "Macro.h"
#include "EnvelopeFollower.h"

#define DebugAudioRateEnv false

namespace vibrato
{
	static constexpr double Tau = 6.283185307179586476925286766559;
	static constexpr double Pi = 3.1415926535897932384626433832795;
	static constexpr double PiHalf = Pi * .5;

	using SmoothD = smooth::Smooth<double>;
	using String = juce::String;
	using Identifier = juce::Identifier;
	using ValueTree = juce::ValueTree;
	using SIMD = juce::FloatVectorOperations;

	enum class ObjType
	{
		ModType,
		InterpolationType,
		DelaySize,
		Wavetable,
		PerlinSeed,
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
		case ObjType::PerlinSeed: return "PerlinSeed";
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
	
	inline ModType getModType(const String& str)
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
			using PlayHeadPos = perlin2::PlayHeadPos;
			using Shape = perlin2::Shape;

			Perlin() :
				perlin(),
				rateHz(1.), rateBeats(1.),
				octaves(1.), width(0.), phs(0.), bias(0.),
				shape(Shape::Spline),
				temposync(false)
			{
			}

			void prepare(double sampleRate, int blockSize, int latency, int osFactor)
			{
				perlin.prepare(sampleRate, blockSize, latency, osFactor);
			}

			void setParameters(double _rateHz, double _rateBeats,
				double _octaves, double _width, double _phs, double _bias,
				Shape _shape, bool _temposync) noexcept
			{
				rateHz = _rateHz;
				rateBeats = _rateBeats;
				octaves = _octaves;
				width = _width;
				phs = _phs;
				bias = _bias;
				shape = _shape;
				temposync = _temposync;
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
					bias,
					shape,
					temposync
				);
			}
		
			const int getSeed() const noexcept
			{
				return perlin.seed.load();
			}

			void setSeed(int s) noexcept
			{
				perlin.setSeed(s);
			}

		protected:
			perlin2::Perlin2 perlin;
			double rateHz, rateBeats;
			double octaves, width, phs, bias;
			Shape shape;
			bool temposync;
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

		struct EnvFol
		{
			EnvFol() :
				envFol(),
				attackMs(1.),
				releaseMs(1.),
				gain(1.),
				width(0.),
				cutoffHP(20.),
				scEnabled(false)
			{}
			
			void prepare(double sampleRate, int blockSize)
			{
				envFol.prepare(sampleRate, blockSize);
			}
			
			void setParameters(double _attackMs, double _releaseMs, double _gain, double _width, double _cutoffHP, bool _scEnabled) noexcept
			{
				attackMs = _attackMs;
				releaseMs = _releaseMs;
				gain = _gain;
				width = _width;
				cutoffHP = _cutoffHP;
				scEnabled = _scEnabled;
			}
			
			void operator()(Buffer& buffer, const double* const* samples, const double* const* samplesSC,
				int numChannels, int numSamples) noexcept
			{
				double* samplesIn[] = { buffer[0].data(), buffer[1].data() };
				for(auto ch = 0; ch < numChannels; ++ch)
					SIMD::copy(samplesIn[ch], samples[ch], numSamples);

				envFol(samplesIn, samplesSC, attackMs, releaseMs, gain, width, cutoffHP, numChannels, numSamples, scEnabled);
			}
			
		protected:
			envfol::EnvFol envFol;
			double attackMs, releaseMs, gain, width, cutoffHP, scEnabled;
		};

		struct Macro
		{
			Macro() :
				macaroni()
			{}
			
			void prepare(double sampleRate, int blockSize) noexcept
			{
				macaroni.prepare(sampleRate, blockSize);
			}
			
			void setParameters(double macro, double smoothingHz, double scGain) noexcept
			{
				macaroni.setParameters(macro, smoothingHz, scGain);
			}
			
			void operator()(Buffer& buffer, const double* const* scSamples,
				int numChannels, int numSamples) noexcept
			{
				double* samples[] = { buffer[0].data(), buffer[1].data() };

				macaroni(samples, scSamples, numChannels, numSamples);
			}
			
		protected:
			macro::Macro macaroni;
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
				const PosInfo& transport) noexcept
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
			tables(),
			perlin(),
			audioRate(),
			envFol(),
			macro(),
			pitchbend(),
			lfo(tables),
			
			type(ModType::Perlin)
		{
			tables.makeTablesWeierstrass();
		}
		
		void savePatch(ValueTree& state, int mIdx)
		{
			{
				const Identifier id(toString(ObjType::Wavetable) + String(mIdx));
				auto child = state.getChildWithName(id);
				if (!child.isValid())
				{
					child = ValueTree(id);
					state.appendChild(child, nullptr);
				}
				child.setProperty(id, tables.name, nullptr);
			}
			const auto firstTime = static_cast<bool>(state.getProperty("firstTimeUwU", true));
			if(firstTime)
			{
				const Identifier id(toString(ObjType::PerlinSeed) + String(mIdx));
				auto child = state.getChildWithName(id);
				if (!child.isValid())
				{
					child = ValueTree(id);
					state.appendChild(child, nullptr);
				}
				child.setProperty(id, perlin.getSeed(), nullptr);
			}
		}

		void loadPatch(ValueTree& state, int mIdx)
		{
			{
				const Identifier id(toString(ObjType::Wavetable) + String(mIdx));
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
			{
				const Identifier id(toString(ObjType::PerlinSeed) + String(mIdx));
				const auto child = state.getChildWithName(id);
				if (child.isValid())
				{
					const auto seed = static_cast<int>(child.getProperty(id));
					perlin.setSeed(seed);
				}
			}
		}

		void setType(ModType t) noexcept
		{
			type = t;
		}
		
		void prepare(double sampleRate, int maxBlockSize, int latency, int oversamplingFactor)
		{
			for(auto& b: buffer)
				b.resize(maxBlockSize + 4, 0.f); // compensate for potential spline interpolation
			perlin.prepare(sampleRate, maxBlockSize, latency, oversamplingFactor);
			audioRate.prepare(sampleRate, maxBlockSize);
			envFol.prepare(sampleRate, maxBlockSize);
			macro.prepare(sampleRate, maxBlockSize);
			pitchbend.prepare(sampleRate);
			lfo.prepare(sampleRate, maxBlockSize, static_cast<double>(latency), oversamplingFactor);
		}

		int getSeed() const noexcept
		{
			return perlin.getSeed();
		}

		// parameters
		void setParametersPerlin(double _rateHz, double _rateBeats,
			double _octaves, double _width, double _phs, double _bias,
			perlin2::Shape _shape, bool _temposync) noexcept
		{
			perlin.setParameters(_rateHz, _rateBeats, _octaves, _width, _phs, _bias, _shape, _temposync);
		}
		
		void setParametersAudioRate(double oct, double semi, double fine, double width, double retuneSpeed,
			double attack, double decay, double sustain, double release) noexcept
		{
			const auto noteOffset = oct + semi + fine;
			audioRate.setParameters(noteOffset, width, retuneSpeed, attack, decay, sustain, release);
		}
		
		void setParametersEnvFol(double atk, double rls, double gain, double width, bool scEnabled, double cutoffHP) noexcept
		{
			envFol.setParameters(atk, rls, gain, width, cutoffHP, scEnabled);
		}
		
		void setParametersMacro(double m, double smoothingHz, double scGain) noexcept
		{
			macro.setParameters(m, smoothingHz, scGain);
		}
		
		void setParametersPitchbend(double rate) noexcept
		{
			pitchbend.setParameters(rate);
		}
		
		void setParametersLFO(bool isSync, double rateFree, double rateSync, double waveform, double phase, double width) noexcept
		{
			lfo.setParameters(isSync, rateFree, rateSync, waveform, phase, width);
		}

		void processBlock(const double* const* samples, const double* const* samplesSC,
			const juce::MidiBuffer& midi, const PosInfo& transport,
			int numChannels, int numSamples) noexcept
		{
			switch (type)
			{
			case ModType::Perlin: return perlin(buffer, numChannels, numSamples, transport);
			case ModType::AudioRate: return audioRate(buffer, midi, numChannels, numSamples);
			case ModType::EnvFol: return envFol(buffer, samples, samplesSC, numChannels, numSamples);
			case ModType::Macro: return macro(buffer, samplesSC, numChannels, numSamples);
			case ModType::Pitchwheel: return pitchbend(buffer, numChannels, numSamples, midi);
			case ModType::LFO: return lfo(buffer, numChannels, numSamples, transport);
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
		Tables tables;

		Perlin perlin;
		AudioRate audioRate;
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

new modulator ideas (need 2 mods)
	step sequencer / performer (massive)
	onset detection
	spline
dimensions
	vibrato (delay) + feedback, damp
	tremolo (gain)
	drive (drive)
	balance (balance)
	lp4 (lowpass filter)
	bounce (frequency shifter)
vibrato method
	delay/allpass
post-modulator waveshapers
fixed blocksize

make konami mod

*/

#undef DebugAudioRateEnv
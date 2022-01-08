#pragma once
#include "Interpolation.h"
#include "../modsys/ModSys.h"
#include <random>
#define DebugAudioRateEnv false

namespace vibrato
{
	enum class ObjType
	{
		ModType, InterpolationType, DelaySize, Wavetable, NumTypes
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
		
		void setFrequencyMs(Float f) noexcept { inc = static_cast<Float>(1000) / (fs * f); }
		void setFrequencySecs(Float f) noexcept { inc = static_cast<Float>(1) / (fs * f); }
		void setFrequencyHz(Float f) noexcept { inc = fsInv * f; }
		
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

			smooth()
		{}
		void prepare(float sampleRate)
		{
			Fs = sampleRate;
			switch (state)
			{
			case State::A: modSys6::Smooth::makeFromDecayInMs(smooth, attack, Fs);
			case State::D: modSys6::Smooth::makeFromDecayInMs(smooth, decay, Fs);
			case State::R: modSys6::Smooth::makeFromDecayInMs(smooth, release, Fs);
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

		float attack, decay, sustain, release;

		float Fs, env;
		State state;
		bool noteOn;
	protected:
		modSys6::Smooth smooth;

		float processSample() noexcept
		{
			switch (state)
			{
			case State::A:
				if (noteOn)
					if (env < .99f)
						env = smooth(1.f);
					else
					{
						state = State::D;
						modSys6::Smooth::makeFromDecayInMs(smooth, decay, Fs);
					}
				else
				{
					state = State::R;
					modSys6::Smooth::makeFromDecayInMs(smooth, release, Fs);
				}
				break;
			case State::D:
				if (noteOn)
					env = smooth(sustain);
				else
				{
					state = State::R;
					modSys6::Smooth::makeFromDecayInMs(smooth, release, Fs);
				}
				break;
			case State::R:
				if (!noteOn)
					env = smooth(0.f);
				else
				{
					state = State::A;
					modSys6::Smooth::makeFromDecayInMs(smooth, attack, Fs);
				}
				break;
			}
			return env;
		}
	};

	template<size_t Size>
	struct Wavetable
	{
		using Func = std::function<float(float x)>;

		void makeTableWeierstrasz(float a, float b, int N)
		{
			fill([N, b, a](float x)
			{
				auto smpl = 0.f;
				for (auto n = 0; n < N; ++n)
				{
					const auto nF = static_cast<float>(n);
					smpl += std::pow(a, nF) * std::cos(std::pow(b, nF) * x * 3.14159265359f);
				}
				return smpl;
			}, true, true);
		}

		void makeTableTriangle(int n)
		{
			const auto pi = 3.14159265359f;
			const auto tri = [](float x, float fc, float phase)
			{
				return 1.f - 2.f * std::acos(std::cos(fc * x * pi + phase * pi)) / pi;
			};

			fill([tri, n](float x)
				{
					const auto nF = static_cast<float>(n + 1);
					const auto n8 = static_cast<float>(n) / 8.f;
					return tri(x, 1.f, 0.f) + tri(x, nF, n8) / nF;

				}, true, true
			);
		}

		void makeTableSinc(bool window, int N)
		{
			const auto pi = 3.14159265359f;

			const auto wndw = window ? [](float xpi)
			{
				if (xpi == 0.f) return 1.f;
				return std::sin(xpi) / xpi;
			} : [](float xpi)
			{
				return 1.f;
			};
			const auto sinc = [](float xPiA)
			{
				if (xPiA == 0.f) return 1.f;
				return 2.f * std::sin(xPiA) / xPiA - 1.f;
			};

			fill([pi, wndw, sinc, N](float x)
			{
				const auto xpi = x * pi;
				const auto tablesInv = 1.f / static_cast<float>(N);
				
				auto smpl = 0.f;
				for (auto n = 0; n < N; ++n)
				{
					const auto nF = static_cast<float>(n);
					const auto x2 = (nF + 2) * x * .5f;
					const auto a2 = 2.f * nF + 1.f;
					
					smpl += wndw(xpi) * sinc(x2 * a2) * tablesInv;
				}
				return smpl;

			}, false, true);
		}

		Wavetable() :
			table()
		{}
		
		void fill(const Func& func, bool removeDC, bool normalize)
		{
			static constexpr float SizeInv = 1.f / static_cast<float>(Size);

			// SYNTHESIZE WAVE
			for (auto s = 0; s < Size; ++s)
			{
				auto x = 2.f * static_cast<float>(s) * SizeInv - 1.f;
				table[s] = func(x);
			}

			if (removeDC)
			{
				auto sum = 0.f;
				for (const auto& s : table)
					sum += s;
				sum *= SizeInv;
				if (sum != 0.f)
					for (auto& s : table)
						s -= sum;
			}

			if (normalize)
			{
				auto max = 0.f;
				for (const auto& s : table)
				{
					const auto a = std::abs(s);
					if (max < a)
						max = a;
				}
				if (max != 0.f && max != 1.f)
				{
					const auto g = 1.f / max;
					for (auto& s : table)
						s *= g;
				}
			}

			// COPY FIRST ENTRY/IES FOR INTERPOLATION
			for (auto s = Size; s < table.size(); ++s)
				table[s] = table[s - Size];
		}
		
		float operator[](float x) const noexcept
		{
			static constexpr float SizeF = static_cast<float>(Size);
			x = x * SizeF;
			const auto xFloor = std::floor(x);
			const auto i0 = static_cast<int>(xFloor);
			const auto i1 = i0 + 1;
			const auto frac = x - xFloor;
			return table[i0] + frac * (table[i1] - table[i0]);
		}
		float operator[](int idx) const noexcept
		{
			return table[idx];
		}
	protected:
		std::array<float, Size + 1> table;
	};

	template<size_t WTSize, size_t NumTables>
	struct Wavetable2D
	{
		using Func = std::function<float(float x)>;
		static constexpr float MaxTablesF = static_cast<float>(NumTables - 1);
		using Table = Wavetable<WTSize>;
		using Tables = std::array<Table, NumTables + 1>;

		Wavetable2D() :
			tables()
		{}
		
		void fill(const Func& func, int tablesIdx, bool removeDC, bool normalize)
		{
			tables[tablesIdx].fill(func, removeDC, normalize);
		}
		void finishFills()
		{
			for(auto i = NumTables; i < tables.size(); ++i)
				tables[i] = tables[i - NumTables];
		}
		
		float operator()(int tablesIdx, int tableIdx) const noexcept
		{
			return tables[tablesIdx][tableIdx];
		}
		float operator()(int tablesIdx, float tablePhase) const noexcept
		{
			return tables[tablesIdx][tablePhase];
		}
		float operator()(float tablesPhase, int tableIdx) const noexcept
		{
			const auto x = tablesPhase * MaxTablesF;
			const auto xFloor = std::floor(x);
			const auto i0 = static_cast<int>(xFloor);
			const auto i1 = i0 + 1;
			const auto frac = x - xFloor;
			const auto v0 = tables[i0][tableIdx];
			const auto v1 = tables[i1][tableIdx];

			return v0 + frac * (v1 - v0);
		}
		float operator()(float tablesPhase, float tablePhase) const noexcept
		{
			static constexpr float MaxTablesF = static_cast<float>(NumTables - 1);

			const auto x = tablesPhase * MaxTablesF;
			const auto xFloor = std::floor(x);
			const auto i0 = static_cast<int>(xFloor);
			const auto i1 = i0 + 1;
			const auto frac = x - xFloor;
			const auto v0 = tables[i0][tablePhase];
			const auto v1 = tables[i1][tablePhase];

			return v0 + frac * (v1 - v0);
		}
		
		Table& operator[](int i) noexcept { return tables[i]; }
		const Table& operator[](int i) const noexcept { return tables[i]; }
	protected:
		Tables tables;
	};

	template<size_t WTSize, size_t NumTables>
	struct Wavetable3D
	{
		using Func = std::function<float(float x)>;
		using Funcs = std::array<Func, NumTables>;
		using Tables = Wavetable3D<WTSize, NumTables>;

		void makeTablesWeierstrasz()
		{
			name = "Weierstrasz";
			tables[0].makeTableWeierstrasz(0.f, 0.f, 1);
			tables[1].makeTableWeierstrasz(.0625f, 7.f, 8);
			tables[2].makeTableWeierstrasz(.125f, 5.f, 5);
			tables[3].makeTableWeierstrasz(.1875f, 4.f, 4);
			tables[4].makeTableWeierstrasz(.25f, 3.f, 3);
			tables[5].makeTableWeierstrasz(.3125f, 3.f, 4);
			tables[6].makeTableWeierstrasz(.375f, 3.f, 3);
			tables[7].makeTableWeierstrasz(.4375f, 2.f, 6);
		}
		void makeTablesTriangles()
		{
			name = "Triangle";
			for (auto n = 0; n < NumTables; ++n)
				tables[n].makeTableTriangle(n);
		}
		void makeTablesSinc()
		{
			name = "Sinc";
			for (auto n = 0; n < NumTables; ++n)
				tables[n].makeTableSinc(true, n + 1);
		}

		Wavetable3D() :
			tables(),
			name("empty table")
		{}
		
		void fill(const Funcs& funcs, bool removeDC, bool normalize)
		{
			for (auto f = 0; f < NumTables; ++f)
				tables.fill(funcs[f], f, removeDC, normalize);
			tables.finishFills();
		}
		
		float operator()(float tablesPhase, float tablePhase) const noexcept
		{
			return tables(tablesPhase, tablePhase);
		}
		float operator()(float tablesPhase, int tableIdx) const noexcept
		{
			return tables(tablesPhase, tableIdx);
		}
		float operator()(int tablesIdx, float tablePhase) const noexcept
		{
			return tables(tablesIdx, tablePhase);
		}
		float operator()(int tablesIdx, int tableIdx) const noexcept
		{
			return tables(tablesIdx, tableIdx);
		}

		Wavetable2D<WTSize, NumTables> tables;
		juce::String name;
	};

	enum TableType { Weierstrasz, Tri, Sinc, NumTypes };
	inline juce::String toString(TableType t)
	{
		switch (t)
		{
		case TableType::Weierstrasz: return "Weierstrasz";
		case TableType::Tri: return "Triangle";
		case TableType::Sinc: return "Sinc";
		default: return "";
		}
	}

	static constexpr int LFOTableSize = 1 << 11;
	static constexpr int LFONumTables = 8;
	using LFOTables = Wavetable3D<LFOTableSize, LFONumTables>;

	// creates a modulator curve mapped to [-1, 1]
	// of some ModType (like perlin, audiorate, dropout etc.)
	class Modulator
	{
		using Buffer = std::array<std::vector<float>, 4>;
		using BeatsData = modSys6::BeatsData;
		using Tables = LFOTables;

		struct Perlin
		{
			Perlin(int _numChannels, int _maxNumOctaves) :
				freqSmooth(), widthSmooth(),

				phasor(),

				noise(), scl(), sclInv(),

				fs(0.f),

				rate(-1.f), width(-1.f),
				octaves(-1),

				maxNumOctaves(_maxNumOctaves),
				noiseSize(1 << maxNumOctaves),
				noiseSizeF(static_cast<float>(noiseSize)),
				noiseSizeInv(.5f / noiseSizeF),
				noiseSizeHalf(noiseSizeF * .5f),

				gainAccum(1.f), widthV(1.f),

				numChannels(_numChannels)
			{
				scl.reserve(maxNumOctaves + 1);
				sclInv.reserve(maxNumOctaves + 1);
				for (auto o = 0; o < maxNumOctaves + 1; ++o)
				{
					scl.emplace_back(static_cast<float>(1 << o));
					sclInv.emplace_back(1.f / scl[o]);
				}

				noise.resize(noiseSize + 4); // + splineSize

				unsigned int seed = 420 * 69 / 666 * 42;
				std::random_device rd;
				std::mt19937 mt(rd());
				std::uniform_real_distribution<float> dist(-.89f, .89f); // compensate spline overshoot

				for (auto s = 0; s < noiseSize; ++s, ++seed)
				{
					mt.seed(seed);
					noise[s] = dist(mt);
				}
				for (auto s = noiseSize; s < noise.size(); ++s)
					noise[s] = noise[s - noiseSize];
			}

			void prepare(float sampleRate) noexcept
			{
				if (fs != sampleRate)
				{
					fs = sampleRate;
					phasor.prepare(static_cast<double>(sampleRate));
					widthSmooth.reset();
					freqSmooth.reset();
					modSys6::Smooth::makeFromDecayInMs(widthSmooth, 10.f, sampleRate);
					modSys6::Smooth::makeFromDecayInMs(freqSmooth, 10.f, sampleRate);
				}
			}

			void setParameters(float _rate, int _octaves, float _width) noexcept
			{
				rate = _rate;
				if (octaves != _octaves)
				{
					octaves = _octaves;
					gainAccum = 0.f;
					for (auto o = 0; o < octaves; ++o)
						gainAccum += sclInv[o];
					gainAccum = 1.f / gainAccum;
				}
				if (width != _width)
				{
					width = _width;
					static constexpr float bias = .8f;
					static constexpr float b = 1.f / (1.f - bias);
					widthV = std::pow(width, b);
				}
			}

			void operator()(Buffer& buffer, int numChannelsOut, int numSamples) noexcept
			{
				// SYNTHESIZE PHASE
				auto phasorBuf = buffer[2].data();
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto freqHz = static_cast<double>(freqSmooth(rate) * noiseSizeInv);
					phasor.setFrequencyHz(freqHz);
					phasorBuf[s] = static_cast<float>(phasor.process()) * noiseSizeF;
				}
				
				// PERFORM PERLIN NOISE MAGIC
				for (auto s = 0; s < numSamples; ++s)
				{
					auto smpl = 0.f;
					for (int o = 0; o < octaves; ++o)
					{
						auto x = phasorBuf[s] * scl[o];
						while (x >= noiseSizeF)
							x -= noiseSizeF;
						smpl += interpolation::cubicHermiteSpline(noise.data(), x) * sclInv[o];
					}
					smpl *= gainAccum;
					buffer[0][s] = smpl;
				}

				if (numChannelsOut == 2)
				{ // PERFORM STEREO STUFF
					// PERFORM EVEN MORE PERLIN NOISE MAGIC
					for (auto s = 0; s < numSamples; ++s)
					{
						auto smpl = 0.f;
						for (int o = 0; o < octaves; ++o)
						{
							auto x = phasorBuf[s] * scl[o] + noiseSizeHalf;
							while (x >= noiseSizeF)
								x -= noiseSizeF;
							smpl += interpolation::cubicHermiteSpline(noise.data(), x) * sclInv[o];
						}
						smpl *= gainAccum;
						buffer[1][s] = smpl;
					}

					// PROCESS WIDTH
					for (auto s = 0; s < numSamples; ++s)
						buffer[1][s] = buffer[0][s] + widthSmooth(widthV) * (buffer[1][s] - buffer[0][s]);
				}
			}
		protected:
			modSys6::Smooth freqSmooth, widthSmooth;

			Phasor<double> phasor;
			std::vector<float> noise, scl, sclInv;

			float fs;

			float rate, width;
			int octaves;

			const int maxNumOctaves;
			const int noiseSize;
			const float noiseSizeF, noiseSizeInv, noiseSizeHalf;

			float gainAccum, widthV;

			const int numChannels;
		};

		class AudioRate
		{
			static constexpr float PBGain = 2.f / static_cast<float>(0x3fff) - 1.f;

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
					return std::cos(approx::Tau * phasor.phase);
				}
				
				inline float withPhaseOffset(Osc& other, float offset) noexcept
				{
					const auto phase = other.phasor.phase + offset;
					return std::cos(approx::Tau * phase);
				}

				Phasor<float> phasor;
			};
		public:
			AudioRate(int _numChannels) :
				retuneSpeedSmooth(),

				numChannels(_numChannels),
				osc(),
				env(),

				noteValue(0.f), pitchbendValue(0.f),

				noteOffset(0.f), width(0.f), retuneSpeed(0.f),
				attack(1.f), decay(1.f), sustain(1.f), release(1.f),

				Fs(1.f)
			{
				osc.resize(numChannels);
			}
			void prepare(float sampleRate) noexcept
			{
				Fs = sampleRate;
				for(auto& o: osc)
					o.prepare(sampleRate);
				env.prepare(Fs);
				modSys6::Smooth::makeFromDecayInMs(retuneSpeedSmooth, retuneSpeed, Fs);
			}
			void setParameters(float _noteOffset, float _width, float _retuneSpeed,
				float _attack, float _decay, float _sustain, float _release) noexcept
			{
				noteOffset = _noteOffset;
				width = _width * .5f;
				if (retuneSpeed != _retuneSpeed)
				{
					retuneSpeed = _retuneSpeed;
					modSys6::Smooth::makeFromDecayInMs(retuneSpeedSmooth, retuneSpeed, Fs);
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

			void operator()(Buffer& buffer, const juce::MidiBuffer& midi, int numChannelsOut, int numSamples) noexcept
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
										noteValue = static_cast<float>(msg.getNoteNumber());
										currentValue = noteValue + pitchbendValue;
										noteOn = true;
									}
									else if (msg.isNoteOff())
									{
										noteOn = false;
									}
									else if (msg.isPitchWheel())
									{
										pitchbendValue = static_cast<float>(msg.getPitchWheelValue()) * PBGain;
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
						const auto freq = 440.f * std::pow(2.f, (midiN - 69.f) * .083333333333f);
						buffer[1][s] = juce::jlimit(1.f, 22049.f, freq);
					}
				}
				{ // PROCESS RETUNE SPEED OF OSC (FILTER CUTOFF)
					retuneSpeedSmooth(buffer[1].data(), numSamples);
				}
#if DebugAudioRateEnv
				{ // COPY ENVELOPE ONLY
					for(auto ch = 0; ch < numChannelsOut; ++ch)
						for (auto s = 0; s < numSamples; ++s)
							buffer[ch][s] = bufEnv[s];
				}
#else
				{ // SYNTHESIZE OSCILLATOR
					if (numChannelsOut == 1)
					{
						for (auto s = 0; s < numSamples; ++s)
						{
							const auto freq = buffer[1][s];
							osc[0].setFrequencyHz(freq);
							buffer[0][s] = osc[0]() * bufEnv[s];
						}
					}
					else
					{ // PROCESS STEREO WIDTH
						if(width == 0.f)
							for (auto s = 0; s < numSamples; ++s)
							{
								const auto freq = buffer[1][s];
								osc[0].setFrequencyHz(freq);
								buffer[0][s] = osc[0]() * bufEnv[s];
								buffer[1][s] = buffer[0][s];
							}
						else
							for (auto s = 0; s < numSamples; ++s)
							{
								const auto freq = buffer[1][s];
								osc[0].setFrequencyHz(freq);
								buffer[0][s] = osc[0]() * bufEnv[s];
								buffer[1][s] = osc[1].withPhaseOffset(osc[0], width) * bufEnv[s];
							}
					}
				}
#endif
			}
		protected:
			modSys6::Smooth retuneSpeedSmooth;

			const int numChannels;
			std::vector<Osc> osc;
			EnvGen env;
			float noteValue, pitchbendValue;

			float noteOffset, width, retuneSpeed, attack, decay, sustain, release;
			float Fs;
		};

		class Dropout
		{
			static constexpr float pi = 3.14159265359f;
			static constexpr float FreqCoeff = pi * 10.f * 10.f * 10.f * 10.f * 10.f;
		public:
			Dropout(int _numChannels) :
				smoothSmooth(), widthSmooth(),

				phasor(),
				decay(1.f), spin(1.f), freqChance(0.f), freqSmooth(0.f), width(0.f),
				rand(),

				numChannels(_numChannels),

				accel{ 0.f, 0.f },
				speed{ 0.f, 0.f },
				dest{ 0.f, 0.f },
				env{ 0.f, 0.f },
				smooth(),
				fs(44100.f), dcy(1.f), spinV(420.f)
			{}
			void prepare(float sampleRate)
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
					modSys6::Smooth::makeFromDecayInHz(s, freqSmooth, fs);
				spinV = 1.f / (fs * FreqCoeff / (spin * spin));

				modSys6::Smooth::makeFromDecayInMs(smoothSmooth, 40.f, fs);
				modSys6::Smooth::makeFromDecayInMs(widthSmooth, 10.f, fs);
			}
			void setParameters(float _decay, float _spin, float _freqChance, float _freqSmooth, float _width)
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
			void operator()(Buffer& buffer, int numChannelsOut, int numSamples) noexcept
			{
				{ // FILL BUFFERS WITH IMPULSES
					for (auto ch = 0; ch < numChannelsOut; ++ch)
					{
						auto& phasr = phasor[ch];
						auto impulseBuf = buffer[ch].data();
						juce::FloatVectorOperations::fill(impulseBuf, 0.f, numSamples);
						for (auto s = 0; s < numSamples; ++s)
							if (phasr())
								impulseBuf[s] = 2.f * (rand.nextFloat() - .5f);
								
					}
				}
				auto smoothBuf = buffer[2].data();
				{ // PROCESS SMOOTH SMOOTH
					for (auto s = 0; s < numSamples; ++s)
						smoothBuf[s] = smoothSmooth(freqSmooth);
				}
				{ // SYNTHESIZE MOD
					for (auto ch = 0; ch < numChannelsOut; ++ch)
					{
						auto buf = buffer[ch].data();
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

							modSys6::Smooth::makeFromDecayInHz(smooth[ch], smoothBuf[s], fs);

							smpl = smooth[ch](smpl);
						}
					}
				}
				{ // PROCESS WIDTH
					if (numChannelsOut == 2)
					{
						for (auto s = 0; s < numSamples; ++s)
							buffer[1][s] = buffer[0][s] + widthSmooth(width) * (buffer[1][s] - buffer[0][s]);
					}
				}
			}
		protected:
			modSys6::Smooth smoothSmooth, widthSmooth;

			std::array<Phasor<float>, 2> phasor;
			float decay, spin, freqChance, freqSmooth, width;
			juce::Random rand;

			int numChannels;

			std::array<float, 2> accel, speed, dest, env;
			std::array<modSys6::Smooth, 2> smooth;
			float fs, dcy, spinV;
		};

		struct EnvFol
		{
			EnvFol(int _numChannels) :
				gainSmooth(), widthSmooth(),

				envelope{ 0.f, 0.f },
				Fs(1.f),

				attackInMs(-1.f), releaseInMs(-1.f), gain(-420.f),

				attackV(1.f), releaseV(1.f), gainV(1.f), widthV(0.f),
				autogainV(1.f),

				numChannels(_numChannels)
			{}
			void prepare(float sampleRate)
			{
				Fs = sampleRate;
				modSys6::Smooth::makeFromDecayInMs(gainSmooth, 10.f, Fs);
				modSys6::Smooth::makeFromDecayInMs(widthSmooth, 10.f, Fs);
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
			void operator()(Buffer& buffer, const float** samples, int numChannelsIn, int numChannelsOut, int numSamples) noexcept
			{
				auto gainBuf = buffer[2].data();
				{ // PROCESS GAIN SMOOTH
					for (auto s = 0; s < numSamples; ++s)
					{
						gainBuf[s] = gainSmooth(gainV * autogainV);
					}
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
					if (numChannelsIn + numChannelsOut == 4)
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
							if (numChannelsOut == 2)
								for (auto s = 0; s < numSamples; ++s)
									buffer[1][s] = buffer[0][s] + widthSmooth(widthV) * (buffer[1][s] - buffer[0][s]);
						}
					}
				}
			}
		protected:
			modSys6::Smooth gainSmooth, widthSmooth;

			std::array<float, 2> envelope;
			float Fs;

			float attackInMs, releaseInMs, gain;

			float attackV, releaseV, gainV, widthV;
			float autogainV;

			int numChannels;

			void updateAutogainV() noexcept
			{
				autogainV = attackV != 0.f ? 1.f + std::sqrt(releaseV / attackV) : 1.f;
			}
		};

		struct Macro
		{
			Macro(int _numChannels) :
				smooth(),
				macro(0.f),
				numChannels(_numChannels)
			{}
			void prepare(float sampleRate) noexcept
			{
				modSys6::Smooth::makeFromDecayInMs(smooth, 40.f, sampleRate);
			}
			void setParameters(float _macro) noexcept
			{
				macro = _macro;
			}
			void operator()(Buffer& buffer, int numChannelsOut, int numSamples) noexcept
			{
				for (auto s = 0; s < numSamples; ++s)
					buffer[0][s] = smooth(macro);
				if (numChannelsOut == 2)
					juce::FloatVectorOperations::copy(buffer[1].data(), buffer[0].data(), numSamples);
			}
		protected:
			modSys6::Smooth smooth;

			float macro;

			int numChannels;
		};

		struct Pitchbend
		{
			Pitchbend(int _numChannels) :
				smooth(),
				bendV(0.f),
				pitchbend(0),
				numChannels(_numChannels)
			{}
			void prepare(float sampleRate) noexcept
			{
				fs = sampleRate;
			}
			void setParameters(float _smoothRate) noexcept
			{
				smoothRate = _smoothRate;
			}
			void operator()(Buffer& buffer, int numChannelsOut, int numSamples, const juce::MidiBuffer& midiBuffer) noexcept
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
					for (auto s = 0; s < numSamples; ++s)
					{
						modSys6::Smooth::makeFromDecayInMs(smooth, smoothRate, fs);
						buffer[0][s] = smooth(buffer[0][s]);
					}
				}
				if (numChannelsOut == 2)
					juce::FloatVectorOperations::copy(buffer[1].data(), buffer[0].data(), numSamples);
			}
		protected:
			modSys6::Smooth smooth;
			
			float fs;

			float bendV, smoothRate;
			int pitchbend;

			int numChannels;
		};
		
		class LFO
		{
		public:
			LFO(int _numChannels, const Tables& _tables, const BeatsData& _beatsData) :
				tables(_tables),
				beatsData(_beatsData),

				waveformSmooth(),
				widthSmooth(),

				transport(),
				phasor(),
				
				fs(1.f),
				fsInv(1.f),
				
				rateFree(-1.f),
				isSync(false),

				rateSyncV(1.f),
				rateSyncInv(1.f),
				waveformV(0.f),
				phaseV(0.f),
				widthV(0.f),

				numChannels(_numChannels), extLatency(0)
			{}
			void prepare(float sampleRate, int latency)
			{
				fs = sampleRate;
				fsInv = 1.f / fs;
				extLatency = latency;
				for (auto& p : phasor)
				{
					p.phase = 0.f;
					p.inc = rateFree * fsInv;
				}
				modSys6::Smooth::makeFromDecayInMs(waveformSmooth, 10.f, fs);
				modSys6::Smooth::makeFromDecayInMs(widthSmooth, 10.f, fs);
			}
			void setParameters(bool _isSync, float _rateFree, float _rateSync, float _waveform, float _phase, float _width) noexcept
			{
				isSync = _isSync;
				rateSyncV = beatsData[static_cast<int>(_rateSync)].val;
				rateSyncInv = 1.f / rateSyncV;
				if (rateFree != _rateFree)
				{
					rateFree = _rateFree;
					for (auto& p : phasor)
						p.inc = rateFree * fsInv;
				}
				waveformV = _waveform;
				phaseV = _phase;
				widthV = _width;
			}
			void operator()(Buffer& buffer, int numChannelsOut, int numSamples, juce::AudioPlayHead* playHead) noexcept
			{
				bool canBeSync = playHead != nullptr;
				{ // SYNTHESIZE PHASOR
					if (isSync && canBeSync)
					{
						canBeSync = playHead->getCurrentPosition(transport) && transport.isPlaying;
						if (canBeSync)
							processTempoSyncStuff();
					}
					synthesizePhase(buffer, 0, numSamples);
				}
				{ // PROCESS WIDTH
					if (numChannelsOut == 2)
					{
						auto buf = buffer[1].data();
						juce::FloatVectorOperations::copy(buf, buffer[0].data(), numSamples);
						for (auto s = 0; s < numSamples; ++s)
						{
							buf[s] += widthSmooth(widthV);
							if (buf[s] >= 1.f)
								--buf[s];
						}
					}
				}
				{ // PROCESS WAVEFORM
					for (auto ch = 0; ch < numChannelsOut; ++ch)
					{
						auto buf = buffer[ch].data();
						for (auto s = 0; s < numSamples; ++s)
							buf[s] = tables(waveformSmooth(waveformV), buf[s]);
					}
				}
			}
		protected:
			const Tables& tables;
			const BeatsData& beatsData;

			modSys6::Smooth waveformSmooth, widthSmooth;

			juce::AudioPlayHead::CurrentPositionInfo transport;
			std::array<Phasor<float>, 2> phasor;

			float fs, fsInv;

			float rateFree;
			bool isSync;

			float rateSyncV, rateSyncInv, waveformV, phaseV, widthV;

			int numChannels, extLatency;

			void synthesizePhase(Buffer& buffer, int ch, int numSamples)
			{
				for (auto s = 0; s < numSamples; ++s)
				{
					phasor[ch]();
					buffer[ch][s] = phasor[ch].phase;
				}
			}

			void processTempoSyncStuff()
			{
				const auto bpm = static_cast<float>(transport.bpm);
				const auto bps = bpm / 60.f;
				const auto quarterNoteLengthInSamples = fs / bps;
				const auto barLengthInSamples = quarterNoteLengthInSamples * 4.f;
				phasor[0].inc = 1.f / (barLengthInSamples * rateSyncV);

				const auto latencyLengthInQuarterNotes = static_cast<float>(extLatency) / quarterNoteLengthInSamples;
				auto ppq = (static_cast<float>(transport.ppqPosition) - latencyLengthInQuarterNotes) * .25f;
				while (ppq < 0.f) ++ppq;
				// latency stuff end
				const auto ppqCh = ppq * rateSyncInv;
				const auto newPhase = (ppqCh - std::floor(ppqCh));
				auto phase = newPhase + phaseV;
				if (phase < 0.f)
					++phase;
				else if (phase >= 1.f)
					--phase;
				phasor[0].phase = phase;
			}
		};

	public:
		Modulator(int _numChannels, const BeatsData& beatsData) :
			buffer(),
			numChannels(_numChannels == 1 ? 1 : 2),

			tables(),

			perlin(numChannels, 8),
			audioRate(numChannels),
			dropout(numChannels),
			envFol(numChannels),
			macro(numChannels),
			pitchbend(numChannels),
			lfo(numChannels, tables, beatsData),
			
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
				const auto tableType = child.getProperty(id);
				if (tableType == toString(TableType::Weierstrasz))
					tables.makeTablesWeierstrasz();
				else if (tableType == toString(TableType::Tri))
					tables.makeTablesTriangles();
				else if (tableType == toString(TableType::Sinc))
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
			for(auto& b: buffer)
				b.resize(maxBlockSize + 4, 0.f); // compensate for potential spline interpolation
			perlin.prepare(sampleRate);
			audioRate.prepare(sampleRate);
			dropout.prepare(sampleRate);
			envFol.prepare(sampleRate);
			macro.prepare(sampleRate);
			pitchbend.prepare(sampleRate);
			lfo.prepare(sampleRate, latency);
		}

		// parameters
		void setParametersPerlin(float rate, int octaves, float width) noexcept
		{
			perlin.setParameters(rate, octaves, width);
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
		void setParametersMacro(float m) noexcept { macro.setParameters(m); }
		void setParametersPitchbend(float rate) noexcept { pitchbend.setParameters(rate); }
		void setParametersLFO(bool isSync, float rateFree, float rateSync, float waveform, float phase, float width) noexcept
		{
			lfo.setParameters(isSync, rateFree, rateSync, waveform, phase, width);
		}

		void processBlock(const float** samples, const juce::MidiBuffer& midi,
			juce::AudioPlayHead* playHead, int numChannelsIn, int numChannelsOut, int numSamples) noexcept
		{
			switch (type)
			{
			case ModType::Perlin: return perlin(buffer, numChannelsOut, numSamples);
			case ModType::AudioRate: return audioRate(buffer, midi, numChannelsOut, numSamples);
			case ModType::Dropout: return dropout(buffer, numChannelsOut, numSamples);
			case ModType::EnvFol: return envFol(buffer, samples, numChannelsIn, numChannelsOut, numSamples);
			case ModType::Macro: return macro(buffer, numChannelsOut, numSamples);
			case ModType::Pitchwheel: return pitchbend(buffer, numChannelsOut, numSamples, midi);
			case ModType::LFO: return lfo(buffer, numChannelsOut, numSamples, playHead);
			}
		}
		
		Tables& getTables() noexcept { return tables; }
		const Tables& getTables() const noexcept { return tables; }

		Buffer buffer;
	protected:
		const int numChannels;

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
	check if needs parameter smoothing
		rate free

make trigger mod
	trigger type (midi note, automation(&button), onset envelope)
	waveform (sinc, tapestop, tapestart, dropout)
	duration
	check parameter smoothing

envFol
	lookahead?

make konami mod

*/

#undef DebugAudioRateEnv
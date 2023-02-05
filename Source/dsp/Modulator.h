#pragma once
#include "../Interpolation.h"
#include "../modsys/ModSys.h"
#include <random>
#include "Smooth.h"

#define DebugAudioRateEnv false
#define DebugPerlinPhase false

namespace vibrato
{
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
			const auto tri = [&](float x, float fc, float phase)
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
			} :
			[](float)
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
		std::array<float, Size + 2> table;
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
		static constexpr float SafetyCoeff = .99f;
		using Buffer = std::array<std::vector<float>, 4>;
		using BeatsData = modSys6::BeatsData;
		using Tables = LFOTables;

		class Perlin
		{
			struct Perlinizer
			{
				Perlinizer() :
					gainAccum(1.f),
					octavesPL(0)
				{}
				
				void setParameters(const float* sclInv, int _octaves) noexcept
				{
					if (octavesPL != _octaves)
					{
						octavesPL = _octaves;
						gainAccum = 0.f;
						for (auto o = 0; o < octavesPL; ++o)
							gainAccum += sclInv[o];
						gainAccum = 1.f / gainAccum;
					}
				}
				
				void operator()(float* buffer, const float* phasorBuf, 
					const float* scl, const float* sclInv, const float* noise, 
					float noiseSizeF, float phaseOffset, int numSamples, const int* octBuf) noexcept
				{
					for (auto s = 0; s < numSamples; ++s)
					{
						setParameters(sclInv, octBuf[s]);

						auto smpl = 0.f;
						for (int o = 0; o < octavesPL; ++o)
						{
							auto x = phasorBuf[s] * scl[o] + phaseOffset;
							while (x >= noiseSizeF)
								x -= noiseSizeF;
							smpl += interpolation::cubicHermiteSpline(noise, x) * sclInv[o];
						}
						smpl *= gainAccum;
						buffer[s] = smpl;
					}
				}
				
				void operator()(float* buffer, const float* phasorBuf,
					const float* scl, const float* sclInv, const float* noise,
					float noiseSizeF, float phaseOffset, int numSamples,
					const int* octBuf, const float* mix) noexcept
				{
					for (auto s = 0; s < numSamples; ++s)
					{
						setParameters(sclInv, octBuf[s]);

						auto smpl = 0.f;
						for (int o = 0; o < octavesPL; ++o)
						{
							auto x = phasorBuf[s] * scl[o] + phaseOffset;
							while (x >= noiseSizeF)
								x -= noiseSizeF;
							smpl += interpolation::cubicHermiteSpline(noise, x) * sclInv[o];
						}
						smpl *= gainAccum;
						buffer[s] += mix[s] * (smpl - buffer[s]);
					}
				}
			
			protected:
				float gainAccum;
				int octavesPL;
			};
			
		public:
			Perlin(int _numChannels, int _maxNumOctaves) :
				freqSmooth(false), widthSmooth(false), octSmooth(4.f),
				freqSmoothBuf(),
				widthBuf(),
				
				phasor(),

				noise(), scl(), sclInv(),

				fs(0.f), fsInv(1.f),

				rate(-1.f), width(-1.f), octaves(0.f), seed(0.f),

				perlinizer0(), perlinizer1(),

				maxNumOctaves(_maxNumOctaves),
				noiseSize(1 << maxNumOctaves),
				noiseSizeF(static_cast<float>(noiseSize)),
				noiseSizeInv(.5f / noiseSizeF),
				noiseSizeHalf(noiseSizeF * .5f),

				widthV(1.f),

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

				unsigned int _seed = 420 * 69 / 666 * 42;
				std::random_device rd;
				std::mt19937 mt(rd());
				std::uniform_real_distribution<float> dist(-.89f, .89f); // compensate spline overshoot

				for (auto s = 0; s < noiseSize; ++s, ++_seed)
				{
					mt.seed(_seed);
					noise[s] = dist(mt);
				}
				
				for (auto s = noiseSize; s < noise.size(); ++s)
					noise[s] = noise[s - noiseSize];
			}

			void prepare(float sampleRate, int blockSize) noexcept
			{
				if (fs != sampleRate)
				{
					const auto oFloor = static_cast<int>(std::floor(octaves));
					octFloorBuf.resize(blockSize, oFloor);
					octCeilBuf.resize(blockSize, oFloor + 1);
					fs = sampleRate;
					fsInv = 1.f / fs;
					phasor.prepare(static_cast<double>(sampleRate));
					octSmooth.makeFromDecayInMs(10.f, sampleRate);
					widthSmooth.makeFromDecayInMs(10.f, sampleRate);
					freqSmooth.makeFromDecayInMs(10.f, sampleRate);
					freqSmoothBuf.resize(blockSize);
					widthBuf.resize(blockSize);
				}
			}

			void setParameters(float _rate, float _octaves, float _width, float _seed) noexcept
			{
				rate = _rate;
				octaves = _octaves;
				if (width != _width)
				{
					width = _width;
					static constexpr float bias = .8f;
					static constexpr float b = 1.f / (1.f - bias);
					widthV = std::pow(width, b);
				}
				
				const auto numSeedWaves = 16.f;
				seed = _seed * numSeedWaves;
			}

			void operator()(Buffer& buffer, int numChannelsOut, int numSamples, juce::AudioPlayHead* playHead) noexcept
			{
				// SEEDED PHASE SYNTHESIS
				auto phasorBuf = buffer[2].data();
				if(seed != 0.f && playHead != nullptr)
				{
					const auto posInfo = playHead->getPosition();
					if (posInfo.hasValue())
					{
						const auto isPlaying = posInfo->getIsPlaying();
						if (!isPlaying)
						{
							for (auto ch = 0; ch < numChannelsOut; ++ch)
								juce::FloatVectorOperations::clear(buffer[ch].data(), numSamples);
							return;
						}
						
						const auto ppqO = posInfo->getPpqPosition();
						if (ppqO.hasValue())
						{
							const auto ppq = static_cast<float>(*ppqO);
							
							//auto phs = seed + ppq * rate;
							auto phs = ppq * rate * .498;
							phs = std::fmod(phs, noiseSizeF);
							phs *= noiseSizeInv;
							
							phasor.phase = static_cast<double>(phs);
							
						}
					}
					//phasor.setFrequencyHz(1.935 * static_cast<double>(rate * noiseSizeInv));
					phasor.setFrequencyHz(static_cast<double>(rate * noiseSizeInv));

					for (auto s = 0; s < numSamples; ++s)
					{
						phasor();
						phasorBuf[s] = static_cast<float>(phasor.phase) * noiseSizeF;
					}
				}
				else
				{
					// SYNTHESIZE PHASOR
					auto freqSmoothing = freqSmooth(freqSmoothBuf.data(), static_cast<double>(rate * noiseSizeInv), numSamples);
					if(freqSmoothing)
						for (auto s = 0; s < numSamples; ++s)
						{
							const auto freqHz = freqSmoothBuf[s];
							phasor.setFrequencyHz(freqHz);
							auto phs = static_cast<float>(phasor.process());
							phasorBuf[s] = phs * noiseSizeF;
						}
					else
					{
						for (auto s = 0; s < numSamples; ++s)
						{
							auto phs = static_cast<float>(phasor.process());
							phasorBuf[s] = phs * noiseSizeF;
						}
					}
				}
			
				// SYNTHESIZE OCT BUFFERS
				auto octBuf = buffer[3].data();
				auto octSmoothing = octSmooth(octBuf, octaves, numSamples);
				if(octSmoothing)
					for (auto s = 0; s < numSamples; ++s)
					{
						const auto octV = octBuf[s];
						const auto oFloorF = std::floor(octV);
						const auto oFloorInt = static_cast<int>(oFloorF);
						const auto oFloorCeil = oFloorInt + 1;
						octBuf[s] = octV - oFloorF;
						octFloorBuf[s] = oFloorInt;
						octCeilBuf[s] = oFloorCeil;
					}
				else
				{
					const auto octV = octaves;
					const auto oFloorF = std::floor(octV);
					const auto oFloorInt = static_cast<int>(oFloorF);
					const auto oCeilInt = oFloorInt + 1;
					octaves = octV - oFloorF;
					for (auto s = 0; s < numSamples; ++s)
					{
						octBuf[s] = octaves;
						octFloorBuf[s] = oFloorInt;
						octCeilBuf[s] = oCeilInt;
					}
				}

#if DebugPerlinPhase
				for (auto ch = 0; ch < numChannelsOut; ++ch)
					for (auto s = 0; s < numSamples; ++s)
						buffer[ch][s] = phasorBuf[s] * noiseSizeInv;
#else
				// PERFORM PERLIN NOISE
				synthesizePerlin(buffer[0].data(), phasorBuf, octBuf, 0.f, numSamples);
				
				if (numChannelsOut == 2)
				{
					synthesizePerlin(buffer[1].data(), phasorBuf, octBuf, noiseSizeHalf, numSamples);

					auto widthSmoothing = widthSmooth(widthBuf.data(), widthV, numSamples);
					if(widthSmoothing)
						for (auto s = 0; s < numSamples; ++s)
							buffer[1][s] = buffer[0][s] + widthBuf[s] * (buffer[1][s] - buffer[0][s]);
					else
						for (auto s = 0; s < numSamples; ++s)
							buffer[1][s] = buffer[0][s] + widthV * (buffer[1][s] - buffer[0][s]);
				}
#endif
			}
		
		protected:
			SmoothD freqSmooth;
			SmoothF widthSmooth, octSmooth;
			std::vector<double> freqSmoothBuf;
			std::vector<float> widthBuf;

			Phasor<double> phasor;
			std::vector<float> noise, scl, sclInv;
			std::vector<int> octFloorBuf, octCeilBuf;

			float fs, fsInv;

			float rate, width, octaves, seed;

			Perlinizer perlinizer0, perlinizer1;

			const int maxNumOctaves;
			const int noiseSize;
			const float noiseSizeF, noiseSizeInv, noiseSizeHalf;

			float widthV;

			const int numChannels;

			void synthesizePerlin(float* buffer, const float* phasorBuf, const float* octBuf,
				float phaseOffset, int numSamples) noexcept
			{
				perlinizer0(buffer, phasorBuf, scl.data(), sclInv.data(),
					noise.data(), noiseSizeF, phaseOffset, numSamples, octFloorBuf.data());
				perlinizer1(buffer, phasorBuf, scl.data(), sclInv.data(),
					noise.data(), noiseSizeF, phaseOffset, numSamples, octCeilBuf.data(), octBuf);
			}
			
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
				retuneSpeedSmooth(0.f),
				widthSmooth(0.f),
				widthBuf(),

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
			
			void prepare(float sampleRate, int blockSize) noexcept
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
					if (numChannelsOut == 1)
					{
						if(retuningNow)
							for (auto s = 0; s < numSamples; ++s)
							{
								const auto freq = buffer[1][s];
								osc[0].setFrequencyHz(freq);
								buffer[0][s] = osc[0]() * bufEnv[s];
							}
						else
						{
							const auto freq = buffer[1][0];
							osc[0].setFrequencyHz(freq);
							for (auto s = 0; s < numSamples; ++s)
								buffer[0][s] = osc[0]() * bufEnv[s];
						}
					}
					else
					{ // PROCESS STEREO WIDTH
						auto smoothingWidth = widthSmooth(widthBuf.data(), width, numSamples);

						if (retuningNow)
							if(smoothingWidth)
								for (auto s = 0; s < numSamples; ++s)
								{
									const auto freq = buffer[1][s];
									osc[0].setFrequencyHz(freq);
									buffer[0][s] = osc[0]() * bufEnv[s];
									buffer[1][s] = osc[1].withPhaseOffset(osc[0], widthBuf[s] * bufEnv[s]);
								}
							else
								for (auto s = 0; s < numSamples; ++s)
								{
									const auto freq = buffer[1][s];
									osc[0].setFrequencyHz(freq);
									buffer[0][s] = osc[0]() * bufEnv[s];
									buffer[1][s] = osc[1].withPhaseOffset(osc[0], width * bufEnv[s]);
								}
						else
						{
							const auto freq = buffer[1][0];
							osc[0].setFrequencyHz(freq);
							if (smoothingWidth)
								for (auto s = 0; s < numSamples; ++s)
								{
									buffer[0][s] = osc[0]() * bufEnv[s];
									buffer[1][s] = osc[1].withPhaseOffset(osc[0], widthSmooth(width)) * bufEnv[s];
								}
							else
								for (auto s = 0; s < numSamples; ++s)
								{
									buffer[0][s] = osc[0]() * bufEnv[s];
									buffer[1][s] = osc[1].withPhaseOffset(osc[0], width) * bufEnv[s];
								}
						}
					}
				}
#endif
			}
			
		protected:
			SmoothF retuneSpeedSmooth, widthSmooth;
			std::vector<float> widthBuf;

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
			static constexpr float piHalf = pi * .5f;
			static constexpr float FreqCoeff = pi * 10.f * 10.f * 10.f * 10.f * 10.f;
		
		public:
			Dropout(int _numChannels) :
				widthSmooth(0.f),
				widthBuf(),
				
				phasor(),
				decay(1.f), spin(1.f), freqChance(0.f), freqSmooth(0.f), width(0.f),
				rand(),

				numChannels(_numChannels),

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
				
				{ // SYNTHESIZE MOD
					for (auto ch = 0; ch < numChannelsOut; ++ch)
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

						smooth[ch].makeFromFreqInHz(freqSmooth, fs);
						for (auto s = 0; s < numSamples; ++s)
						{
							auto& smpl = buf[s];
							smpl = smooth[ch](smpl);
						}	
					}
				}
				
				{ // PROCESS WIDTH
					if (numChannelsOut != 2)
						return;
					
					auto widthSmoothing = widthSmooth(widthBuf.data(), width, numSamples);
					
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

			int numChannels;

			std::array<float, 2> accel, speed, dest, env;
			std::array<SmoothD, 2> smooth;
			float fs, dcy, spinV;
		};

		struct EnvFol
		{
			
			EnvFol(int _numChannels) :
				gainSmooth(0.f), widthSmooth(0.f),
				widthBuf(),

				envelope{ 0.f, 0.f },
				envSmooth(),
				Fs(1.f),

				attackInMs(-1.f), releaseInMs(-1.f), gain(-420.f),

				attackV(1.f), releaseV(1.f), gainV(1.f), widthV(0.f),
				autogainV(1.f),

				numChannels(_numChannels)
			{}
			
			void prepare(float sampleRate, int blockSize)
			{
				Fs = sampleRate;
				gainSmooth.makeFromDecayInMs(10.f, Fs);
				widthSmooth.makeFromDecayInMs(10.f, Fs);
				widthBuf.resize(blockSize);
				for (auto ch = 0; ch < numChannels; ++ch)
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
			
			void operator()(Buffer& buffer, const float* const* samples, int numChannelsIn, int numChannelsOut, int numSamples) noexcept
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
							if (numChannelsOut != 2)
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
					for (auto ch = 0; ch < numChannelsOut; ++ch)
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

			int numChannels;

			void updateAutogainV() noexcept
			{
				autogainV = attackV != 0.f ? 1.f + std::sqrt(releaseV / attackV) : 1.f;
			}
		};

		struct Macro
		{
			Macro(int _numChannels) :
				smooth(0.f),
				macro(0.f),
				numChannels(_numChannels)
			{}
			
			void prepare(float sampleRate) noexcept
			{
				smooth.makeFromDecayInMs(40.f, sampleRate);
			}
			
			void setParameters(float _macro) noexcept
			{
				macro = _macro;
			}
			
			void operator()(Buffer& buffer, int numChannelsOut, int numSamples) noexcept
			{
				auto smoothing = smooth(buffer[0].data(), macro, numSamples);
				if(!smoothing)
					juce::FloatVectorOperations::fill(buffer[0].data(), macro, numSamples);

				if (numChannelsOut != 2)
					return;
				
				juce::FloatVectorOperations::copy(buffer[1].data(), buffer[0].data(), numSamples);
			}
			
		protected:
			SmoothF smooth;

			float macro;
			int numChannels;
		};

		struct Pitchbend
		{
			Pitchbend(int _numChannels) :
				smooth(0.f),
				fs(1.f),
				bendV(0.f),
				smoothRate(0.f),
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
					smooth.makeFromDecayInMs(smoothRate, fs);
					for (auto s = 0; s < numSamples; ++s)
						buffer[0][s] = smooth(buffer[0][s]);
				}
				if (numChannelsOut == 2)
					juce::FloatVectorOperations::copy(buffer[1].data(), buffer[0].data(), numSamples);
			}
		
		protected:
			SmoothF smooth;
			
			float fs;

			float bendV, smoothRate;
			int pitchbend;

			int numChannels;
		};
		
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
					transport(),
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
				
				void processTempoSyncStuff(float* buffer, float rateSync, float phase, int numSamples, juce::AudioPlayHead* playHead)
				{
					const auto canBeSync = playHead->getCurrentPosition(transport) && transport.isPlaying;
					if (canBeSync)
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
				juce::AudioPlayHead::CurrentPositionInfo transport;
				const BeatsData& beatsData;
				double fs, extLatency, phasor, inc;
			};
		
		public:
			LFO(int _numChannels, const Tables& _tables, const BeatsData& _beatsData) :
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
				widthV(0.f),

				numChannels(_numChannels)
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
			
			void operator()(Buffer& buffer, int numChannelsOut, int numSamples,
				juce::AudioPlayHead* playHead) noexcept
			{
				bool canBeSync = playHead != nullptr;
				{ // SYNTHESIZE PHASOR
					auto buf = buffer[0].data();

					if (isSync && canBeSync)
						tempoSync.processTempoSyncStuff(buf, rateSync, phaseV, numSamples, playHead);
					else
					{
						if (phaseV != 0.f && playHead != nullptr)
						{
							const auto pos = playHead->getPosition();
							if (pos.hasValue())
							{
								const auto ppqO = pos->getPpqPosition();
								const auto bpmO = pos->getBpm();
								if (ppqO.hasValue() && bpmO.hasValue())
								{
									const auto ppq = *ppqO;
									const auto bpm = *bpmO;
									const auto bps = bpm / 60.;

									const auto inc = rateFree * fsInv;
									const auto phase = ppq / bps * rateFree;

									phasor.inc = inc;
									phasor.phase = phase - std::floor(phase);
									
									for (auto s = 0; s < numSamples; ++s)
									{
										buf[s] = phasor.phase + tempoSync.phaseSmooth(phaseV);
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

									if(rateSmoothing)
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
					if (numChannelsOut != 2)
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
						for (auto ch = 0; ch < numChannelsOut; ++ch)
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
						for (auto ch = 0; ch < numChannelsOut; ++ch)
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

			int numChannels;
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
			perlin.prepare(sampleRate, maxBlockSize);
			audioRate.prepare(sampleRate, maxBlockSize);
			dropout.prepare(sampleRate, maxBlockSize);
			envFol.prepare(sampleRate, maxBlockSize);
			macro.prepare(sampleRate);
			pitchbend.prepare(sampleRate);
			lfo.prepare(sampleRate, maxBlockSize, latency);
		}

		// parameters
		void setParametersPerlin(float rate, float octaves, float width, float seed) noexcept
		{
			perlin.setParameters(rate, octaves, width, seed);
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
			juce::AudioPlayHead* playHead, int numChannelsIn, int numChannelsOut, int numSamples) noexcept
		{
			switch (type)
			{
			case ModType::Perlin: return perlin(buffer, numChannelsOut, numSamples, playHead);
			case ModType::AudioRate: return audioRate(buffer, midi, numChannelsOut, numSamples);
			case ModType::Dropout: return dropout(buffer, numChannelsOut, numSamples);
			case ModType::EnvFol: return envFol(buffer, samples, numChannelsIn, numChannelsOut, numSamples);
			case ModType::Macro: return macro(buffer, numChannelsOut, numSamples);
			case ModType::Pitchwheel: return pitchbend(buffer, numChannelsOut, numSamples, midi);
			case ModType::LFO: return lfo(buffer, numChannelsOut, numSamples, playHead);
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

make trigger mod
	trigger type (midi note, automation(&button), onset envelope)
	waveform (sinc, tapestop, tapestart, dropout)

envFol
	lookahead?

make konami mod

*/

#undef DebugAudioRateEnv
#undef DebugPerlinPhase
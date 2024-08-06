#pragma once
#include <juce_core/juce_core.h>
#include <functional>

namespace dsp
{
	using String = juce::String;
	static constexpr double Pi = 3.1415926535897932384626433832795;
	static constexpr double Tau = 2. * Pi;

	template<typename Float, size_t Size>
	struct Wavetable
	{
		using Func = std::function<Float(Float x)>;

		void makeTableWeierstrass(Float wt)
		{
			auto weierstrass = [](Float a, Float b, Float x, int N)
			{
				auto smpl = static_cast<Float>(0);
				for (auto n = 0; n < N; ++n)
				{
					const auto nF = static_cast<Float>(n);
					smpl += std::pow(a, nF) * std::cos(std::pow(b, nF) * x);
				}
				return smpl;
			};

			fill([weierstrass, wt](Float x)
			{
				const auto xpi = x * static_cast<Float>(Pi);
				const auto a = wt * static_cast<Float>(.5);
				const auto b = static_cast<Float>(1) + static_cast<Float>(2) * sqrt(wt);

				return weierstrass(a, b, xpi, 32);
			}, false, true);
		}

		void makeTableTriangle(Float wt)
		{
			fill([wt](Float x)
			{
				auto wt2 = wt * 2.f - 1.f;

				auto pi = static_cast<Float>(Pi);

				auto A = static_cast<Float>(2) * std::asin(std::sin(x * pi + pi * static_cast<Float>(.5))) / pi;
				auto B = static_cast<Float>(1) - static_cast<Float>(8) * std::pow(x * static_cast<Float>(.5), static_cast<Float>(2));

				return A + wt2 * (B - A);

			}, false, false);
		}

		void makeTableSinc(Float wt)
		{
			fill([wt](Float x)
			{
				if (x == static_cast<Float>(0))
					return static_cast<Float>(1) - wt;
				
				auto pi = static_cast<Float>(Pi);
				auto xpi = x * pi;
				auto xpi2 = xpi * static_cast<Float>(2);
					
				const auto A = std::sin(xpi2) / xpi2;
				const auto B = (static_cast<Float>(1) - cos(xpi2)) / (static_cast<Float>(1.5) * xpi);
				
				return A + wt * (B - A);

			}, false, true);
		}

		void makePWMSine(Float wt)
		{
			// POWER SINE
			/*
			const auto n = static_cast<Float>(.5);

			auto m = [n](Float x)
			{
				const auto sgn = x < 0 ? -1.f : 1.f;
				return sgn * std::pow(std::abs(x), n);
			};

			const auto tmod = [](Float t)
			{
				const auto zero = static_cast<Float>(0);
				const auto one = static_cast<Float>(1);
				auto md = std::fmod(t, one);
				if(t < zero)
					++md;
				return md;
			};

			const auto lpd = [tmod](Float t)
			{
				return tmod(t);
			};

			const auto mpd1 = [](Float t, Float d, Float tmodt)
			{
				const auto p5 = static_cast<Float>(.5);
				const auto p5md = p5 - d;
				return p5md * tmodt / d;
			};

			const auto mpd2 = [](Float t, Float d, Float tmodt)
			{
				const auto p5 = static_cast<Float>(.5);
				const auto one = static_cast<Float>(1);
				const auto p5md = p5 - d;
				return p5md * (one - tmodt) / (one - d);
			};

			const auto mpd = [wt, tmod, mpd1, mpd2](Float t)
			{
				const auto d = wt;
				const auto tmodt = tmod(t);
				if (tmodt < d)
					return mpd1(t, d, tmodt);
				else
					return mpd2(t, d, tmodt);
			};

			const auto fpd = [mpd, lpd](Float t)
			{
				return mpd(t) + lpd(t);
			};

			const auto fsinepd = [fpd](Float t)
			{
				return std::sin(static_cast<Float>(Tau) * fpd(t * static_cast<Float>(.5)));
			};

			fill([m, fsinepd](Float x)
			{
				return m(fsinepd(x));
			}, false, false);
			*/
			
			//UNIT CIRCLE
			/*
			const auto f = [](Float x)
			{
				const auto one = static_cast<Float>(1);
				return std::sqrt(std::abs(one - x * x));
			};

			fill([wt, f](Float x)
			{
				const auto one = static_cast<Float>(1);
				const auto two = static_cast<Float>(2);
				const auto d = wt;
				const auto g = two * d - one;
				const auto d3 = one / d;
				const auto onemd = one - d;
				const auto d4 = one / onemd;
				const auto j = f((x + onemd) * d3);
				const auto k = -f((x - d) * d4);
				const auto y = x <= g ? j : k;
				return y;
			}, false, false);
			*/
			
			//SQRSIN
			/*
			fill([wt](Float x)
			{
				const auto one = static_cast<Float>(1);
				const auto two = static_cast<Float>(2);
				const auto pi = static_cast<Float>(Pi);
				const auto d = wt;
				const auto g = two * d - one;
				const auto d1 = one / (two * d);
				const auto d2 = one / (two * (one - d));
				const auto s1 = std::sin((x + one) * pi * d1);
				const auto s2 = std::sin((x - one) * pi * d2);
				//const auto y = x <= g ? s1 : s2;
				const auto y = x <= g ? std::sqrt(s1) : -std::sqrt(-s2);
				return y;
			}, false, false);
			*/
			
			//MIXEDSIN
			fill([wt](Float x)
			{
				const auto one = static_cast<Float>(1);
				const auto two = static_cast<Float>(2);
				const auto pi = static_cast<Float>(Pi);
				const auto d = wt;
				const auto g = two * d - one;
				const auto d1 = one / (two * d);
				const auto d2 = one / (two * (one - d));
				const auto s1 = std::sin((x + one) * pi * d1);
				const auto s2 = std::sin((x - one) * pi * d2);
				const auto y0 = x <= g ? s1 : s2;
				const auto y1 = x <= g ? std::sqrt(s1) : -std::sqrt(-s2);
				const auto frac = std::abs(two * d - 1);
				const auto y = y0 + frac * (y1 - y0);
				return y;
			}, false, false);
		}

		void makeSqueeze(Float wt)
		{
			const auto one = static_cast<Float>(1);
			const auto two = static_cast<Float>(2);
			const auto zero = static_cast<Float>(0);
			const auto pi = static_cast<Float>(Pi);

			const auto h = [one, two](Float a, Float b)
			{
				const auto ab = a * b;
				return ab / (one - a - b + two * ab);
			};

			const auto f = [zero, wt, h](Float x)
			{
				const auto k = wt;
				return x < zero ? -h(-x, k) : h(x, k);
			};

			fill([wt, f, pi](Float x)
			{
				return std::sin(f(x) * pi);
			}, false, false);
		}

		Wavetable() :
			table()
		{}

		void fill(const Func& func, bool removeDC, bool normalize)
		{
			static constexpr Float SizeInv = static_cast<Float>(1.) / static_cast<Float>(Size);

			// SYNTHESIZE WAVE
			for (auto s = 0; s < Size; ++s)
			{
				auto x = static_cast<Float>(2) * static_cast<Float>(s) * SizeInv - static_cast<Float>(1);
				table[s] = func(x);
			}

			if (removeDC)
			{
				auto sum = static_cast<Float>(0);
				for (const auto& s : table)
					sum += s;
				sum *= SizeInv;
				if (sum != static_cast<Float>(0))
					for (auto& s : table)
						s -= sum;
			}

			if (normalize)
			{
				auto max = static_cast<Float>(0);
				for (const auto& s : table)
				{
					const auto a = std::abs(s);
					if (max < a)
						max = a;
				}
				if (max != static_cast<Float>(0) && max != static_cast<Float>(1))
				{
					const auto g = static_cast<Float>(1) / max;
					for (auto& s : table)
						s *= g;
				}
			}

			// COPY FIRST ENTRY/IES FOR INTERPOLATION
			for (auto s = Size; s < table.size(); ++s)
				table[s] = table[s - Size];
		}

		Float operator[](Float x) const noexcept
		{
			static constexpr Float SizeF = static_cast<Float>(Size);
			x = x * SizeF;
			const auto xFloor = std::floor(x);
			const auto i0 = static_cast<int>(xFloor);
			const auto i1 = i0 + 1;
			const auto frac = x - xFloor;
			return table[i0] + frac * (table[i1] - table[i0]);
		}

		Float operator[](int idx) const noexcept
		{
			return table[idx];
		}

	protected:
		std::array<Float, Size + 2> table;
	};

	template<typename Float, size_t WTSize, size_t NumTables>
	struct Wavetable2D
	{
		static constexpr Float MaxTablesF = static_cast<Float>(NumTables - 1);
		using Table = Wavetable<Float, WTSize>;
        using Func = typename Table::Func;
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
			for (auto i = NumTables; i < tables.size(); ++i)
				tables[i] = tables[i - NumTables];
		}

		Float operator()(int tablesIdx, int tableIdx) const noexcept
		{
			return tables[tablesIdx][tableIdx];
		}

		Float operator()(int tablesIdx, Float tablePhase) const noexcept
		{
			return tables[tablesIdx][tablePhase];
		}

		Float operator()(Float tablesPhase, int tableIdx) const noexcept
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

		Float operator()(Float tablesPhase, Float tablePhase) const noexcept
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

	template<typename Float, size_t WTSize, size_t NumTables>
	struct Wavetable3D
	{
		static constexpr Float NumTablesInv = static_cast<Float>(1) / static_cast<Float>(NumTables);

		using Table = Wavetable2D<Float, WTSize, NumTables>;
        using Func = typename Table::Func;
		using Funcs = std::array<Func, NumTables>;

		void makeTablesWeierstrass()
		{
			name = "Weierstrass";
			auto wt = NumTablesInv * static_cast<Float>(.5);
			for (auto n = 0; n < NumTables; ++n, wt += NumTablesInv)
			{
				tables[n].makeTableWeierstrass(wt);
			}
		}

		void makeTablesTriangles()
		{
			name = "Triangle";
			auto wt = NumTablesInv * static_cast<Float>(.5);
			for (auto n = 0; n < NumTables; ++n, wt += NumTablesInv)
			{
				tables[n].makeTableTriangle(wt);
			}
				
		}

		void makeTablesSinc()
		{
			name = "Sinc";
			auto wt = NumTablesInv * static_cast<Float>(.5);
			for (auto n = 0; n < NumTables; ++n, wt += NumTablesInv)
				tables[n].makeTableSinc(wt);
		}

		void makeTablesPWMSine()
		{
			name = "PWM Sine";
			auto wt = NumTablesInv * static_cast<Float>(.5);
			for (auto n = 0; n < NumTables; ++n, wt += NumTablesInv)
				tables[n].makePWMSine(wt);
		}

		void makeSqueeze()
		{
			name = "Squeeze";
			auto wt = NumTablesInv * static_cast<Float>(.5);
			for (auto n = 0; n < NumTables; ++n, wt += NumTablesInv)
				tables[n].makeSqueeze(wt);
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

		Float operator()(Float tablesPhase, Float tablePhase) const noexcept
		{
			return tables(tablesPhase, tablePhase);
		}

		Float operator()(Float tablesPhase, int tableIdx) const noexcept
		{
			return tables(tablesPhase, tableIdx);
		}

		Float operator()(int tablesIdx, Float tablePhase) const noexcept
		{
			return tables(tablesIdx, tablePhase);
		}

		Float operator()(int tablesIdx, int tableIdx) const noexcept
		{
			return tables(tablesIdx, tableIdx);
		}

		Table tables;
		String name;
	};

	enum TableType { Weierstrass, Tri, Sinc, PWMSine, Squeeze, NumTypes };

	inline String toString(TableType t)
	{
		switch (t)
		{
		case TableType::Weierstrass: return "Weierstrass";
		case TableType::Tri: return "Triangle";
		case TableType::Sinc: return "Sinc";
		case TableType::PWMSine: return "PWM Sine";
		case TableType::Squeeze: return "Squeeze";
		default: return "";
		}
	}

	static constexpr int LFOTableSize = 1 << 11;
	static constexpr int LFONumTables = (1 << 5) + 1;
	using LFOTables = Wavetable3D<double, LFOTableSize, LFONumTables>;
}

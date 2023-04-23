#pragma once
#include <juce_core/juce_core.h>
#include <functional>

namespace dsp
{
	using String = juce::String;
	static constexpr double Pi = 3.1415926535897932384626433832795;

	template<typename Float, size_t Size>
	struct Wavetable
	{
		using Func = std::function<Float(Float x)>;

		void makeTableWeierstrasz(Float a, Float b, int N)
		{
			fill([N, b, a](Float x)
			{
				auto smpl = 0.;
				for (auto n = 0; n < N; ++n)
				{
					const auto nF = static_cast<Float>(n);
					smpl += std::pow(a, nF) * std::cos(std::pow(b, nF) * x * Pi);
				}
				return smpl;
			}, true, true);
		}

		void makeTableTriangle(int n)
		{
			const auto tri = [&](Float x, Float fc, Float phase)
			{
				return static_cast<Float>(1) - static_cast<Float>(2) * std::acos(std::cos(fc * x * Pi + phase * Pi)) / Pi;
			};

			fill([tri, n](Float x)
			{
				const auto nF = static_cast<Float>(n + 1);
				const auto n8 = static_cast<Float>(n) / static_cast<Float>(8);
				return tri(x, static_cast<Float>(1), static_cast<Float>(0)) + tri(x, nF, n8) / nF;

			}, true, true);
		}

		void makeTableSinc(bool window, int N)
		{
			const auto wndw = window ? [](Float xpi)
			{
				if (xpi == static_cast<Float>(0)) return static_cast<Float>(1);
				return std::sin(xpi) / xpi;
			} :
			[](Float)
			{
				return static_cast<Float>(1);
			};
			const auto sinc = [](Float xPiA)
			{
				if (xPiA == static_cast<Float>(0.f)) return static_cast<Float>(1);
				return static_cast<Float>(2) * std::sin(xPiA) / xPiA - static_cast<Float>(1);
			};

			fill([wndw, sinc, N](Float x)
			{
				const auto xpi = x * Pi;
				const auto tablesInv = static_cast<Float>(1) / static_cast<Float>(N);

				auto smpl = static_cast<Float>(0);
				for (auto n = 0; n < N; ++n)
				{
					const auto nF = static_cast<float>(n);
					const auto x2 = (nF + static_cast<Float>(2)) * x * static_cast<Float>(.5);
					const auto a2 = static_cast<Float>(2) * nF + static_cast<Float>(1);

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
		using Func = Table::Func;
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
		using Table = Wavetable2D<Float, WTSize, NumTables>;
		using Func = Table::Func;
		using Funcs = std::array<Func, NumTables>;

		void makeTablesWeierstrasz()
		{
			name = "Weierstrasz";
			tables[0].makeTableWeierstrasz(static_cast<Float>(0), static_cast<Float>(0.), 1);
			tables[1].makeTableWeierstrasz(static_cast<Float>(.0625), static_cast<Float>(7.), 8);
			tables[2].makeTableWeierstrasz(static_cast<Float>(.125), static_cast<Float>(5.), 5);
			tables[3].makeTableWeierstrasz(static_cast<Float>(.1875), static_cast<Float>(4.), 4);
			tables[4].makeTableWeierstrasz(static_cast<Float>(.25), static_cast<Float>(3.), 3);
			tables[5].makeTableWeierstrasz(static_cast<Float>(.3125), static_cast<Float>(3.), 4);
			tables[6].makeTableWeierstrasz(static_cast<Float>(.375), static_cast<Float>(3.), 3);
			tables[7].makeTableWeierstrasz(static_cast<Float>(.4375), static_cast<Float>(2.), 6);
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

	enum TableType { Weierstrasz, Tri, Sinc, NumTypes };

	inline String toString(TableType t)
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
	using LFOTables = Wavetable3D<double, LFOTableSize, LFONumTables>;
}
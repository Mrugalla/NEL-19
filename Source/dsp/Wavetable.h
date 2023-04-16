#pragma once
#include <juce_core/juce_core.h>
#include <functional>

namespace dsp
{
	using String = juce::String;

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

			}, true, true);
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
			for (auto i = NumTables; i < tables.size(); ++i)
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
	using LFOTables = Wavetable3D<LFOTableSize, LFONumTables>;
}
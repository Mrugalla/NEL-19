#pragma once

namespace interpolation {
	static constexpr float Pi = 3.14159265359f;
	
	static inline float sinc(const float xPi) noexcept { return std::sin(xPi) / xPi; }

	/*
	* I0 => 0th-order modified bessel function
	* L => window duration
	* a > 0 => determines shape of window (trade-off between main lobe and side lobe)
	*/
	static float kaiser(float x, float I0, float a, float L) {
		const auto LHalf = L * .5f;
		if (std::abs(x) <= LHalf) {
			const auto x2InvL = 2.f * x / L;
			const auto beta = a * Pi;
			return I0 * (beta * std::sqrt(1.f - x2InvL * x2InvL)) / (I0 * beta);
		}
		else
			return 0.f;
	}
	/*
	* 0 <= n <= N
	* n => ???
	* N + 1 => window length (can be even or odd)
	* L => ???
	* w0 => ???
	*/
	static float kaiser2(float n, float N, float w0, float L) {
		return w0 * (L / N * (n - N * .5f));
	}

	static inline float lanczosSinc(const float* buffer, const float readHead, const int size, const int alpha = 7) noexcept {
		const auto iFloor = std::floor(readHead);		// N [0, size)
		const auto iFloorInt = static_cast<int>(iFloor);// N [0, size)
		const auto x = readHead - iFloor;				// R [0, 1)
		const auto alphaInv = 1.f / static_cast<float>(alpha);
		
		auto sum = 0.f;
		for (auto i = -alpha; i < alpha; ++i) { // N [-alpha, alpha)
			const auto lx = x - i;				// R [-alpha, alpha)
			float ly;							// R [0, 1]
			if (lx == 0.f) ly = 1.f;
			else if (lx != 0.f && -alpha <= lx && lx < alpha) {
				const auto lxPi = lx * Pi;
				ly = sinc(lxPi) * sinc(lxPi * alphaInv);
			}
			else ly = 0.f;

			auto idx = iFloorInt + i; // N [iFloor - alpha, iFloor + alpha]
			if (idx < 0) idx += size;
			else if (idx >= size) idx -= size;
			sum += ly * buffer[idx];
		}
		return sum;
	}


	static inline float cubicHermitSpline(const float* buffer, const float readHead, const int size) {
		auto i1 = static_cast<int>(readHead);
		auto i0 = i1 - 1;
		auto i2 = i1 + 1;
		auto i3 = i1 + 2;
		while (i3 >= size) i3 -= size;
		while (i2 >= size) i2 -= size;
		while (i0 < 0) i0 += size;

		const auto frac = readHead - i1;
		const auto v0 = buffer[i0];
		const auto v1 = buffer[i1];
		const auto v2 = buffer[i2];
		const auto v3 = buffer[i3];

		const auto c0 = v1;
		const auto c1 = .5f * (v2 - v0);
		const auto c2 = v0 - 2.5f * v1 + 2.f * v2 - .5f * v3;
		const auto c3 = 1.5f * (v1 - v2) + .5f * (v3 - v0);

		return ((c3 * frac + c2) * frac + c1) * frac + c0;
	}
}

/*

find out why this sinc interpolator makes noise in the synfun project's 3rd loop
	(-80db but still audible)

*/
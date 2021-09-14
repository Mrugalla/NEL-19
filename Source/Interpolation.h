#pragma once

namespace interpolation {
	static constexpr float Pi = 3.14159265359f;
	
	static float sinc(const float xPi) noexcept { return std::sin(xPi) / xPi; }

	namespace window {
		static float lanczos(const float xPi, const float alphaInv) noexcept { return sinc(xPi * alphaInv); }
		static float circular(const float x, const float alphaInv) noexcept {
			const auto a = x * alphaInv;
			return std::sqrt(1.f - a * a);
		}
	}

	static float lanczosSinc(const float* buffer, const float readHead, const int size, const int alpha) noexcept {
		const auto iFloor = std::floor(readHead);
		const auto iFloorInt = static_cast<int>(iFloor);
		const auto x = readHead - iFloor;
		const auto alphaInv = 1.f / static_cast<float>(alpha);
		
		auto sum = 0.f;
		for (auto i = -alpha; i <= alpha; ++i) {
			const auto lx = x - static_cast<float>(i);
			float ly;
			if (lx == 0.f) ly = 1.f;
			else if (-alpha < lx && lx <= alpha) {
				const auto lxPi = lx * Pi;
				ly = sinc(lxPi) * sinc(lxPi * alphaInv);
			}
			else ly = 0.f;

			auto idx = iFloorInt + i;
			if (idx < 0) idx += size;
			else if (idx >= size) idx -= size;
			sum += ly * buffer[idx];
		}
		return sum;
	}

	static float lerp(float a, float b, float x) noexcept { return a + x * (b - a); }

	static float lerp(const float* buffer, const float readHead, const int size) {
		const auto iFloor = std::floor(readHead);
		const auto i0 = static_cast<int>(iFloor);
		auto i1 = i0 + 1;
		if (i1 >= size)
			i1 -= size;
		const auto x = readHead - iFloor;
		return lerp(buffer[i0], buffer[i1], x);
	}

	static float cubicHermiteSpline(const float* buffer, const float readHead, const int size) {
		const auto iFloor = std::floor(readHead);
		auto i1 = static_cast<int>(iFloor);
		auto i0 = i1 - 1;
		auto i2 = i1 + 1;
		auto i3 = i1 + 2;
		if (i3 >= size) i3 -= size;
		if (i2 >= size) i2 -= size;
		if (i0 < 0) i0 += size;

		const auto t = readHead - iFloor;
		const auto v0 = buffer[i0];
		const auto v1 = buffer[i1];
		const auto v2 = buffer[i2];
		const auto v3 = buffer[i3];

		const auto c0 = v1;
		const auto c1 = .5f * (v2 - v0);
		const auto c2 = v0 - 2.5f * v1 + 2.f * v2 - .5f * v3;
		const auto c3 = 1.5f * (v1 - v2) + .5f * (v3 - v0);

		return ((c3 * t + c2) * t + c1) * t + c0;
	}

	static float lagrange(const float* buffer, const float readHead, const int size, int N) {
		const auto iFloor = std::floor(readHead);
		const auto iFloorInt = static_cast<int>(iFloor);
		auto yp = 0.f;
		for (int i = 1; i <= N; ++i) {
			auto p = buffer[iFloorInt + i];
			for (int j = 1; j <= N; ++j)
				if (j != i)
					p *= (readHead - static_cast<float>(iFloorInt - j)) / static_cast<float>(i - j);
			yp += p;
		}
		return yp;
	}
}

/*

find out why this sinc interpolator makes noise in the synfun project's 3rd loop
	(-80db but still audible)
	more apparent on bigger resampling speed (oversampling needed?)

*/
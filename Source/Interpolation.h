#pragma once

namespace interpolation {
	// INTERPOLATION CONST
	static constexpr int MaxInterpolationOrder = 7;
	static constexpr int SincResolution = 1 << 9;

	struct Lanczos {
		Lanczos(int a = MaxInterpolationOrder, int sincRes = SincResolution) :
			sinc(a, sincRes),
			alpha(a)
		{}
		struct SincLUT {
			SincLUT(int alpha, int sincReso) :
				lut()
			{
				sincRes = static_cast<float>(sincReso);
				const auto size = sincReso + 2;
				lut.reserve(size);
				max = alpha * util::Pi;
				lut.emplace_back(1.f);
				for (auto i = 1; i < size; ++i) {
					const auto x = static_cast<float>(i) / SincResolution;
					const auto mapped = x * alpha * util::Pi;
					lut.emplace_back(sinc(mapped));
				}
			}
			const float operator[](const float idx) const {
				const auto normal = std::abs(idx / max);
				const auto upscaled = normal * sincRes;
				const auto mapF = static_cast<int>(upscaled);
				const auto mapC = mapF + 1;
				const auto x = upscaled - mapF;
				const auto s0 = lut[mapF];
				const auto s1 = lut[mapC];
				return s0 + x * (s1 - s0);
			}
		private:
			std::vector<float> lut;
			float max, sincRes;

			float sinc(float xPi) { return std::sin(xPi) / xPi; }
		};
		const float operator()(const std::vector<float>& buffer, const float idx) const {
			const auto iFloor = static_cast<int>(idx);
			const auto x = idx - iFloor;
			const auto size = static_cast<int>(buffer.size());

			auto sum = 0.f;
			for (auto i = -alpha + 1; i < alpha; ++i) {
				auto iLegal = i + iFloor;
				if (iLegal < 0) iLegal += size;
				else if (iLegal >= size) iLegal -= size;

				auto xi = x - i;
				if (xi == 0.f) xi = 1.f;
				else if (x > -alpha && x < alpha) {
					auto xPi = xi * util::Pi;
					xi = sinc[xPi] * sinc[xPi / alpha];
				}
				else xi = 0.f;

				sum += buffer[iLegal] * xi;
			}
			return sum;
		}
		const float operator()(const std::vector<juce::Point<float>>& buffer, const float idx) const {
			const auto iFloor = static_cast<int>(idx);
			const auto x = idx - iFloor;
			const auto size = static_cast<int>(buffer.size());

			auto sum = 0.f;
			for (auto i = -alpha + 1; i < alpha; ++i) {
				auto iLegal = i + iFloor;
				if (iLegal < 0)
					while (iLegal < 0) iLegal += size;
				else if (iLegal >= size)
					while (iLegal >= size) iLegal -= size;

				auto xi = x - i;
				if (xi == 0.f) xi = 1.f;
				else if (x > -alpha && x < alpha) {
					auto xPi = xi * util::Pi;
					xi = sinc[xPi] * sinc[xPi / alpha];
				}
				else xi = 0.f;

				sum += buffer[iLegal].y * xi;
			}
			return sum;
		}
	private:
		SincLUT sinc;
		int alpha;
	};

	struct Spline {
		// hermit cubic spline
		// hornersheme
		// thx peter
		static float process(const std::vector<juce::Point<float>>& data, float x) {
			const auto size = static_cast<int>(data.size());
			auto i1 = static_cast<int>(x);
			auto i0 = i1 - 1;
			auto i2 = i1 + 1;
			auto i3 = i1 + 2;
			while (i3 >= size) i3 -= size;
			while (i2 >= size) i2 -= size;
			while (i0 < 0) i0 += size;

			const auto frac = x - i1;
			const auto v0 = data[i0].y;
			const auto v1 = data[i1].y;
			const auto v2 = data[i2].y;
			const auto v3 = data[i3].y;

			const auto c0 = v1;
			const auto c1 = .5f * (v2 - v0);
			const auto c2 = v0 - 2.5f * v1 + 2.f * v2 - .5f * v3;
			const auto c3 = 1.5f * (v1 - v2) + .5f * (v3 - v0);

			return ((c3 * frac + c2) * frac + c1) * frac + c0;
		}
	};
}


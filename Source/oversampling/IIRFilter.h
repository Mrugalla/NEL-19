#pragma once
#include "Filter.h"
#include <array>

namespace oversampling
{
	template<typename Float>
	struct IIR
	{
		void makeChebyshev_lp_4pole_fc45_ripl5() noexcept
		{
			a0 = static_cast<Float>(6.291693e-01);
			a1 = static_cast<Float>(2.516677e+00);
			a2 = static_cast<Float>(3.775016e+00);
			a3 = static_cast<Float>(2.516677e+00);
			a4 = static_cast<Float>(6.291693e-01);
			b1 = static_cast<Float>(-3.077062e+00);
			b2 = static_cast<Float>(-3.641323e+00);
			b3 = static_cast<Float>(-1.949229e+00);
			b4 = static_cast<Float>(-3.990945e-01);
		}

		IIR() :
			a0(1.f), a1(0.f), a2(0.f), a3(0.f), a4(0.f),
			b1(0.f), b2(0.f), b3(0.f), b4(0.f),
			x1(0.f), x2(0.f), x3(0.f), x4(0.f),
			y1(0.f), y2(0.f), y3(0.f), y4(0.f)
		{
			
		}
		IIR(float _a0, float _a1, float _a2, float _a3, float _a4,
			float _b1, float _b2, float _b3, float _b4) :
			a0(_a0), a1(_a1), a2(_a2), a3(_a3), a4(_a4),
			b1(_b1), b2(_b2), b3(_b3), b4(_b4),
			x1(0.f), x2(0.f), x3(0.f), x4(0.f),
			y1(0.f), y2(0.f), y3(0.f), y4(0.f)
		{

		}

		void processBlock(Float* samples, int numSamples) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				samples[s] = processSample(samples[s]);
		}
		Float processSample(Float x0) noexcept
		{
			const auto y0 =
				x0 * a0
				+ x1 * a1
				+ x2 * a2
				+ x3 * a3
				+ x4 * a4
				+ y1 * b1
				+ y2 * b2
				+ y3 * b3
				+ y4 * b4;

			x4 = x3;
			x3 = x2;
			x2 = x1;
			x1 = x0;

			y4 = y3;
			y3 = y2;
			y2 = y1;
			y1 = y0;

			return y0;
		}
	protected:
		Float a0, a1, a2, a3, a4, b1, b2, b3, b4;
		Float 	  x1, x2, x3, x4, y1, y2, y3, y4;
	};

	// FC = fc [0, .5]
	// LH = isHighpass [0, 1]
	// PR = ripple [0, 29]
	// NP = numPoles [2, 4, ..., 20]
	template<typename Float>
	struct MakeChebyshev
	{
		MakeChebyshev(IIR<Float>& filter, Float FC, int LH, Float PR, Float NP)
		{
			fc = FC;
			lh = LH;
			pr = PR;
			np = NP;

			for (int i = 0; i < 22; ++i)
				a[i] = b[i] = 0;

			a[2] = b[2] = 1;

			npHalf = np * static_cast<Float>(.5);
			npInv = static_cast<Float>(1) / np;
			piInvNp = pi / np;
			piInvNp2 = pi / (np * static_cast<Float>(2));
			if (pr != static_cast<Float>(0))
			{
				es = std::sqrt(std::pow(static_cast<Float>(100) / (static_cast<Float>(100) - pr), static_cast<Float>(2)) - static_cast<Float>(1));
				esInv = static_cast<Float>(1) / es;
				esesInv = static_cast<Float>(1) / (es * es);
				vx = npInv * std::log(esInv + std::sqrt(esesInv + static_cast<Float>(1)));
				kx = npInv * std::log(esInv + std::sqrt(esesInv - static_cast<Float>(1)));
				kx = (std::exp(kx) + std::exp(-kx)) * static_cast<Float>(.5);
				kxInv = static_cast<Float>(1) / kx;
				vxExp = std::exp(vx);
				vxExpM = std::exp(-vx);
				vxY = (vxExp - vxExpM) * static_cast<Float>(.5) * kxInv;
				vxZ = (vxExp + vxExpM) * static_cast<Float>(.5) * kxInv;
			}
			t = static_cast<Float>(2) * std::tan(static_cast<Float>(.5));
			tt = t * t;
			w = tau * fc;

			for (int p = 1; p <= npHalf; ++p)
			{
				subRoutine(p);

				for (int i = 0; i < 22; ++i)
				{
					ta[i] = a[i];
					tb[i] = b[i];
				}

				for (int i = 2; i < 22; ++i)
				{
					int i1 = i - 1;
					int i2 = i - 2;

					a[i] = a0 * ta[i] + a1 * ta[i1] + a2 * ta[i2];
					b[i] = tb[i] - b1 * tb[i1] - b2 * tb[i2];
				}
			}

			b[2] = static_cast<Float>(0);
			for (int i = 0; i < 20; ++i)
			{
				a[i] = a[i + 2];
				b[i] = b[i + 2];
			}

			sa = 0.;
			sb = 0.;

			for (int i = 0; i < 20; ++i)
			{
				if (LH == 0)
				{
					sa += a[i];
					sb += b[i];
				}
				else
				{
					double iPow = std::pow(static_cast<Float>(-1), static_cast<Float>(i));
					sa += a[i] * iPow;
					sa += b[i] * iPow;
				}
			}

			gain = static_cast<Float>(1) / (sa / (static_cast<Float>(1) - sb));

			for (int i = 0; i < 20; ++i)
			{
				a[i] *= gain;
			}

			filter = IIR<Float>(a0, a1, a2, 0, 0, b1, b2, 0, 0);
		}

		void subRoutine(int p)
		{
			int p1 = p - 1;

			rp = -std::cos(piInvNp2 + p1 * piInvNp);
			ip = std::sin(piInvNp2 + p1 * piInvNp);

			if (pr != static_cast<Float>(0))
			{
				rp = rp * vxY;
				ip = ip * vxZ;
			}

			m = rp * rp + ip * ip;
			d = static_cast<Float>(4) - static_cast<Float>(4) * rp * t + m * tt;
			dInv = 1. / d;

			x0 = tt * dInv;
			x1 = static_cast<Float>(2) * x0;
			x2 = x0;
			y1 = (static_cast<Float>(8) - static_cast<Float>(2) * m * tt) * dInv;
			y2 = (static_cast<Float>(-4) - static_cast<Float>(4) * rp * t - m * tt) * dInv;

			wHalf = w * static_cast<Float>(.5);
			if (lh == 1)
				k = -std::cos(wHalf + static_cast<Float>(.5)) / std::cos(wHalf - static_cast<Float>(.5));
			else
				k = std::sin(static_cast<Float>(.5) - wHalf) / std::sin(static_cast<Float>(.5) + wHalf);

			kk = k * k;
			d = static_cast<Float>(1) + y1 * k - y2 * kk;
			dInv = static_cast<Float>(1) / d;

			a0 = (x0 - x1 * k + x2 * kk) * dInv;
			a1 = (static_cast<Float>(-2) * x0 * k + x1 + x1 * kk - static_cast<Float>(2) * x2 * k) * dInv;
			a2 = (x0 * kk - x1 * k + x2) * dInv;
			b1 = (static_cast<Float>(2) * k + y1 + y1 * kk - static_cast<Float>(2) * y2 * k) * dInv;
			b2 = (-kk - y1 * k + y2) * dInv;

			if (lh == 1)
			{
				a1 = -a1;
				b1 = -b1;
			}
		}

		std::array<Float, 22> a, b, ta, tb;
		Float sa, sb, gain, fc, lh, pr, np, npHalf, npInv, piInvNp, piInvNp2;
		Float rp, ip, es, esInv, esesInv, vx, vxExp, vxExpM, vxY, vxZ, kx, kxInv;
		Float t, tt, w, wHalf, m, d, dInv, x0, x1, x2, y1, y2, k, kk, a0, a1, a2, b1, b2;
	};

	template<typename Float>
	struct LowkeyChebyshevFilter
	{
		LowkeyChebyshevFilter(int _numChannels = 0) :
			filters(),
			numChannels(_numChannels)
		{
			filters.resize(numChannels);
			//MakeChebyshev<Float>(filters[0], .1, 0, 0, 4);
			//for (auto ch = 1; ch < numChannels; ++ch)
			//	filters[ch] = filters[0];
			
			for (auto& filter : filters)
				filter.makeChebyshev_lp_4pole_fc45_ripl5();
		}
		int getLatency() const noexcept { return 0; }
		void processBlock(Float** audioBuffer, const int numSamples) noexcept
		{
			for (auto ch = 0; ch < numChannels; ++ch)
				filters[ch].processBlock(audioBuffer[ch], numSamples);
		}
		float processSample(Float sample, int ch) noexcept
		{
			return filters[ch].processSample(sample);
		}
	protected:
		std::vector<IIR<Float>> filters;
		int numChannels;
	};
}
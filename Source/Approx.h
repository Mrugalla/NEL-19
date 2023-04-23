#pragma once

namespace approx
{
	static constexpr double Pi = 3.1415926535897932384626433832795;
    static constexpr double PiHalf = Pi * .5;
	static constexpr double Tau = Pi * 2.;
    
    /*
    * ranges:
    * x [-pi, pi]; y [-1, 1]
    */
    template<typename Float>
    inline Float taylor_sin(const Float x) noexcept
    {
        // https://www.wolframalpha.com/input/?i=sin%28x%29%2C+%28x+-+x%5E3%2F6+%2B+x%5E5%2F120+-+x%5E7%2F5040%29
        const auto x3 = x * x * x;
        const auto x5 = x3 * x * x;
        const auto x7 = x5 * x * x;
        return x - x3 * static_cast<Float>(.166666666667) +
            x5 * static_cast<Float>(.00833333333333) -
            x7 * static_cast<Float>(0.000198412698413);
    }
    /*
    * ranges:
    * x [-pi, pi]; y [-1, 1]
    */
    template<typename Float>
    inline Float taylor_cos(const Float x) noexcept
    {
        // https://www.wolframalpha.com/input/?i=Series%5Bcos%5Bx%5D%2C+%7Bx%2C+0%2C+7%7D
        const auto x2 = x * x;
        const auto x4 = x2 * x * x;
        const auto x6 = x4 * x * x;
        const auto x8 = x6 * x * x;
        return static_cast<Float>(1) -
            x2 * static_cast<Float>(.5) +
            x4 * static_cast<Float>(.0416666666667) -
            x6 * static_cast<Float>(.00138888888889) +
            x8 * static_cast<Float>(.0000248015873016);
    }

    /* if x == -3: y = -1, if x == 3: y = 1 */
    template<typename Float>
    inline Float tanh(Float x) noexcept
    {
        const auto xx = x * x;
        return x * (static_cast<Float>(27) + xx) / (static_cast<Float>(27) + static_cast<Float>(9) * xx);
    }
}
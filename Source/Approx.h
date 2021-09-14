#pragma once

namespace approx {
    static constexpr float Pi = 3.14159265359f;
    static constexpr float PiHalf = 1.57079632679f;
    static constexpr float Tau = 6.28318530718f;
    
    /*
    * ranges:
    * x [-pi, pi]; y [-1, 1]
    */
    static inline float taylor_sin(const float x) noexcept {
        // https://www.wolframalpha.com/input/?i=sin%28x%29%2C+%28x+-+x%5E3%2F6+%2B+x%5E5%2F120+-+x%5E7%2F5040%29
        const auto x3 = x * x * x;
        const auto x5 = x3 * x * x;
        const auto x7 = x5 * x * x;
        return x - x3 * .166666666667f + x5 * .00833333333333f - x7 * 0.000198412698413f;
    }
    /*
    * ranges:
    * x [-pi, pi]; y [-1, 1]
    */
    static inline float taylor_cos(const float x) noexcept {
        // https://www.wolframalpha.com/input/?i=Series%5Bcos%5Bx%5D%2C+%7Bx%2C+0%2C+7%7D
        const auto x2 = x * x;
        const auto x4 = x2 * x * x;
        const auto x6 = x4 * x * x;
        const auto x8 = x6 * x * x;
        return 1.f - x2 * .5f + x4 * .0416666666667f - x6 * .00138888888889f + x8 * .0000248015873016f;
    }
}
#pragma once

#include <fmt/format.h>
#include <hwy/highway.h>

#include <cstdint>
#include <string>

namespace simdtag {

#if 0
HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

class GradientPoint {
    using GradientPointV32 = hw::VFromD<ScalableTag<uint32_t>>;
    constexpr hw::ScalableTag<uint32_t> d_;

    GradientPoint
};

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();
#endif

class GradientPoint {
   public:
    GradientPoint(uint32_t value) : value_(value) {
    }

    GradientPoint() : value_(0) {
    }

    void SetX(int x) {
        value_ = (value_ & 0x000FFFFFu) | ((x * 2) << 20);
    }

    void SetY(int y) {
        value_ = (value_ & 0xFFF000FFu) | ((y * 2) << 8);
    }

    // dx and dy must be in range [-1, 1]
    void SetDxDy(int dx, int dy) {
        value_ &= ~0xF0;

        dx += 1;
        dy += 1;

        value_ |= (dx << 6 | dy << 4);
    }

    void SetBlackToWhite(int v0, int v1) {
        value_ &= ~1;
        value_ |= (v0 < v1);
    }

    float GetX() const {
        return static_cast<float>(value_ >> 20) / 2.0f;
    }

    float GetY() const {
        return static_cast<float>((value_ & 0x000FFF00u) >> 8) / 2.0f;
    }

    int GetIntX() const {
        return static_cast<float>(value_ >> 20) / 2.0f;
    }

    int GetIntY() const {
        return static_cast<float>((value_ & 0x000FFF00u) >> 8) / 2.0f;
    }

    int GetDx() const {
        return ((value_ >> 6) & 0x3) - 1;
    }

    int GetDy() const {
        return ((value_ >> 4) & 0x3) - 1;
    }

    bool GetBlackToWhite() const {
        return value_ & 1;
    }

    uint32_t RawValue() {
        return value_;
    }

    std::string ToString() {
        std::string result;
        fmt::format_to(std::back_inserter(result), "(x, y) ({},{}), Grad (x, y): ({},{}) B-->W: {}",
                       GetX(), GetY(), GetDx(), GetDy(), GetBlackToWhite());
        return result;
    }

   private:
    uint32_t value_;
};

}  // namespace simdtag

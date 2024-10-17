#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <string>

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

    void SetDxDy(int dx, int dy) {
        value_ &= ~0x6;
        uint32_t tmp = 0;

        value_ |= tmp << 1;
    }

    void SetBlackToWhite(int v0, int v1) {
        value_ &= ~1;
        value_ |= (v0 < v1);
    }

    float GetX() {
        return static_cast<float>(value_ >> 20) / 2.0f;
    }

    float GetY() {
        return static_cast<float>((value_ & 0x000FFF00u) >> 8) / 2.0f;
    }

    int GetDx() {
        switch (GradientValue()) {
            case 0:
            case 1:
                return 1;
            case 3:
                return -1;
            default:
                return 0;
        }
    }

    int GetDy() {
        return GradientValue() != 0;
    }

    bool GetBlackToWhite() {
        return value_ & 1;
    }

    int GradientValue() {
        return static_cast<int>((value_ & 0x6u) >> 1);
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

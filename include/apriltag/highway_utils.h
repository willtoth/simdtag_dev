#pragma once

#include <fmt/format.h>
#include <hwy/highway.h>
#include <hwy/print-inl.h>

namespace hw = hwy::HWY_NAMESPACE;

namespace apriltag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

template <class D>
inline void PrintMask(D d, const char* text, auto const& mask) {
    constexpr auto N = hw::Lanes(d);
    uint64_t converted_mask = 0;
    hw::StoreMaskBits(d, mask, (uint8_t*)&converted_mask);

    if (text) {
        fmt::println("{} {:x}", text, converted_mask);
    } else {
        fmt::println("{:x}", converted_mask);
    }
}

template <typename T>
constexpr T SetTopBit(T value, bool state) {
    constexpr auto msb = (sizeof(T) * CHAR_BIT) - 1;
    return (value & ~(T{1} << msb)) | (T{state} << msb);
}

template <class D>
inline auto ShiftLeftOneWithCarry(D d, auto const& vin) {
    using T = hw::TFromD<D>;

    T msb_set_mask = SetTopBit<T>(0, true);
    const auto va = hw::Set(d, msb_set_mask);

    const auto should_carry = (va & vin) > hw::Zero(d);
    const auto mask = hw::SlideMask1Up(d, should_carry);

    // Option 1
    const auto vshifted = hw::ShiftLeft<1>(vin);
    const auto vresult = hw::MaskedAddOr(vshifted, mask, vshifted, Set(d, 1));

    return vresult;
}

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

}  // namespace apriltag

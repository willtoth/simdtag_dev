#include "simdtag/highway_utils.h"

#include <gtest/gtest.h>
#include <stdint.h>

#include "hwy/highway.h"

namespace hw = hwy::HWY_NAMESPACE;

namespace simdtag {

HWY_BEFORE_NAMESPACE();
namespace HWY_NAMESPACE {

TEST(CarryShift, ButDoesItCarry) {
    uint64_t input[8] = {0x12AB34CD56789EFF, 0xABCD12345678A9BC, 0xDEADBEEF01234567,
                         0x1122334455667788, 0x2233445566778899, 0x99AABBCCDDEEFF00,
                         0xF0F1F2F3F4F5F6F7, 0x7F7E7D7C7B7A7978};

    uint64_t output[8] = {0x2556699AACF13DFE, 0x579A2468ACF15378, 0xBD5B7DDE02468ACF,
                          0x22446688AACCEF11, 0x446688AACCEF1132, 0x33557799BBDDFE00,
                          0xE1E3E5E7E9EBEDEF, 0xFEFCFAF8F6F4F2F1};

    constexpr hw::ScalableTag<uint64_t> d;
    constexpr int N = hw::Lanes(d);

    uint64_t result[8] = {0};

    for (int i = 0; i < 8; i += N) {
        const auto vin = hw::LoadU(d, &input[i]);
        const auto vout = ShiftLeftOneWithCarry(d, vin);
        hw::StoreU(vout, d, &result[i]);
    }

    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(result[i], output[i]);
    }
}

}  // namespace HWY_NAMESPACE
HWY_AFTER_NAMESPACE();

}  // namespace simdtag

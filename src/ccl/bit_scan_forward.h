// Copyright (c) 2021, the YACCLAB contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <version>

namespace simdtag {

#if defined _MSC_VER
#include <intrin.h>
inline unsigned char YacclabBitScanForward64(unsigned long* Index, uint64_t Mask) {
    return _BitScanForward64(Index, Mask);
}

#elif defined MY_PLATFORM_MACRO
// Define YacclabBitScanForward64 using the proper compiler intrinsic for your platform.
// Don't forget to open a pull request! :)

#elif defined __GNUC__ &&                                         \
        (__GNUC__ > 3 || (__GNUC__ == 3 && (__GNUC_MINOR__ > 4 || \
                                            (__GNUC_MINOR__ == 4 && __GNUC_PATCHLEVEL__ > 0))))
#include <cstdint>
inline unsigned char YacclabBitScanForward64(unsigned long* Index, uint64_t Mask) {
    if (!Mask) return 0;

    *Index = static_cast<unsigned long>(__builtin_ctzll(Mask));
    return 1;
}

#elif defined __clang__ && (__clang_major__ >= 5)
#include <cstdint>
inline unsigned char YacclabBitScanForward64(unsigned long* Index, uint64_t Mask) {
    if (!Mask) return 0;

    *Index = static_cast<unsigned long>(__builtin_ctzll(Mask));
    return 1;
}

#elif defined __cpp_lib_bitops
#include <bit>
#include <cstdint>
inline unsigned char YacclabBitScanForward64(unsigned long* Index, uint64_t Mask) {
    if (!Mask) return 0;

    *Index = static_cast<unsigned long>(std::countr_zero(Mask));
    return 1;
}

#else
#include <cstdint>
#include <stdexcept>
inline unsigned char YacclabBitScanForward64(unsigned long* Index, uint64_t Mask) {
    throw std::runtime_error(
            "YacclabBitScanForward64 undefined."
            " Define it for your platform in \"bit_scan_forward.h\","
            " or enable C++20 for the generic version.");
}
#endif

}  // namespace simdtag

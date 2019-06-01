#pragma once

#include <stdint.h>

//---------------------------------------------------------------------------------------------------------------------
// Utility functions for byte swapping.
//
// Needed because the 3DO data for doom is stored in big endian format. Both the dev machine (68K Mac) and the
// 3DO hardware itself were big endian, hence it made sense to store the data this way.
//---------------------------------------------------------------------------------------------------------------------
static inline uint16_t byteSwappedU16(const uint16_t num) {
    return (
        ((num & uint16_t(0x00FFU)) << 8) |
        ((num & uint16_t(0xFF00U)) >> 8)
    );
}

static inline uint32_t byteSwappedU32(const uint32_t num) {
    return (
        ((num & uint32_t(0x000000FFU)) << 24) |
        ((num & uint32_t(0x0000FF00U)) << 8) |
        ((num & uint32_t(0x00FF0000U)) >> 8) |
        ((num & uint32_t(0xFF000000U)) >> 24)
    );
}

#ifdef __cplusplus

static inline void byteSwapU16(uint16_t& num) noexcept {
    num = byteSwappedU16(num);
}

static inline void byteSwapU32(uint32_t& num) noexcept {
    num = byteSwappedU32(num);
}

#endif  // #ifdef __cplusplus

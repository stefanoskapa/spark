#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

/**
 * @brief A 64-bit bitboard
 */
#define BB uint64_t

/**
 * @brief 32-bit move representation
 * Use ENCODE_* and GET_MOVE_* macros to interact.
 */
#define MOVE uint32_t

#endif //TYPES_H

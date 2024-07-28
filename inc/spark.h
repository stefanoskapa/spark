#ifndef SPARK_H
#define SPARK_H
#include <stdint.h>

/**
 * @brief A 64-bit bitboard
 */
#define BB uint64_t


/** @brief Initializes attack tables for all pieces
 *
 * Should be called at engine startup and before
 * making use of any library functionality.
 *
 */
void init_attack_tables(void);



#endif

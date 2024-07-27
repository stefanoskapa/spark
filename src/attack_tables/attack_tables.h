#ifndef ATTACK_TABLES_H
#define ATTACK_TABLES_H

#include "../../inc/spark.h"

/**
 * @param square current square
 * @param side pawn color
 *
 * @returns Bitboard representing the pawn's attacks
 */
BB get_pawn_attacks(int square, int side);

/**
 * @param square current square
 *
 * @returns Bitboard representing the knights's attacks
 */
BB get_knight_attacks(int square);

/**
 * @param square current square
 * @param total_occupancy Bitboard of total board occupancy
 *
 * @returns Bitboard representing the bishop's attacks
 */
BB get_bishop_attacks(int square, BB total_occupancy);

/**
 * @param square current square
 * @param total_occupancy Bitboard of total board occupancy
 *
 * @returns Bitboard representing the rook's attacks
 */
BB get_rook_attacks(int square, BB total_occupancy);

/**
 * @param square current square
 * @param total_occupancy Bitboard of total board occupancy
 *
 * @returns Bitboard representing the queens's attacks
 */
BB get_queen_attacks(int square, BB total_occupancy);

/**
 * @param square current square
 *
 * @returns Bitboard representing the kings's attacks
 */
BB get_king_attacks(int square);

#endif

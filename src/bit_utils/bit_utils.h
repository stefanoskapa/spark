#ifndef BIT_UTILS_H
#define BIT_UTILS_H

#define is_set(bitboard, square) bitboard & (1ULL << square)
#define set_bit(bitboard, bit_nr) bitboard |= (1ULL << bit_nr)
#define clear_bit(bitboard, bit_nr) bitboard &= ~(1ULL << bit_nr)

void print_bitboard(U64 bitboard);
int popcnt(U64 x);
int first_set_bit(U64 x);

#endif

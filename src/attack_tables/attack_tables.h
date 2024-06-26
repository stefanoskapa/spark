#ifndef ATTACK_TABLES_H
#define ATTACK_TABLES_H

#include "../board/board.h"

#define NOT_H 0x7F7F7F7F7F7F7F7FULL
#define NOT_GH 0x3F3F3F3F3F3F3F3FULL
#define NOT_A 0xFEFEFEFEFEFEFEFEULL
#define NOT_AB 0xFCFCFCFCFCFCFCFCULL

extern U64 king_attacks[64];
extern U64 knight_attacks[64];
extern U64 pawn_attacks[2][64];
void init_attack_tables(void);

U64 get_bishop_attacks(int square, U64 total_occupancy);
U64 get_rook_attacks(int square, U64 total_occupancy);
extern U64 get_queen_attacks(int square, U64 total_occupancy);
U64 get_occupancy_variation(int index, U64 attack_mask);
U64 init_king_attacks(int square);
U64 init_knight_attacks(int square);
U64 init_pawn_attacks(int square, int side);
U64 get_rook_attack_mask_with_blockers(int square, U64 blocker);
U64 get_rook_attack_mask(int square);
U64 get_bishop_attack_mask_with_blockers(int square, U64 blocker);
U64 get_bishop_attack_mask(int square);

#endif

#include <stdio.h>
#include "../constants.h"
#include "bit_utils.h"

void print_bitboard(U64 bitboard) {
    printf("\n");
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            printf(" %c ", is_set(bitboard, square) ? 'X' : '-');
        }
        printf("\n");
    }
    printf("%llx\n", bitboard);

}

inline int popcnt(U64 x) {
	return __builtin_popcountll(x);
}

inline int first_set_bit(U64 x) {
  return __builtin_ctzll(x);
}

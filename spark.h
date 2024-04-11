#ifndef SPARK_H
#define SPARK_H

#define U64 unsigned long long

#define NOT_H1 ~(1ULL << h1)
#define F1 1ULL << f1
#define NOT_A1 ~(1ULL << a1)
#define D1 1ULL << d1
#define NOT_H8 ~(1ULL << h8)
#define F8 1ULL << f8
#define NOT_A8 ~(1ULL << a8)
#define D8 1ULL << d8
#define NOT_F1 ~(1ULL << f1)
#define H1 1ULL << h1
#define NOT_D1 ~(1ULL << d1)
#define A1 1ULL << a1
#define NOT_F8 ~(1ULL << f8)
#define H8 1ULL << h8
#define NOT_D8 ~(1ULL << d8)
#define A8 1ULL << a8
#define F1G1 ((1ULL << f1) | (1ULL << g1))
#define D1C1B1 ((1ULL << d1) | (1ULL << c1) | (1ULL << b1))
#define F8G8 ((1ULL << f8) | (1ULL << g8))
#define D8C8B8 ((1ULL << d8) | (1ULL << c8) | (1ULL << b8))

#define IS_SET(bitboard, square) bitboard & (1ULL << square)
#define SET_BIT(bitboard, bit_nr) bitboard |= (1ULL << bit_nr)
#define CLEAR_BIT(bitboard, bit_nr) bitboard &= ~(1ULL << bit_nr)
#define POPCNT(x) __builtin_popcountll(x)
#define FIRST_SET_BIT(x) __builtin_ctzll(x)
#define PRINT_BB(x)                                    \
    printf("\n")                                             \
    for (int rank = 0; rank < 8; rank++) {                   \
        for (int file = 0; file < 8; file++) {               \
            int square = rank * 8 + file;                    \
            printf(" %c ", IS_SET(x, square) ? 'X' : '-');   \
        }                                                    \
        printf("\n");                                        \
    }                                                        \
    printf("%llx\n", bitboard);

#define IS_KING_IN_CHECK(side) is_square_attacked(FIRST_SET_BIT(pos_pieces[side == WHITE ? K : k]), !side)

#define WHITE 0
#define BLACK 1
#define BOTH 2

typedef struct {
  int items[400];
  int index;
} int_stack;

typedef struct {
  int moves[256];
  int current_index;
  int capture_count;
} moves;

enum {
  a8,b8,c8,d8,e8,f8,g8,h8,
  a7,b7,c7,d7,e7,f7,g7,h7,
  a6,b6,c6,d6,e6,f6,g6,h6,
  a5,b5,c5,d5,e5,f5,g5,h5,
  a4,b4,c4,d4,e4,f4,g4,h4,
  a3,b3,c3,d3,e3,f3,g3,h3,
  a2,b2,c2,d2,e2,f2,g2,h2,
  a1,b1,c1,d1,e1,f1,g1,h1,
  none
};

/**
 * Castling
 *
 * 0001 == 1 -> white short pos_castling
 * 0010 == 1 -> white long pos_castling
 * 0100 == 1 -> black short pos_castling
 * 1000 == 1 -> black long pos_castling
 */
enum { wk = 1, wq = 2, bk = 4, bq = 8 };

// piece encoding
enum { P, N, B, R, Q, K, p, n, b, r, q, k };


extern const char ascii_pieces[12];
extern const int char_pieces[];
extern const int promoted_pieces[];
extern const char *square_to_coordinates[];
extern U64 pos_pieces[12];
extern U64 pos_occupancies[3];
extern int pos_occupancy[64];
extern int pos_side;
extern int pos_ep;
extern int pos_castling;
extern int first_pos_ep;
extern int_stack pos_castling_stack;
extern int_stack pos_captured;
extern int_stack pos_moves;
void make_move(int move);
void takeback(void);
void push(int_stack *is, int item);
int pop(int_stack *is);

#endif

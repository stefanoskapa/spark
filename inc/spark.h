#ifndef SPARK_H
#define SPARK_H
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief A 64-bit bitboard
 */
#define BB uint64_t

/**
 * @brief 32-bit move representation
 * Use ENCODE_* and GET_MOVE_* macros to interact.
 */
#define MOVE uint32_t

/**
 * @brief Struct for storing the generator results
 */
typedef struct MoveList {
 MOVE moves[256];
 int current_index;
 int capture_count;
} MoveList;

/** @brief Initializes attack tables for all pieces
 *
 * Should be called at engine startup and before
 * making use of any library functionality.
 *
 */
void init_attack_tables(void);

/** @brief Initializes board with FEN position
 *
 * @param fen_string String containg FEN notation
 *
 */
void parse_fen(char *fen_string);

/** @brief Generates all legal moves for the current position
 *
 * @returns MoveList containing all legal moves
 *
 */
MoveList generate_moves(void);

/**
 *
 * @returns A string with the UCI move notation
 *
 */
char *get_move_UCI(MOVE move);

/**
 * @brief Makes a move on the global board.
 *
 */
void make_move(MOVE move);

/**
 * @brief Takes back the last move on the global board.
 *
 */
void takeback(void);

#define ENCODE_MOVE(piece, source, target, prom_piece, capture, double_push, ep, castling) \
(piece) 					  |\
(source << 4) 		  |\
(target << 10) 		  |\
(prom_piece << 16)  |\
(capture << 20)     |\
(double_push << 21) |\
(ep << 22) 					|\
(castling << 23)

#define GET_MOVE_PIECE(move)         ((move & 0xF )    >> 0)
#define GET_MOVE_SOURCE(move)        ((move & 0x3F0)    >> 4)
#define GET_MOVE_TARGET(move)        ((move & 0xFC00)    >> 10)
#define GET_MOVE_PROMOTION(move)     ((move & 0xF0000)      >> 16)
#define GET_MOVE_CAPTURE(move)       ((move & 0x100000))
#define GET_MOVE_DOUBLE(move)        ((move & 0x200000))
#define GET_MOVE_EP(move)            ((move & 0x400000))
#define GET_MOVE_CASTLING(move)      ((move & 0x800000))
#define GET_MOVE_CHECK(move)         ((move & 0x1000000))

#define IS_SET(bitboard, square) bitboard & (1ULL << square)
#define SET_BIT(bitboard, bit_nr) bitboard |= (1ULL << bit_nr)
#define CLEAR_BIT(bitboard, bit_nr) bitboard &= ~(1ULL << bit_nr)
#define POPCNT(x) __builtin_popcountll(x)
#define FIRST_SET_BIT(x) __builtin_ctzll(x)

#define WHITE 0
#define BLACK 1
#define BOTH 2

//Helpers
extern const char ascii_pieces[12];
extern const int char_pieces[];
extern const int promoted_pieces[];
extern const char *square_to_coordinates[];


// Global State
extern BB pos_pieces[12];
extern BB pos_occupancies[3];
extern int pos_occupancy[64];
extern int pos_side;
extern int pos_ep;
extern int pos_castling;
extern int pos_cap_piece;

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


enum { P, N, B, R, Q, K, p, n, b, r, q, k };


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

/**
 * @param square The square to check
 * @param side The attacking side
 * @returns True if the square is attacked, otherwise false
 */
bool is_square_attacked(int square, int side);

/**
 * @param fen_string A chess position in FEN notation 
 * Sets up a board position on the global board
 */
void parse_fen(char *fen_string);


#define IS_KING_IN_CHECK(side) is_square_attacked(FIRST_SET_BIT(pos_pieces[side == WHITE ? K : k]), !side)

#endif

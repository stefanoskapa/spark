#ifndef SPARK_H
#define SPARK_H
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

#define ENCODE_EP(piece, source, target) \
(piece) 					  |\
(source << 4) 		  |\
(target << 10) 		  |\
(1ULL << 20)        |\
(1ULL << 22)

#define ENCODE_PROM(piece, source, target, prom_piece) \
(piece) 					  |\
(source << 4) 		  |\
(target << 10) 		  |\
(prom_piece << 16)

#define ENCODE_DOUBLE(piece, source, target) \
(piece) 					  |\
(source << 4) 		  |\
(target << 10) 		  |\
(1ULL << 21)

#define ENCODE_SIMPLE_MOVE(piece, source, target) \
(piece) 					  |\
(source << 4) 		  |\
(target << 10)

#define ENCODE_SIMPLE_CAPTURE(piece, source, target) \
(piece) 					  |\
(source << 4) 		  |\
(target << 10) 		  |\
(1ULL << 20)

#define ENCODE_CASTLING(piece, source, target) \
(piece) 					  |\
(source << 4) 		  |\
(target << 10) 		  |\
(1ULL << 23)

#define ENCODE_CAP_PROM(piece, source, target, prom_piece) \
(piece) 					  |\
(source << 4) 		  |\
(target << 10) 		  |\
(prom_piece << 16)  |\
(1ULL << 20)

#define GET_MOVE_PIECE(move)         ((move & 0xF )    >> 0)
#define GET_MOVE_SOURCE(move)        ((move & 0x3F0)    >> 4)
#define GET_MOVE_TARGET(move)        ((move & 0xFC00)    >> 10)
#define GET_MOVE_PROMOTION(move)     ((move & 0xF0000)      >> 16)
#define GET_MOVE_CAPTURE(move)       ((move & 0x100000))
#define GET_MOVE_DOUBLE(move)        ((move & 0x200000))
#define GET_MOVE_EP(move)            ((move & 0x400000))
#define GET_MOVE_CASTLING(move)      ((move & 0x800000))
#define GET_MOVE_CHECK(move)         ((move & 0x1000000))

#define SET_MOVE_CHECK(move)         ((move | 0x1000000))


extern const char ascii_pieces[12];
extern const int char_pieces[];
extern const int promoted_pieces[];
extern const char *square_to_coordinates[];
extern BB pos_pieces[12];
extern BB pos_occupancies[3];
extern int pos_occupancy[64];
extern int pos_side;
extern int pos_ep;
extern int pos_castling;
extern int pos_cap_piece;

#endif

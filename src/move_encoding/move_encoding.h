#ifndef SPARK_MOVE_ENCODING_H
#define SPARK_MOVE_ENCODING_H

#include "../Types.h"

/*
	MSB                                 LSB
	0000 0000 0000 0000 0000 0000 0000 1111   piece
	0000 0000 0000 0000 0000 0011 1111 0000   source
	0000 0000 0000 0000 1111 1100 0000 0000   target
	0000 0000 0000 1111 0000 0000 0000 0000   promoted piece
	0000 0000 0001 0000 0000 0000 0000 0000   capture flag
	0000 0000 0010 0000 0000 0000 0000 0000   double pawn push flag
	0000 0000 0100 0000 0000 0000 0000 0000   en passant flag
	0000 0000 1000 0000 0000 0000 0000 0000   castling flag
	0000 0001 0000 0000 0000 0000 0000 0000   is check
	1111 1110 0000 0000 0000 0000 0000 0000   reserved
*/


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

char *get_move_UCI(MOVE move);

void print_move_list(MoveList *move_list);

void print_move(MOVE move);

void print_move_UCI(MOVE move);

#endif

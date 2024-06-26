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


#ifndef SPARK_MOVE_ENCODING_H
#define SPARK_MOVE_ENCODING_H

#include "../board/board.h"
#define ENC_PIECE 		0xF
#define ENC_SOURCE		0x3F0
#define ENC_TARGET    0xFC00
#define ENC_PROM      0xF0000
#define ENC_CAPTURE   0x100000
#define ENC_DOUBLE    0x200000
#define ENC_EP        0x400000
#define ENC_CAST      0x800000
#define ENC_CHECK     0x1000000

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


#define GET_MOVE_PIECE(move)         ((move & ENC_PIECE )    >> 0)
#define GET_MOVE_SOURCE(move)        ((move & ENC_SOURCE)    >> 4)
#define GET_MOVE_TARGET(move)        ((move & ENC_TARGET)    >> 10)
#define GET_MOVE_PROMOTION(move)     ((move & ENC_PROM)      >> 16)
#define GET_MOVE_CAPTURE(move)       ((move & ENC_CAPTURE))
#define GET_MOVE_DOUBLE(move)        ((move & ENC_DOUBLE))
#define GET_MOVE_EP(move)            ((move & ENC_EP))
#define GET_MOVE_CASTLING(move)      ((move & ENC_CAST))
#define GET_MOVE_CHECK(move)         ((move & ENC_CHECK))

#define SET_MOVE_CHECK(move)         ((move | ENC_CHECK))

char* get_move_UCI(int move);
void print_move_list(moves* move_list);
void print_move(int move);
void print_move_UCI(int move);

#endif

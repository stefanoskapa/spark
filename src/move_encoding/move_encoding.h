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

#define enc_piece 		0xF
#define enc_source		0x3F0
#define enc_target    0xFC00
#define enc_prom      0xF0000
#define enc_capture   0x100000
#define enc_double    0x200000
#define enc_ep        0x400000
#define enc_cast      0x800000
#define enc_check     0x1000000

#define encode_move(piece, source, target, prom_piece, capture, double_push, ep, castling) \
  (piece) 					  |\
	(source << 4) 		  |\
	(target << 10) 		  |\
	(prom_piece << 16)  |\
	(capture << 20)     |\
  (double_push << 21) |\
  (ep << 22) 					|\
  (castling << 23)


#define get_move_piece(move)         ((move & enc_piece )    >> 0)
#define get_move_source(move)        ((move & enc_source)    >> 4)
#define get_move_target(move)        ((move & enc_target)    >> 10)
#define get_move_promotion(move)     ((move & enc_prom)      >> 16)
#define get_move_capture(move)       ((move & enc_capture))
#define get_move_double(move)        ((move & enc_double))
#define get_move_ep(move)            ((move & enc_ep))
#define get_move_castling(move)      ((move & enc_cast))
#define get_move_check(move)         ((move & enc_check))

#define set_move_check(move)         ((move | enc_check))
char* get_move_UCI(int move);
void print_move_list(moves* move_list);
void print_move(int move);
void print_move_UCI(int move);

#endif //SPARK_MOVE_ENCODING_H

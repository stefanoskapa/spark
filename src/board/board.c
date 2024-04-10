#include "board.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

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

// board state
U64 pos_pieces[12];
U64 pos_occupancies[3]; // 0 = White, 1 = Black, 2 = Both
int pos_occupancy[64];
int pos_side = 1;
int pos_ep = none;
int pos_castling;

int_stack pos_moves = {{0}, 0};
int_stack pos_captured = {{0}, 0};
int_stack pos_castling_stack = {{0}, 0};

char ascii_pieces[] = "PNBRQKpnbrqk";

int char_pieces[] = {
  ['P'] = P, ['N'] = N, ['B'] = B, ['R'] = R, ['Q'] = Q, ['K'] = K,
  ['p'] = p, ['n'] = n, ['b'] = b, ['r'] = r, ['q'] = q, ['k'] = k};

int promoted_pieces[] = {
  [Q] = 'q', [R] = 'r', [B] = 'b', [N] = 'n',
  [q] = 'q', [r] = 'r', [b] = 'b', [n] = 'n',
};

char *square_to_coordinates[] = {
  "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "a7", "b7", "c7",
  "d7", "e7", "f7", "g7", "h7", "a6", "b6", "c6", "d6", "e6", "f6",
  "g6", "h6", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a4",
  "b4", "c4", "d4", "e4", "f4", "g4", "h4", "a3", "b3", "c3", "d3",
  "e3", "f3", "g3", "h3", "a2", "b2", "c2", "d2", "e2", "f2", "g2",
  "h2", "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"};
int first_pos_ep = none;

int isKingInCheck(int side) {
  U64 king_bb = pos_pieces[side == white ? K : k]; // king to check
  int king_sq = first_set_bit(king_bb);
  return is_square_attacked(king_sq, !side);
}

void make_move(int move) {

  push(&pos_castling_stack, pos_castling); // save castling state

  int const piece = get_move_piece(move);
  int const source = get_move_source(move);
  int const target = get_move_target(move);
  U64 const sourceBB = 1ULL << source;
  U64 const targetBB = 1ULL << target;

  
  /*
    Branch outline

    
    if capture 
      if ep 
        1. push pawn to capture stack (might not be necessary)
        2. remove captured pawn from state
      else
        1. push captured piece to capture stack
        2. remove captured piece from state
     
    1. remove piece from state
    
    if promotion
      add promoted piece to state
    else
      add piece to target

    2. Add target to occupancy

    if R
      disable castling if rook's source is a1 or h1
    
    if K
      if castling
        1. castle
      disable all castling rights  

    if move was double pawn push update pos_ep
  
   push new move
   change turns

  */
  if (pos_side == white) {
    if (get_move_capture(move)) {
      if (get_move_ep(move)) {
        push(&pos_captured, p);
        pos_occupancy[pos_ep + 8] = INT_MAX;
        U64 pawn_kill = ~(1ULL << (pos_ep + 8));
        pos_pieces[p] &= pawn_kill;
        pos_occupancies[1] &= pawn_kill;
        pos_occupancies[2] &= pawn_kill;
      } else {                              // not ep
        int dead_piece = pos_occupancy[target];
        if (dead_piece == r) { // rook captured, adjust black's castling rights
          if (target == a8)
            pos_castling &= 7;
          else if (target == h8)
            pos_castling &= 11;
        }

        pos_pieces[dead_piece] &= (~targetBB); // remove captured piece
        push(&pos_captured, dead_piece);       // push captured piece to stack
        pos_occupancy[target] = INT_MAX;	    // remove captured piece from occupancy array			
        pos_occupancies[1] &= ~targetBB;     // remove captured piece from black's occupancy
      }
    }

    // Move piece to target
    pos_pieces[piece] &= (~sourceBB); // remove piece from source
    pos_occupancy[source] = INT_MAX; // remove piece from occupancy array
    pos_occupancies[2] &= (~sourceBB);        // remove source from total occupancy
    pos_occupancies[0] &= (~sourceBB); // remove source to white's  occupancy
    
    if (get_move_promotion(move)){     // promotion
      pos_pieces[get_move_promotion(move)] |= targetBB; // add promoted piece to bitboard
      pos_occupancy[target] = get_move_promotion(move); // add promoted piece to occupancy array
    } else { //not promotion
      pos_pieces[piece] |= targetBB; // add piece to bitboard
      pos_occupancy[target] = piece; // add piece to occupancy array
    }
    pos_occupancies[2] |= targetBB;           // add target to total occupancy
    pos_occupancies[0] |= targetBB;    // add target to white's  occupancy

    pos_ep = none;

    // special cases
    switch (piece) {
      case R:
        if (source == a1)
          pos_castling &= 13; // 1101 = disable white long
        else if (source == h1)
          pos_castling &= 14; // 1110 = disable white short
        break;

      case K:
        if (get_move_castling(move)) {
          if (target == g1) {
            pos_pieces[R] &= NOT_H1; // remove rook from h1
            pos_occupancy[h1] = INT_MAX;
            pos_occupancies[0] &= NOT_H1;
            pos_occupancies[2] &= NOT_H1;
            pos_pieces[R] |= F1; // place rook on f1
            pos_occupancy[f1] = R;
            pos_occupancies[0] |= F1;
            pos_occupancies[2] |= F1;
          } else if (target == c1) {  
            pos_pieces[R] &= NOT_A1; // remove rook from a1
            pos_occupancy[a1] = INT_MAX;
            pos_occupancies[0] &= NOT_A1;
            pos_occupancies[2] &= NOT_A1;
            pos_pieces[R] |= D1; // place rook on d1
            pos_occupancy[d1] = R; 
            pos_occupancies[0] |= D1;
            pos_occupancies[2] |= D1;
          }
        }
        pos_castling &= 12; // disable all castling for white
        break;
      case P:
        if (get_move_double(move)) {
          pos_ep = target + 8;
        }
    }

  } else { //black
    if (get_move_capture(move)) {
      if (get_move_ep(move)) {
        push(&pos_captured, P);
        pos_occupancy[pos_ep - 8] = INT_MAX;
        U64 pawn_kill = ~(1ULL << (pos_ep - 8));
        pos_pieces[P] &= pawn_kill;
        pos_occupancies[0] &= pawn_kill;
        pos_occupancies[2] &= pawn_kill;
      } else {                              // not ep
        int dead_piece = pos_occupancy[target];
        if (dead_piece == R) { // rook captured, adjust white's castling rights
          if (target == a1)
            pos_castling &= 13;
          else if (target == h1)
            pos_castling &= 14;
        }

        pos_pieces[dead_piece] &= (~targetBB); // remove captured piece
        push(&pos_captured, dead_piece);       // push captured piece to stack
        pos_occupancy[target] = INT_MAX;	    // remove captured piece from occupancy array			
        pos_occupancies[0] &= ~targetBB;     // remove captured piece from white's occupancy
      }
    }

    // Move piece to target
    pos_pieces[piece] &= (~sourceBB); // remove piece from source
    pos_occupancy[source] = INT_MAX; // remove piece from occupancy array
    pos_occupancies[2] &= (~sourceBB);        // remove source from total occupancy
    pos_occupancies[1] &= (~sourceBB); // remove source from black's  occupancy
    
    if (get_move_promotion(move)){     // promotion
      pos_pieces[get_move_promotion(move)] |= targetBB; // add promoted piece to bitboard
      pos_occupancy[target] = get_move_promotion(move); // add promoted piece to occupancy array
    } else { //not promotion
      pos_pieces[piece] |= targetBB; // add piece to bitboard
      pos_occupancy[target] = piece; // add piece to occupancy array
    }
    pos_occupancies[2] |= targetBB;           // add target to total occupancy
    pos_occupancies[1] |= targetBB;    // add target to black's  occupancy

    pos_ep = none;
    // special cases
    switch (piece) {
      case r:
        if (source == a8)
          pos_castling &= 7; // 0111 = disable black long
        else if (source == h8)
          pos_castling &= 11; // 1011 = disable black short
        break;

      case k:
        if (get_move_castling(move)) {
          if (target == g8) {
            pos_pieces[r] &= NOT_H8; // remove rook from h8
            pos_occupancy[h8] = INT_MAX;
            pos_occupancies[1] &= NOT_H8;
            pos_occupancies[2] &= NOT_H8;
            pos_pieces[r] |= F8; // place rook on f8
            pos_occupancy[f8] = r;
            pos_occupancies[1] |= F8;
            pos_occupancies[2] |= F8;
          } else if (target == c8) {
            pos_pieces[r] &= NOT_A8; // remove rook from a8
            pos_occupancy[a8] = INT_MAX;
            pos_occupancies[1] &= NOT_A8;
            pos_occupancies[2] &= NOT_A8;
            pos_pieces[r] |= D8; // place rook on d8
            pos_occupancy[d8] = r;
            pos_occupancies[1] |= D8;
            pos_occupancies[2] |= D8;
          }
        } 
        pos_castling &= 3; // disable all castling for black
        break;
      case p:
        if (get_move_double(move)) {
          pos_ep = target - 8;
        }
    }

  }


  push(&pos_moves, move); // push new move
  pos_side = !(pos_side); // change turns
}

void takeback(void) {

  int lmove = pop(&pos_moves);

  int const source = get_move_source(lmove);
  int const target = get_move_target(lmove);
  int const piece = get_move_piece(lmove);
  int const pr_piece = get_move_promotion(lmove);
  U64 const sourceBB = 1ULL << source;
  U64 const targetBB = 1ULL << target;

  if (pr_piece) {
    pos_pieces[pos_side ? P : p] |= sourceBB; // put pawn back to source
    pos_occupancy[source] = pos_side ? P : p;
    pos_pieces[pr_piece] &= (~targetBB);      // remove promoted piece
    pos_occupancy[target] = INT_MAX;
  } else {
    pos_pieces[piece] |= sourceBB;    // put piece back to source
    pos_occupancy[source] = piece;
    pos_pieces[piece] &= (~targetBB); // remove piece from target
    pos_occupancy[target] = INT_MAX;
  }
  pos_occupancies[2] |= sourceBB;         // add piece to total occupancy
  pos_occupancies[!pos_side] |= sourceBB; // add piece to its color's occupancy
  pos_occupancies[2] &= (~targetBB);      // remove piece from total occupancy
  pos_occupancies[!pos_side] &=
    (~targetBB); // remove piece from its color's occupancy

  if (get_move_capture(lmove)) { // restore captured piece
    int cap_piece = pop(&pos_captured);

    if (!get_move_ep(lmove)) {
      pos_pieces[cap_piece] |= targetBB;
      pos_occupancy[target] = cap_piece;
      pos_occupancies[2] |= targetBB;
      pos_occupancies[pos_side] |= targetBB;
    } else {
      int ep_target = target + (cap_piece == P ? -8 : +8);
      pos_pieces[cap_piece] |= 1ULL << ep_target;
      pos_occupancy[ep_target] = cap_piece;
      pos_occupancies[2] |= 1ULL << ep_target;
      pos_occupancies[pos_side] |= 1ULL << ep_target;
    }
  }

  // check if last move now was a double pawn push, if yes, set pos_ep
  if (pos_moves.index > 0) {
    int in = pos_moves.index - 1;
    int lastmove = pos_moves.items[in];
    if (get_move_double(lastmove)) {
      pos_ep =
        get_move_target(lastmove) + (get_move_piece(lastmove) == P ? +8 : -8);
    } else {
      pos_ep = none;
    }
  } else {
    pos_ep = first_pos_ep;
  }

  // castling
  if (get_move_castling(lmove)) {

    switch (target) {
      case g1: // white short
        pos_pieces[R] &= NOT_F1;
        pos_occupancy[f1] = INT_MAX;
        pos_pieces[R] |= H1;
        pos_occupancy[h1] = R;
        pos_occupancies[0] &= NOT_F1;
        pos_occupancies[2] &= NOT_F1;
        pos_occupancies[0] |= H1;
        pos_occupancies[2] |= H1;
        break;
      case c1: // white long
        pos_pieces[R] &= NOT_D1;
        pos_occupancy[d1] = INT_MAX;
        pos_pieces[R] |= A1;
        pos_occupancy[a1] = R;
        pos_occupancies[0] &= NOT_D1;
        pos_occupancies[2] &= NOT_D1;
        pos_occupancies[0] |= A1;
        pos_occupancies[2] |= A1;
        break;
      case g8: // black short
        pos_pieces[r] &= NOT_F8;
        pos_occupancy[f8] = INT_MAX;
        pos_pieces[r] |= H8;
        pos_occupancy[h8] = r;
        pos_occupancies[1] &= NOT_F8;
        pos_occupancies[2] &= NOT_F8;
        pos_occupancies[1] |= H8;
        pos_occupancies[2] |= H8;
        break;
      case c8: // white short
        pos_pieces[r] &= NOT_D8;
        pos_occupancy[d8] = INT_MAX;
        pos_pieces[r] |= A8;
        pos_occupancy[a8] = r;
        pos_occupancies[1] &= NOT_D8;
        pos_occupancies[2] &= NOT_D8;
        pos_occupancies[1] |= A8;
        pos_occupancies[2] |= A8;
        break;
    }
  }

  pos_castling = pop(&pos_castling_stack); // restore last castling state
  pos_side = !pos_side;                    // change turn
}

// int_stack functions

inline void push(int_stack *is, int item) { is->items[is->index++] = item; }

inline int pop(int_stack *is) {
  is->index--;
  return is->items[is->index];
}

void show_stack(int_stack *is) {

  printf("Stack contents\n");
  for (int i = 0; i < is->index; i++) {
    printf("%d\n", is->items[i]);
  }
}

const U64 NOT_H = 0x7F7F7F7F7F7F7F7FULL;
const U64 NOT_GH = 0x3F3F3F3F3F3F3F3FULL;
const U64 NOT_A = 0xFEFEFEFEFEFEFEFEULL;
const U64 NOT_AB = 0xFCFCFCFCFCFCFCFCULL;

U64 pawn_attacks[2][64];
U64 knight_attacks[64];
U64 king_attacks[64];
U64 bishop_masks[64];
U64 bishop_attacks[64][512];
U64 rook_masks[64];
U64 rook_attacks[64][4096];
U64 queen_attacks[64];

U64 bishop_magic_numbers[] = {
	0x24a000b0c05860ULL , 0x316004040041C200ULL, 0x0110110A04A20404ULL, 0x1208208020204000ULL, 
	0x0002021008008009ULL, 0x0001100210006000ULL, 0x0086080402082418ULL, 0x8001050101202200ULL,
  0x000C420401061200ULL, 0x0205020828188490ULL, 0x1000420409002801ULL, 0x9000880600400000ULL, 
	0x4404020211608188ULL, 0x080061822021C000ULL, 0x0410020284200882ULL, 0x8001014202108200ULL,
  0x00C003C450820248ULL, 0x200804A109040080ULL, 0x0024050808005010ULL, 0x0000880802024002ULL, 
	0x0004105202022000ULL, 0x0100480202022000ULL, 0x0281080288095063ULL, 0x0000808602828804ULL,
  0x0004101084600810ULL, 0x1410862028080100ULL, 0x4802060211080600ULL, 0x1810040020C40008ULL, 
	0x61048C0180802004ULL, 0x820800A052020904ULL, 0x1028450002010102ULL, 0x0009020000228420ULL,
  0x210C302408182100ULL, 0x1034100400080108ULL, 0x0005140200500080ULL, 0x0000420080180080ULL, 
	0x0804090400420028ULL, 0x0010060020020089ULL, 0x0010108300048400ULL, 0x0004008602008041ULL,
  0x0242842120021800ULL, 0x00C088042A0030A0ULL, 0x4080104030040802ULL, 0x1018224200808810ULL, 
	0x400118010040040AULL, 0x0040610053014080ULL, 0x0004084A44040441ULL, 0x0804444040440202ULL,
  0x18840084100B0012ULL, 0x0102090401248000ULL, 0x1240110888240002ULL, 0x000014404202212AULL, 
	0x81400010020A1050ULL, 0x0080401002058820ULL, 0x0888101001850000ULL, 0x001090808100440DULL,
  0x4029002610040440ULL, 0x0000110411090800ULL, 0x3011090061814180ULL, 0x0240401004420210ULL, 
	0x40005590100A0200ULL, 0x460044400C084080ULL, 0xC209040818080688ULL, 0x0502102602014200ULL
};

U64 rook_magic_numbers[] = {
	0x2480004000201180L, 0x8240001002402000L, 0x0200084202201080L, 0x4480080080300004L, 
	0x0A00140A00102018L, 0x5080040080010200L, 0x1200084100860004L, 0x020004004202812BL,
  0x0328801382204000L, 0x0000806000400080L, 0x140100104C200100L, 0x0408801000808804L, 
	0x0985000800841101L, 0x0112000802011004L, 0x8514000410420D08L, 0x0040800100005080L,
  0x0040A08000804004L, 0x0040008020014092L, 0x029101001140A002L, 0x0110808028021000L, 
	0x0004808004011800L, 0x1288808004000200L, 0x0840040008100142L, 0x4002020002A40941L,
  0x00A040028006208CL, 0x0000200480400281L, 0x0000100180200080L, 0x0010010100110820L, 
	0x0020080080800400L, 0x0001000900020400L, 0x0803020080800100L, 0x1001008200006104L,
  0x0004400020801080L, 0x0028810602004020L, 0x2480801001802008L, 0x0023009001002008L, 
	0xA003000801000412L, 0x0200142008011040L, 0x0800210204001028L, 0xC0800918C200008CL,
  0x1820810642020020L, 0x4002406010024000L, 0x4051002001490010L, 0x8001000810050020L, 
	0x0488008004018008L, 0x1404400420080110L, 0x0002000804520001L, 0x090001440082000BL,
  0x240200C184210600L, 0x0118400891200080L, 0x0020001002806080L, 0x200100AA20100100L, 
	0x1120830400380080L, 0x2242040080020080L, 0x0208104842010400L, 0x0280494C01108200L,
  0x40020080C1003022L, 0x184284400130E101L, 0x100080204012000AL, 0x4003000410026009L, 
	0x0802011448201002L, 0x20C2000810010412L, 0x8440019041220804L, 0x5200002104408402L
};

const int bishop_relevant_bit_count[64] = {
	6, 5, 5, 5, 5, 5, 5, 6,
	5, 5, 5, 5, 5, 5, 5, 5,
 	5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6
};

const int rook_relevant_bit_count[64] = {
	12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};


U64 init_pawn_attacks(int square, int side) {
	U64 bitboard = 0ULL;
  U64 attacks = 0ULL;
  set_bit(bitboard, square);

  if (!side) { //white
    attacks |= bitboard >> 7 & NOT_A;
    attacks |= bitboard >> 9 & NOT_H;
  } else {
  	attacks |= bitboard << 7 & NOT_H;
  	attacks |= bitboard << 9 & NOT_A;
  }
	return attacks;
}

U64 init_knight_attacks(int square) {
  U64 bitboard = 0ULL;
  U64 attacks = 0ULL;
  set_bit(bitboard, square);

  attacks |= bitboard >> 6 & NOT_AB;
  attacks |= bitboard << 6 & NOT_GH;
  attacks |= bitboard >> 10 & NOT_GH;
  attacks |= bitboard << 10 & NOT_AB;
  attacks |= bitboard >> 15 & NOT_A;
  attacks |= bitboard << 15 & NOT_H;
  attacks |= bitboard >> 17 & NOT_H;
  attacks |= bitboard << 17 & NOT_A;

  return attacks;
}


U64 init_king_attacks(int square) {
  U64 bitboard = 0ULL;
  U64 attacks = 0ULL;
  set_bit(bitboard, square);

  attacks |= bitboard >> 1 & NOT_H;
  attacks |= bitboard << 1 & NOT_A;
  attacks |= bitboard >> 7 & NOT_A;
  attacks |= bitboard << 7 & NOT_H;
  attacks |= bitboard >> 8;
  attacks |= bitboard << 8;
  attacks |= bitboard >> 9 & NOT_H;
  attacks |= bitboard << 9 & NOT_A;

  return attacks;
}

U64 get_bishop_attacks(int square, U64 total_occupancy) {
  total_occupancy &= bishop_masks[square];
  total_occupancy *= bishop_magic_numbers[square];
  total_occupancy >>= 64 - bishop_relevant_bit_count[square];
	return bishop_attacks[square][total_occupancy];
}

U64 get_rook_attacks(int square, U64 total_occupancy) {
  total_occupancy &= rook_masks[square];
  total_occupancy *= rook_magic_numbers[square];
  total_occupancy >>= 64 - rook_relevant_bit_count[square];
  return rook_attacks[square][total_occupancy];
}

U64 get_queen_attacks(int square, U64 total_occupancy) {
  return get_bishop_attacks(square, total_occupancy) | get_rook_attacks(square, total_occupancy);
}


void init_attack_tables(void) {
  int occupancy_indexes;
  int relevant_bits_count;
  int magic_index;
  U64 attack_mask;
  U64 occupancy;

  for (int square = 0; square < 64; square++) {
   
	 //init pawns
  	pawn_attacks[white][square] = init_pawn_attacks(square, white);
    pawn_attacks[black][square] = init_pawn_attacks(square, black);
		//init knights
    knight_attacks[square] = init_knight_attacks(square);
		// init kings
    king_attacks[square] = init_king_attacks(square);

		//init bishops
    bishop_masks[square] = get_bishop_attack_mask(square);
    attack_mask = bishop_masks[square];
    relevant_bits_count = popcnt(attack_mask);
    occupancy_indexes = (1 << relevant_bits_count);
    for (int index = 0; index < occupancy_indexes; index++) {
    	occupancy = get_occupancy_variation(index,attack_mask);
      magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bit_count[square]);
 			bishop_attacks[square][magic_index] = get_bishop_attack_mask_with_blockers(square,occupancy);
    }

    //init rooks
    rook_masks[square] = get_rook_attack_mask(square);
    attack_mask = rook_masks[square];
    relevant_bits_count = popcnt(attack_mask);
    occupancy_indexes = (1 << relevant_bits_count);
    for (int index = 0; index < occupancy_indexes; index++) {
    	occupancy = get_occupancy_variation(index,attack_mask);
      magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bit_count[square]);
      rook_attacks[square][magic_index] = get_rook_attack_mask_with_blockers(square,occupancy);
    }
 

  }
  
}

U64 get_bishop_attack_mask(int square) {
    U64 attacks = 0ULL;
    int rank, file;
    int start_rank = square / 8;
    int start_file = square % 8;

    for (rank = start_rank + 1, file = start_file + 1; rank <= 6 && file <=6; rank++, file++) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank + 1, file = start_file - 1; rank <= 6 && file >= 1; rank++, file--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank - 1, file = start_file + 1; rank >= 1 && file <= 6; rank--, file++) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank - 1, file = start_file - 1; rank >= 1 && file >= 1; rank--, file--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    return attacks;

}

U64 get_bishop_attack_mask_with_blockers(int square, U64 blocker) {
    U64 attacks = 0ULL;
    int rank, file;
    int start_rank = square / 8;
    int start_file = square % 8;

    for (rank = start_rank + 1, file = start_file + 1; rank <= 7 && file <=7; rank++, file++) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank + 1, file = start_file - 1; rank <= 7 && file >= 0; rank++, file--) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank - 1, file = start_file + 1; rank >= 0 && file <= 7; rank--, file++) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank - 1, file = start_file - 1; rank >= 0 && file >= 0; rank--, file--) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    return attacks;

}

U64 get_rook_attack_mask(int square) {
    U64 attacks = 0ULL;
    int rank, file;
    int start_rank = square / 8;
    int start_file = square % 8;

    for (rank = start_rank + 1, file = start_file; rank <= 6; rank++) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank - 1, file = start_file; rank >= 1; rank--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank, file = start_file + 1; file <= 6;file++) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank, file = start_file - 1; file >= 1; file--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    return attacks;

}

U64 get_rook_attack_mask_with_blockers(int square, U64 blocker) {
    U64 attacks = 0ULL;
    int rank, file;
    int start_rank = square / 8;
    int start_file = square % 8;

    for (rank = start_rank + 1, file = start_file; rank <= 7; rank++) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank - 1, file = start_file; rank >= 0; rank--) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank, file = start_file + 1; file <= 7;file++) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank, file = start_file - 1; file >= 0; file--) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    return attacks;

}

U64 get_occupancy_variation(int index, U64 attack_mask) {

    int bit_count = popcnt(attack_mask);
    U64 occupancy = 0ULL;

    for (int count = 0; count < bit_count; count++) {
        U64 square = first_set_bit(attack_mask);
        clear_bit(attack_mask,square);

        if (index & (1 << count)) {  //decide whether to include the count-th bit of the attack mask
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}
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


int is_square_attacked(int square, int side) { // attacking side

  if (!side) {
    return      
      (get_bishop_attacks(square, pos_occupancies[2]) & (pos_pieces[B] | pos_pieces[Q])) ||
      (get_rook_attacks(square, pos_occupancies[2]) & (pos_pieces[R] | pos_pieces[Q])) ||
      (knight_attacks[square] & pos_pieces[N]) ||
      (pawn_attacks[!side][square] & pos_pieces[P]) ||
      (king_attacks[square] & pos_pieces[K]);
  } else {
    return       
      (get_bishop_attacks(square, pos_occupancies[2]) & (pos_pieces[b] | pos_pieces[q])) ||
      (get_rook_attacks(square, pos_occupancies[2]) & (pos_pieces[r] | pos_pieces[q])) ||
      (knight_attacks[square] & pos_pieces[n]) ||
      (pawn_attacks[!side][square] & pos_pieces[p]) ||
      (king_attacks[square] & pos_pieces[k]);
  }
}

void show_board(void) {
  for (int square = 0; square < 64; square++) {
    if (square % 8 == 0)
      printf("\n");
    char char_to_show = '.';
    for (int piece = 0; piece < 12; piece++) {
      if (is_set(pos_pieces[piece], square)) {
        char_to_show = ascii_pieces[piece];
        break;
      }
    }
    printf(" %c ", char_to_show);
  }

  printf("\n\nside:        ");
  if (pos_side == white)
    printf("white");
  else if (pos_side == black)
    printf("black");
  printf("\n");

  printf("castling:    ");
  if (pos_castling & wk) printf("K");
  if (pos_castling & wq) printf("Q");
  if (pos_castling & bk) printf("k");
  if (pos_castling & bq) printf("q");
  printf("\n");

  unsigned char file = pos_ep == none ? '-' : pos_ep % 8 + 'a';
  unsigned char rank = pos_ep == none ? ' ' : '8' - pos_ep / 8;
  printf("en passant:  %c%c\n", file, rank);


}


void show_occ_board(void) {
  for (int square = 0; square < 64; square++) {
    if (square % 8 == 0)
      printf("\n");
    char char_to_show = '.';
    if (pos_occupancy[square] != INT_MAX) {
      char_to_show = ascii_pieces[pos_occupancy[square]];
    }

    printf(" %c ", char_to_show);
  }

}



void fen_error(void) {
  printf("Invalid FEN string\n");
  exit(1);
}

void clean_board(void) {
  for (size_t i = 0; i < sizeof(pos_pieces)/sizeof(pos_pieces[0]); i++)
    pos_pieces[i]=0ULL;

  for (int i = 0; i < 64; i++)
    pos_occupancy[i] = INT_MAX;

  pos_occupancies[0] = 0ULL;
  pos_occupancies[1] = 0ULL;
  pos_occupancies[2] = 0ULL;
  first_pos_ep = none;
  pos_moves.index = 0;
  pos_captured.index = 0;
  pos_castling_stack.index = 0;

}
void parse_fen(char *fen_string) {
  if (strlen(fen_string) < 24 || strlen(fen_string) > 80)
    fen_error();

  int fen_index;
  int square = 0;
  unsigned char ch;

  clean_board();
  // Piece Placement Data
  for (fen_index = 0; fen_index <= 72; fen_index++) {
    ch = fen_string[fen_index];
    if (ch == ' ')
      break;
    if (ch >= '1' && ch <= '8') {
      square += ch - '0';
    } else if (ch != '/') {
      pos_pieces[char_pieces[ch]] |= 1ULL << square;
      pos_occupancy[square] = char_pieces[ch];
      pos_occupancies[both] |= 1ULL << square;
      pos_occupancies[ch > 'Z' ? black : white] |= 1ULL << square;
      square++;
    }
  }

  // Active Color
  fen_index++;
  ch = fen_string[fen_index];
  if (ch == 'w')
    pos_side = white;
  else if (ch == 'b')
    pos_side = black;
  else
    fen_error();

  //pos_castling
  fen_index += 2;

  while (1) {
    ch = fen_string[fen_index];
    if (ch == ' ')
      break;

    switch (ch) {
      case 'K':
        pos_castling |= wk;
        break;
      case 'Q':
        pos_castling |= wq;
        break;
      case 'k':
        pos_castling |= bk;
        break;
      case 'q':
        pos_castling |= bq;
        break;
      case '-':
        pos_castling = 0;
        break;
      default:
        fen_error();

    }
    fen_index++;
  }

  // en passant
  fen_index++;

  if (fen_string[fen_index] == '-') {
    pos_ep = none;
  } else {
    pos_ep = fen_string[fen_index] - 'a';
    pos_ep += ('8' - fen_string[fen_index + 1]) * 8;
    first_pos_ep = pos_ep;
  }


}


static inline void add_prio(moves *mlist, int move);
static inline void sort_caps(moves *mlist);
static inline void add_move(moves *mlist, int move);

int nextCapIndex = 0;
int piece_values[] = {
  [P] = 1, [p] = 1,
  [N] = 3, [n] = 3,
  [B] = 3, [b] = 3,
  [R] = 5, [r] = 5,
  [Q] = 9, [q] = 9,
  [K] = 999, [k] = 999
};
/*
 * A precheck is performed before resorting to
 * make-unmake: If the piece that is to move
 * is removed from the board and the moving side is not
 * in check, then the execution of this move can't 
 * possibly result in a check. Exceptions: king moves and EP!
 * This is to avoid make/unmake whenever possible, as those
 * are expensive operations.
 *
 */
static inline void add_move(moves *mlist, int move) {

/*
  int piece = get_move_piece(move);

  //precheck
  if (piece != K && piece != k && !get_move_ep(move)) {
    const U64 sourceBB = 1ULL << get_move_source(move);
    pos_pieces[piece] &= ~(sourceBB);
    pos_occupancies[2] &= ~(sourceBB);
    int isInCheck = isKingInCheck(pos_side);
    pos_pieces[piece] |= sourceBB;
    pos_occupancies[2] |= sourceBB;
    if (!isInCheck) {
      add_prio(mlist, move);
      return;
    }
  }
*/
 // full check
  make_move(move);
  if (!isKingInCheck(!pos_side)) { //legal move
    if (isKingInCheck(pos_side)) {
      move = set_move_check(move); 
    }
    add_prio(mlist, move);
  }
  takeback();
}

static inline void add_prio(moves *mlist, int move) {

  if (get_move_capture(move)) {
    if (nextCapIndex < mlist->current_index) {
      int temp = mlist->moves[nextCapIndex];
      mlist->moves[nextCapIndex] = move;
      move = temp; 
    }
    nextCapIndex++;
    mlist->capture_count++;
  }
  mlist->moves[mlist->current_index++] = move;
}

//MVV - LVA
static inline void sort_caps(moves *mlist) { 


  for (int i = 0; i < nextCapIndex; i++) {
    int best_index = 0;
    int max = INT_MIN;

    for (int j = i; j < nextCapIndex; j++) { 
      int move = mlist->moves[j];
      int profit;
      if (get_move_ep(move))
        profit = 0;
      else  
        profit = piece_values[pos_occupancy[get_move_target(move)]] - piece_values[get_move_piece(move)];
      
      if (profit > max) {
        max = profit;
        best_index = j;
      }
    }
    int temp = mlist->moves[i];
    mlist->moves[i] = mlist->moves[best_index];
    mlist->moves[best_index] = temp;
  }
}




moves generate_moves(void) {
  moves glist;
  glist.current_index = 0;
  glist.capture_count = 0;
  nextCapIndex = 0;
  int source, target;
  U64 bitboard, attacks;
  int const min = pos_side ? p : P;
  int const max = min + 5;

  for (int piece = min; piece <= max; piece++) {

    bitboard = pos_pieces[piece];

    switch (piece) {

      case P:
        while (bitboard) {
          source = first_set_bit(bitboard);
          clear_bit(bitboard, source);
          target = source - 8; 

          if (!(is_set(pos_occupancies[both], target))) { // target not occupied
            if (source < a6) { // promotion
              add_move(&glist, encode_move(P, source, target, Q, 0, 0, 0, 0));
              add_move(&glist, encode_move(P, source, target, R, 0, 0, 0, 0));
              add_move(&glist, encode_move(P, source, target, B, 0, 0, 0, 0));
              add_move(&glist, encode_move(P, source, target, N, 0, 0, 0, 0));
            } else { 
              add_move(&glist, encode_move(P, source, target, 0, 0, 0, 0, 0)); // normal move
              if (source > h3 && !(is_set(pos_occupancies[both], (target - 8)))) { // double push
                add_move(&glist, encode_move(P, source, (target - 8), 0, 0, 1, 0, 0));
              }
            }
          }

          // captures
          attacks = pawn_attacks[pos_side][source];

          if ((pos_ep != none) && (attacks & (1ULL << pos_ep))) { // en passant
            add_move(&glist, encode_move(P, source, pos_ep, 0, 1, 0, 1, 0));
            clear_bit(attacks, pos_ep);
          }

          attacks &= pos_occupancies[black];

          while (attacks) {
            target = first_set_bit(attacks);
            if (source < a6) { // capture and promotion
              add_move(&glist, encode_move(P, source, target, Q, 1, 0, 0, 0));
              add_move(&glist, encode_move(P, source, target, R, 1, 0, 0, 0));
              add_move(&glist, encode_move(P, source, target, B, 1, 0, 0, 0));
              add_move(&glist, encode_move(P, source, target, N, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(P, source, target, 0, 1, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }
        }
        break;

      case N:
        while (bitboard) {
          source = first_set_bit(bitboard);
          attacks = knight_attacks[source] & (~pos_occupancies[white]); // don't capture own pieces

          while (attacks) { // loop over target squares
            target = first_set_bit(attacks);

            if (is_set(pos_occupancies[black], target)) {
              add_move(&glist, encode_move(N, source, target, 0, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(N, source, target, 0, 0, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }
          clear_bit(bitboard, source);
        }
        break;

      case B:
        while (bitboard) {
          source = first_set_bit(bitboard);
          attacks = get_bishop_attacks(source, pos_occupancies[both]) &
            (~pos_occupancies[white]);
          while (attacks) { // loop over target squares
            target = first_set_bit(attacks);

            if (is_set(pos_occupancies[black], target)) {
              add_move(&glist, encode_move(B, source, target, 0, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(B, source, target, 0, 0, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }
          clear_bit(bitboard, source);
        }
        break;

      case R:
        while (bitboard) {
          source = first_set_bit(bitboard);

          attacks = get_rook_attacks(source, pos_occupancies[both]) &
            (~pos_occupancies[white]);
          while (attacks) { // loop over target squares
            target = first_set_bit(attacks);

            if (is_set(pos_occupancies[black], target)) {
              add_move(&glist, encode_move(R, source, target, 0, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(R, source, target, 0, 0, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }
          clear_bit(bitboard, source);
        }
        break;

      case Q:
        while (bitboard) {
          source = first_set_bit(bitboard);
          attacks = get_queen_attacks(source, pos_occupancies[both]) &
            (~pos_occupancies[white]);
          while (attacks) { // loop over target squares
            target = first_set_bit(attacks);

            if (is_set(pos_occupancies[black], target)) {
              add_move(&glist, encode_move(Q, source, target, 0, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(Q, source, target, 0, 0, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }
          clear_bit(bitboard, source);
        }
        break;

      case K:
        if (pos_castling & wk) {
          if (!(pos_occupancies[both] & f1g1)) { // f1 and g1 are not occupied
                                                 // make sure e1 and f1 are not under attack
            if (!is_square_attacked(e1, black) &&
                !is_square_attacked(f1, black)) {
              add_move(&glist, encode_move(K, e1, g1, 0, 0, 0, 0, 1));
            }
          }
        }
        if (pos_castling & wq) {
          if (!(pos_occupancies[both] & d1c1b1)) { // d1,c1 and b1 are not
                                                   // occupied
                                                   // make sure e1 and d1 are not under attack
            if (!is_square_attacked(e1, black) &&
                !is_square_attacked(d1, black)) {
              add_move(&glist, encode_move(K, e1, c1, 0, 0, 0, 0, 1));
            }
          }
        }

        source = first_set_bit(bitboard);
        attacks = king_attacks[source] & (~pos_occupancies[white]);
        while (attacks) { // loop over target squares
          target = first_set_bit(attacks);

          if (is_set(pos_occupancies[black], target)) {
            add_move(&glist, encode_move(K, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(&glist, encode_move(K, source, target, 0, 0, 0, 0, 0));
          }
          clear_bit(attacks, target);
        }
        break;

      case p:
        while (bitboard) {
          source = first_set_bit(bitboard);
          clear_bit(bitboard, source);
          target = source + 8;

          if (!(is_set(pos_occupancies[both], target))) { // target not occupied
            if (source > h3) { // promotion
              add_move(&glist, encode_move(p, source, target, q, 0, 0, 0, 0));
              add_move(&glist, encode_move(p, source, target, r, 0, 0, 0, 0));
              add_move(&glist, encode_move(p, source, target, b, 0, 0, 0, 0));
              add_move(&glist, encode_move(p, source, target, n, 0, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(p, source, target, 0, 0, 0, 0, 0)); // normal move
              if (source < a6 && !(is_set(pos_occupancies[both], (target + 8)))) { // double push
                add_move(&glist, encode_move(p, source, (target + 8), 0, 0, 1, 0, 0));
              }
            }
          }

          // captures
          attacks = pawn_attacks[pos_side][source];

          if ((pos_ep != none) && (attacks & (1ULL << pos_ep))) { // en passant
            add_move(&glist, encode_move(p, source, pos_ep, 0, 1, 0, 1, 0));
            clear_bit(attacks, pos_ep);
          }

          attacks &= pos_occupancies[white];

          while (attacks) {
            target = first_set_bit(attacks);
            if (source > h3) { // capture and promotion
              add_move(&glist, encode_move(p, source, target, q, 1, 0, 0, 0));
              add_move(&glist, encode_move(p, source, target, r, 1, 0, 0, 0));
              add_move(&glist, encode_move(p, source, target, b, 1, 0, 0, 0));
              add_move(&glist, encode_move(p, source, target, n, 1, 0, 0, 0));
            } else { // simple capture
              add_move(&glist, encode_move(p, source, target, 0, 1, 0, 0, 0));
            }

            clear_bit(attacks, target);
          }
        }
        break;

      case n:

        while (bitboard) {
          source = first_set_bit(bitboard);
          attacks = knight_attacks[source] & (~pos_occupancies[black]); // don't capture own pieces
                                                                        // loop over target squares
          while (attacks) {
            target = first_set_bit(attacks);

            if (is_set(pos_occupancies[white], target)) {
              add_move(&glist, encode_move(n, source, target, 0, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(n, source, target, 0, 0, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }

          clear_bit(bitboard, source);
        }

        break;

      case b:
        while (bitboard) {
          source = first_set_bit(bitboard);

          attacks = get_bishop_attacks(source, pos_occupancies[both]) &
            (~pos_occupancies[black]);
          while (attacks) { // loop over target squares
            target = first_set_bit(attacks);

            if (is_set(pos_occupancies[white], target)) {
              add_move(&glist, encode_move(b, source, target, 0, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(b, source, target, 0, 0, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }
          clear_bit(bitboard, source);
        }
        break;

      case r:
        while (bitboard) {
          source = first_set_bit(bitboard);

          attacks = get_rook_attacks(source, pos_occupancies[both]) &
            (~pos_occupancies[black]);
          while (attacks) { // loop over target squares
            target = first_set_bit(attacks);

            if (is_set(pos_occupancies[white], target)) {
              add_move(&glist, encode_move(r, source, target, 0, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(r, source, target, 0, 0, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }
          clear_bit(bitboard, source);
        }
        break;

      case q:
        while (bitboard) {
          source = first_set_bit(bitboard);
          attacks = get_queen_attacks(source, pos_occupancies[both]) &
            (~pos_occupancies[black]);
          while (attacks) { // loop over target squares
            target = first_set_bit(attacks);

            if (is_set(pos_occupancies[white], target)) {
              add_move(&glist, encode_move(q, source, target, 0, 1, 0, 0, 0));
            } else {
              add_move(&glist, encode_move(q, source, target, 0, 0, 0, 0, 0));
            }
            clear_bit(attacks, target);
          }
          clear_bit(bitboard, source);
        }
        break;

      case k:
        if (pos_castling & bk) {
          if (!(pos_occupancies[both] & f8g8)) { // f8 and g8 are not occupied
                                                 // make sure e8 and f8 are not under attack
            if (!is_square_attacked(e8, white) &&
                !is_square_attacked(f8, white)) {
              add_move(&glist, encode_move(k, e8, g8, 0, 0, 0, 0, 1));
            }
          }
        }
        if (pos_castling & bq) {
          if (!(pos_occupancies[both] & d8c8b8)) { // d8,c8 and b8 are not
                                                   // occupied
                                                   // make sure e8 and d8 are not under attack
            if (!is_square_attacked(e8, white) &&
                !is_square_attacked(d8, white)) {
              add_move(&glist, encode_move(k, e8, c8, 0, 0, 0, 0, 1));
            }
          }
        }

        source = first_set_bit(bitboard);
        attacks = king_attacks[source] & (~pos_occupancies[black]);
        while (attacks) { // loop over target squares
          target = first_set_bit(attacks);

          if (is_set(pos_occupancies[white], target)) {
            add_move(&glist, encode_move(k, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(&glist, encode_move(k, source, target, 0, 0, 0, 0, 0));
          }
          clear_bit(attacks, target);
        }
        break;
    }
  }


  sort_caps(&glist);
  return glist;
}


void print_move_UCI(int move) {
    printf("%s\n",get_move_UCI(move));
}

char* get_move_UCI(int move) {
  static char str[6];
  str[0] = '\0';
  strcat(str, square_to_coordinates[get_move_source(move)]);
  strcat(str, square_to_coordinates[get_move_target(move)]);
  if (get_move_promotion(move)) {
    str[4] = promoted_pieces[get_move_promotion(move)];
    str[5] = '\0';
  } else {
    str[4] = '\0';
  }
  return str;
}


void print_move_list(moves* move_list) {
    for (int i = 0; i < move_list->current_index; i++) {
        print_move_UCI(move_list->moves[i]);
    }
}

void print_move(int move) {

	char prom_piece = ascii_pieces[get_move_promotion(move)];
  
	printf("------------------------\n");
	printf("Piece:.............%c\n", ascii_pieces[get_move_piece(move)]);
	printf("Source:............%s\n", square_to_coordinates[get_move_source(move)]);
	printf("Target:............%s\n", square_to_coordinates[get_move_target(move)]);
  printf("Promoted piece:....%c\n", prom_piece == 'P' ? ' ' : prom_piece);
  printf("Capture:...........%s\n", get_move_capture(move) ? "true" : "false");
  printf("Double Pawn Move:..%s\n", get_move_double(move) ? "true" : "false");	
  printf("En Passant:........%s\n", get_move_ep(move) ? "true" : "false");	
  printf("Castling:..........%s\n", get_move_castling(move) ? "true" : "false");		
  printf("Is Check:..........%s\n", get_move_check(move) ? "true" : "false");	
  printf("------------------------\n\n");


}


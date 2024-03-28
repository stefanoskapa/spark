#include "../attack_tables/attack_tables.h"
#include "../bit_utils/bit_utils.h"
#include "../board_utils/board_utils.h"
#include "../generator/generator.h"
#include "../move_encoding/move_encoding.h"
#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

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

void takeback() {

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



void fast_make(int move) {

  int const piece = get_move_piece(move);
  int const source = get_move_source(move);
  int const target = get_move_target(move);
  U64 const sourceBB = 1ULL << source;
  U64 const targetBB = 1ULL << target;

  if (pos_side == white) {
    if (get_move_capture(move)) {
      if (get_move_ep(move)) {
        push(&pos_captured, p);
        U64 pawn_kill = ~(1ULL << (pos_ep + 8));
        pos_pieces[p] &= pawn_kill;
        pos_occupancies[2] &= pawn_kill;
      } else {                              // not ep
        int dead_piece = pos_occupancy[target];
        pos_pieces[dead_piece] &= (~targetBB); // remove captured piece
        push(&pos_captured, dead_piece);       // push captured piece to stack
      }
    }

    // Move piece to target
    pos_pieces[piece] &= (~sourceBB); // remove piece from source
    pos_occupancies[2] &= (~sourceBB);        // remove source from total occupancy
    
    pos_pieces[piece] |= targetBB; // add piece to bitboard
    pos_occupancies[2] |= targetBB;           // add target to total occupancy

  } else { //black
    if (get_move_capture(move)) {
      if (get_move_ep(move)) {
        push(&pos_captured, P);
        U64 pawn_kill = ~(1ULL << (pos_ep - 8));
        pos_pieces[P] &= pawn_kill;
        pos_occupancies[2] &= pawn_kill;
      } else {                              // not ep
        int dead_piece = pos_occupancy[target];
        pos_pieces[dead_piece] &= (~targetBB); // remove captured piece
        push(&pos_captured, dead_piece);       // push captured piece to stack
      }
    }

    // Move piece to target
    pos_pieces[piece] &= (~sourceBB); // remove piece from source
    pos_occupancies[2] &= (~sourceBB);        // remove source from total occupancy
    
    pos_pieces[piece] |= targetBB; // add piece to bitboard
    pos_occupancies[2] |= targetBB;           // add target to total occupancy

  }


  push(&pos_moves, move); // push new move
  pos_side = !(pos_side); // change turns
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

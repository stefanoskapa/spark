#include <stdio.h>
#include <limits.h>
#include "board.h"
#include "../move_encoding/move_encoding.h"

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

const char ascii_pieces[] = "PNBRQKpnbrqk";

const int char_pieces[] = {
  ['P'] = P, ['N'] = N, ['B'] = B, ['R'] = R, ['Q'] = Q, ['K'] = K,
  ['p'] = p, ['n'] = n, ['b'] = b, ['r'] = r, ['q'] = q, ['k'] = k};

const int promoted_pieces[] = {
  [Q] = 'q', [R] = 'r', [B] = 'b', [N] = 'n',
  [q] = 'q', [r] = 'r', [b] = 'b', [n] = 'n',
};

const char *square_to_coordinates[] = {
  "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "a7", "b7", "c7",
  "d7", "e7", "f7", "g7", "h7", "a6", "b6", "c6", "d6", "e6", "f6",
  "g6", "h6", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a4",
  "b4", "c4", "d4", "e4", "f4", "g4", "h4", "a3", "b3", "c3", "d3",
  "e3", "f3", "g3", "h3", "a2", "b2", "c2", "d2", "e2", "f2", "g2",
  "h2", "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"};

int first_pos_ep = none;

void make_move(int move) {

  push(&pos_castling_stack, pos_castling); // save castling state

  int const piece = GET_MOVE_PIECE(move);
  int const source = GET_MOVE_SOURCE(move);
  int const target = GET_MOVE_TARGET(move);
  int const prom_piece = GET_MOVE_PROMOTION(move);
  U64 const sourceBB = 1ULL << source;
  U64 const targetBB = 1ULL << target;
  
  if (pos_side == WHITE) {
    if (GET_MOVE_CAPTURE(move)) {
      if (GET_MOVE_EP(move)) {
        push(&pos_captured, p);
        pos_occupancy[pos_ep + 8] = INT_MAX;
        U64 pawn_kill = ~(1ULL << (pos_ep + 8));
        pos_pieces[p] &= pawn_kill;
        pos_occupancies[BLACK] &= pawn_kill;
        pos_occupancies[BOTH] &= pawn_kill;
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
        pos_occupancies[BLACK] &= ~targetBB;     // remove captured piece from black's occupancy
      }
    }

    // Move piece to target
    pos_pieces[piece] &= (~sourceBB); // remove piece from source
    pos_occupancy[source] = INT_MAX; // remove piece from occupancy array
    pos_occupancies[BOTH] &= (~sourceBB);        // remove source from total occupancy
    pos_occupancies[WHITE] &= (~sourceBB); // remove source to white's  occupancy
   
    if (prom_piece){     // promotion
      pos_pieces[prom_piece] |= targetBB; // add promoted piece to bitboard
      pos_occupancy[target] = prom_piece; // add promoted piece to occupancy array
    } else { //not promotion
      pos_pieces[piece] |= targetBB; // add piece to bitboard
      pos_occupancy[target] = piece; // add piece to occupancy array
    }
    pos_occupancies[BOTH] |= targetBB;           // add target to total occupancy
    pos_occupancies[WHITE] |= targetBB;    // add target to white's  occupancy

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
        if (GET_MOVE_CASTLING(move)) {
          if (target == g1) {
            pos_pieces[R] &= NOT_H1; // remove rook from h1
            pos_occupancy[h1] = INT_MAX;
            pos_occupancies[WHITE] &= NOT_H1;
            pos_occupancies[BOTH] &= NOT_H1;
            pos_pieces[R] |= F1; // place rook on f1
            pos_occupancy[f1] = R;
            pos_occupancies[WHITE] |= F1;
            pos_occupancies[BOTH] |= F1;
          } else if (target == c1) {  
            pos_pieces[R] &= NOT_A1; // remove rook from a1
            pos_occupancy[a1] = INT_MAX;
            pos_occupancies[WHITE] &= NOT_A1;
            pos_occupancies[BOTH] &= NOT_A1;
            pos_pieces[R] |= D1; // place rook on d1
            pos_occupancy[d1] = R; 
            pos_occupancies[WHITE] |= D1;
            pos_occupancies[BOTH] |= D1;
          }
        }
        pos_castling &= 12; // disable all castling for white
        break;
      case P:
        if (GET_MOVE_DOUBLE(move)) {
          pos_ep = target + 8;
        }
    }

  } else { //black
    if (GET_MOVE_CAPTURE(move)) {
      if (GET_MOVE_EP(move)) {
        push(&pos_captured, P);
        pos_occupancy[pos_ep - 8] = INT_MAX;
        U64 pawn_kill = ~(1ULL << (pos_ep - 8));
        pos_pieces[P] &= pawn_kill;
        pos_occupancies[WHITE] &= pawn_kill;
        pos_occupancies[BOTH] &= pawn_kill;
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
        pos_occupancies[WHITE] &= ~targetBB;     // remove captured piece from white's occupancy
      }
    }

    // Move piece to target
    pos_pieces[piece] &= (~sourceBB); // remove piece from source
    pos_occupancy[source] = INT_MAX; // remove piece from occupancy array
    pos_occupancies[BOTH] &= (~sourceBB);        // remove source from total occupancy
    pos_occupancies[BLACK] &= (~sourceBB); // remove source from black's  occupancy
    
    if (prom_piece){     // promotion
      pos_pieces[prom_piece] |= targetBB; // add promoted piece to bitboard
      pos_occupancy[target] = prom_piece; // add promoted piece to occupancy array
    } else { //not promotion
      pos_pieces[piece] |= targetBB; // add piece to bitboard
      pos_occupancy[target] = piece; // add piece to occupancy array
    }
    pos_occupancies[BOTH] |= targetBB;           // add target to total occupancy
    pos_occupancies[BLACK] |= targetBB;    // add target to black's  occupancy

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
        if (GET_MOVE_CASTLING(move)) {
          if (target == g8) {
            pos_pieces[r] &= NOT_H8; // remove rook from h8
            pos_occupancy[h8] = INT_MAX;
            pos_occupancies[BLACK] &= NOT_H8;
            pos_occupancies[BOTH] &= NOT_H8;
            pos_pieces[r] |= F8; // place rook on f8
            pos_occupancy[f8] = r;
            pos_occupancies[BLACK] |= F8;
            pos_occupancies[BOTH] |= F8;
          } else if (target == c8) {
            pos_pieces[r] &= NOT_A8; // remove rook from a8
            pos_occupancy[a8] = INT_MAX;
            pos_occupancies[BLACK] &= NOT_A8;
            pos_occupancies[BOTH] &= NOT_A8;
            pos_pieces[r] |= D8; // place rook on d8
            pos_occupancy[d8] = r;
            pos_occupancies[BLACK] |= D8;
            pos_occupancies[BOTH] |= D8;
          }
        } 
        pos_castling &= 3; // disable all castling for black
        break;
      case p:
        if (GET_MOVE_DOUBLE(move)) {
          pos_ep = target - 8;
        }
    }

  }


  push(&pos_moves, move); // push new move
  pos_side = !(pos_side); // change turns
}

void takeback(void) {

  int lmove = pop(&pos_moves);

  int const source = GET_MOVE_SOURCE(lmove);
  int const target = GET_MOVE_TARGET(lmove);
  int const piece = GET_MOVE_PIECE(lmove);
  int const pr_piece = GET_MOVE_PROMOTION(lmove);
  U64 const sourceBB = 1ULL << source;
  U64 const targetBB = 1ULL << target;

  if (pr_piece) {
    if (pos_side) {
      pos_pieces[P] |= sourceBB; // put pawn back to source
      pos_occupancy[source] = P;
    } else {
      pos_pieces[p] |= sourceBB; // put pawn back to source
      pos_occupancy[source] = p;
    }

    pos_pieces[pr_piece] &= (~targetBB);      // remove promoted piece
  } else {
    pos_pieces[piece] |= sourceBB;    // put piece back to source
    pos_occupancy[source] = piece;
    pos_pieces[piece] &= (~targetBB); // remove piece from target
  }
  pos_occupancy[target] = INT_MAX;
  
  pos_occupancies[BOTH] |= sourceBB;         // add piece to total occupancy
  pos_occupancies[!pos_side] |= sourceBB; // add piece to its color's occupancy
  pos_occupancies[BOTH] &= (~targetBB);      // remove piece from total occupancy
  pos_occupancies[!pos_side] &= (~targetBB); // remove piece from its color's occupancy

  if (GET_MOVE_CAPTURE(lmove)) { // restore captured piece
    int cap_piece = pop(&pos_captured);

    if (GET_MOVE_EP(lmove)) {
      int ep_target = target + (cap_piece == P ? -8 : +8);
      pos_pieces[cap_piece] |= 1ULL << ep_target;
      pos_occupancy[ep_target] = cap_piece;
      pos_occupancies[BOTH] |= 1ULL << ep_target;
      pos_occupancies[pos_side] |= 1ULL << ep_target;
    } else {
      pos_pieces[cap_piece] |= targetBB; //put captured piece back (target of last move)
      pos_occupancy[target] = cap_piece; //update occupancies
      pos_occupancies[BOTH] |= targetBB; 
      pos_occupancies[pos_side] |= targetBB; 
    }
 
  }

  // check if last move now was a double pawn push, if yes, set pos_ep
  if (pos_moves.index > 0) {
    int in = pos_moves.index - 1;
    int lastmove = pos_moves.items[in];
    if (GET_MOVE_DOUBLE(lastmove)) {
      pos_ep =
        GET_MOVE_TARGET(lastmove) + (GET_MOVE_PIECE(lastmove) == P ? +8 : -8);
    } else {
      pos_ep = none;
    }
  } else {
    pos_ep = first_pos_ep;
  }

  // castling
  if (GET_MOVE_CASTLING(lmove)) {

    switch (target) {
      case g1: // white short
        pos_pieces[R] &= NOT_F1;
        pos_occupancy[f1] = INT_MAX;
        pos_pieces[R] |= H1;
        pos_occupancy[h1] = R;
        pos_occupancies[WHITE] &= NOT_F1;
        pos_occupancies[BOTH] &= NOT_F1;
        pos_occupancies[WHITE] |= H1;
        pos_occupancies[BOTH] |= H1;
        break;
      case c1: // white long
        pos_pieces[R] &= NOT_D1;
        pos_occupancy[d1] = INT_MAX;
        pos_pieces[R] |= A1;
        pos_occupancy[a1] = R;
        pos_occupancies[WHITE] &= NOT_D1;
        pos_occupancies[BOTH] &= NOT_D1;
        pos_occupancies[WHITE] |= A1;
        pos_occupancies[BOTH] |= A1;
        break;
      case g8: // black short
        pos_pieces[r] &= NOT_F8;
        pos_occupancy[f8] = INT_MAX;
        pos_pieces[r] |= H8;
        pos_occupancy[h8] = r;
        pos_occupancies[BLACK] &= NOT_F8;
        pos_occupancies[BOTH] &= NOT_F8;
        pos_occupancies[BLACK] |= H8;
        pos_occupancies[BOTH] |= H8;
        break;
      case c8: // white short
        pos_pieces[r] &= NOT_D8;
        pos_occupancy[d8] = INT_MAX;
        pos_pieces[r] |= A8;
        pos_occupancy[a8] = r;
        pos_occupancies[BLACK] &= NOT_D8;
        pos_occupancies[BOTH] &= NOT_D8;
        pos_occupancies[BLACK] |= A8;
        pos_occupancies[BOTH] |= A8;
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

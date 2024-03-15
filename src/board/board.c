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
  int const min = pos_side ? P : p;
  int const max = min + 4;

  // handle captures: Remove captured piece and adjust castling
  if (get_move_capture(move)) {
    if (get_move_ep(move)) {
      push(&pos_captured, piece == P ? p : P);
      U64 pawn_kill = ~(1ULL << (pos_ep + (piece == P ? 8 : -8)));
      pos_pieces[piece < 6 ? p : P] &= pawn_kill;
      pos_occupancies[piece < 6 ? 1 : 0] &= pawn_kill;
      pos_occupancies[2] &= pawn_kill;
    } else {                              // not ep
      for (int i = min; i <= max; i++) {  // search for the captured piece
        if (pos_pieces[i] & (targetBB)) { // found

          if (i == R || i == r) { // rook captured, adjust castling rights
            switch (target) {
            case a1:
              pos_castling &= 13;
              break;
            case h1:
              pos_castling &= 14;
              break;
            case a8:
              pos_castling &= 7;
              break;
            case h8:
              pos_castling &= 11;
            }
          }

          pos_pieces[i] &= (~targetBB); // remove captured piece
          push(&pos_captured, i);       // push captured piece to stack
          break;
        }
      }
      pos_occupancies[piece < 6 ? 1 : 0] &= ~targetBB;
    }
  }

  // Move piece to target
  pos_pieces[piece] &= (~sourceBB); // remove piece from source
  if (get_move_promotion(move))     // add piece to target
    pos_pieces[get_move_promotion(move)] |= targetBB; // promotion
  else
    pos_pieces[piece] |= targetBB;
  pos_occupancies[2] &= (~sourceBB);        // remove from total occupancy
  pos_occupancies[2] |= targetBB;           // add to total occupancy
  pos_occupancies[pos_side] &= (~sourceBB); // remove source from my occupancy
  pos_occupancies[pos_side] |= targetBB;    // add target to my occupancy

  // special cases
  switch (piece) {
  case R:
  case r:
    switch (source) {
    case a1:
      pos_castling &= 13; // 1101 = disable white long
      break;
    case h1:
      pos_castling &= 14; // 1110 = disable white short
      break;
    case a8:
      pos_castling &= 7; // 0111 = disable black long
      break;
    case h8:
      pos_castling &= 11; // 1011 = disable black short
    }
    break;

  case k:
  case K:
    if (get_move_castling(move)) {
      switch (target) {
      case g1:
        pos_pieces[R] &= NOT_H1; // remove rook from h1
        pos_occupancies[0] &= NOT_H1;
        pos_occupancies[2] &= NOT_H1;
        pos_pieces[R] |= F1; // place rook on f1
        pos_occupancies[0] |= F1;
        pos_occupancies[2] |= F1;
        break;
      case c1:
        pos_pieces[R] &= NOT_A1; // remove rook from h1
        pos_occupancies[0] &= NOT_A1;
        pos_occupancies[2] &= NOT_A1;
        pos_pieces[R] |= D1; // place rook on f1
        pos_occupancies[0] |= D1;
        pos_occupancies[2] |= D1;
        break;
      case g8:
        pos_pieces[r] &= NOT_H8; // remove rook from h8
        pos_occupancies[1] &= NOT_H8;
        pos_occupancies[2] &= NOT_H8;
        pos_pieces[r] |= F8; // place rook on f8
        pos_occupancies[1] |= F8;
        pos_occupancies[2] |= F8;
        break;
      case c8:
        pos_pieces[r] &= NOT_A8; // remove rook from a8
        pos_occupancies[1] &= NOT_A8;
        pos_occupancies[2] &= NOT_A8;
        pos_pieces[r] |= D8; // place rook on d8
        pos_occupancies[1] |= D8;
        pos_occupancies[2] |= D8;
      }
    }
    pos_castling &= pos_side ? 3 : 12; // disable all castling for me
  }

  // update pos_ep
  pos_ep = get_move_double(move) ? (target + (piece == P ? 8 : -8)) : none;
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
    pos_pieces[pr_piece] &= (~targetBB);      // remove promoted piece
  } else {
    pos_pieces[piece] |= sourceBB;    // put piece back to source
    pos_pieces[piece] &= (~targetBB); // remove piece from target
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
      pos_occupancies[2] |= targetBB;
      pos_occupancies[pos_side] |= targetBB;
    } else {
      int ep_target = target + (cap_piece == P ? -8 : +8);
      pos_pieces[cap_piece] |= 1ULL << ep_target;
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
      pos_pieces[R] |= H1;
      pos_occupancies[0] &= NOT_F1;
      pos_occupancies[2] &= NOT_F1;
      pos_occupancies[0] |= H1;
      pos_occupancies[2] |= H1;
      break;
    case c1: // white long
      pos_pieces[R] &= NOT_D1;
      pos_pieces[R] |= A1;
      pos_occupancies[0] &= NOT_D1;
      pos_occupancies[2] &= NOT_D1;
      pos_occupancies[0] |= A1;
      pos_occupancies[2] |= A1;
      break;
    case g8: // black short
      pos_pieces[r] &= NOT_F8;
      pos_pieces[r] |= H8;
      pos_occupancies[1] &= NOT_F8;
      pos_occupancies[2] &= NOT_F8;
      pos_occupancies[1] |= H8;
      pos_occupancies[2] |= H8;
      break;
    case c8: // white short
      pos_pieces[r] &= NOT_D8;
      pos_pieces[r] |= A8;
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

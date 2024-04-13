#include <stdio.h>
#include <limits.h>
#include "../attack_tables/attack_tables.h"
#include "../board_utils/board_utils.h"
#include "../board/board.h"
#include "../move_encoding/move_encoding.h"
#include "generator.h"

static inline void add_prio(moves *mlist, int move);
static inline void sort_caps(moves *mlist);
static inline void add_move(moves *mlist, int move);

int nextCapIndex = 0;
static const int piece_values[] = {
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


  int piece = GET_MOVE_PIECE(move);

  //precheck
  if (piece != K && piece != k && !GET_MOVE_EP(move)) {
    const U64 sourceBB = 1ULL << GET_MOVE_SOURCE(move);
    pos_pieces[piece] &= ~(sourceBB);
    pos_occupancies[2] &= ~(sourceBB);
    int isInCheck = IS_KING_IN_CHECK(pos_side);
    pos_pieces[piece] |= sourceBB;
    pos_occupancies[2] |= sourceBB;
    if (!isInCheck) {
      add_prio(mlist, move);
      return;
    }
  }

 // full check
  make_move(move);
  if (!IS_KING_IN_CHECK((!pos_side))) { //legal move
//    if (IS_KING_IN_CHECK(pos_side)) {
 //     move = SET_MOVE_CHECK(move); 
 //   }
    add_prio(mlist, move);
  }
  takeback();
}

static inline void add_prio(moves *mlist, int move) {

  if (GET_MOVE_CAPTURE(move)) {
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
      if (GET_MOVE_EP(move))
        profit = 0;
      else { 
        show_occ_board();
        printf("captured piece: %d\n", pos_occupancy[GET_MOVE_TARGET(move)]);      
        printf("capturer: %d\n", GET_MOVE_PIECE(move));
        profit = piece_values[pos_occupancy[GET_MOVE_TARGET(move)]] - piece_values[GET_MOVE_PIECE(move)];
      
      }
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
  U64 const my_neg_occ = ~pos_occupancies[pos_side]; //my negative occupancy
  U64 const his_occ = pos_occupancies[!pos_side];

  for (int piece = min; piece <= max; piece++) {

    bitboard = pos_pieces[piece];

    switch (piece) {

      case P:
        while (bitboard) {
          source = FIRST_SET_BIT(bitboard);
          CLEAR_BIT(bitboard, source);
          target = source - 8; 

          if (!(IS_SET(pos_occupancies[BOTH], target))) { // target not occupied
            if (source < a6) { // promotion
              add_move(&glist, ENCODE_PROM(P, source, target, Q));
              add_move(&glist, ENCODE_PROM(P, source, target, R));
              add_move(&glist, ENCODE_PROM(P, source, target, B));
              add_move(&glist, ENCODE_PROM(P, source, target, N));
            } else { 
              add_move(&glist, ENCODE_SIMPLE_MOVE(P, source, target)); // normal move
              if (source > h3 && !(IS_SET(pos_occupancies[BOTH], (target - 8)))) { // double push
                add_move(&glist, ENCODE_DOUBLE(P, source, (target - 8)));
              }
            }
          }

          // captures
          attacks = pawn_attacks[pos_side][source];

          if ((pos_ep != none) && (attacks & (1ULL << pos_ep))) { // en passant
            add_move(&glist, ENCODE_EP(P, source, pos_ep));
            CLEAR_BIT(attacks, pos_ep);
          }

          attacks &= his_occ;

          while (attacks) {
            target = FIRST_SET_BIT(attacks);
            if (source < a6) { // capture and promotion
              add_move(&glist, ENCODE_CAP_PROM(P, source, target, Q));
              add_move(&glist, ENCODE_CAP_PROM(P, source, target, R));
              add_move(&glist, ENCODE_CAP_PROM(P, source, target, B));
              add_move(&glist, ENCODE_CAP_PROM(P, source, target, N));
            } else {
              add_move(&glist, ENCODE_SIMPLE_CAPTURE(P, source, target));
            }
            CLEAR_BIT(attacks, target);
          }
        }
        break;

      case N:
      case n:
        while (bitboard) {
          source = FIRST_SET_BIT(bitboard);
          attacks = knight_attacks[source] & my_neg_occ; // don't capture own pieces

          while (attacks) { // loop over target squares
            target = FIRST_SET_BIT(attacks);

            if (IS_SET(his_occ, target)) {
              add_move(&glist, ENCODE_SIMPLE_CAPTURE(piece, source, target));
            } else {
              add_move(&glist, ENCODE_SIMPLE_MOVE(piece, source, target));
            }
            CLEAR_BIT(attacks, target);
          }
          CLEAR_BIT(bitboard, source);
        }
        break;

      case B:
      case b:
        while (bitboard) {
          source = FIRST_SET_BIT(bitboard);
          attacks = get_bishop_attacks(source, pos_occupancies[BOTH]) & my_neg_occ;
          while (attacks) { // loop over target squares
            target = FIRST_SET_BIT(attacks);

            if (IS_SET(his_occ, target)) {
              add_move(&glist, ENCODE_SIMPLE_CAPTURE(piece, source, target));
            } else {
              add_move(&glist, ENCODE_SIMPLE_MOVE(piece, source, target));
            }
            CLEAR_BIT(attacks, target);
          }
          CLEAR_BIT(bitboard, source);
        }
        break;

      case R:
      case r:
        while (bitboard) {
          source = FIRST_SET_BIT(bitboard);

          attacks = get_rook_attacks(source, pos_occupancies[BOTH]) & my_neg_occ;
          while (attacks) { // loop over target squares
            target = FIRST_SET_BIT(attacks);

            if (IS_SET(his_occ, target)) {
              add_move(&glist, ENCODE_SIMPLE_CAPTURE(piece, source, target));
            } else {
              add_move(&glist, ENCODE_SIMPLE_MOVE(piece, source, target));
            }
            CLEAR_BIT(attacks, target);
          }
          CLEAR_BIT(bitboard, source);
        }
        break;

      case Q:
      case q:
        while (bitboard) {
          source = FIRST_SET_BIT(bitboard);
          attacks = get_queen_attacks(source, pos_occupancies[BOTH]) & my_neg_occ;
          while (attacks) { // loop over target squares
            target = FIRST_SET_BIT(attacks);

            if (IS_SET(his_occ, target)) {
              add_move(&glist, ENCODE_SIMPLE_CAPTURE(piece, source, target));
            } else {
              add_move(&glist, ENCODE_SIMPLE_MOVE(piece, source, target));
            }
            CLEAR_BIT(attacks, target);
          }
          CLEAR_BIT(bitboard, source);
        }
        break;


      case K:
        if (pos_castling & wk) {
          if (!(pos_occupancies[BOTH] & F1G1)) { // f1 and g1 are not occupied
                                                 // make sure e1 and f1 are not under attack
            if (!is_square_attacked(e1, BLACK) &&
                !is_square_attacked(f1, BLACK)) {
              add_move(&glist, ENCODE_CASTLING(K, e1, g1));
            }
          }
        }
        if (pos_castling & wq) {
          if (!(pos_occupancies[BOTH] & D1C1B1)) { // d1,c1 and b1 are not
                                                   // occupied
                                                   // make sure e1 and d1 are not under attack
            if (!is_square_attacked(e1, BLACK) &&
                !is_square_attacked(d1, BLACK)) {
              add_move(&glist, ENCODE_CASTLING(K, e1, c1));
            }
          }
        }

        source = FIRST_SET_BIT(bitboard);
        attacks = king_attacks[source] & (~pos_occupancies[WHITE]);
        while (attacks) { // loop over target squares
          target = FIRST_SET_BIT(attacks);

          if (IS_SET(pos_occupancies[BLACK], target)) {
            add_move(&glist, ENCODE_SIMPLE_CAPTURE(K, source, target));
          } else {
            add_move(&glist, ENCODE_SIMPLE_MOVE(K, source, target));
          }
          CLEAR_BIT(attacks, target);
        }
        break;

      case p:
        while (bitboard) {
          source = FIRST_SET_BIT(bitboard);
          CLEAR_BIT(bitboard, source);
          target = source + 8;

          if (!(IS_SET(pos_occupancies[BOTH], target))) { // target not occupied
            if (source > h3) { // promotion
              add_move(&glist, ENCODE_PROM(p, source, target, q));
              add_move(&glist, ENCODE_PROM(p, source, target, r));
              add_move(&glist, ENCODE_PROM(p, source, target, b));
              add_move(&glist, ENCODE_PROM(p, source, target, n));
            } else {
              add_move(&glist, ENCODE_SIMPLE_MOVE(p, source, target)); // normal move
              if (source < a6 && !(IS_SET(pos_occupancies[BOTH], (target + 8)))) { // double push
                add_move(&glist, ENCODE_DOUBLE(p, source, (target + 8)));
              }
            }
          }

          // captures
          attacks = pawn_attacks[pos_side][source];

          if ((pos_ep != none) && (attacks & (1ULL << pos_ep))) { // en passant
            add_move(&glist, ENCODE_EP(p, source, pos_ep));
            CLEAR_BIT(attacks, pos_ep);
          }

          attacks &= pos_occupancies[WHITE];

          while (attacks) {
            target = FIRST_SET_BIT(attacks);
            if (source > h3) { // capture and promotion
              add_move(&glist, ENCODE_CAP_PROM(p, source, target, q));
              add_move(&glist, ENCODE_CAP_PROM(p, source, target, r));
              add_move(&glist, ENCODE_CAP_PROM(p, source, target, b));
              add_move(&glist, ENCODE_CAP_PROM(p, source, target, n));
            } else { // simple capture
              add_move(&glist, ENCODE_SIMPLE_CAPTURE(p, source, target));
            }

            CLEAR_BIT(attacks, target);
          }
        }
        break;
      
      case k:
        if (pos_castling & bk) {
          if (!(pos_occupancies[BOTH] & F8G8)) { // f8 and g8 are not occupied
                                                 // make sure e8 and f8 are not under attack
            if (!is_square_attacked(e8, WHITE) &&
                !is_square_attacked(f8, WHITE)) {
              add_move(&glist, ENCODE_CASTLING(k, e8, g8));
            }
          }
        }
        if (pos_castling & bq) {
          if (!(pos_occupancies[BOTH] & D8C8B8)) { // d8,c8 and b8 are not
                                                   // occupied
                                                   // make sure e8 and d8 are not under attack
            if (!is_square_attacked(e8, WHITE) &&
                !is_square_attacked(d8, WHITE)) {
              add_move(&glist, ENCODE_CASTLING(k, e8, c8));
            }
          }
        }

        source = FIRST_SET_BIT(bitboard);
        attacks = king_attacks[source] & (~pos_occupancies[BLACK]);
        while (attacks) { // loop over target squares
          target = FIRST_SET_BIT(attacks);

          if (IS_SET(pos_occupancies[WHITE], target)) {
            add_move(&glist, ENCODE_SIMPLE_CAPTURE(k, source, target));
          } else {
            add_move(&glist, ENCODE_SIMPLE_MOVE(k, source, target));
          }
          CLEAR_BIT(attacks, target);
        }
        break;
    }
  }


  sort_caps(&glist);
  return glist;
}

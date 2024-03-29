
#include <stdio.h>
#include <limits.h>
#include "../attack_tables/attack_tables.h"
#include "../bit_utils/bit_utils.h"
#include "../board_utils/board_utils.h"
#include "../board/board.h"
#include "../move_encoding/move_encoding.h"
#include "generator.h"

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

 // full check
  make_move(move);
  if (!isKingInCheck(!pos_side)) { //legal move
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




moves* generate_moves() {
  static moves glist;
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
  return &glist;
}

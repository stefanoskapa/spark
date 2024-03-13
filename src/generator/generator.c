
#include <stdio.h>

#include "../attack_tables/attack_tables.h"
#include "../bit_utils/bit_utils.h"
#include "../board_utils/board_utils.h"
#include "../constants.h"
#include "../move_encoding/move_encoding.h"
#include "generator.h"

void add_move(moves *mlist, int move) {
  make_move(move);
  if (!isKingInCheck(!pos_side)) {
    mlist->moves[mlist->current_index] = move;
    mlist->current_index++;
  }
  takeback();
}

int generate_moves(moves *glist) {

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
            add_move(glist, encode_move(P, source, target, Q, 0, 0, 0, 0));
            add_move(glist, encode_move(P, source, target, R, 0, 0, 0, 0));
            add_move(glist, encode_move(P, source, target, B, 0, 0, 0, 0));
            add_move(glist, encode_move(P, source, target, N, 0, 0, 0, 0));
          } else { 
            add_move(glist, encode_move(P, source, target, 0, 0, 0, 0, 0)); // normal move
            if (source > h3 && !(is_set(pos_occupancies[both], (target - 8)))) { // double push
              add_move(glist, encode_move(P, source, (target - 8), 0, 0, 1, 0, 0));
            }
          }
        }

        // captures
        attacks = pawn_attacks[pos_side][source];

        if ((pos_ep != none) && (attacks & (1ULL << pos_ep))) { // en passant
          add_move(glist, encode_move(P, source, pos_ep, 0, 1, 0, 1, 0));
          clear_bit(attacks, pos_ep);
        }

        attacks &= pos_occupancies[black];

        while (attacks) {
          target = first_set_bit(attacks);
          if (source < a6) { // capture and promotion
            add_move(glist, encode_move(P, source, target, Q, 1, 0, 0, 0));
            add_move(glist, encode_move(P, source, target, R, 1, 0, 0, 0));
            add_move(glist, encode_move(P, source, target, B, 1, 0, 0, 0));
            add_move(glist, encode_move(P, source, target, N, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(P, source, target, 0, 1, 0, 0, 0));
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
            add_move(glist, encode_move(N, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(N, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(B, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(B, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(R, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(R, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(Q, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(Q, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(K, e1, g1, 0, 0, 0, 0, 1));
          }
        }
      }
      if (pos_castling & wq) {
        if (!(pos_occupancies[both] & d1c1b1)) { // d1,c1 and b1 are not
                                                 // occupied
          // make sure e1 and d1 are not under attack
          if (!is_square_attacked(e1, black) &&
              !is_square_attacked(d1, black)) {
            add_move(glist, encode_move(K, e1, c1, 0, 0, 0, 0, 1));
          }
        }
      }

      source = first_set_bit(bitboard);
      attacks = king_attacks[source] & (~pos_occupancies[white]);
      while (attacks) { // loop over target squares
        target = first_set_bit(attacks);

        if (is_set(pos_occupancies[black], target)) {
          add_move(glist, encode_move(K, source, target, 0, 1, 0, 0, 0));
        } else {
          add_move(glist, encode_move(K, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(p, source, target, q, 0, 0, 0, 0));
            add_move(glist, encode_move(p, source, target, r, 0, 0, 0, 0));
            add_move(glist, encode_move(p, source, target, b, 0, 0, 0, 0));
            add_move(glist, encode_move(p, source, target, n, 0, 0, 0, 0));
          } else {
            add_move(glist, encode_move(p, source, target, 0, 0, 0, 0, 0)); // normal move
            if (source < a6 && !(is_set(pos_occupancies[both], (target + 8)))) { // double push
              add_move(glist, encode_move(p, source, (target + 8), 0, 0, 1, 0, 0));
            }
          }
        }

        // captures
        attacks = pawn_attacks[pos_side][source];

        if ((pos_ep != none) && (attacks & (1ULL << pos_ep))) { // en passant
          add_move(glist, encode_move(p, source, pos_ep, 0, 1, 0, 1, 0));
          clear_bit(attacks, pos_ep);
        }

        attacks &= pos_occupancies[white];

        while (attacks) {
          target = first_set_bit(attacks);
          if (source > h3) { // capture and promotion
            add_move(glist, encode_move(p, source, target, q, 1, 0, 0, 0));
            add_move(glist, encode_move(p, source, target, r, 1, 0, 0, 0));
            add_move(glist, encode_move(p, source, target, b, 1, 0, 0, 0));
            add_move(glist, encode_move(p, source, target, n, 1, 0, 0, 0));
          } else { // simple capture
            add_move(glist, encode_move(p, source, target, 0, 1, 0, 0, 0));
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
            add_move(glist, encode_move(n, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(n, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(b, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(b, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(r, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(r, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(q, source, target, 0, 1, 0, 0, 0));
          } else {
            add_move(glist, encode_move(q, source, target, 0, 0, 0, 0, 0));
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
            add_move(glist, encode_move(k, e8, g8, 0, 0, 0, 0, 1));
          }
        }
      }
      if (pos_castling & bq) {
        if (!(pos_occupancies[both] & d8c8b8)) { // d8,c8 and b8 are not
                                                 // occupied
          // make sure e8 and d8 are not under attack
          if (!is_square_attacked(e8, white) &&
              !is_square_attacked(d8, white)) {
            add_move(glist, encode_move(k, e8, c8, 0, 0, 0, 0, 1));
          }
        }
      }

      source = first_set_bit(bitboard);
      attacks = king_attacks[source] & (~pos_occupancies[black]);
      while (attacks) { // loop over target squares
        target = first_set_bit(attacks);

        if (is_set(pos_occupancies[white], target)) {
          add_move(glist, encode_move(k, source, target, 0, 1, 0, 0, 0));
        } else {
          add_move(glist, encode_move(k, source, target, 0, 0, 0, 0, 0));
        }
        clear_bit(attacks, target);
      }
      break;
    }
  }

  U64 total = glist->current_index;
  return total;
}

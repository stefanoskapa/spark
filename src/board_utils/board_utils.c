#include "../attack_tables/attack_tables.h"
#include "../board/board.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "board_utils.h"


int is_square_attacked(int square, int side) { // attacking side

  if (!side) {
    return      
      (get_bishop_attacks(square, pos_occupancies[BOTH]) & (pos_pieces[B] | pos_pieces[Q])) ||
      (get_rook_attacks(square, pos_occupancies[BOTH]) & (pos_pieces[R] | pos_pieces[Q])) ||
      (knight_attacks[square] & pos_pieces[N]) ||
      (pawn_attacks[!side][square] & pos_pieces[P]) ||
      (king_attacks[square] & pos_pieces[K]);
  } else {
    return       
      (get_bishop_attacks(square, pos_occupancies[BOTH]) & (pos_pieces[b] | pos_pieces[q])) ||
      (get_rook_attacks(square, pos_occupancies[BOTH]) & (pos_pieces[r] | pos_pieces[q])) ||
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
      if (IS_SET(pos_pieces[piece], square)) {
        char_to_show = ascii_pieces[piece];
        break;
      }
    }
    printf(" %c ", char_to_show);
  }

  printf("\n\nside:        ");
  if (pos_side == WHITE)
    printf("white");
  else if (pos_side == BLACK)
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

  pos_occupancies[WHITE] = 0ULL;
  pos_occupancies[BLACK] = 0ULL;
  pos_occupancies[BOTH] = 0ULL;
  pos_moves.index = 0;
  irrev_aspects.index = 0; 
  pos_ep = none;
  pos_cap_piece = 0;

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
      pos_occupancies[BOTH] |= 1ULL << square;
      pos_occupancies[ch > 'Z' ? BLACK : WHITE] |= 1ULL << square;
      square++;
    }
  }

  // Active Color
  fen_index++;
  ch = fen_string[fen_index];
  if (ch == 'w')
    pos_side = WHITE;
  else if (ch == 'b')
    pos_side = BLACK;
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
  }


}

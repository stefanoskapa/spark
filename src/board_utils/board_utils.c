
#include "../attack_tables/attack_tables.h"
#include "../bit_utils/bit_utils.h"
#include "../board/board.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../bit_utils/bit_utils.h"
#include "board_utils.h"


int is_square_attacked(int square, int side) { // attacking side
    int side_offset = side ? 6 : 0;
    //TODO reorder with most probable attacker
    return (knight_attacks[square] & pos_pieces[N + side_offset]) ||
           (get_bishop_attacks(square, pos_occupancies[2]) & (pos_pieces[B + side_offset] | pos_pieces[Q + side_offset])) ||
           (get_rook_attacks(square, pos_occupancies[2]) & (pos_pieces[R + side_offset] | pos_pieces[Q + side_offset])) ||
           (pawn_attacks[!side][square] & pos_pieces[P + side_offset]) ||
           (king_attacks[square] & pos_pieces[K + side_offset]);

}

void show_board() {
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

void fen_error() {
    printf("Invalid FEN string\n");
    exit(1);
}

void clean_board() {
	for (size_t i = 0; i < sizeof(pos_pieces)/sizeof(pos_pieces[0]); i++)
		pos_pieces[i]=0ULL;

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

#include <stdio.h>
#include "../board/board.h"
#include "move_encoding.h"
#include <string.h>

void print_move_UCI(MOVE const move) {
    printf("%s\n", get_move_UCI(move));
}

char *get_move_UCI(MOVE const move) {
    static char str[6];
    str[0] = '\0';
    strcat(str, square_to_coordinates[GET_MOVE_SOURCE(move)]);
    strcat(str, square_to_coordinates[GET_MOVE_TARGET(move)]);
    if (GET_MOVE_PROMOTION(move)) {
        str[4] = promoted_pieces[GET_MOVE_PROMOTION(move)];
        str[5] = '\0';
    } else {
        str[4] = '\0';
    }
    return str;
}


void print_move_list(MoveList *move_list) {
    for (int i = 0; i < move_list->current_index; i++) {
        print_move_UCI(move_list->moves[i]);
    }
}

void print_move(MOVE const move) {
    char prom_piece = ascii_pieces[GET_MOVE_PROMOTION(move)];

    printf("------------------------\n");
    printf("Piece:.............%c\n", ascii_pieces[GET_MOVE_PIECE(move)]);
    printf("Source:............%s\n", square_to_coordinates[GET_MOVE_SOURCE(move)]);
    printf("Target:............%s\n", square_to_coordinates[GET_MOVE_TARGET(move)]);
    printf("Promoted piece:....%c\n", prom_piece == 'P' ? ' ' : prom_piece);
    printf("Capture:...........%s\n", GET_MOVE_CAPTURE(move) ? "true" : "false");
    printf("Double Pawn Move:..%s\n", GET_MOVE_DOUBLE(move) ? "true" : "false");
    printf("En Passant:........%s\n", GET_MOVE_EP(move) ? "true" : "false");
    printf("Castling:..........%s\n", GET_MOVE_CASTLING(move) ? "true" : "false");
    printf("Is Check:..........%s\n", GET_MOVE_CHECK(move) ? "true" : "false");
    printf("------------------------\n\n");
}

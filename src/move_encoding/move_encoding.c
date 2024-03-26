#include <stdio.h>
#include "../board/board.h"
#include "move_encoding.h"
#include <string.h>
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



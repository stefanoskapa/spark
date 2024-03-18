//
// Created by Stefanos on 7/1/2023.
//

#ifndef SPARK_BOARD_UTILS_H
#define SPARK_BOARD_UTILS_H

void show_occ_board();
int is_square_attacked(int square, int side);
void show_board();
void fen_error();
void parse_fen(char *fen_string);
void clean_board();
#endif //SPARK_BOARD_UTILS_H

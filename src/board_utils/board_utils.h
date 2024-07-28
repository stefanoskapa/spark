#ifndef SPARK_BOARD_UTILS_H
#define SPARK_BOARD_UTILS_H

void show_occ_board(void);
int is_square_attacked(int square, int side);
void show_board(void);
void fen_error(void);
void parse_fen(char *fen_string);
void clean_board(void);
#endif

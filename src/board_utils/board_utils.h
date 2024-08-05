#ifndef SPARK_BOARD_UTILS_H
#define SPARK_BOARD_UTILS_H
#include <stdbool.h>

bool is_square_attacked(int square, int side);
void parse_fen(char *fen_string);
#endif

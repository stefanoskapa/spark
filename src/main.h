#ifndef MAIN_H
#define MAIN_H
#include "board/board.h"

void divide(int depth);
void run_perft(int depth);
U64 perft(int depth);
void perft_suite(int max_depth);
#endif

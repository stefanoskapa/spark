#ifndef PERFT_H
#define PERFT_H

#include "../board/board.h"

void divide(int depth);
void run_perft(int depth);
U64 perft(int depth);
void perft_suite(int max_depth);
void benchmark(void);

#endif

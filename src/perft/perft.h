#ifndef PERFT_H
#define PERFT_H

#include "../../inc/spark.h"

void divide(int depth);
void run_perft(int depth);
BB perft(int depth);
void perft_suite(int max_depth);
void benchmark(void);

#endif

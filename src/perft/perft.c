#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../attack_tables/attack_tables.h"
#include "../board_utils/board_utils.h"
#include "../board/board.h"
#include "../generator/generator.h"
#include "../move_encoding/move_encoding.h"
#include "perft.h"

U64 perft_captures = 0;
U64 perft_eps = 0;
U64 perft_castles = 0;
U64 perft_promotions = 0;
struct perf_test {
  char title[20];
  char pos[256];
  U64 nodes[20];
  int d_count;
};

struct perf_test pos_list[] = { // results taken from chessprogramming.org/Perft_Results
  {
    "Position 1", 
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    {20ULL, 400ULL, 8902ULL, 197281ULL, 4865609ULL, 119060324ULL, 3195901860ULL, 84998978956ULL}, 8
  },
  {
    "Position 2",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    {48ULL, 2039ULL, 97862ULL, 4085603ULL, 193690690ULL, 8031647685ULL}, 6
  },
  {
    "Position 3",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
    {14ULL, 191ULL, 2812ULL, 43238ULL, 674624ULL, 11030083ULL, 178633661ULL, 3009794393ULL}, 8
  },
  {
    "Position 4",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    {6ULL, 264ULL, 9467ULL, 422333ULL, 15833292ULL, 706045033ULL}, 6
  },
  {
    "Position 5",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    {44ULL, 1486ULL, 62379ULL, 2103487ULL, 89941194ULL}, 5
  },
  {
    "Position 6",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    {46ULL, 2079ULL, 89890ULL, 3894594ULL, 164075551ULL, 6923051137ULL, 287188994746ULL}, 7
  }
};



int main(void) {
    return -1;
  printf("engine started\n\n");
  init_attack_tables();
//  benchmark();  
  perft_suite(8);
  
  return EXIT_SUCCESS;
}



void benchmark(void) {
  printf("\n--> Benchmarking (Initial Position)...\n");
  parse_fen(pos_list[0].pos);
  run_perft(6);
}

void perft_suite(int max_depth) {
  
  printf("\n--> Running perft positions\n");

  int pos_count = sizeof(pos_list) / sizeof(pos_list[0]);
  for (int i = 0; i < pos_count; i++) {
    printf("\n[ %s ]\n",pos_list[i].title);
    parse_fen(pos_list[i].pos);
    for (int j = 0; j <= max_depth && j < pos_list[i].d_count; j++) {
      printf("depth %d: ", j + 1);
      fflush(stdout);
      if (perft(j + 1) != pos_list[i].nodes[j]) {
        printf("failed :(\n");
        exit(1);
      } else {
        printf("success :)\n");
      }
    }
  }

  printf("\nAll tests passed!\n");
}

void run_perft(int depth) {

  U64 nodes;
  clock_t start, end;
  U64 time_used;

  for (int i = 1; i <= depth; i++) {

    perft_captures = 0;
    perft_eps = 0;
    perft_castles = 0;
    perft_promotions = 0;

    start = clock();
    nodes = perft(i);
    end = clock();
    time_used = ((end - start) * 1000) / CLOCKS_PER_SEC;

    printf("\nDepth %d\n", i);
    printf("=================\n");
    printf("Total moves: %llu\n", nodes);
    printf("Captures: %llu\n", perft_captures);
    printf("Eps: %llu\n", perft_eps);
    printf("Castles: %llu\n", perft_castles);
    printf("Promotions: %llu\n", perft_promotions);
    printf("Time taken: %llu ms\n", time_used);
  }
}

void divide(int depth) {

  U64 nodes = 0;

  moves move_list = generate_moves();

  for (int i = 0; i < move_list.current_index; i++) {

    nodes = 0;
    //print_move_UCI(move_list.moves[i]);
    printf("%s: ",get_move_UCI(move_list.moves[i]));
    fflush(stdout);
    make_move(move_list.moves[i]);
    nodes += perft(depth - 1);
    takeback();
    printf("%llu\n", nodes);
  }
}

U64 perft(int depth) {

  //moves move_list = {{0}, 0};
  U64 nodes = 0;

  if (depth == 0)
    return 1ULL;

  moves move_list = generate_moves();

  for (int i = 0; i < move_list.current_index; i++) {

    if (depth == 1) {
      if (GET_MOVE_CAPTURE(move_list.moves[i]))
        perft_captures++;

      if (GET_MOVE_EP(move_list.moves[i]))
        perft_eps++;

      if (GET_MOVE_CASTLING(move_list.moves[i]))
        perft_castles++;

      if (GET_MOVE_PROMOTION(move_list.moves[i]))
        perft_promotions++;
    }

    make_move(move_list.moves[i]);
    nodes += perft(depth - 1);
    takeback();
  }
  return nodes;
}



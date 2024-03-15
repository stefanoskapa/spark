#include "attack_tables/attack_tables.h"
#include "bit_utils/bit_utils.h"
#include "board_utils/board_utils.h"
#include "board/board.h"
#include "generator/generator.h"
#include "move_encoding/move_encoding.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

U64 perft_captures = 0;
U64 perft_eps = 0;
U64 perft_castles = 0;
U64 perft_promotions = 0;

int main() {

  printf("engine started\n");
  init_attack_tables();

  parse_fen(
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "); // initial
                                                                    // position

  // parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -
  // "); //position 2 (Kiwipete) parse_fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w -
  // -  "); //position 3
  // parse_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1
  // "); //position 4

  show_board();

  run_perft(6);

  return 0;
}

void run_perft(int depth) {

  U64 nodes;
  clock_t start, end;
  double time_used;

  for (int i = 1; i <= depth; i++) {

    perft_captures = 0;
    perft_eps = 0;
    perft_castles = 0;
    perft_promotions = 0;

    start = clock();
    nodes = perft(i);
    end = clock();
    time_used = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;

    printf("\nDepth %d\n", i);
    printf("=================\n");
    printf("Total moves: %llu\n", nodes);
    printf("Captures: %llu\n", perft_captures);
    printf("Eps: %llu\n", perft_eps);
    printf("Castles: %llu\n", perft_castles);
    printf("Promotions: %llu\n", perft_promotions);
    printf("Time taken: %f ms\n", time_used);
  }
}

void divide(int depth) {

  moves move_list = {{0}, 0};
  U64 nodes = 0;

  generate_moves(&move_list);

  for (int i = 0; i < move_list.current_index; i++) {

    nodes = 0;
    print_move_UCI(move_list.moves[i]);
    make_move(move_list.moves[i]);
    nodes += perft(depth - 1);
    takeback();
    printf("%llu\n\n", nodes);
  }
}

U64 perft(int depth) {

  moves move_list = {{0}, 0};
  U64 nodes = 0;

  if (depth == 0)
    return 1ULL;

  generate_moves(&move_list);

  for (int i = 0; i < move_list.current_index; i++) {

    if (depth == 1) {
      if (get_move_capture(move_list.moves[i]))
        perft_captures++;

      if (get_move_ep(move_list.moves[i]))
        perft_eps++;

      if (get_move_castling(move_list.moves[i]))
        perft_castles++;

      if (get_move_promotion(move_list.moves[i]))
        perft_promotions++;
    }

    make_move(move_list.moves[i]);
    nodes += perft(depth - 1);
    takeback();
  }
  return nodes;
}



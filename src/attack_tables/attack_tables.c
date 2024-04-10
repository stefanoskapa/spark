#include "../board/board.h"
#include "../bit_utils/bit_utils.h"
#include "attack_tables.h"

/*
	The purpose of this code is to initialize arrays that contain
	bitboards of the squares a piece can attack (from every given square)
	and provide access to those arrays via piece methods. 
  IMPORTANT: 	You must call init_attack_tables(), before using the
							other functions. 
	
		- Knights, pawns, kings 
			the board situation doesn't matter, as they will always attack 
			certain squares on their range.

		- Queens, Rooks, Bishops
			Here the attacked squares are depending on the current
			board configuration, as an attack ray may be blocked.

	Note #1: 	This is strictly about attack, not about piece movement!
	Note #2: 	For now, the attack tables are exposed through functions.
						If this has a significant overhead, maybe it is worth
						exposing the arrays directly.


	Brief explanation of how magic bitboards work. As an example let's
	take the get_bishop_attacks function. "Bishop mask" is a bitboard 
	that represents the squares the bishop can attack on an empty board,
	minus the squares on the edges.
		
	Step 1: The Total Occupancy is AND'ed with the bishop mask. This will
					result in a bitboard that shows the relevant blockers (the
					blockers that are within the bishop's attack ray). Note that
					we are not interested in a blocker at the edge of a board.

	Step 2: With that resulting bitboard, we calculate the magic index, 
					which will retrieve the board-specific attack table for the 
					bishop.

	The bishop_attacks is a 2d array which has all possible bishop attacks
	from every possible square. The index of the bishop attacks "happen" to
	be the magic indexes calculated at step 2.

  If I understood it right, any improvement in the magic numbers will
	result in a smaller memory footprint, but there should be no
	performance improvement.


  about the choice of arrays bishop_relevant_bit_count:
	A performance test proved that accessing an array by index
  is faster than performing a popcount.
*/


const U64 NOT_H = 0x7F7F7F7F7F7F7F7FULL;
const U64 NOT_GH = 0x3F3F3F3F3F3F3F3FULL;
const U64 NOT_A = 0xFEFEFEFEFEFEFEFEULL;
const U64 NOT_AB = 0xFCFCFCFCFCFCFCFCULL;

U64 pawn_attacks[2][64];
U64 knight_attacks[64];
U64 king_attacks[64];
U64 bishop_masks[64];
U64 bishop_attacks[64][512];
U64 rook_masks[64];
U64 rook_attacks[64][4096];
U64 queen_attacks[64];

U64 bishop_magic_numbers[] = {
	0x24a000b0c05860ULL , 0x316004040041C200ULL, 0x0110110A04A20404ULL, 0x1208208020204000ULL, 
	0x0002021008008009ULL, 0x0001100210006000ULL, 0x0086080402082418ULL, 0x8001050101202200ULL,
  0x000C420401061200ULL, 0x0205020828188490ULL, 0x1000420409002801ULL, 0x9000880600400000ULL, 
	0x4404020211608188ULL, 0x080061822021C000ULL, 0x0410020284200882ULL, 0x8001014202108200ULL,
  0x00C003C450820248ULL, 0x200804A109040080ULL, 0x0024050808005010ULL, 0x0000880802024002ULL, 
	0x0004105202022000ULL, 0x0100480202022000ULL, 0x0281080288095063ULL, 0x0000808602828804ULL,
  0x0004101084600810ULL, 0x1410862028080100ULL, 0x4802060211080600ULL, 0x1810040020C40008ULL, 
	0x61048C0180802004ULL, 0x820800A052020904ULL, 0x1028450002010102ULL, 0x0009020000228420ULL,
  0x210C302408182100ULL, 0x1034100400080108ULL, 0x0005140200500080ULL, 0x0000420080180080ULL, 
	0x0804090400420028ULL, 0x0010060020020089ULL, 0x0010108300048400ULL, 0x0004008602008041ULL,
  0x0242842120021800ULL, 0x00C088042A0030A0ULL, 0x4080104030040802ULL, 0x1018224200808810ULL, 
	0x400118010040040AULL, 0x0040610053014080ULL, 0x0004084A44040441ULL, 0x0804444040440202ULL,
  0x18840084100B0012ULL, 0x0102090401248000ULL, 0x1240110888240002ULL, 0x000014404202212AULL, 
	0x81400010020A1050ULL, 0x0080401002058820ULL, 0x0888101001850000ULL, 0x001090808100440DULL,
  0x4029002610040440ULL, 0x0000110411090800ULL, 0x3011090061814180ULL, 0x0240401004420210ULL, 
	0x40005590100A0200ULL, 0x460044400C084080ULL, 0xC209040818080688ULL, 0x0502102602014200ULL
};

U64 rook_magic_numbers[] = {
	0x2480004000201180L, 0x8240001002402000L, 0x0200084202201080L, 0x4480080080300004L, 
	0x0A00140A00102018L, 0x5080040080010200L, 0x1200084100860004L, 0x020004004202812BL,
  0x0328801382204000L, 0x0000806000400080L, 0x140100104C200100L, 0x0408801000808804L, 
	0x0985000800841101L, 0x0112000802011004L, 0x8514000410420D08L, 0x0040800100005080L,
  0x0040A08000804004L, 0x0040008020014092L, 0x029101001140A002L, 0x0110808028021000L, 
	0x0004808004011800L, 0x1288808004000200L, 0x0840040008100142L, 0x4002020002A40941L,
  0x00A040028006208CL, 0x0000200480400281L, 0x0000100180200080L, 0x0010010100110820L, 
	0x0020080080800400L, 0x0001000900020400L, 0x0803020080800100L, 0x1001008200006104L,
  0x0004400020801080L, 0x0028810602004020L, 0x2480801001802008L, 0x0023009001002008L, 
	0xA003000801000412L, 0x0200142008011040L, 0x0800210204001028L, 0xC0800918C200008CL,
  0x1820810642020020L, 0x4002406010024000L, 0x4051002001490010L, 0x8001000810050020L, 
	0x0488008004018008L, 0x1404400420080110L, 0x0002000804520001L, 0x090001440082000BL,
  0x240200C184210600L, 0x0118400891200080L, 0x0020001002806080L, 0x200100AA20100100L, 
	0x1120830400380080L, 0x2242040080020080L, 0x0208104842010400L, 0x0280494C01108200L,
  0x40020080C1003022L, 0x184284400130E101L, 0x100080204012000AL, 0x4003000410026009L, 
	0x0802011448201002L, 0x20C2000810010412L, 0x8440019041220804L, 0x5200002104408402L
};

const int bishop_relevant_bit_count[64] = {
	6, 5, 5, 5, 5, 5, 5, 6,
	5, 5, 5, 5, 5, 5, 5, 5,
 	5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6
};

const int rook_relevant_bit_count[64] = {
	12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};


U64 init_pawn_attacks(int square, int side) {
	U64 bitboard = 0ULL;
  U64 attacks = 0ULL;
  set_bit(bitboard, square);

  if (!side) { //white
    attacks |= bitboard >> 7 & NOT_A;
    attacks |= bitboard >> 9 & NOT_H;
  } else {
  	attacks |= bitboard << 7 & NOT_H;
  	attacks |= bitboard << 9 & NOT_A;
  }
	return attacks;
}

U64 init_knight_attacks(int square) {
  U64 bitboard = 0ULL;
  U64 attacks = 0ULL;
  set_bit(bitboard, square);

  attacks |= bitboard >> 6 & NOT_AB;
  attacks |= bitboard << 6 & NOT_GH;
  attacks |= bitboard >> 10 & NOT_GH;
  attacks |= bitboard << 10 & NOT_AB;
  attacks |= bitboard >> 15 & NOT_A;
  attacks |= bitboard << 15 & NOT_H;
  attacks |= bitboard >> 17 & NOT_H;
  attacks |= bitboard << 17 & NOT_A;

  return attacks;
}


U64 init_king_attacks(int square) {
  U64 bitboard = 0ULL;
  U64 attacks = 0ULL;
  set_bit(bitboard, square);

  attacks |= bitboard >> 1 & NOT_H;
  attacks |= bitboard << 1 & NOT_A;
  attacks |= bitboard >> 7 & NOT_A;
  attacks |= bitboard << 7 & NOT_H;
  attacks |= bitboard >> 8;
  attacks |= bitboard << 8;
  attacks |= bitboard >> 9 & NOT_H;
  attacks |= bitboard << 9 & NOT_A;

  return attacks;
}

U64 get_bishop_attacks(int square, U64 total_occupancy) {
  total_occupancy &= bishop_masks[square];
  total_occupancy *= bishop_magic_numbers[square];
  total_occupancy >>= 64 - bishop_relevant_bit_count[square];
	return bishop_attacks[square][total_occupancy];
}

U64 get_rook_attacks(int square, U64 total_occupancy) {
  total_occupancy &= rook_masks[square];
  total_occupancy *= rook_magic_numbers[square];
  total_occupancy >>= 64 - rook_relevant_bit_count[square];
  return rook_attacks[square][total_occupancy];
}

U64 get_queen_attacks(int square, U64 total_occupancy) {
  return get_bishop_attacks(square, total_occupancy) | get_rook_attacks(square, total_occupancy);
}


void init_attack_tables(void) {
  int occupancy_indexes;
  int relevant_bits_count;
  int magic_index;
  U64 attack_mask;
  U64 occupancy;

  for (int square = 0; square < 64; square++) {
   
	 //init pawns
  	pawn_attacks[white][square] = init_pawn_attacks(square, white);
    pawn_attacks[black][square] = init_pawn_attacks(square, black);
		//init knights
    knight_attacks[square] = init_knight_attacks(square);
		// init kings
    king_attacks[square] = init_king_attacks(square);

		//init bishops
    bishop_masks[square] = get_bishop_attack_mask(square);
    attack_mask = bishop_masks[square];
    relevant_bits_count = popcnt(attack_mask);
    occupancy_indexes = (1 << relevant_bits_count);
    for (int index = 0; index < occupancy_indexes; index++) {
    	occupancy = get_occupancy_variation(index,attack_mask);
      magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bit_count[square]);
 			bishop_attacks[square][magic_index] = get_bishop_attack_mask_with_blockers(square,occupancy);
    }

    //init rooks
    rook_masks[square] = get_rook_attack_mask(square);
    attack_mask = rook_masks[square];
    relevant_bits_count = popcnt(attack_mask);
    occupancy_indexes = (1 << relevant_bits_count);
    for (int index = 0; index < occupancy_indexes; index++) {
    	occupancy = get_occupancy_variation(index,attack_mask);
      magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bit_count[square]);
      rook_attacks[square][magic_index] = get_rook_attack_mask_with_blockers(square,occupancy);
    }
 

  }
  
}

U64 get_bishop_attack_mask(int square) {
    U64 attacks = 0ULL;
    int rank, file;
    int start_rank = square / 8;
    int start_file = square % 8;

    for (rank = start_rank + 1, file = start_file + 1; rank <= 6 && file <=6; rank++, file++) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank + 1, file = start_file - 1; rank <= 6 && file >= 1; rank++, file--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank - 1, file = start_file + 1; rank >= 1 && file <= 6; rank--, file++) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank - 1, file = start_file - 1; rank >= 1 && file >= 1; rank--, file--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    return attacks;

}

U64 get_bishop_attack_mask_with_blockers(int square, U64 blocker) {
    U64 attacks = 0ULL;
    int rank, file;
    int start_rank = square / 8;
    int start_file = square % 8;

    for (rank = start_rank + 1, file = start_file + 1; rank <= 7 && file <=7; rank++, file++) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank + 1, file = start_file - 1; rank <= 7 && file >= 0; rank++, file--) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank - 1, file = start_file + 1; rank >= 0 && file <= 7; rank--, file++) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank - 1, file = start_file - 1; rank >= 0 && file >= 0; rank--, file--) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    return attacks;

}

U64 get_rook_attack_mask(int square) {
    U64 attacks = 0ULL;
    int rank, file;
    int start_rank = square / 8;
    int start_file = square % 8;

    for (rank = start_rank + 1, file = start_file; rank <= 6; rank++) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank - 1, file = start_file; rank >= 1; rank--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank, file = start_file + 1; file <= 6;file++) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    for (rank = start_rank, file = start_file - 1; file >= 1; file--) {
        attacks |= (1ULL << (rank * 8 + file));
    }

    return attacks;

}

U64 get_rook_attack_mask_with_blockers(int square, U64 blocker) {
    U64 attacks = 0ULL;
    int rank, file;
    int start_rank = square / 8;
    int start_file = square % 8;

    for (rank = start_rank + 1, file = start_file; rank <= 7; rank++) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank - 1, file = start_file; rank >= 0; rank--) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank, file = start_file + 1; file <= 7;file++) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    for (rank = start_rank, file = start_file - 1; file >= 0; file--) {
        attacks |= (1ULL << (rank * 8 + file));
        if ((1ULL << (rank * 8 + file)) & blocker)
            break;
    }

    return attacks;

}

U64 get_occupancy_variation(int index, U64 attack_mask) {

    int bit_count = popcnt(attack_mask);
    U64 occupancy = 0ULL;

    for (int count = 0; count < bit_count; count++) {
        U64 square = first_set_bit(attack_mask);
        clear_bit(attack_mask,square);

        if (index & (1 << count)) {  //decide whether to include the count-th bit of the attack mask
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}

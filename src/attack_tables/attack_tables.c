#include "../board/board.h"
#include "attack_tables.h"

static BB init_king_attacks(int square);
static BB init_knight_attacks(int square);
static BB init_pawn_attacks(int square, int side);
static BB get_occupancy_variation(int index, BB attack_mask);
static BB get_rook_attacks_with_blockers(int square, BB blocker);
static BB get_bishop_attacks_with_blockers(int square, BB blocker);
static BB get_rook_attack_mask(int square);
static BB get_bishop_attack_mask(int square);

BB pawn_attacks[2][64];
BB knight_attacks[64];
BB king_attacks[64];
BB bishop_attacks[64][512];
BB rook_attacks[64][4096];
BB queen_attacks[64];

BB rook_masks[64];
BB bishop_masks[64];

static const BB bishop_magic_numbers[] = {
    0x024a000b0c05860ULL , 0x316004040041C200ULL, 0x0110110A04A20404ULL, 0x1208208020204000ULL,
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

static const BB rook_magic_numbers[] = {
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

static const int bishop_relevant_bit_count[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

static const int rook_relevant_bit_count[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

BB get_pawn_attacks(int const square, int const side) {
    return pawn_attacks[side][square];
}

BB get_knight_attacks(int const square) {
    return knight_attacks[square];
}

BB get_king_attacks(int const square) {
    return king_attacks[square];
}

BB get_bishop_attacks(int const square, BB total_occupancy) {
    total_occupancy &= bishop_masks[square];
    total_occupancy *= bishop_magic_numbers[square];
    total_occupancy >>= 64 - bishop_relevant_bit_count[square];
    return bishop_attacks[square][total_occupancy];
}

BB get_rook_attacks(int const square, BB total_occupancy) {
    total_occupancy &= rook_masks[square];
    total_occupancy *= rook_magic_numbers[square];
    total_occupancy >>= 64 - rook_relevant_bit_count[square];
    return rook_attacks[square][total_occupancy];
}

BB get_queen_attacks(int const square, BB const total_occupancy) {
    return get_bishop_attacks(square, total_occupancy) | get_rook_attacks(square, total_occupancy);
}


void init_attack_tables(void) {
    int occupancy_indexes;
    int relevant_bits_count;
    int magic_index;
    BB attack_mask;
    BB occupancy;

    for (int square = 0; square < 64; square++) {

        //init pawns
        pawn_attacks[WHITE][square] = init_pawn_attacks(square, WHITE);
        pawn_attacks[BLACK][square] = init_pawn_attacks(square, BLACK);
        //init knights
        knight_attacks[square] = init_knight_attacks(square);
        // init kings
        king_attacks[square] = init_king_attacks(square);

        //init bishops
        bishop_masks[square] = get_bishop_attack_mask(square);
        attack_mask = bishop_masks[square];
        relevant_bits_count = POPCNT(attack_mask);
        occupancy_indexes = (1 << relevant_bits_count);
        for (int index = 0; index < occupancy_indexes; index++) {
            occupancy = get_occupancy_variation(index,attack_mask);
            magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bit_count[square]);
            bishop_attacks[square][magic_index] = get_bishop_attacks_with_blockers(square,occupancy);
        }

        //init rooks
        rook_masks[square] = get_rook_attack_mask(square);
        attack_mask = rook_masks[square];
        relevant_bits_count = POPCNT(attack_mask);
        occupancy_indexes = (1 << relevant_bits_count);
        for (int index = 0; index < occupancy_indexes; index++) {
            occupancy = get_occupancy_variation(index,attack_mask);
            magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bit_count[square]);
            rook_attacks[square][magic_index] = get_rook_attacks_with_blockers(square,occupancy);
        }


    }

}

BB init_pawn_attacks(int const square, int const side) {
    BB bitboard = 0ULL;
    BB attacks = 0ULL;
    SET_BIT(bitboard, square);

    if (side == WHITE) {
        attacks |= bitboard >> 7 & NOT_A;
        attacks |= bitboard >> 9 & NOT_H;
    } else {
        attacks |= bitboard << 7 & NOT_H;
        attacks |= bitboard << 9 & NOT_A;
    }
    return attacks;
}

BB init_knight_attacks(int const square) {
    BB bitboard = 0ULL;
    BB attacks = 0ULL;
    SET_BIT(bitboard, square);

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

BB init_king_attacks(int const square) {
    BB bitboard = 0ULL;
    BB attacks = 0ULL;
    SET_BIT(bitboard, square);

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

/*
Returns a bitboard with all the squares the bishop would attack
on an empty board, excluding squares on the edge.
*/
BB get_bishop_attack_mask(int const square) {
    BB attacks = 0ULL;
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


/*
Returns a bitboard representing the squares that the bishop
can attack, given a specific blocker configuration.
*/
BB get_bishop_attacks_with_blockers(int const square, BB const blocker) {
    BB attacks = 0ULL;
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

/*
Returns a bitboard with all the squares the rook would attack
on an empty board, excluding squares on the edge.
*/
BB get_rook_attack_mask(int const square) {
    BB attacks = 0ULL;
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

/*
Returns a bitboard representing the squares that the rook
can attack, given a specific blocker configuration
*/
BB get_rook_attacks_with_blockers(int const square, BB const blocker) {
    BB attacks = 0ULL;
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

/*
index will be a number between [0, 2^n), where n is
number of set bits in the attack_mask. An attack_mask
has 2^n occupancy variations.

The number of set bits in index determine which of the
set bits in attack mask will be present in the result.
*/
static BB get_occupancy_variation(int const index, BB attack_mask) {

    int const bit_count = POPCNT(attack_mask);
    BB occupancy = 0ULL;

    for (int count = 0; count < bit_count; count++) {
        BB square = FIRST_SET_BIT(attack_mask);
        CLEAR_BIT(attack_mask, square);

        if ((1 << count) & index) {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}

#ifndef ATTACK_TABLES_H
#define ATTACK_TABLES_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>

typedef uint64_t U64;

class attackTables
{
public:
    static void initializeLeaping();
    static void initialiseBishopAttacks();
    static void initialiseRookAttacks();
    static U64 getBishopAttacks(int square, U64 blockers);
    static U64 getRookAttacks(int square, U64 blockers);
    static U64 getKnightAttacks(int square);
    static U64 getKingAttacks(int square);
    static U64 getQueenAttacks(int square, U64 blockers);
    static void printBitboard(U64 bitboard, std::ofstream &outFile);

private:
    // Bishop member functions
    static void initialiseBishopRayAndMask();
    static void determineBishopRelevantSquares(int pos, int relevantSquares[]);
    static U64 determineBishopPossibleMoves(U64 blockerBitboard, int pos);

    // Rook member functions
    static void initialiseRookRayAndMask();
    static void determineRookRelevantSquares();
    static U64 determineRookPossibleMoves(U64 blockerBitboard, int pos);

    // Leaping piece variables
    static U64 KNIGHT_ATTACKS[64];
    static U64 KING_ATTACKS[64];

    // Bishop variables
    static U64 BISHOP_RAYS[4][64];
    static U64 BISHOP_MASKS[4][64];
    static U64 BISHOP_INDEX_BITS[64];
    static U64 BISHOP_MAGIC[64];
    static U64 BISHOP_TABLE[64][512];

    // Rook variables
    static U64 ROOK_RAYS[4][64];
    static U64 ROOK_MASKS[4][64];
    static U64 ROOK_INDEX_BITS[64];
    static U64 ROOK_RELEVANT_SQUARES[12][64];
    static U64 ROOK_MAGIC[64];
    static U64 ROOK_TABLE[64][4096];
};

#endif
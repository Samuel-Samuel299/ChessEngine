#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <vector>
#include <sstream>
#include <array>
#include <chrono>
#include "move.h"

typedef uint64_t U64;
typedef std::vector<Move> MoveVec;

// Constants
const U64 FILE_A = 0x0101010101010101ULL;
const U64 FILE_B = 0x0202020202020202ULL;
const U64 FILE_C = 0x0404040404040404ULL;
const U64 FILE_D = 0x0808080808080808ULL;
const U64 FILE_E = 0x1010101010101010ULL;
const U64 FILE_F = 0x2020202020202020ULL;
const U64 FILE_G = 0x4040404040404040ULL;
const U64 FILE_H = 0x8080808080808080ULL;

const U64 RANK_1 = 0x00000000000000ffULL;
const U64 RANK_2 = 0x000000000000ff00ULL;
const U64 RANK_3 = 0x0000000000ff0000ULL;
const U64 RANK_4 = 0x00000000ff000000ULL;
const U64 RANK_5 = 0x000000ff00000000ULL;
const U64 RANK_6 = 0x0000ff0000000000ULL;
const U64 RANK_7 = 0x00ff000000000000ULL;
const U64 RANK_8 = 0xff00000000000000ULL;

// Inline functions for bit manipulation
inline void set_bit(U64 &b, int i)
{
    b |= (1ULL << i);
}

inline bool get_bit(U64 b, int i)
{
    return (b & (1ULL << i)) != 0;
}

inline void clear_bit(U64 &b, int i)
{
    b &= ~(1ULL << i);
}

inline int get_LSB(U64 b)
{
    if (b == 0)
    {
        return -1;
    }

    return __builtin_ctzll(b);
}

inline int get_MSB(U64 b)
{
    // Handle the case where b is zero to avoid undefined behavior
    if (b == 0)
        return -1;

    // __builtin_clzll counts the number of leading zeros
    return 63 - __builtin_clzll(b);
}

inline int pop_LSB(U64 &b)
{
    int i = get_LSB(b);
    b = b & (b - 1);
    return i;
}

inline U64 west(U64 b)
{
    return ((b >> 1) & ~(FILE_H));
}

inline U64 east(U64 b)
{
    return ((b << 1) & ~(FILE_A));
}

inline U64 north(U64 b)
{
    return ((b << 8));
}

inline U64 south(U64 b)
{
    return ((b >> 8));
}

inline U64 north_west(U64 b)
{
    return ((b << 7) & ~(FILE_H));
}

inline U64 north_east(U64 b)
{
    return ((b << 9) & ~(FILE_A));
}

inline U64 south_east(U64 b)
{
    return ((b >> 7) & ~(FILE_A));
}

inline U64 south_west(U64 b)
{
    return ((b >> 9) & ~(FILE_H));
}

class Board
{
public:
    Board();
    void resetBoard();
    void loadFromFEN(const std::string &fen);

    std::vector<Move> generateLegalMoves();
    bool determineIfKingIsInCheck(int kingColour, int square) const;
    void printAllInformation(std::ofstream &output) const;

    // Getters and Setters
    int getTurn() const { return turn; }
    const U64 *getBitboards() const { return bitboards; }
    std::array<bool, 4> getCastlingRights() const { return {castlingRights[0], castlingRights[1], castlingRights[2], castlingRights[3]}; }
    int getEnPassantSquare() const { return enPassantSquare; }
    int getHalfMoveClock() const { return halfMoveClock; }
    int getFullMoveNumber() const { return fullMoveNumber; }
    void setBoard(int pieceToPlay);
    void setEnPassantSquare(int square);
    void setCastlingRights(int caslingRight, bool right);

    // Function for making moves
    void applyMove(const Move &move);
    void undoMove(const Move &move, const std::array<bool, 4> &prevCastlingRights, int prevEnPassantSquare, int prevHalfMoveClock);

    int positionToIndex(const std::string &position);
    long long getTotalTimeSpentInPseudoFunction();
    long long getTotalTimeSpentInLegalFunction();
    long long getTotalTimeSpentInOtherFunction();

private:
    // Private member functions
    std::vector<Move> generatePseudoLegalMoves() const;
    void generatePawnPseudoLegalMoves(MoveVec &pawnMoves, U64 allPieces, U64 friendlyPieces, U64 enemyPieces) const;
    std::string getPieceAt(int pos) const;
    int getPieceIntAtPosition(int pos) const;
    int charToPieceIndex(char pieceChar) const;

    // Private member variables
    U64 bitboards[12];
    int turn;
    bool castlingRights[4];
    int enPassantSquare;
    int halfMoveClock;
    int fullMoveNumber;
    static std::chrono::microseconds totalTimeSpentInPseudo;
    static std::chrono::microseconds totalTimeSpentInLegal;
    static std::chrono::microseconds totalTimeSpentInOther;
};

#endif
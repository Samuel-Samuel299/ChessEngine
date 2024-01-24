#include "board.h"
#include "attackTables.h"
#include "move.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <algorithm>

std::chrono::microseconds Board::totalTimeSpentInPseudo(0);
std::chrono::microseconds Board::totalTimeSpentInLegal(0);
std::chrono::microseconds Board::totalTimeSpentInOther(0);

Board::Board()
{
    resetBoard();
}

void Board::resetBoard()
{
    // Set up the white pieces
    // The white piece information is contained within the first 6 bitboards
    bitboards[0] = 0x000000000000FF00ULL;
    bitboards[1] = 0x0000000000000042ULL;
    bitboards[2] = 0x0000000000000024ULL;
    bitboards[3] = 0x0000000000000081ULL;
    bitboards[4] = 0x0000000000000008ULL;
    bitboards[5] = 0x0000000000000010ULL;

    // Set up the black pieces
    // The black piece information is contained within the last 6 bitboards
    bitboards[6] = 0x00FF000000000000ULL;
    bitboards[7] = 0x4200000000000000ULL;
    bitboards[8] = 0x2400000000000000ULL;
    bitboards[9] = 0x8100000000000000ULL;
    bitboards[10] = 0x0800000000000000ULL;
    bitboards[11] = 0x1000000000000000ULL;

    turn = 0;
    enPassantSquare = -1;

    castlingRights[0] = true;
    castlingRights[1] = true;
    castlingRights[2] = true;
    castlingRights[3] = true;

    halfMoveClock = 0;
    fullMoveNumber = 1;
}

void Board::loadFromFEN(const std::string &fen)
{
    std::fill(std::begin(bitboards), std::end(bitboards), 0ULL);

    std::istringstream fenStream(fen);
    std::string board, turn, castling, enPassant, halfMove, fullMove;

    fenStream >> board >> turn >> castling >> enPassant >> halfMove >> fullMove;

    // Setting up the position of the pieces
    std::istringstream stream(board);
    std::string rank;
    int a = 56;
    int b = 0;
    while ((std::getline(stream, rank, '/')) && (a != -8))
    {
        b = 0;
        int size = rank.size();
        for (int i = 0; i < size; i++)
        {
            char letter = rank[i];
            if (std::isdigit(letter))
            {
                int letterDigit = letter - '0';
                b = b + letterDigit;
            }
            else
            {
                int piece = charToPieceIndex(letter);
                set_bit(bitboards[piece], (a + b));
                b++;
            }
        }
        a = a - 8;
    }

    // Setting which player it is to move
    if (turn == "w")
    {
        this->turn = 0;
    }
    else if (turn == "b")
    {
        this->turn = 1;
    }

    // Setting up the castling rights
    castlingRights[0] = false;
    castlingRights[1] = false;
    castlingRights[2] = false;
    castlingRights[3] = false;

    if (castling != "-")
    {
        for (int i = 0; i < castling.length(); i++)
        {
            char letter = castling[i];
            int index = -1;
            switch (letter)
            {
            case 'K':
                index = 0;
                break;
            case 'Q':
                index = 1;
                break;
            case 'k':
                index = 2;
                break;
            case 'q':
                index = 3;
                break;
            }
            if (index != -1)
            {
                castlingRights[index] = true;
            }
        }
    }

    // Setting up the En Passant target square
    int enIndex = positionToIndex(enPassant);
    if (enIndex == -1)
    {
        enPassantSquare = -1;
    }
    else
    {
        enPassantSquare = enIndex;
    }

    // Setting up the half move clock and the full
    int halfMoveInt = std::stoi(halfMove);
    halfMoveClock = halfMoveInt;
    int fullMoveInt = std::stoi(fullMove);
    fullMoveNumber = fullMoveInt;
}

bool compareMoves(const Move &a, const Move &b)
{
    return (a.getScore() > b.getScore());
}

std::vector<Move> Board::generateLegalMoves()
{
    std::vector<Move> legalMoves;
    std::vector<Move> pseudoLegalMoves;
    pseudoLegalMoves = generatePseudoLegalMoves();

    int kingColour = turn;
    std::array<bool, 4> prevCastlingRights = {castlingRights[0], castlingRights[1], castlingRights[2], castlingRights[3]};
    int prevEnPassantSquare = enPassantSquare;
    int prevHalfMoveClock = halfMoveClock;

    auto start = std::chrono::high_resolution_clock::now();
    // Board board = *this;
    for (int i = 0; i < pseudoLegalMoves.size(); i++)
    {

        applyMove(pseudoLegalMoves[i]);
        if (determineIfKingIsInCheck(kingColour, -1) != 1)
        {
            legalMoves.push_back(pseudoLegalMoves[i]);
        }
        undoMove(pseudoLegalMoves[i], prevCastlingRights, prevEnPassantSquare, prevHalfMoveClock);
    }

    std::sort(legalMoves.begin(), legalMoves.end(), compareMoves);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    totalTimeSpentInLegal += duration;

    return legalMoves;
}

std::vector<Move> Board::generatePseudoLegalMoves() const
{
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Move> pseudoLegalMoves;
    int index = turn * 6;

    // Also determine the friendly pieces bitboard and the enemy pieces bitboard
    U64 friendlyPieces = 0ULL;
    U64 enemyPieces = 0ULL;
    U64 allPieces;
    for (int i = 0; i < 6; i++)
    {
        friendlyPieces = (index == 0) ? (friendlyPieces | bitboards[i]) : (friendlyPieces | bitboards[i + 6]);
        enemyPieces = (index == 0) ? (enemyPieces | bitboards[i + 6]) : enemyPieces = (enemyPieces | bitboards[i]);
    }
    allPieces = friendlyPieces | enemyPieces;

    generatePawnPseudoLegalMoves(pseudoLegalMoves, allPieces, friendlyPieces, enemyPieces);
    index++;

    // Generate all of the pseudo legal knight moves
    U64 knights = bitboards[index];
    int knightPosition = pop_LSB(knights);
    while (knightPosition != -1)
    {
        U64 knightAttacks = 0ULL;
        knightAttacks = attackTables::getKnightAttacks(knightPosition);

        for (int i = 0; i < 64; i++)
        {
            U64 shift = (1ULL << i);
            if (((knightAttacks & shift) != 0) && ((friendlyPieces & shift) == 0))
            {
                int capturedPiece = 12;
                if ((enemyPieces & shift) != 0)
                {
                    std::string pieceType = getPieceAt(i);
                    char pieceTypeCharacter = pieceType[0];
                    capturedPiece = charToPieceIndex(pieceTypeCharacter);
                }
                Move move = Move(knightPosition, i, index, capturedPiece, 12, false, false);
                move.setMoveScore();
                pseudoLegalMoves.push_back(move);
            }
        }
        knightPosition = pop_LSB(knights);
    }
    index++;

    auto start1 = std::chrono::high_resolution_clock::now();
    // Generate all of the pseudo legal bishop moves
    U64 bishops = bitboards[index];
    int bishopPosition = pop_LSB(bishops);
    while (bishopPosition != -1)
    {
        U64 bishopAttacks = attackTables::getBishopAttacks(bishopPosition, allPieces);
        for (int i = 0; i < 64; i++)
        {
            U64 shift = 1ULL << i;
            if (((bishopAttacks & shift) != 0) && ((friendlyPieces & shift) == 0))
            {
                int capturedPiece = 12;
                if ((enemyPieces & shift) != 0)
                {
                    std::string pieceType = getPieceAt(i);
                    char pieceTypeCharacter = pieceType[0];
                    capturedPiece = charToPieceIndex(pieceTypeCharacter);
                }

                Move move = Move(bishopPosition, i, index, capturedPiece, 12, false, false);
                move.setMoveScore();
                pseudoLegalMoves.push_back(move);
            }
        }
        bishopPosition = pop_LSB(bishops);
    }
    index++;
    auto stop1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);
    totalTimeSpentInOther += duration1;

    // Generate all of the pseudo legal rook moves
    U64 rooks = bitboards[index];
    int rookPosition = pop_LSB(rooks);

    while (rookPosition != -1)
    {
        U64 rookAttacks = attackTables::getRookAttacks(rookPosition, allPieces);
        for (int i = 0; i < 64; i++)
        {
            U64 shift = 1ULL << i;
            if (((rookAttacks & shift) != 0) && ((friendlyPieces & shift) == 0))
            {
                int capturedPiece = 12;
                if ((enemyPieces & shift) != 0)
                {
                    std::string pieceType = getPieceAt(i);
                    char pieceTypeCharacter = pieceType[0];
                    capturedPiece = charToPieceIndex(pieceTypeCharacter);
                }

                Move move = Move(rookPosition, i, index, capturedPiece, 12, false, false);
                move.setMoveScore();
                pseudoLegalMoves.push_back(move);
            }
        }
        rookPosition = pop_LSB(rooks);
    }
    index++;

    // Generate all the of the pseudo legal queen moves
    U64 queen = bitboards[index];
    int queenPosition = pop_LSB(queen);

    if (queenPosition != -1)
    {
        U64 queenAttacks = attackTables::getQueenAttacks(queenPosition, allPieces);

        for (int i = 0; i < 64; i++)
        {
            U64 shift = 1ULL << i;
            if (((queenAttacks & shift) != 0) && ((friendlyPieces & shift) == 0))
            {
                int capturedPiece = 12;
                if ((enemyPieces & shift) != 0)
                {
                    std::string pieceType = getPieceAt(i);
                    char pieceTypeCharacter = pieceType[0];
                    capturedPiece = charToPieceIndex(pieceTypeCharacter);
                }
                Move move = Move(queenPosition, i, index, capturedPiece, 12, false, false);
                move.setMoveScore();
                pseudoLegalMoves.push_back(move);
            }
        }
    }
    index++;

    // Generate all of the king moves
    U64 king = bitboards[index];
    int kingPosition = pop_LSB(king);
    U64 kingAttacks = attackTables::getKingAttacks(kingPosition);

    for (int i = 0; i < 64; i++)
    {
        U64 shift = 1ULL << i;
        if (((shift & kingAttacks) != 0) && ((shift & friendlyPieces) == 0))
        {
            int capturedPiece = 12;
            if ((enemyPieces & shift) != 0)
            {
                std::string pieceType = getPieceAt(i);
                char pieceTypeCharacter = pieceType[0];
                capturedPiece = charToPieceIndex(pieceTypeCharacter);
            }
            Move move = Move(kingPosition, i, index, capturedPiece, 12, false, false);
            move.setMoveScore();
            pseudoLegalMoves.push_back(move);
        }
    }

    if (index == 5)
    {
        if ((castlingRights[0]) == true)
        {
            U64 piecesBetween = 0ULL;
            set_bit(piecesBetween, 5);
            set_bit(piecesBetween, 6);
            int count = 0;
            if ((piecesBetween & allPieces) == 0)
            {
                for (int i = 4; i < 7; i++)
                {
                    bool check = determineIfKingIsInCheck(0, i);
                    count = count + check;
                }
                if (count == 0)
                {
                    Move move = Move(kingPosition, 6, index, 12, 12, false, true);
                    pseudoLegalMoves.push_back(move);
                }
            }
        }

        if (castlingRights[1] == true)
        {
            U64 piecesBetween = 0ULL;
            set_bit(piecesBetween, 1);
            set_bit(piecesBetween, 2);
            set_bit(piecesBetween, 3);
            int count = 0;
            if ((piecesBetween & allPieces) == 0)
            {
                for (int i = 2; i < 5; i++)
                {
                    bool check = determineIfKingIsInCheck(0, i);
                    count = count + check;
                }
                if (count == 0)
                {
                    Move move = Move(kingPosition, 2, index, 12, 12, false, true);
                    pseudoLegalMoves.push_back(move);
                }
            }
        }
    }
    else if (index == 11)
    {
        if ((castlingRights[2]) == true)
        {
            U64 piecesBetween = 0ULL;
            set_bit(piecesBetween, 61);
            set_bit(piecesBetween, 62);
            int count = 0;
            if ((piecesBetween & allPieces) == 0)
            {
                for (int i = 60; i < 63; i++)
                {
                    bool check = determineIfKingIsInCheck(1, i);
                    count = count + check;
                }
                if (count == 0)
                {
                    Move move = Move(kingPosition, 62, index, 12, 12, false, true);
                    pseudoLegalMoves.push_back(move);
                }
            }
        }

        if ((castlingRights[3]) == true)
        {
            U64 piecesBetween = 0ULL;
            set_bit(piecesBetween, 57);
            set_bit(piecesBetween, 58);
            set_bit(piecesBetween, 59);
            int count = 0;
            if ((piecesBetween & allPieces) == 0)
            {
                for (int i = 58; i < 61; i++)
                {
                    bool check = determineIfKingIsInCheck(1, i);
                    count = count + check;
                }
                if (count == 0)
                {
                    Move move = Move(kingPosition, 58, index, 12, 12, false, true);
                    pseudoLegalMoves.push_back(move);
                }
            }
        }
    }

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    totalTimeSpentInPseudo += duration;
    return pseudoLegalMoves;
}

void Board::generatePawnPseudoLegalMoves(MoveVec &pawnMoves, U64 allPieces, U64 friendlyPieces, U64 enemyPieces) const
{
    int index = turn * 6;
    int enemyIndex = (turn == 0) ? 6 : 0;

    bool isWhite = (turn == 0) ? true : false;
    U64 startingRank = (isWhite) ? RANK_2 : RANK_7;
    U64 promotionRank = (isWhite) ? RANK_8 : RANK_1;
    U64 pawns = bitboards[index];
    U64 emptySquares = ~allPieces;

    while (pawns)
    {
        int startSquare = pop_LSB(pawns);
        U64 pawn = 0ULL;
        set_bit(pawn, startSquare);

        U64 singleMove = 0ULL;
        U64 doubleMove = 0ULL;
        singleMove = (isWhite) ? north(pawn) : south(pawn);
        singleMove = singleMove & emptySquares;

        if (singleMove)
        {
            int endSquare = get_LSB(singleMove);
            if (singleMove & ~promotionRank)
            {
                Move move = Move(startSquare, endSquare, index, 12, 12, false, false);
                move.setMoveScore();
                pawnMoves.push_back(move);
            }
            else
            {
                int queen = (isWhite) ? 4 : 10;
                Move move = Move(startSquare, endSquare, index, 12, queen, false, false);
                move.setMoveScore();
                pawnMoves.push_back(move);
            }

            if (pawn & startingRank)
            {
                doubleMove = (isWhite) ? north(singleMove) : south(singleMove);
                doubleMove = doubleMove & emptySquares;

                if (doubleMove)
                {
                    int endSquare = get_LSB(doubleMove);
                    Move move = Move(startSquare, endSquare, index, 12, 12, false, false);
                    move.setMoveScore();
                    pawnMoves.push_back(move);
                }
            }
        }

        U64 leftCapture = 0ULL;
        leftCapture = (isWhite) ? (north_west(pawn) & ~FILE_H) : (south_east(pawn) & ~FILE_A);
        leftCapture = leftCapture & enemyPieces;
        U64 rightCapture = 0ULL;
        rightCapture = (isWhite) ? (north_east(pawn) & ~FILE_A) : (south_west(pawn) & ~FILE_H);
        rightCapture = rightCapture & enemyPieces;
        U64 regularCaptures = leftCapture | rightCapture;

        while (regularCaptures)
        {
            int endSquare = pop_LSB(regularCaptures);
            U64 capture = 0ULL;
            set_bit(capture, endSquare);
            int capturedPiece = getPieceIntAtPosition(endSquare);
            if (capture & ~promotionRank)
            {
                Move move = Move(startSquare, endSquare, index, capturedPiece, 12, false, false);
                move.setMoveScore();
                pawnMoves.push_back(move);
            }
            else
            {
                int queen = (isWhite) ? 4 : 10;
                Move move = Move(startSquare, endSquare, index, capturedPiece, queen, false, false);
                move.setMoveScore();
                pawnMoves.push_back(move);
            }
        }

        if (enPassantSquare != -1)
        {
            U64 enPassantRank = (isWhite) ? RANK_5 : RANK_4;
            if (pawn & enPassantRank)
            {
                U64 enPassantBoard = 0ULL;
                set_bit(enPassantBoard, enPassantSquare);

                U64 leftCapture = 0ULL;
                leftCapture = (isWhite) ? (north_west(pawn) & ~FILE_H) : (south_east(pawn) & ~FILE_A);
                leftCapture = leftCapture & enPassantBoard;

                if (leftCapture)
                {
                    int enemyPawn = (isWhite) ? 6 : 0;
                    int endSquare = get_LSB(leftCapture);
                    Move move = Move(startSquare, endSquare, index, enemyPawn, 12, true, false);
                    move.setMoveScore();
                    pawnMoves.push_back(move);
                }

                U64 rightCapture = 0ULL;
                rightCapture = (isWhite) ? (north_east(pawn) & ~FILE_A) : (south_west(pawn) & ~FILE_H);
                rightCapture = rightCapture & enPassantBoard;

                if (rightCapture)
                {
                    int enemyPawn = (isWhite) ? 6 : 0;
                    int endsquare = get_LSB(rightCapture);
                    Move move = Move(startSquare, endsquare, index, enemyPawn, 12, true, false);
                    move.setMoveScore();
                    pawnMoves.push_back(move);
                }
            }
        }
    }
}

bool Board::determineIfKingIsInCheck(int kingColour, int square) const
{
    U64 blockers = 0ULL;
    for (int i = 0; i < 12; i++)
    {
        blockers |= bitboards[i];
    }

    int kingIndex = (kingColour == 0) ? 5 : 11;
    int pawnIndex = kingIndex + 1;
    if (kingColour == 1)
    {
        pawnIndex = 0;
    }

    U64 kingBitboard = bitboards[kingIndex];
    U64 pawnBitboard = bitboards[pawnIndex];

    if (square != -1)
    {
        clear_bit(kingBitboard, get_LSB(kingBitboard));
        set_bit(kingBitboard, square);
    }

    // Check for pawn attacks
    U64 pawnAttacks = (kingColour == 0) ? (south_west(pawnBitboard) | south_east(pawnBitboard))
                                        : (north_west(pawnBitboard) | north_east(pawnBitboard));
    if (pawnAttacks & kingBitboard)
    {
        return true;
    }

    // Check for knight attacks
    U64 knightBitboard = bitboards[pawnIndex + 1];
    while (knightBitboard)
    {
        int knightPos = pop_LSB(knightBitboard);
        if (attackTables::getKnightAttacks(knightPos) & kingBitboard)
        {
            return true;
        }
    }

    // Check for sliding piece attacks (bishops, rooks, and queens)
    for (int i = 0; i < 3; ++i)
    {
        U64 pieceBitboard = bitboards[pawnIndex + 2 + i];
        while (pieceBitboard)
        {
            int piecePos = pop_LSB(pieceBitboard);
            U64 attacks = (i == 0) ? attackTables::getBishopAttacks(piecePos, blockers) : (i == 1) ? attackTables::getRookAttacks(piecePos, blockers)
                                                                                                   : attackTables::getQueenAttacks(piecePos, blockers);
            if (attacks & kingBitboard)
            {
                return true;
            }
        }
    }

    return false;
}

void Board::applyMove(const Move &move)
{
    int startSquare = move.getStartSquare();
    int endSquare = move.getEndSquare();
    int movedPiece = move.getMovedPiece();
    int capturedPiece = move.getCapturedPiece();
    int promotionPiece = move.getPromotionPiece();
    bool isEnPassant = move.getIsEnPassant();
    bool isCastling = move.getIsCastling();
    enPassantSquare = -1;

    clear_bit(bitboards[movedPiece], startSquare);
    set_bit(bitboards[movedPiece], endSquare);
    if ((capturedPiece != 12))
    {
        clear_bit(bitboards[capturedPiece], endSquare);
    }

    if (promotionPiece != 12)
    {
        set_bit(bitboards[promotionPiece], endSquare);
        clear_bit(bitboards[movedPiece], endSquare);
    }

    else if (((movedPiece == 0) || (movedPiece == 11)) && (abs(startSquare - endSquare) == 16))
    {
        enPassantSquare = (movedPiece == 0) ? (startSquare + 8) : (startSquare - 8);
    }

    else if (isEnPassant == true)
    {
        if (movedPiece == 0)
        {
            clear_bit(bitboards[6], endSquare - 8);
        }
        else
        {
            clear_bit(bitboards[0], endSquare + 8);
        }
    }
    else if (isCastling == true)
    {
        if ((movedPiece == 5) && (endSquare == 6))

        {
            clear_bit(bitboards[3], 7);
            set_bit(bitboards[3], 5);
        }
        else if ((movedPiece == 5) && (endSquare == 2))
        {
            clear_bit(bitboards[3], 0);
            set_bit(bitboards[3], 3);
        }

        else if ((movedPiece == 11) && (endSquare == 62))
        {
            clear_bit(bitboards[9], 63);
            set_bit(bitboards[9], 61);
        }
        else if ((movedPiece == 11) && (endSquare == 58))
        {
            clear_bit(bitboards[9], 56);
            set_bit(bitboards[9], 59);
        }
    }

    if (movedPiece == 5)
    {
        castlingRights[0] = false;
        castlingRights[1] = false;
    }
    else if (movedPiece == 11)
    {
        castlingRights[2] = false;
        castlingRights[3] = false;
    }

    if ((movedPiece == 3) && (startSquare == 7))
    {
        castlingRights[0] = false;
    }
    else if ((movedPiece == 3) && (startSquare == 0))
    {
        castlingRights[1] = false;
    }
    else if ((movedPiece == 3) && (startSquare == 0))
    {
        castlingRights[2] = false;
    }
    else if ((movedPiece == 3) && (startSquare == 0))
    {
        castlingRights[3] = false;
    }

    if (turn == 0)
    {
        turn = 1;
    }
    else
    {
        fullMoveNumber++;
        turn = 0;
    }
    halfMoveClock = ((capturedPiece != 12) || (movedPiece == 6) || (movedPiece == 0)) ? 0 : (halfMoveClock + 1);
}

void Board::undoMove(const Move &move, const std::array<bool, 4> &prevCastlingRights, int prevEnPassantSquare, int prevHalfMoveClock)
{
    int startSquare = move.getStartSquare();
    int endSquare = move.getEndSquare();
    int movedPiece = move.getMovedPiece();
    int capturedPiece = move.getCapturedPiece();
    int promotionPiece = move.getPromotionPiece();
    bool isEnPassant = move.getIsEnPassant();
    bool isCastling = move.getIsCastling();

    enPassantSquare = prevEnPassantSquare;
    std::copy(prevCastlingRights.begin(), prevCastlingRights.end(), castlingRights);
    halfMoveClock = prevHalfMoveClock;

    clear_bit(bitboards[movedPiece], endSquare);
    set_bit(bitboards[movedPiece], startSquare);
    if (capturedPiece != 12)
    {
        set_bit(bitboards[capturedPiece], endSquare);
    }

    if (promotionPiece != 12)
    {
        clear_bit(bitboards[promotionPiece], endSquare);
    }
    else if (isEnPassant)
    {
        clear_bit(bitboards[capturedPiece], endSquare);
        if (movedPiece == 0)
        {
            set_bit(bitboards[capturedPiece], endSquare - 8);
        }
        else
        {
            set_bit(bitboards[capturedPiece], endSquare + 8);
        }
    }
    else if (isCastling)
    {
        if ((movedPiece == 5) && (endSquare == 6))
        {
            clear_bit(bitboards[3], 5);
            set_bit(bitboards[3], 7);
        }
        else if ((movedPiece == 5) && (endSquare == 2))
        {
            clear_bit(bitboards[3], 3);
            set_bit(bitboards[3], 0);
        }
        else if ((movedPiece == 11) && (endSquare == 62))
        {
            clear_bit(bitboards[9], 61);
            set_bit(bitboards[9], 63);
        }
        else if ((movedPiece == 11) && (endSquare == 58))
        {
            clear_bit(bitboards[9], 59);
            set_bit(bitboards[9], 56);
        }
    }

    turn = (turn == 0) ? 1 : 0;
    if (turn == 1)
    {
        fullMoveNumber++;
    }
}

void Board::setBoard(int pieceToPlay)
{
    turn = pieceToPlay;
    std::fill(std::begin(bitboards), std::end(bitboards), 0ULL);
    std::ifstream inputFile("testing/setBoard.txt");
    std::string line;

    for (int i = 7; i >= 0; i--)
    {
        getline(inputFile, line);
        for (int j = 0; j < 8; j++)
        {
            char piece = line[j * 2];
            int pieceBitoard = charToPieceIndex(piece);
            if (pieceBitoard != 12)
            {
                set_bit(bitboards[pieceBitoard], (i * 8 + j));
            }
        }
    }

    inputFile.close();
}

void Board::setEnPassantSquare(int square)
{
    enPassantSquare = square;
}

void Board::setCastlingRights(int caslingRight, bool right)
{
    castlingRights[caslingRight] = right;
}

void Board::printAllInformation(std::ofstream &output) const

{
    output << "\nThe Information of the board is as follows: \n";
    output << "En Passant Square: " << enPassantSquare << "\n";
    output << "Castling rights 0: " << castlingRights[0] << "\n";
    output << "Castling rights 1: " << castlingRights[1] << "\n";
    output << "Castling rights 2: " << castlingRights[2] << "\n";
    output << "Castling rights 3: " << castlingRights[3] << "\n";
    if (turn == 0)
    {
        output << "It is white to play and the board looks like\n";
    }
    else
    {
        output << "It is black to play and the board looks like\n";
    }
    for (int i = 7; i >= 0; i--)
    {
        output << "Row " << (i + 1) << "\t";
        for (int j = 0; j < 8; j++)
        {
            output << getPieceAt(i * 8 + j) << " ";
        }
        output << "\n";
    }
    output << " File\tA B C D E F G H\n\n";
    output << "\n\n";
}

int Board::positionToIndex(const std::string &position)
{
    if (position.length() != 2)
    {
        return -1; // Error case, invalid input
    }

    int file = position[0] - 'A'; // Convert file (A-H) to (0-7)
    int rank = position[1] - '1'; // Convert rank (1-8) to (0-7)

    return 8 * rank + file; // Calculate the index
}

int Board::charToPieceIndex(char pieceChar) const
{
    switch (pieceChar)
    {
    case 'P':
        return 0;
    case 'N':
        return 1;
    case 'B':
        return 2;
    case 'R':
        return 3;
    case 'Q':
        return 4;
    case 'K':
        return 5;
    case 'p':
        return 6;
    case 'n':
        return 7;
    case 'b':
        return 8;
    case 'r':
        return 9;
    case 'q':
        return 10;
    case 'k':
        return 11;
    default:
        return 12;
    }
}

std::string Board::getPieceAt(int pos) const
{
    int count = 0;
    int type = -1;
    std::string piece = "";
    while (type == -1 && count < 12)
    {
        if (get_bit(bitboards[count], pos) == 1)
        {
            type = count;
        }
        count++;
    }
    switch (type)
    {
    case 0:
        piece = "P";
        break;
    case 1:
        piece = "N";
        break;
    case 2:
        piece = "B";
        break;
    case 3:
        piece = "R";
        break;
    case 4:
        piece = "Q";
        break;
    case 5:
        piece = "K";
        break;
    case 6:
        piece = "p";
        break;
    case 7:
        piece = "n";
        break;
    case 8:
        piece = "b";
        break;
    case 9:
        piece = "r";
        break;
    case 10:
        piece = "q";
        break;
    case 11:
        piece = "k";
        break;
    default:
        piece = "0";
    }
    return piece;
}

int Board::getPieceIntAtPosition(int pos) const
{
    int count = 0;
    while ((count != 12))
    {
        if (get_bit(bitboards[count], pos))
        {
            return count;
        }
        count++;
    }
    return 12;
}

long long Board::getTotalTimeSpentInPseudoFunction()
{
    return totalTimeSpentInPseudo.count();
}

long long Board::getTotalTimeSpentInLegalFunction()
{
    return totalTimeSpentInLegal.count();
}

long long Board::getTotalTimeSpentInOtherFunction()
{
    return totalTimeSpentInOther.count();
}
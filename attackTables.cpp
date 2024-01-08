#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include "attackTables.h"
#include "board.h"
//
typedef uint64_t U64;

// Leaping variables
U64 attackTables::KNIGHT_ATTACKS[64];
U64 attackTables::KING_ATTACKS[64];

// Bishop variables
U64 attackTables::BISHOP_RAYS[4][64];
U64 attackTables::BISHOP_MASKS[4][64];
U64 attackTables::BISHOP_TABLE[64][512];

// Rook variables
U64 attackTables::ROOK_RAYS[4][64];
U64 attackTables::ROOK_MASKS[4][64];
U64 attackTables::ROOK_TABLE[64][4096];
U64 attackTables::ROOK_RELEVANT_SQUARES[12][64];

void attackTables::initializeLeaping()
{
    U64 knights, widthAttacks, heightAttacks, allAttacks;

    for (int i = 0; i < 64; i++)
    {
        knights = 0ULL;
        widthAttacks = 0ULL;
        heightAttacks = 0ULL;
        allAttacks = 0ULL;

        set_bit(knights, i);
        widthAttacks = (((knights >> 6) | (knights << 10)) & ~(FILE_A | FILE_B)) | (((knights << 6) | (knights >> 10)) & ~(FILE_G | FILE_H));
        heightAttacks = (((knights >> 15) | (knights << 17)) & ~(FILE_A)) | (((knights << 15) | (knights >> 17)) & ~(FILE_H));
        allAttacks = widthAttacks | heightAttacks;
        KNIGHT_ATTACKS[i] = allAttacks;
    }

    U64 king, leftAttacks, rightAttacks, verticalAttacks;

    for (int i = 0; i < 64; i++)
    {
        king = 0ULL;
        leftAttacks = 0ULL;
        rightAttacks = 0ULL;
        verticalAttacks = 0ULL;
        allAttacks = 0ULL;

        set_bit(king, i);
        leftAttacks = (((king >> 1) | (king << 7) | (king >> 9)) & ~(FILE_H));
        rightAttacks = (((king << 1) | (king >> 7) | (king << 9)) & ~(FILE_A));
        verticalAttacks = ((king << 8) | (king >> 8));
        allAttacks = leftAttacks | rightAttacks | verticalAttacks;
        KING_ATTACKS[i] = allAttacks;
    }
}

U64 attackTables::getKnightAttacks(int square)
{
    return KNIGHT_ATTACKS[square];
}

U64 attackTables::getKingAttacks(int square)
{
    return KING_ATTACKS[square];
}

void attackTables::initialiseBishopAttacks()
{
    initialiseBishopRayAndMask();

    U64 blockerBitboard, possibleMoves, key;

    for (int square = 0; square < 64; square++)
    {

        int relevantSquares[1ULL << BISHOP_INDEX_BITS[square]];
        determineBishopRelevantSquares(square, relevantSquares);
        int N = BISHOP_INDEX_BITS[square];

        for (int combination = 0; combination < (1ULL << N); combination++)
        {
            blockerBitboard = 0;
            for (int k = 0; k < N; k++)
            {
                if ((combination & (1 << k)) != 0)
                {
                    set_bit(blockerBitboard, relevantSquares[k]);
                }
            }
            possibleMoves = determineBishopPossibleMoves(blockerBitboard, square);
            key = (blockerBitboard * BISHOP_MAGIC[square]) >> (64 - BISHOP_INDEX_BITS[square]);
            BISHOP_TABLE[square][key] = possibleMoves;
            // std::ofstream outputFile("output.txt", std::ios::app);
            // outputFile << "When the bishop is on square " << square << " which looks like this\n";
            // U64 bishop = 0ULL;
            // set_bit(bishop, square);
            // printBitboard(bishop, outputFile);
            // outputFile << "And the blocker config is \n";
            // printBitboard(blockerBitboard, outputFile);
            // outputFile << "Then the bishop attacks are\n";
            // printBitboard(possibleMoves, outputFile);
            // outputFile.close();
        }
    }
}

void attackTables::initialiseBishopRayAndMask()
{
    U64 bishopBoard, rightUp, rightDown, leftUp, leftDown;

    for (int i = 0; i < 64; i++)
    {
        bishopBoard = 0ULL;
        set_bit(bishopBoard, i);
        rightUp = north_east(bishopBoard);
        rightDown = south_east(bishopBoard);
        leftUp = north_west(bishopBoard);
        leftDown = south_west(bishopBoard);

        while (rightUp != 0 || rightDown != 0 || leftUp != 0 || leftDown != 0)
        {
            if (rightUp != 0)
            {
                BISHOP_RAYS[0][i] = BISHOP_RAYS[0][i] | rightUp;
                rightUp = north_east(rightUp);
            }

            if (rightDown != 0)
            {
                BISHOP_RAYS[1][i] = BISHOP_RAYS[1][i] | rightDown;
                rightDown = south_east(rightDown);
            }

            if (leftUp != 0)
            {
                BISHOP_RAYS[2][i] = BISHOP_RAYS[2][i] | leftUp;
                leftUp = north_west(leftUp);
            }

            if (leftDown != 0)
            {
                BISHOP_RAYS[3][i] = BISHOP_RAYS[3][i] | leftDown;
                leftDown = south_west(leftDown);
            }
        }
        // remove the 1's on the border (simply for optimisation)
        BISHOP_MASKS[0][i] = BISHOP_RAYS[0][i] & ~0xFF818181818181FF;
        BISHOP_MASKS[1][i] = BISHOP_RAYS[1][i] & ~0xFF818181818181FF;
        BISHOP_MASKS[2][i] = BISHOP_RAYS[2][i] & ~0xFF818181818181FF;
        BISHOP_MASKS[3][i] = BISHOP_RAYS[3][i] & ~0xFF818181818181FF;
    }
}

void attackTables::determineBishopRelevantSquares(int pos, int relevantSquares[])
{
    int count = 0;
    for (int i = 0; i < 64; i++)
    {
        if (((BISHOP_MASKS[0][pos] | BISHOP_MASKS[1][pos] | BISHOP_MASKS[2][pos] | BISHOP_MASKS[3][pos]) & (1ULL << i)) != 0)
        {
            relevantSquares[count] = i;
            count++;
        }
    }
}

U64 attackTables::determineBishopPossibleMoves(U64 blockerBitboard, int pos)
{
    // Determine the North East positions the bishop can't move and filter them out
    U64 northEastMoves = 0ULL;
    U64 mask;
    int leastSig;
    if ((blockerBitboard & BISHOP_RAYS[0][pos]) != 0)
    {
        leastSig = get_LSB(blockerBitboard & BISHOP_RAYS[0][pos]);
        mask = 0ULL;
        set_bit(mask, leastSig);
        mask = BISHOP_RAYS[0][leastSig];
        northEastMoves = BISHOP_RAYS[0][pos] & ~mask;
    }
    else
    {
        northEastMoves = BISHOP_RAYS[0][pos];
    }

    // Determine the South East positions the bishop can't move to and filter them out
    int mostSig;
    U64 southEastMoves = 0ULL;
    if ((blockerBitboard & BISHOP_RAYS[1][pos]) != 0)
    {
        mostSig = get_MSB(blockerBitboard & BISHOP_RAYS[1][pos]);
        mask = 0ULL;
        set_bit(mask, mostSig);
        mask = BISHOP_RAYS[1][mostSig];
        southEastMoves = BISHOP_RAYS[1][pos] & ~mask;
    }
    else
    {
        southEastMoves = BISHOP_RAYS[1][pos];
    }

    U64 northWestMoves = 0ULL;
    if ((blockerBitboard & BISHOP_RAYS[2][pos]) != 0)
    {
        leastSig = get_LSB(blockerBitboard & BISHOP_RAYS[2][pos]);
        mask = 0ULL;
        set_bit(mask, leastSig);
        mask = BISHOP_RAYS[2][leastSig];
        northWestMoves = BISHOP_RAYS[2][pos] & ~mask;
    }
    else
    {
        northWestMoves = BISHOP_RAYS[2][pos];
    }

    U64 southWestMoves = 0ULL;
    if ((blockerBitboard & BISHOP_RAYS[3][pos]) != 0)
    {
        mostSig = get_MSB(blockerBitboard & BISHOP_RAYS[3][pos]);
        mask = 0ULL;
        set_bit(mask, mostSig);
        mask = BISHOP_RAYS[3][mostSig];
        southWestMoves = BISHOP_RAYS[3][pos] & ~mask;
    }
    else
    {
        southWestMoves = BISHOP_RAYS[3][pos];
    }

    return (southEastMoves | southWestMoves | northEastMoves | northWestMoves);
}

U64 attackTables::getBishopAttacks(int square, U64 blockers)
{
    // Mask blockers to only include bits on the diagonals
    blockers = blockers & (BISHOP_MASKS[0][square] | BISHOP_MASKS[1][square] | BISHOP_MASKS[2][square] | BISHOP_MASKS[3][square]);

    // Generate the key using using a multiplication and a right shift
    U64 key = (blockers * BISHOP_MAGIC[square]) >> (64 - BISHOP_INDEX_BITS[square]);

    // return BISHOP_TABLE[square][key];
    return BISHOP_TABLE[square][key];
}

void attackTables::initialiseRookAttacks()
{
    initialiseRookRayAndMask();
    determineRookRelevantSquares();

    U64 blockerBitboard, possibleMoves, key;

    for (int square = 0; square < 64; square++)
    {
        int N = ROOK_INDEX_BITS[square];
        for (int combination = 0; combination < (1ULL << N); combination++)
        {
            blockerBitboard = 0ULL;
            for (int k = 0; k < N; k++)
            {
                if ((combination & (1 << k)) != 0)
                {
                    set_bit(blockerBitboard, ROOK_RELEVANT_SQUARES[k][square]);
                }
            }

            possibleMoves = determineRookPossibleMoves(blockerBitboard, square);
            key = (blockerBitboard * ROOK_MAGIC[square]) >> (64 - ROOK_INDEX_BITS[square]);
            ROOK_TABLE[square][key] = possibleMoves;

            U64 Rook = 0ULL;
            set_bit(Rook, square);
        }
    }
}

void attackTables::initialiseRookRayAndMask()
{
    U64 rookBoard, up, right, down, left;

    for (int i = 0; i < 64; i++)
    {
        rookBoard = 0ULL;
        set_bit(rookBoard, i);
        up = north(rookBoard);
        right = east(rookBoard);
        down = south(rookBoard);
        left = west(rookBoard);

        while (up != 0 || right != 0 || down != 0 || left != 0)
        {
            if (up != 0)
            {
                ROOK_RAYS[0][i] = ROOK_RAYS[0][i] | up;
                up = north(up);
            }

            if (right != 0)
            {
                ROOK_RAYS[1][i] = ROOK_RAYS[1][i] | right;
                right = east(right);
            }

            if (down != 0)
            {
                ROOK_RAYS[2][i] = ROOK_RAYS[2][i] | down;
                down = south(down);
            }

            if (left != 0)
            {
                ROOK_RAYS[3][i] = ROOK_RAYS[3][i] | left;
                left = west(left);
            }
        }

        U64 edgeMask = 0ULL;

        // Calculate edgeMask for each direction
        if (i % 8 != 0)
        {
            edgeMask |= 1ULL << (i - i % 8);
        }
        if (i % 8 != 7)
        {
            edgeMask |= 1ULL << (i - i % 8 + 7);
        }
        if (i / 8 != 0)
        {
            edgeMask |= 1ULL << (i % 8);
        }
        if (i / 8 != 7)
        {
            edgeMask |= 1ULL << (56 + i % 8);
        }

        // Initialize ROOK_MASKS by removing the edge square in each direction
        ROOK_MASKS[0][i] = ROOK_RAYS[0][i] & ~edgeMask;
        ROOK_MASKS[1][i] = ROOK_RAYS[1][i] & ~edgeMask;
        ROOK_MASKS[2][i] = ROOK_RAYS[2][i] & ~edgeMask;
        ROOK_MASKS[3][i] = ROOK_RAYS[3][i] & ~edgeMask;

        std::ofstream outputFile("testing/testingRayAndMask.txt", std::ios::app);
        outputFile << "------------------\nWhen the rook is on square " << i << " the rays and mask is as follows\n";

        // Assuming printBitboard can take std::ofstream as an argument
        printBitboard(ROOK_MASKS[0][i] | ROOK_MASKS[1][i] | ROOK_MASKS[2][i] | ROOK_MASKS[3][i], outputFile);
        printBitboard(ROOK_RAYS[0][i] | ROOK_RAYS[1][i] | ROOK_RAYS[2][i] | ROOK_RAYS[3][i], outputFile);

        int num = ROOK_INDEX_BITS[i];
        outputFile << "The number of combinations is " << num << "\n";
        outputFile.close(); // Close the file after all writing is done
    }
}

void attackTables::determineRookRelevantSquares()
{
    std::ofstream outputSquares("testing/testingRelevantSquares.txt", std::ios::app);
    int count = 0;
    for (int j = 0; j < 64; j++)
    {
        U64 rookMask = (ROOK_MASKS[0][j] | ROOK_MASKS[1][j] | ROOK_MASKS[2][j] | ROOK_MASKS[3][j]);
        outputSquares << "-------------------------------------------------------------\nThe square is rook is on is " << j << "\nThis means that the mask will look like this\n\n";
        printBitboard(rookMask, outputSquares);
        count = 0;
        for (int i = 0; i < 64; i++)
        {
            outputSquares << "**********************************************************\nThe square we are checking is " << i << "\nIt looks like this on the board\n";
            U64 createBoard = 0ULL;
            set_bit(createBoard, i);
            printBitboard(createBoard, outputSquares);

            outputSquares << "The rookMask ANDED with the create board equals " << (rookMask & createBoard) << "\n";
            if ((rookMask & createBoard) != 0)
            {
                ROOK_RELEVANT_SQUARES[count][j] = i;
                outputSquares << "The " << count << "th relevant position is " << i << "\n";
                count++;
                outputSquares << "The count variable is then incremented " << count << "\n\n\n";
            }
        }
    }
    outputSquares.close();
}

U64 attackTables::determineRookPossibleMoves(U64 blockerBitboard, int pos)
{
    // Determine the North East positions the rook can't move and filter them out
    U64 northMoves = 0ULL;
    U64 mask;
    int leastSig;
    if ((blockerBitboard & ROOK_RAYS[0][pos]) != 0)
    {
        leastSig = get_LSB(blockerBitboard & ROOK_RAYS[0][pos]);
        mask = 0ULL;
        set_bit(mask, leastSig);
        mask = ROOK_RAYS[0][leastSig];
        northMoves = ROOK_RAYS[0][pos] & ~mask;
    }
    else
    {
        northMoves = ROOK_RAYS[0][pos];
    }

    // Determine the South East positions the bishop can't move to and filter them out
    U64 eastMoves = 0ULL;
    if ((blockerBitboard & ROOK_RAYS[1][pos]) != 0)
    {
        leastSig = get_LSB(blockerBitboard & ROOK_RAYS[1][pos]);
        mask = 0ULL;
        set_bit(mask, leastSig);
        mask = ROOK_RAYS[1][leastSig];
        eastMoves = ROOK_RAYS[1][pos] & ~mask;
    }
    else
    {
        eastMoves = ROOK_RAYS[1][pos];
    }

    U64 southMoves = 0ULL;
    int mostSig;
    if ((blockerBitboard & ROOK_RAYS[2][pos]) != 0)
    {
        mostSig = get_MSB(blockerBitboard & ROOK_RAYS[2][pos]);
        mask = 0ULL;
        set_bit(mask, mostSig);
        mask = ROOK_RAYS[2][mostSig];
        southMoves = ROOK_RAYS[2][pos] & ~mask;
    }
    else
    {
        southMoves = ROOK_RAYS[2][pos];
    }

    U64 westMoves = 0ULL;
    if ((blockerBitboard & ROOK_RAYS[3][pos]) != 0)
    {
        mostSig = get_MSB(blockerBitboard & ROOK_RAYS[3][pos]);
        mask = 0ULL;
        set_bit(mask, mostSig);
        mask = ROOK_RAYS[3][mostSig];
        westMoves = ROOK_RAYS[3][pos] & ~mask;
    }
    else
    {
        westMoves = ROOK_RAYS[3][pos];
    }

    return (eastMoves | southMoves | northMoves | westMoves);
}

U64 attackTables::getRookAttacks(int square, U64 blockers)
{
    blockers = blockers & (ROOK_MASKS[0][square] | ROOK_MASKS[1][square] | ROOK_MASKS[2][square] | ROOK_MASKS[3][square]);
    U64 key = (blockers * ROOK_MAGIC[square]) >> (64 - ROOK_INDEX_BITS[square]);
    return ROOK_TABLE[square][key];
}

void attackTables::printBitboard(U64 bitboard, std::ofstream &outFile)
{
    int position;
    for (int i = 7; i >= 0; i--)
    {
        outFile << "Row " << i + 1 << "\t";
        for (int j = 0; j <= 7; j++)
        {
            position = 8 * i + j;
            if (get_bit(bitboard, position) == true)
            {
                outFile << "1 ";
            }
            else
            {
                outFile << "0 ";
            }
        }
        outFile << "\n";
    }
    outFile << " File\tA B C D E F G H\n";
    outFile << "\n"; // Optionally, add an extra newline for separation between writes.
    // Do not close the file here. Let the caller handle it.
}

U64 attackTables::getQueenAttacks(int square, U64 blockers)
{
    U64 rookAttacks = getRookAttacks(square, blockers);
    U64 bishopAttacks = getBishopAttacks(square, blockers);
    return (rookAttacks | bishopAttacks);
}
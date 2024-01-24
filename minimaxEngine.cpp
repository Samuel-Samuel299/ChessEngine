#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include "minimaxEngine.h"
#include "move.h"
#include "board.h"
#include "attackTables.h"

MinimaxEngine::MinimaxEngine(int depth)
{
    engineDepth = depth;
}

Move MinimaxEngine::findBestMove(Board &board, int depth)
{
    int turn = board.getTurn();
    std::array<bool, 4> castlingRights = board.getCastlingRights();
    int enPassantSquare = board.getEnPassantSquare();
    int halfMoveClock = board.getHalfMoveClock();
    bool maximizingPlayer = ((board.getTurn()) == 0) ? false : true;
    double bestValue = (turn == 0) ? -INFINITY : INFINITY;
    std::vector<Move> legalMoves = board.generateLegalMoves();

    Move bestMove;

    if (legalMoves.empty() == true)
    {
        Move move = Move(-1, -1, -1, -1, -1, false, false);
        return move;
    }

    for (int i = 0; i < legalMoves.size(); i++)
    {
        Move move = legalMoves[i];
        board.applyMove(move);
        double value = minimax(board, depth - 1, maximizingPlayer, -INFINITY, INFINITY);
        board.undoMove(move, castlingRights, enPassantSquare, halfMoveClock);

        if (turn == 0)
        {
            if (value > bestValue)
            {
                bestValue = value;
                bestMove = move;
            }
        }
        else
        {
            if (value < bestValue)
            {
                bestValue = value;
                bestMove = move;
            }
        }
    }
    return bestMove;
}

double MinimaxEngine::minimax(Board &board, int depth, bool maximizingPlayer, double alpha, double beta)
{
    // Generate a vector containing all of the legal moves
    std::vector<Move> legalMoves = board.generateLegalMoves();
    int number = legalMoves.size();

    // Save the state information of the board
    std::array<bool, 4> castlingRights = board.getCastlingRights();
    int enPassantSquare = board.getEnPassantSquare();
    int halfMoveClock = board.getHalfMoveClock();

    if ((depth == 0) || (legalMoves.empty() == true))
    {

        double val = evaluateBoard(board);
        return val;
    }

    if (maximizingPlayer)
    {

        double maxVal = -INFINITY;
        for (int i = 0; i < number; i++)
        {
            Move move = legalMoves[i];
            board.applyMove(move);
            double value = minimax(board, depth - 1, false, alpha, beta);
            board.undoMove(move, castlingRights, enPassantSquare, halfMoveClock);
            maxVal = std::max(maxVal, value);
            alpha = std::max(alpha, value);
            if (beta <= alpha)
            {
                break;
            }
        }
        return maxVal;
    }
    else
    {
        double minVal = INFINITY;
        for (int i = 0; i < number; i++)
        {
            Move move = legalMoves[i];
            board.applyMove(move);
            double value = minimax(board, depth - 1, true, alpha, beta);
            board.undoMove(move, castlingRights, enPassantSquare, halfMoveClock);
            minVal = std::min(minVal, value);
            beta = std::min(beta, value);
            if (beta <= alpha)
            {
                break;
            }
        }
        return minVal;
    }
}

double MinimaxEngine::evaluateBoard(Board &board)
{
    double overallScore = 0;
    MoveVec legalMoves = board.generateLegalMoves();

    if ((board.getTurn() == 0) && (legalMoves.empty() == 1))
    {
        overallScore = (board.determineIfKingIsInCheck(0, -1)) ? -INFINITY : 0;
        return overallScore;
    }
    else if ((board.getTurn() == 1) && (legalMoves.empty() == 1))
    {
        overallScore = (board.determineIfKingIsInCheck(1, -1)) ? INFINITY : 0;
        return overallScore;
    }

    double material = evaluateMaterial(board);
    overallScore = material;
    return overallScore;
}

double MinimaxEngine::evaluateMaterial(const Board &board)
{
    const U64 *bitboards = board.getBitboards();
    double materialSum = 0;
    for (int i = 0; i < 12; i++)
    {
        int pieceCount = 0;
        pieceCount = determineNumberOfSpecificPieces(i, bitboards);
        materialSum = materialSum + (pieceCount * pieceValues[i]);
    }
    return materialSum;
}

int MinimaxEngine::determineNumberOfSpecificPieces(int index, const U64 *bitboards)
{
    U64 pieceBitboard = bitboards[index];
    int i = 0;
    int position = pop_LSB(pieceBitboard);
    while (position != -1)
    {
        position = pop_LSB(pieceBitboard);
        i++;
    }
    return i;
}
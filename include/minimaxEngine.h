#ifndef MINIMAX_ENGINE_H
#define MINIMAX_ENGINE_H

#include "move.h"
#include "board.h"

const double pieceValues[12] = {1, 3, 3, 5, 9, 0, -1, -3, -3, -5, -9, 0};

class MinimaxEngine
{
public:
    MinimaxEngine(int depth = 5);
    Move findBestMove(Board &board, int depth);

private:
    double minimax(Board &board, int depth, bool maximizingPlayer, double alpha, double beta);
    double evaluateBoard(Board &board);
    double evaluateMaterial(const Board &board);
    int determineNumberOfSpecificPieces(int index, const U64 *bitboards);
    int engineDepth;
};

#endif
#ifndef MINIMAX_ENGINE_H
#define MINIMAX_ENGINE_H

#include "move.h"
#include "board.h"

class MinimaxEngine
{
public:
    MinimaxEngine(/* parameters like search depth, etc. */);

    Move findBestMove(const Board &board, int depth);

private:
    int minimax(const Board &board, int depth, bool maximizingPlayer);
    int evaluateBoard(const Board &board);
};

#endif
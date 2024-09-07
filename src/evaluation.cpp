#include "evaluation.h"
#include "move.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>

const double Evaluation::pieceValues[12] = {1, 3, 3, 5, 9, 0, -1, -3, -3, -5, -9, 0};

Evaluation::Evaluation(const EvaluationParameters &params) : evalParams(params)
{
}

double Evaluation::evaluateBoard(Board &board) const
{

    std::vector<Move> legalMoves;
    legalMoves = board.generateLegalMoves();
    if ((board.getTurn() == 0) && (legalMoves.empty() == 1))
    {

        if (board.determineIfKingIsInCheck(0, -1) == true)
        {

            std::ofstream output("output.txt", std::ios::app);
            board.printAllInformation(output);
            output.close();
            return -INFINITY;
        }
        else
        {
            return 0;
        }
    }
    else if ((board.getTurn() == 1) && (legalMoves.empty() == 1))
    {
        if (board.determineIfKingIsInCheck(1, -1) == true)
        {
            return INFINITY;
        }
        else
        {
            return 0;
        }
    }
    double overallScore = 0;
    double matEvaluation = materialEvaluation(board);
    overallScore = matEvaluation * evalParams.materialWeight;
    return overallScore;
}

double Evaluation::materialEvaluation(const Board &board) const
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

int Evaluation::determineNumberOfSpecificPieces(int index, const U64 *bitboards) const
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
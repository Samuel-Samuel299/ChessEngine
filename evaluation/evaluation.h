#ifndef EVALUATION_H
#define EVALUATION_H

#include "board.h"

struct EvaluationParameters
{
    double materialWeight;
    double kingSafety;
    EvaluationParameters(
        double material = 1.0,
        double king = 1.0) : materialWeight(material), kingSafety(king)
    {
    }
};

class Evaluation
{
public:
    Evaluation(const EvaluationParameters &params);
    double evaluateBoard(Board &board) const;

private:
    double materialEvaluation(const Board &board) const;
    int determineNumberOfSpecificPieces(int index, const U64 *bitboards) const;
    EvaluationParameters evalParams;
    static const double pieceValues[12];
};

#endif
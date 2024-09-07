#ifndef MOVE_H
#define MOVE_H

#include <string>

class Move
{
public:
    Move();
    Move(int startSquare, int endSquare, int movedPiece, int capturedPiece, int promotionPiece = 12, bool isEnPassant = false, bool isCastling = false, double score = 0.0);

    int getStartSquare() const { return startSquare; }
    int getEndSquare() const { return endSquare; }
    int getMovedPiece() const { return movedPiece; }
    int getCapturedPiece() const { return capturedPiece; }
    int getPromotionPiece() const { return promotionPiece; }
    bool getIsEnPassant() const { return isEnPassant; }
    bool getIsCastling() const { return isCastling; }
    double getScore() const { return score; }
    std::string printMove() const;
    void setMoveScore();

private:
    int startSquare;
    int endSquare;
    int movedPiece;
    int capturedPiece;
    int promotionPiece;
    bool isEnPassant;
    bool isCastling;
    double score;
};

#endif
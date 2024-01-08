#ifndef MOVE_H
#define MOVE_H

class Move
{
public:
    Move();
    Move(int startSquare, int endSquare, int movedPiece, int capturedPiece, int promotionPiece = 0, bool isEnPassant = false, bool isCastling = false);

    int getStartSquare() const;
    int getEndSquare() const;
    int getMovedPiece() const;
    int getCapturedPiece() const;
    int getPromotionPiece() const;
    bool getIsEnPassant() const;
    bool getIsCastling() const;

private:
    int startSquare;
    int endSquare;
    int movedPiece;
    int capturedPiece;
    int promotionPiece;
    bool isEnPassant;
    bool isCastling;
};

#endif
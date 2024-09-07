#include <string>
#include <iostream>
#include <sstream>
#include "move.h"
#include "minimaxEngine.h"

Move::Move()
{
}

Move::Move(int startSquare, int endSquare, int movedPiece, int capturedPiece, int promotionPiece, bool isEnPassant, bool isCastling, double score)
{
    this->startSquare = startSquare;
    this->endSquare = endSquare;
    this->movedPiece = movedPiece;
    this->capturedPiece = capturedPiece;
    this->promotionPiece = promotionPiece;
    this->isEnPassant = isEnPassant;
    this->isCastling = isCastling;
    this->score = score;
}

std::string Move::printMove() const
{
    std::ostringstream oss;
    oss << "Starting Square: " << startSquare
        << "\nEnd Square: " << endSquare
        << "\nMoved Piece: " << movedPiece
        << "\nCaptured Piece: " << capturedPiece
        << "\nPromotion Piece: " << promotionPiece
        << "\nEn Passant: " << isEnPassant
        << "\nCastling: " << isCastling << "\n";

    return oss.str();
}

void Move::setMoveScore()
{
    double capture = 0;

    if (capturedPiece != 12)
    {
        if (abs(pieceValues[movedPiece]) < abs(pieceValues[capturedPiece]))
        {
            capture = abs(pieceValues[movedPiece]) - abs(pieceValues[capturedPiece]);
        }
        else if (abs(pieceValues[movedPiece]) == abs(pieceValues[capturedPiece]))
        {
            capture = 1;
        }
    }

    score = capture;
}

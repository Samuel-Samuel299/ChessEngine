#include <string>
#include <iostream>
#include <sstream>
#include "move.h"

Move::Move()
{
}

Move::Move(int startSquare, int endSquare, int movedPiece, int capturedPiece, int promotionPiece, bool isEnPassant, bool isCastling)
{
    this->startSquare = startSquare;
    this->endSquare = endSquare;
    this->movedPiece = movedPiece;
    this->capturedPiece = capturedPiece;
    this->promotionPiece = promotionPiece;
    this->isEnPassant = isEnPassant;
    this->isCastling = isCastling;
}

int Move::getStartSquare() const
{
    return startSquare;
}

int Move::getEndSquare() const
{
    return endSquare;
}

int Move::getMovedPiece() const
{
    return movedPiece;
}

int Move::getCapturedPiece() const
{
    return capturedPiece;
}

int Move::getPromotionPiece() const
{
    return promotionPiece;
}

bool Move::getIsEnPassant() const
{
    return isEnPassant;
}

bool Move::getIsCastling() const
{
    return isCastling;
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
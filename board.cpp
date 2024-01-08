#include "board.h"
#include "attackTables.h"
#include <iostream>
#include <string>

Board::Board()
{
    resetBoard();
}

void Board::resetBoard()
{
    // Set up the white pieces
    // The white piece information is contained within the first 6 bitboards
    bitboards[0] = 0x000000000000FF00ULL;
    bitboards[1] = 0x0000000000000042ULL;
    bitboards[2] = 0x0000000000000024ULL;
    bitboards[3] = 0x0000000000000081ULL;
    bitboards[4] = 0x0000000000000008ULL;
    bitboards[5] = 0x0000000000000010ULL;

    // Set up the black pieces
    // The black piece information is contained within the last 6 bitboards
    bitboards[6] = 0x00FF000000000000ULL;
    bitboards[7] = 0x4200000000000000ULL;
    bitboards[8] = 0x2400000000000000ULL;
    bitboards[9] = 0x8100000000000000ULL;
    bitboards[10] = 0x0800000000000000ULL;
    bitboards[11] = 0x1000000000000000ULL;

    turn = 0;
    castlingRights[0] = true;
    castlingRights[1] = true;
    castlingRights[2] = true;
    castlingRights[3] = true;
}

std::vector<Move> Board::generateLegalMoves() const
{
    // The legal moves is every possible move taking into consideration if the king is in check
    // First create a vector to store all the legal moves
    std::vector<Move> legalMoves;

    bool isKingInCheck = determineIfKingIsInCheck();

    return legalMoves;
}

bool Board::determineIfKingIsInCheck() const
{
    // If it is white's turn then check if any of the black pieces can move onto the king's square
    U64 attackSquares = 0ULL;
    U64 blockers = 0ULL;
    int a, b;
    for (int i = 0; i < 12; i++)
    {
        blockers = blockers | bitboards[i];
    }

    if (turn == 0)
    {
        std::cout << "It is White's turn" << std::endl;
        a = 5;
        b = 6;
    }
    else
    {
        std::cout << "It is Black's turn" << std::endl;
        a = 11;
        b = 0;
    }
    U64 king = bitboards[a];
    std::cout << "The king is in position " << get_LSB(king) << std::endl;

    U64 pawns = bitboards[b];
    if (turn == 0)
    {
        attackSquares = attackSquares | (north_west(pawns) | north_east(pawns));
    }
    else
    {
        attackSquares = attackSquares | (south_west(pawns) | south_east(pawns));
    }
    b++;

    U64 knights = bitboards[b];
    int knightPos = pop_LSB(knights);
    while (knightPos != -1)
    {
        attackSquares = attackSquares | attackTables::getKnightAttacks(knightPos);
        knightPos = pop_LSB(knights);
    }
    b++;

    U64 bishops = bitboards[b];
    int bishopPos = pop_LSB(bishops);
    std::cout << "Bishop 1 is in position " << bishopPos << std::endl;
    while (bishopPos != -1)
    {

        attackSquares = attackSquares | attackTables::getBishopAttacks(bishopPos, blockers);
        std::ofstream output("output.txt", std::ios::app);
        output << "The bishop is in position " << bishopPos << "\n";
        output << "In this position the attacks look like\n";
        attackTables::printBitboard(attackTables::getBishopAttacks(bishopPos, blockers), output);
        attackTables::printBitboard(blockers, output);
        bishopPos = pop_LSB(bishops);
        output.close();
    }
    b++;

    U64 rooks = bitboards[b];
    int rookPos = pop_LSB(rooks);
    while (rookPos != -1)
    {
        attackSquares = attackSquares | attackTables::getRookAttacks(rookPos, blockers);
        rookPos = pop_LSB(rooks);
    }
    b++;

    U64 queen = bitboards[b];
    int queenPos = pop_LSB(queen);

    if (queenPos != -1)
    {
        attackSquares = attackSquares | attackTables::getQueenAttacks(queenPos, blockers);
    }
    std::ofstream output("output.txt", std::ios::app);
    output << "Finally we get to all of the attacks combined\nThis looks like\n";
    attackTables::printBitboard(attackSquares, output);
    output << "The king bitboard looks like\n";
    attackTables::printBitboard(king, output);
    output.close();

    if ((attackSquares & king) != 0)
    {
        return true;
    }

    return false;
}

void Board::printBoard() const
{

    std::ofstream outputFile("testing/printBoard.txt", std::ios::app);

    for (int i = 7; i >= 0; i--)
    {
        outputFile << "Row " << (i + 1) << "\t";
        for (int j = 0; j < 8; j++)
        {
            outputFile << getPieceAt(i * 8 + j) << " ";
        }
        outputFile << "\n";
    }
    outputFile << " File\tA B C D E F G H\n\n";
    outputFile.close();
}

std::string Board::getPieceAt(int pos) const

{

    int count = 0;
    int type = -1;
    std::string piece = "";

    while (type == -1 && count < 12)
    {
        if (get_bit(bitboards[count], pos) == 1)
        {
            type = count;
        }
        count++;
    }

    switch (type)
    {
    case 0:
        piece = "P";
        break;
    case 1:
        piece = "H";
        break;
    case 2:
        piece = "B";
        break;
    case 3:
        piece = "R";
        break;
    case 4:
        piece = "Q";
        break;
    case 5:
        piece = "K";
        break;
    case 6:
        piece = "p";
        break;
    case 7:
        piece = "h";
        break;
    case 8:
        piece = "b";
        break;
    case 9:
        piece = "r";
        break;
    case 10:
        piece = "q";
        break;
    case 11:
        piece = "k";
        break;
    default:
        piece = "0";
    }

    return piece;
}

void Board::setBoard(int turn)
{
    this->turn = turn;
    std::fill(std::begin(bitboards), std::end(bitboards), 0ULL);
    std::ifstream inputFile("testing/setBoard.txt");
    std::string line;

    for (int i = 7; i >= 0; i--)
    {
        getline(inputFile, line);
        for (int j = 0; j < 8; j++)
        {
            char piece = line[j * 2];
            int pieceBitoard = charToPieceIndex(piece);
            if (pieceBitoard != -1)
            {
                set_bit(bitboards[pieceBitoard], (i * 8 + j));
            }
        }
    }

    inputFile.close();
}

int Board::charToPieceIndex(char pieceChar)
{
    switch (pieceChar)
    {
    case 'P':
        return 0;
    case 'H':
        return 1;
    case 'B':
        return 2;
    case 'R':
        return 3;
    case 'Q':
        return 4;
    case 'K':
        return 5;
    case 'p':
        return 6;
    case 'h':
        return 7;
    case 'b':
        return 8;
    case 'r':
        return 9;
    case 'q':
        return 10;
    case 'k':
        return 11;
    default:
        return -1;
    }
}

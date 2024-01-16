#include "board.h"
#include "attackTables.h"
#include "move.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>

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
    enPassantSquare = -1;

    castlingRights[0] = true;
    castlingRights[1] = true;
    castlingRights[2] = true;
    castlingRights[3] = true;

    halfMoveClock = 0;
    fullMoveNumber = 1;
}

Board Board::createWithModifiction() const
{
    Board modifiedBoard = *this;
    return modifiedBoard;
}

void Board::loadFromFEN(const std::string &fen)
{
    std::fill(std::begin(bitboards), std::end(bitboards), 0ULL);

    std::istringstream fenStream(fen);
    std::string board, turn, castling, enPassant, halfMove, fullMove;

    fenStream >> board >> turn >> castling >> enPassant >> halfMove >> fullMove;

    // Setting up the position of the pieces
    std::istringstream stream(board);
    std::string rank;
    int a = 56;
    int b = 0;
    while ((std::getline(stream, rank, '/')) && (a != -8))
    {
        b = 0;
        int size = rank.size();
        for (int i = 0; i < size; i++)
        {
            char letter = rank[i];
            if (std::isdigit(letter))
            {
                int letterDigit = letter - '0';
                b = b + letterDigit;
            }
            else
            {
                int piece = charToPieceIndex(letter);
                set_bit(bitboards[piece], (a + b));
                b++;
            }
        }
        a = a - 8;
    }

    // Setting which player it is to move
    if (turn == "w")
    {
        this->turn = 0;
    }
    else if (turn == "b")
    {
        this->turn = 1;
    }

    // Setting up the castling rights
    castlingRights[0] = false;
    castlingRights[1] = false;
    castlingRights[2] = false;
    castlingRights[3] = false;

    if (castling != "-")
    {
        for (int i = 0; i < castling.length(); i++)
        {
            char letter = castling[i];
            int index = -1;
            switch (letter)
            {
            case 'K':
                index = 0;
                break;
            case 'Q':
                index = 1;
                break;
            case 'k':
                index = 2;
                break;
            case 'q':
                index = 3;
                break;
            }
            if (index != -1)
            {
                castlingRights[index] = true;
            }
        }
    }

    // Setting up the En Passant target square
    int enIndex = positionToIndex(enPassant);
    if (enIndex == -1)
    {
        enPassantSquare = -1;
    }
    else
    {
        enPassantSquare = enIndex;
    }

    // Setting up the half move clock and the full
    int halfMoveInt = std::stoi(halfMove);
    halfMoveClock = halfMoveInt;
    int fullMoveInt = std::stoi(fullMove);
    fullMoveNumber = fullMoveInt;
}

std::vector<Move> Board::generateLegalMoves() const
{

    std::vector<Move> legalMoves;
    std::vector<Move> pseudoLegalMoves;
    int kingColour = turn;
    pseudoLegalMoves = generatePseudoLegalMoves();
    int num = pseudoLegalMoves.size();

    for (int i = 0; i < num; i++)
    {
        Move move = pseudoLegalMoves[i];
        Board board = this->createWithModifiction();
        board.applyMove(move);
        bool isCheck = board.determineIfKingIsInCheck(kingColour, -1);
        if (isCheck != 1)
        {
            legalMoves.push_back(move);
        }
    }

    return legalMoves;
}

std::vector<Move> Board::generatePseudoLegalMoves() const
{
    std::vector<Move> pseudoLegalMoves;
    int index = turn * 6;

    // Determine the pawns starting, promotion and en passant ranks
    // Also determine the friendly pieces bitboard and the enemy pieces bitboard
    U64 friendlyPieces = 0ULL;
    U64 enemyPieces = 0ULL;
    U64 allPieces;

    for (int i = 0; i < 6; i++)
    {
        // Set the variables depending on whether it is black or white to move
        if (index == 0)
        {
            friendlyPieces = (friendlyPieces | bitboards[i]);
            enemyPieces = (enemyPieces | bitboards[i + 6]);
        }
        else
        {
            friendlyPieces = (friendlyPieces | bitboards[i + 6]);
            enemyPieces = (enemyPieces | bitboards[i]);
        }
    }

    allPieces = friendlyPieces | enemyPieces;

    pseudoLegalMoves = generatePawnPseudoLegalMoves(index, allPieces, friendlyPieces, enemyPieces);
    index++;

    // std::ofstream output("output.txt", std::ios::app);
    // output << "######################################################################\n";
    // output << "We have determined all of the pseudo legal pawn moves and now it is time to move onto the knight moves\n";
    // output << "The value of the index variable is " << index << "\n";

    // Generate all of the pseudo legal knight moves
    U64 knights = bitboards[index];
    // output << "The knights bitboard looks like\n";
    // attackTables::printBitboard(knights, output);

    int knightPosition = pop_LSB(knights);
    while (knightPosition != -1)
    {
        // output << "\nThe position of the knight is " << knightPosition;
        // output << "\nWhen the knight is in this position it has the following attack  bitboard\n";
        U64 knightAttacks = 0ULL;
        knightAttacks = attackTables::getKnightAttacks(knightPosition);
        // attackTables::printBitboard(knightAttacks, output);
        for (int i = 0; i < 64; i++)
        {
            U64 shift = (1ULL << i);
            if (((knightAttacks & shift) != 0) && ((friendlyPieces & shift) == 0))
            {
                // output << "The knight attacks the square " << i << " and there is no friendly piece on that square\n";
                int capturedPiece = 12;
                if ((enemyPieces & shift) != 0)
                {
                    std::string pieceType = getPieceAt(i);
                    char pieceTypeCharacter = pieceType[0];
                    capturedPiece = charToPieceIndex(pieceTypeCharacter);
                }
                // output << "The piece that is captured is " << capturedPiece << "\nPlease note that 12 means attack square was empty and has no enemy piece\n";
                // output << "We generate the following move\n";
                Move move = Move(knightPosition, i, index, capturedPiece, 12, false, false);
                // output << "MOVE\n"
                //        << move.printMove() << "\n\n";
                pseudoLegalMoves.push_back(move);
            }
        }
        knightPosition = pop_LSB(knights);
        // output << "The new knight position is " << knightPosition << "\n";
    }
    index++;

    // Generate all of the pseudo legal bishop moves
    U64 bishops = bitboards[index];
    // output << "*************************************************\n";
    // output << "The bishops bitboard looks like\n";
    // attackTables::printBitboard(bishops, output);
    int bishopPosition = pop_LSB(bishops);
    while (bishopPosition != -1)
    {
        // output << "\n\nThe bishop is on square " << bishopPosition << "\n";
        // output << "When the bishop is on this square is has the following attack bitboard \n";

        U64 bishopAttacks = attackTables::getBishopAttacks(bishopPosition, allPieces);
        // attackTables::printBitboard(bishopAttacks, output);

        for (int i = 0; i < 64; i++)
        {
            U64 shift = 1ULL << i;
            if (((bishopAttacks & shift) != 0) && ((friendlyPieces & shift) == 0))
            {
                // output << "The bishop attacks the square " << i << " and there are no friendly pieces on this square\n";
                int capturedPiece = 12;
                if ((enemyPieces & shift) != 0)
                {
                    std::string pieceType = getPieceAt(i);
                    char pieceTypeCharacter = pieceType[0];
                    capturedPiece = charToPieceIndex(pieceTypeCharacter);
                }

                Move move = Move(bishopPosition, i, index, capturedPiece, 12, false, false);
                // output << "We therefore create the move\n";
                // output << move.printMove() << "\n";
                pseudoLegalMoves.push_back(move);
            }
        }
        bishopPosition = pop_LSB(bishops);
    }
    index++;

    // Generate all of the pseudo legal rook moves
    U64 rooks = bitboards[index];
    // output << "The rook bitboard looks like\n";
    // attackTables::printBitboard(rooks, output);
    int rookPosition = pop_LSB(rooks);

    while (rookPosition != -1)
    {
        // output << "The rook is on square " << rookPosition << "\n";
        // output << "When the rook is on this square is has the following attack bitboard\n";
        U64 rookAttacks = attackTables::getRookAttacks(rookPosition, allPieces);
        // attackTables::printBitboard(rookAttacks, output);
        for (int i = 0; i < 64; i++)
        {
            U64 shift = 1ULL << i;
            if (((rookAttacks & shift) != 0) && ((friendlyPieces & shift) == 0))
            {
                // output << "The bishop attacks the square " << i << " and there is no friendly piece on that  square\n";
                int capturedPiece = 12;
                if ((enemyPieces & shift) != 0)
                {
                    std::string pieceType = getPieceAt(i);
                    char pieceTypeCharacter = pieceType[0];
                    capturedPiece = charToPieceIndex(pieceTypeCharacter);
                }

                Move move = Move(rookPosition, i, index, capturedPiece, 12, false, false);
                // output << "We therefore create the move \n"
                //        << move.printMove() << "\n";
                pseudoLegalMoves.push_back(move);
            }
        }
        rookPosition = pop_LSB(rooks);
        // output << "The next rook is in position " << rookPosition << "\n";
    }
    index++;

    // Generate all the of the pseudo legal queen moves
    U64 queen = bitboards[index];
    // output << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\nThe queen bitboard looks like\n";
    // attackTables::printBitboard(queen, output);
    int queenPosition = pop_LSB(queen);
    // output << "The queen is in position " << queenPosition << "\n";

    if (queenPosition != -1)
    {
        U64 queenAttacks = attackTables::getQueenAttacks(queenPosition, allPieces);
        // output << "When the queen is in this position it has the following attack bitboard\n";
        // attackTables::printBitboard(queenAttacks, output);

        for (int i = 0; i < 64; i++)
        {
            U64 shift = 1ULL << i;
            if (((queenAttacks & shift) != 0) && ((friendlyPieces & shift) == 0))
            {
                // output << "The queen attacks square " << i << " and there is no friendly piece on this square\n";
                int capturedPiece = 12;
                if ((enemyPieces & shift) != 0)
                {
                    std::string pieceType = getPieceAt(i);
                    char pieceTypeCharacter = pieceType[0];
                    capturedPiece = charToPieceIndex(pieceTypeCharacter);
                }

                Move move = Move(queenPosition, i, index, capturedPiece, 12, false, false);
                // output << "We there create the move \n"
                //        << move.printMove() << "\n";
                pseudoLegalMoves.push_back(move);
            }
        }
    }
    index++;

    // Generate all of the king moves
    U64 king = bitboards[index];
    // output << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\nThe king bitboard looks like\n";
    // attackTables::printBitboard(king, output);
    int kingPosition = pop_LSB(king);
    // output << "The king is in position " << kingPosition << "\n";

    U64 kingAttacks = attackTables::getKingAttacks(kingPosition);
    // output << "The king attacks bitboard looks like\n";
    // attackTables::printBitboard(kingAttacks, output);
    for (int i = 0; i < 64; i++)
    {
        U64 shift = 1ULL << i;
        if (((shift & kingAttacks) != 0) && ((shift & friendlyPieces) == 0))
        {
            // output << "The king attacks square " << i << " and there is no friendly piece on this square\n We therefore create the move\n";
            int capturedPiece = 12;
            if ((enemyPieces & shift) != 0)
            {
                std::string pieceType = getPieceAt(i);
                char pieceTypeCharacter = pieceType[0];
                capturedPiece = charToPieceIndex(pieceTypeCharacter);
            }

            Move move = Move(kingPosition, i, index, capturedPiece, 12, false, false);
            pseudoLegalMoves.push_back(move);
            // output << move.printMove() << "\n";
        }
    }

    if (index == 5)
    {
        if ((castlingRights[0]) == true)
        {
            // output << "The white king has short side castling rights\n";
            U64 piecesBetween = 0ULL;
            set_bit(piecesBetween, 5);
            set_bit(piecesBetween, 6);
            int count = 0;
            if ((piecesBetween & allPieces) == 0)
            {
                // output << "There are no pieces in between the king and the rook";
                for (int i = 4; i < 7; i++)
                {
                    bool check = determineIfKingIsInCheck(0, i);
                    count = count + check;
                }
                if (count == 0)
                {
                    // output << "The king is not in check and neither of the castling squares are being threated\n";
                    // output << "We therefore create the move\n";
                    Move move = Move(kingPosition, 6, index, 12, 12, false, true);
                    pseudoLegalMoves.push_back(move);
                    // output << move.printMove() << "\n";
                }
            }
        }

        if (castlingRights[1] == true)
        {
            // output << "The white king has long side castling rights\n";
            U64 piecesBetween = 0ULL;
            set_bit(piecesBetween, 1);
            set_bit(piecesBetween, 2);
            set_bit(piecesBetween, 3);
            int count = 0;
            if ((piecesBetween & allPieces) == 0)
            {
                // output << "There are no pieces in between the king and the rook\n";
                for (int i = 2; i < 5; i++)
                {
                    bool check = determineIfKingIsInCheck(0, i);
                    count = count + check;
                }
                if (count == 0)
                {
                    Move move = Move(kingPosition, 2, index, 12, 12, false, true);
                    pseudoLegalMoves.push_back(move);
                    // output << "The king is not in check and neither of the castling squares are being threated\nWe therefore create the move\n"
                    //        << move.printMove() << "\n";
                }
            }
        }
    }
    else
    {
        if ((castlingRights[2]) == true)
        {
            // output << "The black king has short side castling rights\n";
            U64 piecesBetween = 0ULL;
            set_bit(piecesBetween, 61);
            set_bit(piecesBetween, 62);
            int count = 0;
            if ((piecesBetween & allPieces) == 0)
            {
                // output << "There are no pieces in between the black king and the rook\n";
                for (int i = 60; i < 63; i++)
                {
                    bool check = determineIfKingIsInCheck(1, i);
                    count = count + check;
                }
                if (count == 0)
                {
                    Move move = Move(kingPosition, 62, index, 12, 12, false, true);
                    pseudoLegalMoves.push_back(move);
                    // output << "The king is not in check and neither of the castling squares are being threatened\nWe therefore create the move\n"
                    //        << move.printMove() << "\n";
                }
            }
        }

        if ((castlingRights[3]) == true)
        {
            // output << "The black king has long side castling rights\n";
            U64 piecesBetween = 0ULL;
            set_bit(piecesBetween, 57);
            set_bit(piecesBetween, 58);
            set_bit(piecesBetween, 59);
            int count = 0;
            if ((piecesBetween & allPieces) == 0)
            {
                // output << "There are no pieces in between the black king and the rook\n";
                for (int i = 58; i < 61; i++)
                {
                    bool check = determineIfKingIsInCheck(1, i);
                    count = count + check;
                }
                if (count == 0)
                {
                    Move move = Move(kingPosition, 58, index, 12, 12, false, true);
                    pseudoLegalMoves.push_back(move);
                    // output << "The king is not in check and neither of the castling squares are being threatened\nWe therefore create the move\n"
                    //        << move.printMove() << "\n";
                }
            }
        }
    }

    // output.close();
    return pseudoLegalMoves;
}

std::vector<Move> Board::generatePawnPseudoLegalMoves(int index, U64 allPieces, U64 friendlyPieces, U64 enemyPieces) const
{
    std::vector<Move> pseudoLegalMoves;
    U64 pawnStartingRank, pawnPromotionRank, pawnEnPassantRank;

    if (index == 0)
    {
        pawnStartingRank = RANK_2;
        pawnPromotionRank = RANK_7;
        pawnEnPassantRank = RANK_5;
    }
    else
    {
        pawnStartingRank = RANK_7;
        pawnPromotionRank = RANK_2;
        pawnEnPassantRank = RANK_4;
    }

    // Generate all of the pseduo legal pawn moves
    if (index == 0)
    {
        U64 whitePawns = bitboards[index];
        int whitePawnPosition = pop_LSB(whitePawns);

        while (whitePawnPosition != -1)
        {
            // Setup a bitboard that has a single pawn in the specified position
            U64 whitePawn = 0ULL;
            set_bit(whitePawn, whitePawnPosition);

            // std::ofstream output("output.txt", std::ios::app);
            // output << "------------------------------------------------------\n";
            // output << "The white pawn is in position " << whitePawnPosition << "\n";
            // output << "This white pawn being in this position has a bitboard that looks like\n";
            // attackTables::printBitboard(whitePawn, output);

            // Single forward move
            if ((north(whitePawn) & allPieces) == 0)
            {
                if ((whitePawn & pawnPromotionRank) != 0)
                {
                    // output << "This pawn is on the 7th rank and has no piece infront of it\nWe create the following moves\n";

                    Move promotion1 = Move(whitePawnPosition, whitePawnPosition + 8, 0, 12, 1, false, false);
                    Move promotion2 = Move(whitePawnPosition, whitePawnPosition + 8, 0, 12, 2, false, false);
                    Move promotion3 = Move(whitePawnPosition, whitePawnPosition + 8, 0, 12, 3, false, false);
                    Move promotion4 = Move(whitePawnPosition, whitePawnPosition + 8, 0, 12, 4, false, false);

                    // output << promotion1.printMove() << "\n";
                    // output << promotion2.printMove() << "\n";
                    // output << promotion3.printMove() << "\n";
                    // output << promotion4.printMove() << "\n";

                    pseudoLegalMoves.push_back(promotion1);
                    pseudoLegalMoves.push_back(promotion2);
                    pseudoLegalMoves.push_back(promotion3);
                    pseudoLegalMoves.push_back(promotion4);
                }
                else
                {
                    // output << "This pawn is not on the 7th rank. It has no piece infront of it\nWe create the following move\n";
                    Move move = Move(whitePawnPosition, whitePawnPosition + 8, 0, 12, 12, false, false);
                    // output << move.printMove() << "\n";
                    pseudoLegalMoves.push_back(move);
                }
            }

            // Double forward move
            if ((((north(whitePawn) | north(north(whitePawn))) & allPieces) == 0) && ((whitePawn & pawnStartingRank) != 0))
            {
                // output << "This pawn is on its starting rank and has no pieces 1 or 2 squares ahead of it\n";
                // output << "We therefore create the move\n";
                Move move = Move(whitePawnPosition, whitePawnPosition + 16, 0, 12, 0, false, false);
                // output << move.printMove() << "\n\n";
                pseudoLegalMoves.push_back(move);
            }

            // Regular captures
            for (int i = 7; i <= 9; i = i + 2)
            {
                U64 diagonal = whitePawn;
                // output << "The pawn bitboard looks like \n";
                // attackTables::printBitboard(diagonal, output);
                // output << "The value of i is " << i << "\n";
                if (i == 7)
                {
                    diagonal = (diagonal << i) & ~(FILE_H);
                }
                else
                {
                    diagonal = (diagonal << i) & ~(FILE_A);
                }

                // output << "The diagonal attack is \n";
                // attackTables::printBitboard(diagonal, output);

                if ((diagonal & enemyPieces) != 0)
                {
                    // output << "There is an enemy piece on the diagonal attack\n";
                    std::string pieceType = getPieceAt(get_LSB(diagonal));
                    char pieceTypeCharacter = pieceType[0];
                    int capturedPiece = charToPieceIndex(pieceTypeCharacter);
                    // output << "The piece that was captured is " << pieceType << "\n";
                    if ((whitePawn & pawnPromotionRank) != 0)
                    {
                        // output << "The pawn is on the 7th rank therefore moves that are generated are as follows";
                        Move promotion1 = Move(whitePawnPosition, whitePawnPosition + i, 0, capturedPiece, 1, false, false);
                        Move promotion2 = Move(whitePawnPosition, whitePawnPosition + i, 0, capturedPiece, 2, false, false);
                        Move promotion3 = Move(whitePawnPosition, whitePawnPosition + i, 0, capturedPiece, 3, false, false);
                        Move promotion4 = Move(whitePawnPosition, whitePawnPosition + i, 0, capturedPiece, 4, false, false);
                        // output << promotion1.printMove() << "\n";
                        // output << promotion2.printMove() << "\n";
                        // output << promotion3.printMove() << "\n";
                        // output << promotion4.printMove() << "\n";
                        pseudoLegalMoves.push_back(promotion1);
                        pseudoLegalMoves.push_back(promotion2);
                        pseudoLegalMoves.push_back(promotion3);
                        pseudoLegalMoves.push_back(promotion4);
                    }
                    else
                    {
                        // output << "The pawn is not on the 7th rank therefore the move that is generated is\n";
                        Move move = Move(whitePawnPosition, whitePawnPosition + i, 0, capturedPiece, false, false);
                        // output << move.printMove();
                        pseudoLegalMoves.push_back(move);
                    }
                }
            }

            // En Passant captures
            if (((whitePawn & pawnEnPassantRank) != 0) && (enPassantSquare > -1))
            {
                // output << "The pawn is on the en passant rank\n";
                for (int i = 7; i <= 9; i = i + 2)
                {
                    U64 diagonal = whitePawn;
                    if (i == 7)
                    {
                        diagonal = (diagonal << i) & ~(FILE_H);
                        // output << "The left diagonal attack bitboard looks like\n";
                    }
                    else
                    {
                        diagonal = (diagonal << i) & ~(FILE_A);
                        // output << "The right diagonal attack bitboard looks like\n";
                    }

                    // attackTables::printBitboard(diagonal, output);

                    U64 enPassant = 0ULL;
                    set_bit(enPassant, enPassantSquare);

                    if ((diagonal & enPassant) != 0)
                    {
                        // output << "The diagonal attack square is the same as the en passant square\n";
                        // output << "We therefore create the en passant move\n";
                        Move move = Move(whitePawnPosition, whitePawnPosition + i, 0, 6, 0, true, false);
                        // output << move.printMove() << "\n";
                        pseudoLegalMoves.push_back(move);
                    }
                }
            }

            whitePawnPosition = pop_LSB(whitePawns);
            // output << "The new white pawn position is " << whitePawnPosition << "\n";
            // output << "The white pawns bitboard now looks like \n";
            // attackTables::printBitboard(whitePawns, output);
            // output.close();
        }
    }
    else
    {
        U64 blackPawns = bitboards[index];
        int blackPawnPosition = pop_LSB(blackPawns);

        while (blackPawnPosition != -1)
        {
            // Setup a bitboard that has a single pawn in the specified position
            U64 blackPawn = 0ULL;
            set_bit(blackPawn, blackPawnPosition);

            // std::ofstream output("output.txt", std::ios::app);
            // output << "------------------------------------------------------\n";
            // output << "The black pawn is in position " << blackPawnPosition << "\n";
            // output << "This black pawn being in this position has a bitboard that looks like\n";
            // attackTables::printBitboard(blackPawn, output);

            // Single forward move
            if ((south(blackPawn) & allPieces) == 0)
            {
                if ((blackPawn & pawnPromotionRank) != 0)
                {
                    // output << "This pawn is on the 2nd rank and has no piece infront of it\nWe create the following moves\n";

                    Move promotion1 = Move(blackPawnPosition, blackPawnPosition - 8, 6, 12, 1, false, false);
                    Move promotion2 = Move(blackPawnPosition, blackPawnPosition - 8, 6, 12, 2, false, false);
                    Move promotion3 = Move(blackPawnPosition, blackPawnPosition - 8, 6, 12, 3, false, false);
                    Move promotion4 = Move(blackPawnPosition, blackPawnPosition - 8, 6, 12, 4, false, false);

                    // output << promotion1.printMove() << "\n";
                    // output << promotion2.printMove() << "\n";
                    // output << promotion3.printMove() << "\n";
                    // output << promotion4.printMove() << "\n";

                    pseudoLegalMoves.push_back(promotion1);
                    pseudoLegalMoves.push_back(promotion2);
                    pseudoLegalMoves.push_back(promotion3);
                    pseudoLegalMoves.push_back(promotion4);
                }
                else
                {
                    // output << "This pawn is not on the 2nd rank. It has no piece infront of it\nWe create the following move\n";
                    Move move = Move(blackPawnPosition, blackPawnPosition - 8, 6, 12, 12, false, false);
                    // output << move.printMove() << "\n";
                    pseudoLegalMoves.push_back(move);
                }
            }

            // Double forward move
            if ((((south(blackPawn) | south(south(blackPawn))) & allPieces) == 0) && ((blackPawn & pawnStartingRank) != 0))
            {
                // output << "This black pawn is on its starting rank and has no pieces 1 or 2 squares ahead of it\n";
                // output << "We therefore create the move\n";
                Move move = Move(blackPawnPosition, blackPawnPosition - 16, 6, 12, 0, false, false);
                // output << move.printMove() << "\n\n";
                pseudoLegalMoves.push_back(move);
            }

            // Regular captures
            for (int i = 7; i <= 9; i = i + 2)
            {
                U64 diagonal = blackPawn;
                // output << "The pawn bitboard looks like \n";
                // attackTables::printBitboard(diagonal, output);
                // output << "The value of i is " << i << "\n";
                if (i == 7)
                {
                    diagonal = (diagonal >> i) & ~(FILE_A);
                }
                else
                {
                    diagonal = (diagonal >> i) & ~(FILE_H);
                }

                // output << "The diagonal attack is \n";
                // attackTables::printBitboard(diagonal, output);

                if ((diagonal & enemyPieces) != 0)
                {
                    // output << "There is an enemy piece on the diagonal attack\n";
                    std::string pieceType = getPieceAt(get_LSB(diagonal));
                    char pieceTypeCharacter = pieceType[0];
                    int capturedPiece = charToPieceIndex(pieceTypeCharacter);
                    // output << "The piece that was captured is " << pieceType << "\n";
                    if ((blackPawn & pawnPromotionRank) != 0)
                    {
                        // output << "The pawn is on the 2nd rank therefore moves that are generated are as follows";
                        Move promotion1 = Move(blackPawnPosition, blackPawnPosition - i, 6, capturedPiece, 1, false, false);
                        Move promotion2 = Move(blackPawnPosition, blackPawnPosition - i, 6, capturedPiece, 2, false, false);
                        Move promotion3 = Move(blackPawnPosition, blackPawnPosition - i, 6, capturedPiece, 3, false, false);
                        Move promotion4 = Move(blackPawnPosition, blackPawnPosition - i, 6, capturedPiece, 4, false, false);
                        // output << promotion1.printMove() << "\n";
                        // output << promotion2.printMove() << "\n";
                        // output << promotion3.printMove() << "\n";
                        // output << promotion4.printMove() << "\n";
                        pseudoLegalMoves.push_back(promotion1);
                        pseudoLegalMoves.push_back(promotion2);
                        pseudoLegalMoves.push_back(promotion3);
                        pseudoLegalMoves.push_back(promotion4);
                    }
                    else
                    {
                        // output << "The pawn is not on the 2nd rank therefore the move that is generated is\n";
                        Move move = Move(blackPawnPosition, blackPawnPosition - i, 6, capturedPiece, false, false);
                        // output << move.printMove();
                        pseudoLegalMoves.push_back(move);
                    }
                }
            }

            // En Passant captures
            if (((blackPawn & pawnEnPassantRank) != 0) && (enPassantSquare > -1))
            {
                // output << "The pawn is on the en passant rank\n";
                for (int i = 7; i <= 9; i = i + 2)
                {
                    U64 diagonal = blackPawn;
                    if (i == 7)
                    {
                        diagonal = (diagonal >> i) & ~(FILE_A);
                        // output << "The right diagonal attack bitboard looks like\n";
                    }
                    else
                    {
                        diagonal = (diagonal >> i) & ~(FILE_H);
                        // output << "The left diagonal attack bitboard looks like\n";
                    }

                    // attackTables::printBitboard(diagonal, output);

                    U64 enPassant = 0ULL;
                    set_bit(enPassant, enPassantSquare);

                    if ((diagonal & enPassant) != 0)
                    {
                        // output << "The diagonal attack square is the same as the en passant square\n";
                        // output << "We therefore create the en passant move\n";
                        Move move = Move(blackPawnPosition, blackPawnPosition - i, 6, 0, 12, true, false);
                        // output << move.printMove() << "\n";
                        pseudoLegalMoves.push_back(move);
                    }
                }
            }

            blackPawnPosition = pop_LSB(blackPawns);
            // output << "The new black pawn position is " << blackPawnPosition << "\n";
            // output << "The black pawns bitboard now looks like \n";
            // attackTables::printBitboard(blackPawns, output);
            // output.close();
        }
    }

    return pseudoLegalMoves;
}

bool Board::determineIfKingIsInCheck(int kingColour, int square) const
{

    U64 attackSquares = 0ULL;
    U64 blockers = 0ULL;
    int a, b;
    for (int i = 0; i < 12; i++)
    {
        blockers = blockers | bitboards[i];
    }

    if (kingColour == 0)
    {
        a = 5;
        b = 6;
    }
    else
    {
        a = 11;
        b = 0;
    }

    U64 king = bitboards[a];
    U64 pawns = bitboards[b];

    if (square != -1)
    {
        pop_LSB(king);
        set_bit(king, square);
    }

    if (kingColour == 0)
    {
        if (((north_west(pawns) | north_east(pawns)) & king) != 0)
        {
            return true;
        }
    }
    else
    {
        if (((south_west(pawns) | south_east(pawns)) & king) != 0)
        {
            return true;
        }
    }
    b++;

    U64 knights = bitboards[b];
    int knightPos = pop_LSB(knights);
    while (knightPos != -1)
    {
        if ((attackTables::getKnightAttacks(knightPos) & king) != 0)
        {
            return true;
        }
        knightPos = pop_LSB(knights);
    }
    b++;

    U64 bishops = bitboards[b];
    int bishopPos = pop_LSB(bishops);
    while (bishopPos != -1)
    {
        if ((attackTables::getBishopAttacks(bishopPos, blockers) & king) != 0)
        {
            return true;
        }
        bishopPos = pop_LSB(bishops);
    }
    b++;

    U64 rooks = bitboards[b];
    int rookPos = pop_LSB(rooks);
    while (rookPos != -1)
    {
        if ((attackTables::getRookAttacks(rookPos, blockers) & king) != 0)
        {
            return true;
        }
        rookPos = pop_LSB(rooks);
    }
    b++;

    U64 queen = bitboards[b];
    int queenPos = pop_LSB(queen);
    if (queenPos != -1)
    {
        if ((attackTables::getQueenAttacks(queenPos, blockers) & king) != 0)
        {
            return true;
        }
    }

    return false;
}

void Board::applyMove(const Move &move)
{
    int startSquare = move.getStartSquare();
    int endSquare = move.getEndSquare();
    int movedPiece = move.getMovedPiece();
    int capturedPiece = move.getCapturedPiece();
    int promotionPiece = move.getPromotionPiece();
    bool isEnPassant = move.getIsEnPassant();
    bool isCastling = move.getIsCastling();

    if (promotionPiece != 12)
    {
        clear_bit(bitboards[movedPiece], startSquare);
        set_bit(bitboards[promotionPiece], endSquare);
        if (capturedPiece != 12)
        {
            clear_bit(bitboards[capturedPiece], endSquare);
        }
    }
    else if (((movedPiece == 0) || (movedPiece == 11)) && (abs(startSquare - endSquare) == 16))
    {
        clear_bit(bitboards[movedPiece], startSquare);
        set_bit(bitboards[movedPiece], endSquare);
        if (movedPiece == 0)
        {
            setEnPassantSquare(startSquare + 8);
        }
        else
        {
            setEnPassantSquare(startSquare - 8);
        }
    }
    else if (isEnPassant == true)
    {
        clear_bit(bitboards[movedPiece], startSquare);
        set_bit(bitboards[movedPiece], endSquare);
        if (movedPiece == 0)
        {
            clear_bit(bitboards[6], endSquare - 8);
        }
        else
        {
            clear_bit(bitboards[0], endSquare + 8);
        }
    }
    else if (isCastling == true)
    {
        if (movedPiece == 5)
        {
            clear_bit(bitboards[5], startSquare);
            set_bit(bitboards[5], endSquare);
            if (endSquare == 6)
            {
                clear_bit(bitboards[3], 7);
                set_bit(bitboards[3], 5);
            }
            else
            {
                clear_bit(bitboards[3], 0);
                set_bit(bitboards[3], 3);
            }
        }
        else
        {
            clear_bit(bitboards[11], startSquare);
            set_bit(bitboards[11], endSquare);
            if (endSquare == 62)
            {
                clear_bit(bitboards[9], 63);
                set_bit(bitboards[9], 61);
            }
            else
            {
                clear_bit(bitboards[9], 56);
                set_bit(bitboards[9], 59);
            }
        }
    }
    else
    {
        clear_bit(bitboards[movedPiece], startSquare);
        set_bit(bitboards[movedPiece], endSquare);
        if (capturedPiece != 12)
        {
            clear_bit(bitboards[capturedPiece], endSquare);
        }
    }

    if (movedPiece == 5)
    {
        castlingRights[0] = false;
        castlingRights[1] = false;
    }
    else if (movedPiece == 11)
    {
        castlingRights[2] = false;
        castlingRights[3] = false;
    }
    else if (movedPiece == 3)
    {
        if (startSquare == 7)
        {
            castlingRights[0] = false;
        }
        else if (startSquare == 0)
        {
            castlingRights[1] = false;
        }
    }
    else if (movedPiece == 9)
    {
        if (startSquare == 63)
        {
            castlingRights[2] = false;
        }
        else if (startSquare == 56)
        {
            castlingRights[3] = false;
        }
    }

    if (turn == 0)
    {
        turn = 1;
    }
    else
    {
        fullMoveNumber++;
        turn = 0;
    }

    if (capturedPiece != 12)
    {
        halfMoveClock = 0;
    }
    else
    {
        halfMoveClock++;
    }
}

void Board::printBoard(std::ofstream &outputFile) const
{
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
        piece = "N";
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
        piece = "n";
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

void Board::setBoard(int pieceToPlay)
{
    turn = pieceToPlay;
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
            if (pieceBitoard != 12)
            {
                set_bit(bitboards[pieceBitoard], (i * 8 + j));
            }
        }
    }

    inputFile.close();
}

int Board::charToPieceIndex(char pieceChar) const
{
    switch (pieceChar)
    {
    case 'P':
        return 0;
    case 'N':
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
    case 'n':
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
        return 12;
    }
}

void Board::setEnPassantSquare(int square)
{
    enPassantSquare = square;
}

void Board::setCastlingRights(int caslingRight, bool right)
{
    castlingRights[caslingRight] = right;
}

void Board::printAllInformation(std::ofstream &output) const

{
    output << "-------------------------------------------------\n\n";
    output << "The Information of the board is as follows: \n";
    output << "En Passant Square: " << enPassantSquare << "\n";
    output << "Castling rights 0: " << castlingRights[0] << "\n";
    output << "Castling rights 1: " << castlingRights[1] << "\n";
    output << "Castling rights 2: " << castlingRights[2] << "\n";
    output << "Castling rights 3: " << castlingRights[3] << "\n";
    if (turn == 0)
    {
        output << "It is white to play and the board looks like\n";
    }
    else
    {
        output << "It is black to play and the board looks like\n";
    }
    printBoard(output);
    output << "-------------------------------------------------\n";
}

int Board::positionToIndex(const std::string &position)
{
    if (position.length() != 2)
    {
        return -1; // Error case, invalid input
    }

    int file = position[0] - 'A'; // Convert file (A-H) to (0-7)
    int rank = position[1] - '1'; // Convert rank (1-8) to (0-7)

    return 8 * rank + file; // Calculate the index
}
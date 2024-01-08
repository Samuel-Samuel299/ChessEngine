#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include "board.h"
#include "attackTables.h"

typedef U64 uint64_t;

using namespace std;

U64 readBitboard(int startLine)
{
    startLine--;
    ifstream inputFile("input.txt");

    string line = "";
    for (int i = 0; i < startLine; ++i)
    {
        if (!std::getline(inputFile, line))
        {
            std::cerr << "Failed to reach start line" << std::endl;
            return 1;
        }
    }

    U64 bitboard = 0;
    for (int row = 7; row >= 0; row--)
    {
        getline(inputFile, line);
        istringstream iss(line.substr(line.find('\t') + 1));
        int bit;
        int col = 0;

        while (iss >> bit)
        {
            if (bit == 1)
            {
                bitboard = bitboard | (static_cast<U64>(1) << (row * 8 + col));
            }
            col++;
        }
    }

    inputFile.close();
    return bitboard;
}

void printBitboard(U64 bitboard)
{
    // Open the file in append mode
    ofstream outFile("output.txt", ios::app);
    int position;
    for (int i = 7; i >= 0; i--)
    {
        outFile << "Row " << i + 1 << "\t";
        for (int j = 0; j <= 7; j++)
        {
            position = 8 * i + j;
            if (get_bit(bitboard, position) == true)
            {
                outFile << "1 ";
            }
            else
            {
                outFile << "0 ";
            }
        }
        outFile << "\n";
    }
    outFile << " File\tA B C D E F G H\n";
    outFile << "\n"; // Optionally, add an extra newline for separation between writes.
    outFile.close(); // It's good practice to close the file when done.
}

int main(int argc, char *argv[])
{
    // Open and immediately close the file to clear its contents.
    ofstream output("output.txt", ios::trunc);
    output.close();

    ofstream outputFile("testing/printBoard.txt", ios::trunc);
    output.close();

    attackTables::initializeLeaping();
    attackTables::initialiseBishopAttacks();
    attackTables::initialiseRookAttacks();

    Board board = Board();
    board.setBoard(0);
    board.printBoard();

    bool isCheck = board.determineIfKingIsInCheck();
    cout << "Statement: The king is in check\n"
         << isCheck << endl;

    return 0;
}

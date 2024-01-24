#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <chrono>
#include "board.h"
#include "attackTables.h"
#include "move.h"
#include "minimaxEngine.h"

typedef U64 uint64_t;

using namespace std;

U64 readBitboard(int startLine)
{
    startLine--;
    ifstream inputFile("testing/input.txt");

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
    auto start = std::chrono::high_resolution_clock::now();

    // Open and immediately close the file to clear its contents.
    ofstream output("output.txt", ios::trunc);
    output.close();

    ofstream output1("output1.txt", ios::trunc);
    output1.close();

    // Setting up the different attacks
    attackTables::initializeLeaping();
    attackTables::initialiseBishopAttacks();
    attackTables::initialiseRookAttacks();

    // Setting up the board
    std::string fen = "5rk1/1p3pp1/1p1Rb2p/1B2p3/8/4P3/rPP2PPP/5RK1 w - - 0 20";
    Board board = Board();
    board.loadFromFEN(fen);
    output1.open("output1.txt", std::ios::app);
    board.printAllInformation(output1);

    MinimaxEngine minimax = MinimaxEngine(3);
    Move move = minimax.findBestMove(board, 5);
    board.printAllInformation(output1);
    output1 << move.printMove() << "\n";

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "Time taken for whole execution: "
              << duration.count() << " milliseconds" << std::endl;
    long long time = board.getTotalTimeSpentInPseudoFunction();
    cout << "Time spent in the pseudo function " << (time / 1000.0) << " milliseconds" << endl;
    long long timeLegal = board.getTotalTimeSpentInLegalFunction();
    cout << "Time spent in the legal function " << (timeLegal / 1000.0) << " milliseconds" << endl;
    long long otherTime = board.getTotalTimeSpentInOtherFunction();
    cout << "Time spent determining bishop pseudo moves " << (otherTime / 1000.0) << " milliseconds" << endl;
    return 0;
}

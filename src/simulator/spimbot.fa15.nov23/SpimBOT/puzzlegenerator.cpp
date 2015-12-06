//
//  main.cpp
//  PuzzleGenerator
//
//  Created by Adel Ejjeh on 9/13/15.
//  Copyright (c) 2015 Adel Ejjeh. All rights reserved.
//

#include <iostream>
#include <ctime>
#include <cstdlib>
#include <boost/random/uniform_int_distribution.hpp>
using namespace std;

#include "puzzlegenerator.h"
#include "robot.h"

//Clear the marking for the "passed" board elements
void clearPassed(boardElem **board, int rowSize, int colSize)
{
    int row, col;
    for (row=0; row<rowSize; row++)
        for (col=0; col<colSize; col++)
            board[row][col].passed = 0;
}

//Check solution against actual word
bool checkSolution(boardElem **board, node_t *head, char* word, int rowSize, int colSize, int wordSize)
{
    int i = 0;
    while (head != NULL && i < wordSize) {
        if (head->x >= rowSize || head->x < 0 || head->y >= colSize || head->y < 0) {
            return false;
        }
        if (board[head->x][head->y].letter == *word) {
            head = head->next;
            word++;
            i++;
        } else {
            return false;
        }
    }

    return i == wordSize;
}

//Generate the puzzle board
void generatePuzzle(boardElem** board, int rowSize, int columns)
{
    boost::random::uniform_int_distribution<int> rand_letter(65, 90);
    int c;
    for (int i=0; i<rowSize; i++)
    {
        for(int j=0; j<columns; j++)
        {
            c = rand_letter(puzzle_rng);
            board[i][j].letter = (char) c;
            board[i][j].passed = 0;
        }
    }
}

//print the puzzle board
void printPuzzle(boardElem** board, int rowSize, int colSize)
{
    for (int i=0; i<rowSize; i++)
    {
        for(int j=0; j<colSize; j++)
        {
            cout<<board[i][j].letter<<board[i][j].passed<<"\t";
        }
        cout<<endl;
    }
}

//print word
void printWord(char* word, int size)
{
    for (int i=0; i<size; i++)
    {
        cout<<word[i];
    }
    cout << endl;
}

//Generate puzzle word from the board
void extractWord(boardElem** board, char* word, int rowSize, int colSize, int *size, int wrap)
{
    boost::random::uniform_int_distribution<int> rand_row(0, rowSize-1);
    boost::random::uniform_int_distribution<int> rand_col(0, colSize-1);
    int row = (rand_row(puzzle_rng) + rowSize/2)%rowSize;
    int col = (rand_col(puzzle_rng) + colSize/2)%colSize;
    int tempRow = row, tempCol = col;
    int index = 0, dir = 0;
    word[index] = board[row][col].letter;
    board[row][col].passed = 1;
    index++;
    boost::random::uniform_int_distribution<int> rand_dir(0, 3);
    dir = rand_dir(puzzle_rng);
    //cout<<row<<'.'<<col<<endl;
    int i;
    while(index < *size)
    {
        const int max_tries = 8;
        i = 0;
        do{
            i++;
            tempRow = row;
            tempCol = col;
            dir = rand_dir(puzzle_rng);
            switch (dir)
            {
                case 0:
                    tempRow = row - 1;
                    if (tempRow == -1 && wrap == 1)
                        tempRow = rowSize - 1;
                    else if (tempRow == -1)
                        tempRow = row;
                    break;
                case 1:
                    tempCol = col + 1;
                    if (tempCol == colSize && wrap==1)
                        tempCol = 0;
                    else if (tempCol == colSize)
                        tempCol = col;
                    break;
                case 2:
                    tempRow = row + 1;
                    if (tempRow == rowSize && wrap==1)
                        tempRow = 0;
                    else if (tempRow == rowSize)
                        tempRow = row;
                    break;
                case 3:
                    tempCol = col - 1;
                    if (tempCol == -1 && wrap == 1)
                        tempCol = colSize - 1;
                    else if (tempCol == -1)
                        tempCol = col;
            }
        } while (board[tempRow][tempCol].passed == 1 && i < max_tries);

        if (i >= max_tries) {
            break;
        } else {
            row = tempRow;
            col = tempCol;
            word[index] = board[row][col].letter;
            board[row][col].passed = 1;
            index ++;
        }
    }
    word[index] = '\0';
    *size = index;
    clearPassed(board,rowSize,colSize);
//    cout<<endl;

}

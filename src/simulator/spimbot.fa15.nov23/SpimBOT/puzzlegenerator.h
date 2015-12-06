//
//  puzzlegenerator.h
//  PuzzleGenerator
//
//  Created by Adel Ejjeh on 11/13/15.
//  Copyright © 2015 Adel Ejjeh. All rights reserved.
//

#ifndef puzzlegenerator_h
#define puzzlegenerator_h

#include <cstddef>

struct boardElem {
    char letter;
    int passed;
};

struct node_t {
    int x,y;
    node_t *next;

    node_t() : x(0), y(0), next(NULL) {}
    node_t(int x, int y) : x(x), y(y), next(NULL) {}
};

void generatePuzzle(boardElem** board, int rowSize, int columns);
void printPuzzle(boardElem** board, int rowSize, int colSize);
void extractWord(boardElem** board, char* word, int rowSize, int colSize, int *size, int wrap);
void printWord(char* word, int size);
void clearPassed(boardElem ** board, int rowSize, int colSize);
bool checkSolution(boardElem **board, node_t *head, char *word, int rowSize, int colSize, int wordSize);
void printList(node_t *head);


#endif /* puzzlegenerator_h */

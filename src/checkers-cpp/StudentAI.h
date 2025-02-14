#ifndef STUDENTAI_H
#define STUDENTAI_H

#include "AI.h"
#include "Node.h"
#include "Board.h"
#include "Move.h"
#include <vector>
#include <cmath>

class MCTS {
public:
    Node* root;
    MCTS(Node* root, Board board, int player);
    Node* selectNode(Node* node);
    Node* expandNode(Node* node);
    int simulation(Node* node);
    void backPropagation(Node* node, int result);
    double getUCT(Node* node);
    void runMCTS(int time);
    Move getBestMove();
    bool causeMoreCaptureByOpponent(Board board, Move move, int player);
    bool leadToForceCapture(Board board, Move move, int player);
    bool isPromoting(Board board, Move move, int player);
    double evaluateBoard(Board board, int player); // Added heuristic evaluation
    void deleteTree(Node* node);
    ~MCTS();
};

class StudentAI : public AI {
public:
    Board board;
    int player;
    StudentAI(int col, int row, int p);
    Move GetMove(Move move);
};

#endif

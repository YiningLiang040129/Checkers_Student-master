#ifndef STUDENTAI_H
#define STUDENTAI_H

#include "AI.h"
#include "Board.h"
#include "Move.h"
#include <vector>
#include <cmath>

#pragma once

// The following part should be completed by students.
// Students can modify anything except the class name and existing functions and variables.

class Node {
public:
    Node* parent;
    Move move;
    Board board;
    int player;
    int visits;
    double wins;
    bool isLeaf;
    std::vector<Node*> children;
    std::vector<Move> unvisitedMoves;

    Node(Node* parent, Move move, Board board, int player)
        : parent(parent), move(move), board(board), player(player), visits(0), wins(0), isLeaf(false) {}
    
    bool isFullyExpanded();
};

class MCTS {
public:
    Node* root;

    MCTS(Node* root, Board board, int player);
    Node* selectNode(Node* node);
    Node* expandNode(Node* node);
    int simulation(Node* node);
    void backPropagation(Node* node, int result);
    void runMCTS(int time);
    Move getBestMove();
    void deleteTree(Node* node);
    ~MCTS();

    // Heuristic Functions
    double evaluateBoard(Board board, int player);
    bool leadToForceCapture(Board board, Move move, int player);
    bool causeMoreCaptureByOpponent(Board board, Move move, int player);
    bool isPromoting(Board board, Move move, int player);
};

// The AI class that implements MCTS
class StudentAI : public AI {
public:
    Board board;
    int player;
    
    StudentAI(int col, int row, int p);
    virtual Move GetMove(Move move);
};

#endif // STUDENTAI_H

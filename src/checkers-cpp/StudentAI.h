#ifndef STUDENTAI_H
#define STUDENTAI_H

#include "AI.h"
#include "Board.h"
#include "Move.h"
#include <vector>
#include <cmath>

class Node {
public:
    Node* parent;
    std::vector<Node*> children;
    Move move;
    Board board;
    int player;
    int wins;
    int visits;
    bool isLeaf;
    std::vector<Move> unvisitedMoves;

    Node(Node* parent, Move move, Board board, int player)
        : parent(parent), move(move), board(board), player(player),
          wins(0), visits(0), isLeaf(false) {}
    
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
    double getUCT(Node* node);
    void runMCTS(int time);
    Move getBestMove();

    double evaluateBoard(Board board, int player);

    void deleteTree(Node* node);
    ~MCTS();
};

class StudentAI : public AI {
public:
    Board board;
    int player;
    
    StudentAI(int col, int row, int p);
    Move GetMove(Move move) override;
};

#endif // STUDENTAI_H

#ifndef STUDENTAI_H
#define STUDENTAI_H

#include "AI.h"
#include "Board.h"
#include <chrono>
#include <random>
#include <algorithm>
#include <vector>
#include <cmath>
using namespace std::chrono;
using namespace std;

#pragma once

// Node represents a state in the search tree.
class Node {
public:
    Node* parent;
    vector<Node*> children;
    vector<Move> unvisitedMoves;
    double wins = 0;
    int visits = 0;
    int player;
    bool initialized = false;
    bool isLeaf = false;
    Move move;
    Board board;
    Node(Node* parent, Move move, Board board, int player) : parent(parent), move(move), board(board), player(player) {}
    bool isFullyExpanded();
};

// MCTS encapsulates the Monte Carlo Tree Search logic.
class MCTS {
public:
    Node* root;
    MCTS(Node* root, Board &board, int player);
    Node* selectNode(Node* node);
    Node* expandNode(Node* node);
    int simulation(Node* node);
    void backPropagation(Node* node, int result);
    double getUCT(Node* node);
    void runMCTS(int time);
    Move getBestMove();
    bool isMultipleCapture(const Move &move);
    double isVulnerableMove(Board &board, const Move &move, int player);
    bool isPromoting(const Board &board, const Move &move, int player);
    double generalBoardPositionEvaluation(Board &board, const Move &move, int player);
    // New evaluation: piece structure (mutual protection)
    double evaluatePieceStructure(Board &board, int player);
    static Node* findChildNode(Node* node, const Move &move);
    static void deleteTree(Node* node);
    static Node* reRoot(Node *root, const Move &move);
};

// StudentAI implements the AI interface using MCTS.
class StudentAI : public AI {
public:
    Board board;
    Node* MCTSRoot = nullptr;
    duration<double, std::milli> timeElapsed = duration<double, std::milli>::zero();
    const duration<double, std::milli> timeLimit = minutes(8); // 8 minutes total time
    StudentAI(int col, int row, int p);
    virtual Move GetMove(Move move);
    Move GetRandomMove(Move move);
    ~StudentAI();
};

#endif // STUDENTAI_H

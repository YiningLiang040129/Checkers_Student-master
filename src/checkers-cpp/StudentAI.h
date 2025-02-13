#ifndef STUDENTAI_H
#define STUDENTAI_H
#include "AI.h"
#include "Board.h"
#pragma once

//The following part should be completed by students.
//Students can modify anything except the class name and exisiting functions and varibles.

class Node {
public:
	Node* parent;
	vector<Node*> children;
	vector<Move> unvisitedMoves;
	int wins = 0;
	int visits = 0;
	int player;
	bool isLeaf = false;
	Move move;
	Board board;
	Node(Node* parent, Move move, Board board, int player) : parent(parent), move(move), board(board), player(player) {}
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
	bool causeMoreCaptureByOpponent(Board board, Move move, int player);

	void deleteTree(Node* node);
	~MCTS();
};


class StudentAI : public AI {
public:
    Board board;
    Node* root;  // 添加 root 变量

    StudentAI(int col, int row, int p);
    virtual Move GetMove(Move board);
    ~StudentAI();  // 确保释放 root，避免内存泄漏
};


#endif //STUDENTAI_H

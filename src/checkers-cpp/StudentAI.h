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
	bool isForceCapture(const Move &move);
	double isVulnerableMove(Board board, const Move &move, int player);
	bool isPromoting(const Board &board, const Move &move, int player);
	double generalBoardPositionEvaluation(Board board, const Move &move, int player);
	static Node* findChildNode(Node* node, const Move &move);
	static void deleteTree(Node* node);
};


class StudentAI :public AI
{
public:
    Board board;
	Node* MCTSRoot = nullptr;
	StudentAI(int col, int row, int p);
	virtual Move GetMove(Move board);
	~StudentAI();
};

#endif //STUDENTAI_H

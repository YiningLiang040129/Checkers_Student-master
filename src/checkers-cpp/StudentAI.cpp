#include "StudentAI.h"
#include <random>
#include <algorithm>

//The following part should be completed by students.
//The students can modify anything except the class name and exisiting functions and varibles.

// interactive MCTS website: https://vgarciasc.github.io/mcts-viz/
// MCTS algorithm explained: https://gibberblot.github.io/rl-notes/single-agent/mcts.html

MCTS::MCTS(Node* root, Board board, int player) {
    this->root = root;
    this->root->board = board;
    this->root->player = player;
}

bool Node::isFullyExpanded() {
    return isLeaf || (unvisitedMoves.size() == 0);
}

double MCTS::getUCT(Node* node) {
    if (node->visits == 0) {
        return INFINITY;
    }
    return (double)node->wins / node->visits + 2.0 * sqrt(log(node->parent->visits) / (node->visits + 1e-6));
}

double MCTS::evaluateBoard(Board board, int player) {
    int myKings = 0, oppKings = 0, myPieces = 0, oppPieces = 0;
    int myMoves = board.getAllPossibleMoves(player).size();
    int oppMoves = board.getAllPossibleMoves(3 - player).size();
    int myCaptureOpportunities = 0, oppCaptureOpportunities = 0;

    for (const auto& row : board.getBoard()) {
        for (const auto& piece : row) {
            if (piece == player) myPieces++;
            else if (piece == (player + 2)) myKings++;
            else if (piece == (3 - player)) oppPieces++;
            else if (piece == (3 - player + 2)) oppKings++;
        }
    }

    for (const auto& moves : board.getAllPossibleMoves(player)) {
        for (const auto& move : moves) {
            if (move.isCapture()) myCaptureOpportunities++;
        }
    }
    
    for (const auto& moves : board.getAllPossibleMoves(3 - player)) {
        for (const auto& move : moves) {
            if (move.isCapture()) oppCaptureOpportunities++;
        }
    }

    return 5 * (myKings - oppKings) + 
           3 * (myPieces - oppPieces) + 
           1.5 * (myMoves - oppMoves) + 
           2 * (myCaptureOpportunities - oppCaptureOpportunities);
}

int MCTS::simulation(Node* node) {
    Board board = node->board;
    int player = node->player;
    while (!board.getAllPossibleMoves(player).empty()) {
        auto moves = board.getAllPossibleMoves(player);
        Move bestMove;
        double bestScore = -INFINITY;
        for (const auto& moveSet : moves) {
            for (const auto& move : moveSet) {
                double score = evaluateBoard(board, player);
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
            }
        }
        board.makeMove(bestMove, player);
        player = (player == 1 ? 2 : 1);
    }
    return board.isWin(player);
}

void MCTS::runMCTS(int time) {
    for (int i = 0; i < time; i++) {
        Node* selectedNode = selectNode(root);
        Node* expandedNode = expandNode(selectedNode);
        if (expandedNode == nullptr) {
            continue;
        }
        int result = simulation(expandedNode);
        backPropagation(expandedNode, result);
    }
}

Move StudentAI::GetMove(Move move) {
    if (move.seq.empty())
    {
        player = 1;
    } else{
        board.makeMove(move,player == 1?2:1);
    }

    Node* root = new Node(NULL, Move(), board, player);
    MCTS mcts = MCTS(root, board, player);
    mcts.runMCTS(5000);
    Move res = mcts.getBestMove();
    board.makeMove(res, player);
    return res;
}

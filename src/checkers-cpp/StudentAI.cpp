#include "StudentAI.h"
#include <random>

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

// New heuristic evaluation function
// Evaluates board based on piece count, king positions, mobility, and positioning.
double MCTS::evaluateBoard(Board board, int player) {
    double score = 0.0;
    int opponent = (player == 1) ? 2 : 1;
    
    int myPieces = 0, oppPieces = 0;
    int myKings = 0, oppKings = 0;
    int myMoves = 0, oppMoves = 0;
    
    vector<vector<Move>> myPossibleMoves = board.getAllPossibleMoves(player);
    vector<vector<Move>> oppPossibleMoves = board.getAllPossibleMoves(opponent);
    
    myMoves = myPossibleMoves.size();
    oppMoves = oppPossibleMoves.size();
    
    for (int i = 0; i < board.row; i++) {
        for (int j = 0; j < board.col; j++) {
            int piece = board.board[i][j].player; 
            if (piece == player) myPieces++;
            else if (piece == opponent) oppPieces++;
            else if (piece == player + 2) myKings++;
            else if (piece == opponent + 2) oppKings++;
        }
    }
    
    // Scoring weights
    score += 5.0 * (myPieces - oppPieces);
    score += 7.0 * (myKings - oppKings);
    score += 1.5 * (myMoves - oppMoves);
    
    return score;
}

bool MCTS::causeMoreCaptureByOpponent(Board board, Move move, int player) {
    int opponent = player == 1 ? 2 : 1;
    int numCapturesBefore = 0, numCapturesAfter = 0;
    
    vector<vector<Move>> allMovesBefore = board.getAllPossibleMoves(opponent);
    for (vector<Move> moves : allMovesBefore) {
        for (Move m : moves) {
            if (m.isCapture()) numCapturesBefore++;
        }
    }
    
    board.makeMove(move, player);
    vector<vector<Move>> allMovesAfter = board.getAllPossibleMoves(opponent);
    for (vector<Move> moves : allMovesAfter) {
        for (Move m : moves) {
            if (m.isCapture()) numCapturesAfter++;
        }
    }
    
    return numCapturesAfter > numCapturesBefore;
}

int MCTS::simulation(Node* node) {
    Board board = node->board;
    int player = node->player;
    int noCaptureCount = 0;
    
    while (true) {
        vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
        if (noCaptureCount >= 40 || allMoves.empty()) {
            break;
        }
        
        // Select the best move using heuristic evaluation
        double bestScore = -INFINITY;
        Move bestMove;
        
        for (vector<Move> moves : allMoves) {
            for (Move move : moves) {
                Board tempBoard = board;
                tempBoard.makeMove(move, player);
                double score = evaluateBoard(tempBoard, player);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
            }
        }
        
        board.makeMove(bestMove, player);
        noCaptureCount = bestMove.isCapture() ? 0 : noCaptureCount + 1;
        player = player == 1 ? 2 : 1;
    }
    
    int result = board.isWin(node->player);
    return (result == node->player) ? 1 : (result == 0 ? -1 : 0);
}

#include "StudentAI.h"
#include <random>

//The following part should be completed by students.
//The students can modify anything except the class name and existing functions and variables.

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
    double score = 0.0;
    int opponent = (player == 1) ? 2 : 1;
    int myPieces = 0, oppPieces = 0;
    int myKings = 0, oppKings = 0;
    int myMoves = board.getAllPossibleMoves(player).size();
    int oppMoves = board.getAllPossibleMoves(opponent).size();
    
    for (int i = 0; i < board.row; i++) {
        for (int j = 0; j < board.col; j++) {
            int piece = board.getPiece(i, j);
            if (piece == player) myPieces++;
            else if (piece == opponent) oppPieces++;
            else if (piece == player + 2) myKings++;
            else if (piece == opponent + 2) oppKings++;
        }
    }
    
    score += 5.0 * (myPieces - oppPieces);
    score += 7.0 * (myKings - oppKings);
    score += 1.5 * (myMoves - oppMoves);
    
    return score;
}

bool MCTS::leadToForceCapture(Board board, Move move, int player) {
    board.makeMove(move, player);
    vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
    for (const auto& moves : allMoves) {
        for (const Move& m : moves) {
            if (!m.isCapture()) {
                return false;
            }
        }
    }
    return !allMoves.empty();
}

bool MCTS::causeMoreCaptureByOpponent(Board board, Move move, int player) {
    int opponent = player == 1 ? 2 : 1;
    int numCapturesBefore = board.countCapturablePieces(opponent);
    board.makeMove(move, player);
    int numCapturesAfter = board.countCapturablePieces(opponent);
    return numCapturesAfter > numCapturesBefore;
}

bool MCTS::isPromoting(Board board, Move move, int player) {
    Position next_pos = move.seq.back();
    return (player == 1 && next_pos[0] == board.row - 1) || (player == 2 && next_pos[0] == 0);
}

Node* MCTS::selectNode(Node* node) {
    Node *current = node;
    while (current->children.size() > 0 && current->isFullyExpanded()) {
        current = *max_element(current->children.begin(), current->children.end(),
                               [](Node* a, Node* b) { return a->wins / (a->visits + 1e-6) < b->wins / (b->visits + 1e-6); });
    }
    return current;
}

Node* MCTS::expandNode(Node* node) {
    vector<vector<Move>> allMoves = node->board.getAllPossibleMoves(node->player);
    if (allMoves.empty()) {
        node->isLeaf = true;
        return nullptr;
    }
    
    if (node->unvisitedMoves.empty()) {
        for (const auto& moves : allMoves) {
            node->unvisitedMoves.insert(node->unvisitedMoves.end(), moves.begin(), moves.end());
        }
    }
    
    int i = rand() % node->unvisitedMoves.size();
    Move randomMove = node->unvisitedMoves[i];
    node->unvisitedMoves.erase(node->unvisitedMoves.begin() + i);
    
    Board newBoard = node->board;
    newBoard.makeMove(randomMove, node->player);
    Node *newNode = new Node(node, randomMove, newBoard, node->player == 1 ? 2 : 1);
    node->children.push_back(newNode);
    return newNode;
}

int MCTS::simulation(Node* node) {
    Board board = node->board;
    int player = node->player;
    while (true) {
        vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
        if (allMoves.empty()) return board.isWin(player);
        int i = rand() % allMoves.size();
        int j = rand() % allMoves[i].size();
        board.makeMove(allMoves[i][j], player);
        player = (player == 1) ? 2 : 1;
    }
}

void MCTS::runMCTS(int time) {
    for (int i = 0; i < time; i++) {
        Node* selectedNode = selectNode(root);
        Node* expandedNode = expandNode(selectedNode);
        if (!expandedNode) continue;
        int result = simulation(expandedNode);
        backPropagation(expandedNode, result);
    }
}

Move MCTS::getBestMove() {
    return max_element(root->children.begin(), root->children.end(),
                       [](Node* a, Node* b) { return a->visits < b->visits; })->move;
}

void MCTS::deleteTree(Node* node) {
    for (Node *child : node->children) deleteTree(child);
    delete node;
}

MCTS::~MCTS() {
    deleteTree(root);
}

StudentAI::StudentAI(int col, int row, int p) : AI(col, row, p) {
    board = Board(col, row, p);
    board.initializeGame();
    player = 2;
}

Move StudentAI::GetMove(Move move) {
    if (move.seq.empty()) player = 1;
    else board.makeMove(move, player == 1 ? 2 : 1);
    
    Node* root = new Node(NULL, Move(), board, player);
    MCTS mcts(root, board, player);
    mcts.runMCTS(5000);
    Move res = mcts.getBestMove();
    board.makeMove(res, player);
    return res;
}

#include "StudentAI.h"
#include <random>
#include <unordered_map>

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
    return isLeaf || (unvisitedMoves.empty() && visits > 0);
}

double MCTS::getUCT(Node* node) {
    if (node->visits == 0) {
        return INFINITY;
    }
    const double explorationFactor = 1.4; // Fine-tuned exploration factor
    return (double)node->wins / node->visits + explorationFactor * sqrt(log(node->parent->visits) / (node->visits + 1e-6));
}

bool MCTS::causeMoreCaptureByOpponent(Board board, Move move, int player) {
    int opponent = player == 1 ? 2 : 1;
    int numCapturesBefore = 0;
    for (const auto& moves : board.getAllPossibleMoves(opponent)) {
        for (const auto& m : moves) {
            if (m.isCapture()) numCapturesBefore++;
        }
    }
    board.makeMove(move, player);
    int numCapturesAfter = 0;
    for (const auto& moves : board.getAllPossibleMoves(opponent)) {
        for (const auto& m : moves) {
            if (m.isCapture()) numCapturesAfter++;
        }
    }
    return numCapturesAfter > numCapturesBefore;
}

Node* MCTS::selectNode(Node* node) {
    while (!node->children.empty() && node->isFullyExpanded()) {
        node = *max_element(node->children.begin(), node->children.end(),
            [&](Node* a, Node* b) { return getUCT(a) < getUCT(b); });
    }
    return node;
}

Node* MCTS::expandNode(Node* node) {
    if (node->unvisitedMoves.empty()) {
        for (const auto& moves : node->board.getAllPossibleMoves(node->player)) {
            for (const auto& move : moves) {
                node->unvisitedMoves.push_back(move);
            }
        }
    }
    if (node->unvisitedMoves.empty()) return nullptr;
    Move bestMove = node->unvisitedMoves[rand() % node->unvisitedMoves.size()];
    Board newBoard = node->board;
    newBoard.makeMove(bestMove, node->player);
    Node *newNode = new Node(node, bestMove, newBoard, node->player == 1 ? 2 : 1);
    node->children.push_back(newNode);
    node->unvisitedMoves.erase(remove(node->unvisitedMoves.begin(), node->unvisitedMoves.end(), bestMove), node->unvisitedMoves.end());
    return newNode;
}

int MCTS::simulation(Node* node) {
    Board board = node->board;
    int player = node->player;
    while (true) {
        auto allMoves = board.getAllPossibleMoves(player);
        if (allMoves.empty()) break;
        Move bestMove;
        double bestScore = -INFINITY;
        for (const auto& moves : allMoves) {
            for (const auto& move : moves) {
                double score = move.isCapture() ? 2.0 : 1.0;
                if (causeMoreCaptureByOpponent(board, move, player)) score -= 1.0;
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
            }
        }
        board.makeMove(bestMove, player);
        player = player == 1 ? 2 : 1;
    }
    return board.isWin(player);
}

void MCTS::backPropagation(Node* node, int result) {
    while (node) {
        node->visits++;
        if (result == 1) node->wins++;
        else if (result == 0) node->wins--;
        node = node->parent;
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
    return (*max_element(root->children.begin(), root->children.end(),
            [](Node* a, Node* b) { return a->visits < b->visits; }))->move;
}

void MCTS::deleteTree(Node* node) {
    for (Node *child : node->children) deleteTree(child);
    delete node;
}

MCTS::~MCTS() { deleteTree(root); }

StudentAI::StudentAI(int col, int row, int p) : AI(col, row, p) {
    board = Board(col, row, p);
    board.initializeGame();
    player = 2;
}

Move StudentAI::GetMove(Move move) {
    if (!move.seq.empty()) board.makeMove(move, player == 1 ? 2 : 1);
    static Node* root = nullptr;
    if (!root || root->board.getBoardState() != board.getBoardState()) {
        if (root) delete root;
        root = new Node(nullptr, Move(), board, player);
    }
    MCTS mcts(root, board, player);
    mcts.runMCTS(2500); // Increased iteration count for better decision making
    Move res = mcts.getBestMove();
    board.makeMove(res, player);
    return res;
}

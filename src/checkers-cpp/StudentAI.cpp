#include "StudentAI.h"
#include <random>
#include <algorithm>

// The following part should be completed by students.
// The students can modify anything except the class name and existing functions and variables.

MCTS::MCTS(Node* root, Board board, int player) {
    this->root = root;
    this->root->board = board;
    this->root->player = player;
}

bool Node::isFullyExpanded() {
    return isLeaf || (unvisitedMoves.empty());
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
            Checker* piece = board.getChecker(i, j);
            if (piece) {
                if (piece->getPlayer() == player) {
                    myPieces++;
                    if (piece->isKing()) myKings++;
                } else if (piece->getPlayer() == opponent) {
                    oppPieces++;
                    if (piece->isKing()) oppKings++;
                }
            }
        }
    }

    score += 5.0 * (myPieces - oppPieces);
    score += 7.0 * (myKings - oppKings);
    score += 1.5 * (myMoves - oppMoves);

    return score;
}

Node* MCTS::selectNode(Node* node) {
    Node* current = node;
    while (!current->children.empty() && current->isFullyExpanded()) {
        current = *std::max_element(current->children.begin(), current->children.end(),
                                    [](Node* a, Node* b) { return a->wins / a->visits < b->wins / b->visits; });
    }
    return current;
}

Node* MCTS::expandNode(Node* node) {
    std::vector<std::vector<Move>> allMoves = node->board.getAllPossibleMoves(node->player);
    if (allMoves.empty()) {
        node->isLeaf = true;
        return nullptr;
    }

    if (node->unvisitedMoves.empty() && node->visits == 0) {
        for (const auto& moves : allMoves) {
            for (const auto& move : moves) {
                node->unvisitedMoves.push_back(move);
            }
        }
    }

    if (node->unvisitedMoves.empty()) {
        return nullptr;
    }

    int i = rand() % node->unvisitedMoves.size();
    Move randomMove = node->unvisitedMoves[i];

    Board newBoard = node->board;
    newBoard.makeMove(randomMove, node->player);
    Node* newNode = new Node(node, randomMove, newBoard, node->player == 1 ? 2 : 1);
    node->children.push_back(newNode);
    node->unvisitedMoves.erase(node->unvisitedMoves.begin() + i);

    return newNode;
}

int MCTS::simulation(Node* node) {
    Board board = node->board;
    int player = node->player;
    int opponent = (player == 1) ? 2 : 1;

    for (int i = 0; i < 40; i++) {
        std::vector<std::vector<Move>> allMoves = board.getAllPossibleMoves(player);
        if (allMoves.empty()) break;

        Move bestMove;
        double bestScore = -INFINITY;

        for (const auto& moves : allMoves) {
            for (const auto& move : moves) {
                double score = evaluateBoard(board, player);
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
            }
        }

        board.makeMove(bestMove, player);
        player = (player == 1) ? 2 : 1;
    }

    int result = board.isWin(opponent);
    return (result == node->player) ? 1 : (result == opponent) ? 0 : -1;
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
    return (*std::max_element(root->children.begin(), root->children.end(),
                              [](Node* a, Node* b) { return a->visits < b->visits; }))->move;
}

void MCTS::deleteTree(Node* node) {
    for (Node* child : node->children) {
        deleteTree(child);
    }
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
    if (move.seq.empty()) {
        player = 1;
    } else {
        board.makeMove(move, player == 1 ? 2 : 1);
    }

    Node* root = new Node(nullptr, Move(), board, player);
    MCTS mcts(root, board, player);
    mcts.runMCTS(5000);
    Move res = mcts.getBestMove();

    board.makeMove(res, player);
    return res;
}

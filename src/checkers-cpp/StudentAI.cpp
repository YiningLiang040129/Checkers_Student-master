#include "StudentAI.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <random>
#include <iostream>
using namespace std;
using namespace std::chrono;

//----------------------- MCTS and Node functions ----------------------------//

MCTS::MCTS(Node* root, Board &board, int player) {
    this->root = root;
    this->root->board = board;
    this->root->player = player;
}

Node* MCTS::findChildNode(Node* node, const Move &move) {
    for (Node *child : node->children) {
        if (child->move.seq == move.seq) {
            return child;
        }
    }
    return nullptr;
}

// reuse the same tree by re-rooting the tree to the new root and deleting unused nodes
Node* MCTS::reRoot(Node *root, const Move &move) {
    Node *newRoot = MCTS::findChildNode(root, move);
    if (newRoot == nullptr) { // delete the tree if for some reason the new root is not found
        MCTS::deleteTree(root);
        return nullptr;
    } else {
        if (newRoot->parent != nullptr && newRoot != root) { // detach the new root so that deleteTree won't delete newRoot
            vector<Node*>& childrenVector = newRoot->parent->children;
            childrenVector.erase(remove(childrenVector.begin(), childrenVector.end(), newRoot), childrenVector.end());
            newRoot->parent = nullptr;
            MCTS::deleteTree(root);
        }
        return newRoot;
    }
}

// custom win check
// return 1 if black wins, 2 if white wins, -1 if tie, 0 if still playing
int MCTS::checkWin(const Board &board) {
    if (board.tieCount >= board.tieMax) {
        return -1;
    }
    if (board.blackCount == 0) {
        return 2;
    } else if (board.whiteCount == 0) {
        return 1;
    }
    return 0;
}

bool Node::isFullyExpanded() {
    return isLeaf || (unvisitedMoves.size() == 0);
}

double MCTS::getUCT(Node* node) {
    if (node->visits == 0) {
        return INFINITY;
    }
    // Balanced UCT: exploitation + exploration
    return (double)node->wins / node->visits + 1.5 * sqrt(log(node->parent->visits) / (node->visits + 1e-6));
}

bool MCTS::isMultipleCapture(const Move &move) {
    return move.seq.size() > 2;
}

// Evaluate if a move leaves our pieces vulnerable to counter-capture
double MCTS::isVulnerableMove(Board &board, const Move &move, int player) {
    int opponent = (player == 1) ? 2 : 1;

    // Get opponent's possible captures BEFORE making the move
    unordered_set<Position> opponentCapturesBefore;
    vector<vector<Move>> allMovesBefore = board.getAllPossibleMoves(opponent);
    for (const auto &moves : allMovesBefore) {
        for (const auto &m : moves) {
            if (m.isCapture()) {
                opponentCapturesBefore.insert(m.seq.back());
            }
        }
    }

    // Make the move
    board.makeMove(move, player);

    // Get opponent's possible captures AFTER making the move
    unordered_set<Position> opponentCapturesAfter;
    vector<vector<Move>> allMovesAfter = board.getAllPossibleMoves(opponent);
    for (const auto &moves : allMovesAfter) {
        for (const auto &m : moves) {
            if (isMultipleCapture(m)) { 
                board.Undo();
                return -5.0;
            }
            opponentCapturesAfter.insert(m.seq.back());
        }
    }

    board.Undo();

    int newVulnerabilities = opponentCapturesAfter.size() - opponentCapturesBefore.size();
    if (newVulnerabilities > 0) {
        return -2.0 * newVulnerabilities;
    }
    return 1.0;
}

bool MCTS::isPromoting(const Board &board, const Move &move, int player) {
    Position next_pos = move.seq[move.seq.size() - 1];
    if (player == 1 && next_pos[0] == board.row - 1) {
        return true;
    } else if (player == 2 && next_pos[0] == 0) {
        return true;
    }
    return false;
}

double MCTS::generalBoardPositionEvaluation(Board &board, const Move &move, int player) {
    double score = 0.0;
    board.makeMove(move, player);
    
    string playerColor = player == 1 ? "B" : "W";
    double kingScore = 0.7;
    double centerScore = 0.5;
    double edgeScore = 0.3;
    double defensiveScore = 0.2;
    double scoreMultiplier = 0.2 * (board.col / 7);

    for (int i = 0; i < board.row; i++) {
        for (int j = 0; j < board.col; j++) {
            Checker checker = board.board[i][j];
            if (checker.color != "B" && checker.color != "W") {
                continue;
            }
            double currentCheckerScore = 0.0;
            
            if (checker.isKing) {
                currentCheckerScore += kingScore;
            }
            double distanceToCenter = sqrt(pow(i - board.row / 2.0, 2) + pow(j - board.col / 2.0, 2));
            currentCheckerScore += centerScore / (distanceToCenter + 1.0);
            if (j == 0 || j == board.col - 1) {
                currentCheckerScore += edgeScore;
            }
            if ((checker.color == "B" && i == 0) || (checker.color == "W" && i == board.row - 1)) {
                currentCheckerScore += defensiveScore;
            }
            if (checker.color == playerColor) {
                score += currentCheckerScore;
            } else {
                score -= currentCheckerScore;
            }
        }
    }
    
    board.Undo();
    return scoreMultiplier * score;
}

double MCTS::evaluatePieceStructure(Board &board, int player) {
    double structureScore = 0.0;
    string playerColor = player == 1 ? "B" : "W";
    int dr[4] = {-1, -1, 1, 1};
    int dc[4] = {-1, 1, -1, 1};
    
    for (int i = 0; i < board.row; i++) {
        for (int j = 0; j < board.col; j++) {
            Checker checker = board.board[i][j];
            if (checker.color != playerColor)
                continue;
            int friendlyNeighbors = 0;
            for (int d = 0; d < 4; d++) {
                int ni = i + dr[d], nj = j + dc[d];
                if (ni >= 0 && ni < board.row && nj >= 0 && nj < board.col) {
                    if (board.board[ni][nj].color == playerColor) {
                        friendlyNeighbors++;
                    }
                }
            }
            if (friendlyNeighbors > 0) {
                structureScore += 0.3 * friendlyNeighbors;
            }
        }
    }
    return structureScore;
}

Node* MCTS::selectNode(Node* node) {
    Node *current = node;
    while (current->children.size() > 0 && current->isFullyExpanded()) {
        double bestUCTValue = -INFINITY;
        Node *bestChild = nullptr;
        for (Node *child : current->children) {
            double uctValue = getUCT(child);
            if (uctValue > bestUCTValue) {
                bestUCTValue = uctValue;
                bestChild = child;
            }
        }
        current = bestChild;
    }
    return current;
}

Node* MCTS::expandNode(Node* node) {
    vector<vector<Move>> allMoves = node->board.getAllPossibleMoves(node->player);
    if (allMoves.size() == 0) {
        node->isLeaf = true;
        int result = -1;
        int opponent = node->player == 1 ? 2 : 1;
        int winning_player = checkWin(node->board);
        if (winning_player == root->player) {
            result = 1;
        } else if (winning_player == opponent) {
            result = 0;
        } else {
            result = -1;
        }
        backPropagation(node, result);
        return nullptr;
    }
    
    if (!node->initialized) { 
        for (vector<Move> moves : allMoves) {
            for (Move move : moves) {
                node->unvisitedMoves.push_back(move);
            }
        }
        node->initialized = true;
    }
    
    if (node->unvisitedMoves.size() == 0) {
        return nullptr;
    }
    
    int i = rand() % (node->unvisitedMoves.size());
    Move randomMove = node->unvisitedMoves[i];
    
    Board newBoard = node->board;
    newBoard.makeMove(randomMove, node->player);
    Node *newNode = new Node(node, randomMove, newBoard, node->player == 1 ? 2 : 1);
    node->children.push_back(newNode);
    node->unvisitedMoves.erase(node->unvisitedMoves.begin() + i);
    
    return newNode;
}

int MCTS::simulation(Node* node) {
    Board board = node->board;
    int player = node->player;
    int lastMovedPlayer = player;
    int noCaptureCount = 0;
    while (true) {
        vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
        if (noCaptureCount >= 40) {
            return -1;
        }
        if (allMoves.size() == 0) {
            break;
        }
        
        double bestScore = -INFINITY;
        Move bestMove;
        for (vector<Move> moves : allMoves) {
            for (Move move : moves) {
                double score = 0.0;
                if (move.isCapture()) {
                    score += 2.0;
                }
                if (isMultipleCapture(move)) {
                    score += 2.0;
                }
                score += isVulnerableMove(board, move, player);
                if (isPromoting(board, move, player)) {
                    score += 1.0;
                }
                // You may choose to include generalBoardPositionEvaluation if desired:
                // score += generalBoardPositionEvaluation(board, move, player);
                score += evaluatePieceStructure(board, player);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }   
            }
        }
        
        board.makeMove(bestMove, player);
        lastMovedPlayer = player;
        
        if (bestMove.isCapture()) {
            noCaptureCount = 0;
        } else {
            noCaptureCount++;
        }
        
        player = player == 1 ? 2 : 1;
    }
    
    int winning_player = 3 - board.isWin(lastMovedPlayer);
    int opponent = root->player == 1 ? 2 : 1;
    if (winning_player == root->player) {
        return 1;
    } else if (winning_player == opponent) {
        return 0;
    } else {
        return -1;
    }
}

void MCTS::backPropagation(Node* node, int result) {
    Node *current = node;
    double winScore = 0.0;
    if (result == 1) {
        winScore = 1.0;
    } else if (result == 0) {
        winScore = 0.0;
    } else {
        winScore = 0.5;
    }
    while (current != nullptr) {
        current->visits++;
        current->wins += winScore;
        current = current->parent;
    }
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

Move MCTS::getBestMove() { 
    double mostWinsPerVisit = -INFINITY;
    Move bestMove;
    for (Node *child : root->children) {
        double currentWinsPerVisit = child->wins / child->visits;
        if (currentWinsPerVisit > mostWinsPerVisit) {
            mostWinsPerVisit = currentWinsPerVisit;
            bestMove = child->move;
        }
    }
    return bestMove;
}

void MCTS::deleteTree(Node* node) {
    if (node == nullptr) {
        return;
    }
    for (Node *child : node->children) {
        deleteTree(child);
    }
    delete node;
}

StudentAI::StudentAI(int col,int row,int p) : AI(col, row, p) {
    board = Board(col,row,p);
    board.initializeGame();
    player = 2;
}

Move StudentAI::GetRandomMove(Move move) {
    if (move.seq.empty()) {
        player = 1;
    } else {
        board.makeMove(move, player == 1 ? 2 : 1);
    }
    vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
    int i = rand() % allMoves.size();
    vector<Move> checker_moves = allMoves[i];
    int j = rand() % checker_moves.size();
    board.makeMove(checker_moves[j], player);
    return checker_moves[j];
}

Move StudentAI::GetMove(Move move) {
    auto start = high_resolution_clock::now();
    auto remainingTime = timeLimit - timeElapsed;
    if (remainingTime < seconds(4)) {
        return GetRandomMove(move);
    }
    if (move.seq.empty()) {
        player = 1;
    } else {
        board.makeMove(move, player == 1 ? 2 : 1);
        if (MCTSRoot) {
            MCTSRoot = MCTS::reRoot(MCTSRoot, move);
        }
    }
    if (MCTSRoot == nullptr) {
        MCTSRoot = new Node(nullptr, Move(), board, player);
    }
    MCTS mcts = MCTS(MCTSRoot, board, player);
    mcts.runMCTS(1000); // fixed number of iterations
    Move res = mcts.getBestMove();
    board.makeMove(res, player);
    MCTSRoot = MCTS::reRoot(MCTSRoot, res);
    auto stop = high_resolution_clock::now();
    timeElapsed += duration_cast<milliseconds>(stop - start);
    return res;
}

StudentAI::~StudentAI() {
    if (MCTSRoot != nullptr) {
        MCTS::deleteTree(MCTSRoot);
    }
}

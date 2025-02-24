#include "StudentAI.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <random>
#include <iostream>
using namespace std;
using namespace std::chrono;

//----------------------- MCTS and Node functions ----------------------------//

// MCTS Constructor
MCTS::MCTS(Node* root, Board &board, int player) {
    this->root = root;
    this->root->board = board;
    this->root->player = player;
}

// Find child node matching the move
Node* MCTS::findChildNode(Node* node, const Move &move) {
    for (Node *child : node->children) {
        if (child->move.seq == move.seq) {
            return child;
        }
    }
    return nullptr;
}

// Re-root: reuse tree by setting a new root and deleting unused nodes
Node* MCTS::reRoot(Node *root, const Move &move) {
    Node *newRoot = MCTS::findChildNode(root, move);
    if (newRoot == nullptr) { // not found, delete whole tree
        MCTS::deleteTree(root);
        return nullptr;
    } else {
        if (newRoot->parent != nullptr && newRoot != root) {
            vector<Node*>& childrenVector = newRoot->parent->children;
            childrenVector.erase(remove(childrenVector.begin(), childrenVector.end(), newRoot), childrenVector.end());
            newRoot->parent = nullptr;
            MCTS::deleteTree(root);
        }
        return newRoot;
    }
}

// Check if a node is fully expanded
bool Node::isFullyExpanded() {
    return isLeaf || (unvisitedMoves.size() == 0);
}

// UCT value for a node (c constant is tuned to 1.5 here)
double MCTS::getUCT(Node* node) {
    if (node->visits == 0) {
        return INFINITY;
    }
    return (double)node->wins / node->visits + 1.5 * sqrt(log(node->parent->visits) / (node->visits + 1e-6));
}

// Return true if the move is a multiple capture (i.e. has more than 2 positions)
bool MCTS::isMultipleCapture(const Move &move) {
    return move.seq.size() > 2;
}

// Evaluate if a move leaves our pieces vulnerable to counter-capture
double MCTS::isVulnerableMove(Board &board, const Move &move, int player) {
    int opponent = player == 1 ? 2 : 1;
    int numCapturesBefore = 0;
    vector<vector<Move>> allMovesBefore = board.getAllPossibleMoves(opponent);
    for (auto moves : allMovesBefore) {
        for (auto m : moves) {
            if (m.isCapture()) {
                numCapturesBefore++;
            }
        }
    }
    
    board.makeMove(move, player);
    
    int numCapturesAfter = 0;
    vector<vector<Move>> allMoves = board.getAllPossibleMoves(opponent);
    for (auto moves : allMoves) {
        for (auto m : moves) {
            if (isMultipleCapture(m)) { // heavy penalty if opponent can chain capture
                board.Undo();
                return -5.0;
            }
            numCapturesAfter++;
        }
    }
    
    board.Undo();
    
    if (numCapturesAfter > numCapturesBefore) {
        return -2.0; // penalty if move increases opponent’s capture opportunities
    }
    
    return 1.0;
}

// Check if the move results in a promotion (to King)
bool MCTS::isPromoting(const Board &board, const Move &move, int player) {
    Position next_pos = move.seq[move.seq.size() - 1];
    if (player == 1 && next_pos[0] == board.row - 1) {
        return true;
    } else if (player == 2 && next_pos[0] == 0) {
        return true;
    }
    return false;
}

// Evaluate board position after a move using several heuristics:
// king bonus, central control (with extra bonus if very close to center),
// edge and defensive positioning.
double MCTS::generalBoardPositionEvaluation(Board &board, const Move &move, int player) {
    double score = 0.0;
    board.makeMove(move, player);
    
    string playerColor = player == 1 ? "B" : "W";
    double kingScore = 0.7;
    double centerScore = 0.5;
    double edgeScore = 0.3;
    double defensiveScore = 0.2;
    double scoreMultiplier = 0.5 * (board.col / 7);
    
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
            // Extra bonus for being in the central area
            if(distanceToCenter < board.row / 4.0) {
                currentCheckerScore += 0.3;
            }
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

// New heuristic: Evaluate piece structure by rewarding pieces that have friendly neighbors.
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

// Traverse the tree to select a promising node using UCT.
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

// Expand the node by selecting an unvisited move and adding a new child.
Node* MCTS::expandNode(Node* node) {
    vector<vector<Move>> allMoves = node->board.getAllPossibleMoves(node->player);
    if (allMoves.size() == 0) {
        node->isLeaf = true;
        int result = -1;
        int opponent = node->player == 1 ? 2 : 1;
        int winning_player = 3 - node->board.isWin(opponent);
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
        for (auto moves : allMoves) {
            for (auto move : moves) {
                node->unvisitedMoves.push_back(move);
            }
        }
        node->initialized = true;
    }
    
    if (node->unvisitedMoves.size() == 0) {
        return nullptr;
    }
    
    int i = rand() % node->unvisitedMoves.size();
    Move randomMove = node->unvisitedMoves[i];
    
    Board newBoard = node->board;
    newBoard.makeMove(randomMove, node->player);
    Node *newNode = new Node(node, randomMove, newBoard, node->player == 1 ? 2 : 1);
    node->children.push_back(newNode);
    node->unvisitedMoves.erase(node->unvisitedMoves.begin() + i);
    
    return newNode;
}

// Simulation: play moves until terminal state is reached.
// Improvements include defensive penalties, promotion bonus adjustments,
// board evaluation and piece structure heuristics.
int MCTS::simulation(Node* node) {
    Board board = node->board;
    int player = node->player;
    int lastMovedPlayer = player;
    int noCaptureCount = 0;
    
    while (true) {
        vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
        if (noCaptureCount >= 40) { // tie detection to prevent infinite play
            return -1;
        }
        if (allMoves.size() == 0) {
            break;
        }
        
        double bestScore = -INFINITY;
        Move bestMove;
        for (auto moves : allMoves) {
            for (auto move : moves) {
                double score = 0.0;
                // Direct capture bonus
                if (move.isCapture()) {
                    score += 2.0;
                }
                // Bonus for multiple captures
                if (isMultipleCapture(move)) {
                    score += 2.0;
                }
                // Apply defensive penalty/reward based on vulnerability
                score += isVulnerableMove(board, move, player);
                
                // Adjust promotion bonus dynamically (more weight in endgame)
                double promotionBonus = 1.0;
                // Manually count the remaining pieces on the board
                int totalPieces = 0;
                for (int i = 0; i < board.row; i++) {
                    for (int j = 0; j < board.col; j++) {
                        if (board.board[i][j].color == "B" || board.board[i][j].color == "W") {
                            totalPieces++;
                        }
                    }
                }
                
                // Adjust promotion importance dynamically
                double promotionBonus = 1.0;
                if (totalPieces < 8) { // Late game, promote bonus is higher
                    promotionBonus = 1.5;
                }
                if (isPromoting(board, move, player)) {
                    score += promotionBonus;
                }
                
                // Evaluate board position (includes center control)
                score += generalBoardPositionEvaluation(board, move, player);
                // Reward piece structure (mutual protection)
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
        
        player = (player == 1 ? 2 : 1);
    }
    
    int winning_player = 3 - board.isWin(lastMovedPlayer);
    int opponent = (root->player == 1 ? 2 : 1);
    if (winning_player == root->player) {
        return 1;
    } else if (winning_player == opponent) {
        return 0;
    } else {
        return -1;
    }
}

// Backpropagation: update wins/visits up the tree (corrected logic).
void MCTS::backPropagation(Node* node, int result) {
    Node *current = node;
    while (current != nullptr) {
        current->visits++;
        if (current->player == root->player) {
            if (result == 1) {
                current->wins += 1;
            } else if (result == 0) {
                current->wins -= 1;
            }
        } else {
            if (result == 1) {
                current->wins -= 1;
            } else if (result == 0) {
                current->wins += 1;
            }
        }
        current = current->parent;
    }
}

// Run MCTS for a fixed number of iterations.
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

// Choose best move based on wins/visits ratio.
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

// Delete tree recursively to free memory.
void MCTS::deleteTree(Node* node) {
    if (node == nullptr) {
        return;
    }
    for (Node *child : node->children) {
        deleteTree(child);
    }
    delete node;
}

//----------------------- StudentAI implementation ----------------------------//

StudentAI::StudentAI(int col, int row, int p) : AI(col, row, p) {
    board = Board(col, row, p);
    board.initializeGame();
    player = 2;
}

// Fallback to a random move if time is nearly up.
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

// Main GetMove: re-root the MCTS tree if possible, run iterations, and return the best move.
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
    mcts.runMCTS(1000); // Number of iterations (adjust if needed)
    Move res = mcts.getBestMove();
    board.makeMove(res, player);
    MCTSRoot = MCTS::reRoot(MCTSRoot, res);
    
    auto stop = high_resolution_clock::now();
    timeElapsed += duration_cast<milliseconds>(stop - start);
    return res;
}

// Destructor: free the MCTS tree.
StudentAI::~StudentAI() {
    if (MCTSRoot != nullptr) {
        MCTS::deleteTree(MCTSRoot);
    }
}

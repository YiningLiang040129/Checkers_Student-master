#include "StudentAI.h"
#include <random>
#include <algorithm>
#include <cmath> // for sqrt, log, INFINITY
#include <iostream>

//The following part should be completed by students.
//The students can modify anything except the class name and existing functions and variables.

// ============ Node =============

bool Node::isFullyExpanded()
{
    // a node is "fully expanded" if it has no more unvisited moves
    // OR if it is a leaf node (no possible moves).
    return isLeaf || (unvisitedMoves.empty());
}

// ============ MCTS =============

MCTS::MCTS(Node* root, Board board, int player)
{
    this->root = root;
    this->root->board = board;
    this->root->player = player;
}

// Return a pointer to the child node whose move matches 'move'
Node* MCTS::findChildNode(Node* node, const Move &move)
{
    for (Node* child : node->children) {
        if (child->move.seq == move.seq) {
            return child;
        }
    }
    return nullptr;
}

// Expand a node by generating possible moves
Node* MCTS::expandNode(Node* node)
{
    // get all possible moves for node->player
    vector<vector<Move>> allMoves = node->board.getAllPossibleMoves(node->player);
    if (allMoves.empty()) {
        // It's a leaf node with no moves, so mark isLeaf = true
        node->isLeaf = true;

        // [MODIFIED] Instead of returning here, we do a small shortcut:
        // For a terminal node, we figure out the result and do backPropagation
        int opponent = (node->player == 1 ? 2 : 1);
        int winning_player = node->board.isWin(node->player);

        // We'll encode:
        //   1.0 => root player wins
        //   0.0 => root player loses
        //   0.5 => tie
        double result = 0.5;  // default tie
        if (winning_player == root->player) {
            result = 1.0;     // root player wins
        } else if (winning_player == opponent) {
            result = 0.0;     // root player loses
        }

        backPropagation(node, result);
        return nullptr;
    }

    // If node not yet initialized, store all possible moves
    if (!node->initialized) {
        for (auto &movesPerChecker : allMoves) {
            for (auto &m : movesPerChecker) {
                node->unvisitedMoves.push_back(m);
            }
        }
        node->initialized = true;
    }

    // If no unvisited moves left, it's fully expanded
    if (node->unvisitedMoves.empty()) {
        return nullptr; // nothing new to expand
    }

    // Randomly select an unvisited move
    int i = rand() % node->unvisitedMoves.size();
    Move randomMove = node->unvisitedMoves[i];

    // Create a child node
    Board newBoard = node->board;
    newBoard.makeMove(randomMove, node->player);
    int nextPlayer = (node->player == 1 ? 2 : 1);

    Node* newNode = new Node(node, randomMove, newBoard, nextPlayer);
    node->children.push_back(newNode);

    // Remove chosen move from unvisited
    node->unvisitedMoves.erase(node->unvisitedMoves.begin() + i);

    return newNode; 
}

// UCT formula
double MCTS::getUCT(Node* node)
{
    if (node->visits == 0) {
        return INFINITY;
    }
    double winRate = node->wins / node->visits;
    double exploreTerm = sqrt(log(node->parent->visits) / (node->visits + 1e-6));
    double c = 1.5; // exploration constant
    return winRate + c * exploreTerm;
}

// Select node by following UCT until we reach a node that is not fully expanded
Node* MCTS::selectNode(Node* node)
{
    Node* current = node;
    // go down the tree while the node is fully expanded & has children
    while (!current->children.empty() && current->isFullyExpanded()) {
        double bestUCTValue = -INFINITY;
        Node* bestChild = nullptr;
        for (Node* child : current->children) {
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

// Mark if a move has multiple captures
bool MCTS::isForceCapture(const Move &move)
{
    return (move.seq.size() > 2);
}

double MCTS::isVulnerableMove(Board board, const Move &move, int player)
{
    // Implementation from your original code:
    int opponent = (player == 1 ? 2 : 1);

    int numCapturesBefore = 0;
    vector<vector<Move>> beforeMoves = board.getAllPossibleMoves(opponent);
    for (auto &mvPerChecker : beforeMoves) {
        for (auto &m : mvPerChecker) {
            if (m.isCapture()) {
                numCapturesBefore++;
            }
        }
    }

    board.makeMove(move, player);

    int numCapturesAfter = 0;
    vector<vector<Move>> afterMoves = board.getAllPossibleMoves(opponent);
    for (auto &mvPerChecker : afterMoves) {
        for (auto &m : mvPerChecker) {
            if (isForceCapture(m)) {
                // returning -5.0 if opponent can force capture after
                return -5.0;
            }
            numCapturesAfter++;
        }
    }

    if (numCapturesAfter > numCapturesBefore) {
        // penalty if the move opens new captures for the opponent
        return -2.0;
    }
    return 1.0;
}

bool MCTS::isPromoting(const Board &board, const Move &move, int player)
{
    Position next_pos = move.seq.back();
    if (player == 1 && next_pos[0] == board.row - 1) {
        return true;
    } 
    else if (player == 2 && next_pos[0] == 0) {
        return true;
    }
    return false;
}

double MCTS::generalBoardPositionEvaluation(Board board, const Move &move, int player)
{
    board.makeMove(move, player);

    // Your original scoring logic
    string playerColor = (player == 1 ? "B" : "W");
    double kingScore = 0.7;
    double centerScore = 0.5;
    double edgeScore = 0.3;
    double defensiveScore = 0.2;
    double scoreMultiplier = 0.5 * (board.col / 7.0);

    double score = 0.0;
    for (int i = 0; i < board.row; i++) {
        for (int j = 0; j < board.col; j++) {
            Checker checker = board.board[i][j];
            if (checker.color != "B" && checker.color != "W") {
                continue; 
            }
            double cScore = 0.0;
            if (checker.isKing) {
                cScore += kingScore;
            }
            double dx = (double)i - (board.row / 2.0);
            double dy = (double)j - (board.col / 2.0);
            double distToCenter = sqrt(dx*dx + dy*dy);
            cScore += centerScore / (distToCenter + 1.0);

            if (j == 0 || j == board.col - 1) {
                cScore += edgeScore;
            }
            // Defensive positions
            if (checker.color == "B" && i == 0) {
                cScore += defensiveScore;
            } else if (checker.color == "W" && i == board.row - 1) {
                cScore += defensiveScore;
            }

            if (checker.color == playerColor) {
                score += cScore;
            } else {
                score -= cScore;
            }
        }
    }
    return scoreMultiplier * score;
}

// ============== Simulation ==============
// [MODIFIED] Return 1.0 if root player eventually wins, 0.0 if root player eventually loses, 0.5 if tie
double MCTS::simulation(Node* node)
{
    Board simBoard = node->board;
    int simPlayer = node->player;
    int lastMovedPlayer = simPlayer;

    int noCaptureCount = 0;
    while (true) {
        // check if it’s a tie by the board’s tieCount or no moves
        if (noCaptureCount >= 40) {
            // a tie
            return 0.5;
        }

        vector<vector<Move>> allMoves = simBoard.getAllPossibleMoves(simPlayer);
        if (allMoves.empty()) {
            // No moves => game ends
            break;
        }

        // pick a "best" move by your heuristic
        double bestScore = -INFINITY;
        Move bestMove;
        for (auto &movesPerChecker : allMoves) {
            for (auto &m : movesPerChecker) {
                double score = 0.0;
                if (m.isCapture()) {
                    score += 2.0;
                }
                if (isForceCapture(m)) {
                    score += 2.0;
                }
                if (isPromoting(simBoard, m, simPlayer)) {
                    score += 1.0;
                }
                // Evaluate board position after the move
                score += generalBoardPositionEvaluation(simBoard, m, simPlayer);

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = m;
                }
            }
        }

        // make the chosen move
        simBoard.makeMove(bestMove, simPlayer);
        lastMovedPlayer = simPlayer;

        if (bestMove.isCapture()) {
            noCaptureCount = 0;
        } else {
            noCaptureCount++;
        }

        // switch player
        simPlayer = (simPlayer == 1 ? 2 : 1);
    }

    // check who won
    int result = simBoard.isWin(lastMovedPlayer);
    int rootOpponent = (root->player == 1 ? 2 : 1);

    // [MODIFIED] unify to: 1.0 => root player wins, 0.0 => root player loses, 0.5 => tie
    if (result == root->player) {
        return 1.0;
    } 
    else if (result == rootOpponent) {
        return 0.0;
    }
    return 0.5; 
}

// ============== Backpropagation ==============
void MCTS::backPropagation(Node* node, double result)
{
    // 'result' is from the perspective of the root player
    //   1.0 => root player wins
    //   0.0 => root player loses
    //   0.5 => tie
    Node* current = node;
    while (current != nullptr) {
        current->visits++;

        // If current->player == root->player => result is "direct" for them
        if (current->player == root->player) {
            // If result=1 => root wins => +1
            // If result=0 => root loses => -1
            // If tie => maybe +0.5 or +0?
            if (result == 1.0) {
                current->wins += 1.0;
            } else if (result == 0.0) {
                current->wins -= 1.0;
            } else {
                // for tie, you can do +0 or +0.5 depending on preference
                current->wins += 0.5; 
            }
        } 
        else {
            // If current->player is the opponent
            // If result=1 => root wins => that means -1 for the opponent
            // If result=0 => root loses => +1 for the opponent
            // If tie => maybe +0.5 for them as well
            if (result == 1.0) {
                current->wins -= 1.0;
            } else if (result == 0.0) {
                current->wins += 1.0;
            } else {
                current->wins += 0.5;
            }
        }
        current = current->parent;
    }
}

// ============== runMCTS / getBestMove ==============

void MCTS::runMCTS(int time)
{
    for (int i = 0; i < time; i++) {
        Node* selectedNode = selectNode(root);
        Node* expandedNode = expandNode(selectedNode);
        if (expandedNode == nullptr) {
            // If expandNode() returned nullptr, it may be a leaf node,
            // so we skip to next iteration.
            continue;
        }
        double result = simulation(expandedNode);
        backPropagation(expandedNode, result);
    }
}

// picking the best move by highest average wins/visits
Move MCTS::getBestMove()
{
    double bestScore = -INFINITY;
    Move bestMove;
    for (Node *child : root->children) {
        if (child->visits == 0) {
            continue;
        }
        double avgScore = child->wins / child->visits;
        if (avgScore > bestScore) {
            bestScore = avgScore;
            bestMove = child->move;
        }
    }
    return bestMove;
}

void MCTS::deleteTree(Node* node)
{
    if (!node) return;
    for (Node* child : node->children) {
        deleteTree(child);
    }
    delete node;
}

// ============== StudentAI ==============

StudentAI::StudentAI(int col,int row,int p) : AI(col, row, p)
{
    board = Board(col,row,p);
    board.initializeGame();
    player = 2;
}

Move StudentAI::GetMove(Move move)
{
    // If the opponent's Move is empty => we are player1
    if (move.seq.empty()) {
        player = 1;
    } 
    else {
        // Apply opponent's move
        board.makeMove(move, (player == 1 ? 2 : 1));

        // Re-root the MCTS tree if possible
        if (MCTSRoot != nullptr) {
            Node* newRoot = MCTS::findChildNode(MCTSRoot, move);
            if (newRoot == nullptr) {
                // Opponent's move not found => must build a new tree
                MCTS::deleteTree(MCTSRoot);
                MCTSRoot = nullptr;
            }
            else {
                // Detach newRoot from old root
                if (newRoot->parent != nullptr && newRoot != MCTSRoot) {
                    vector<Node*>& siblings = newRoot->parent->children;
                    auto it = find(siblings.begin(), siblings.end(), newRoot);
                    if (it != siblings.end()) {
                        siblings.erase(it);
                    }
                    newRoot->parent = nullptr;
                    MCTS::deleteTree(MCTSRoot);
                }
                MCTSRoot = newRoot;
            }
        }
    }

    // If we have no MCTSRoot, create it
    if (MCTSRoot == nullptr) {
        MCTSRoot = new Node(nullptr, Move(), board, player);
    }

    // Build/Run MCTS
    MCTS mcts(MCTSRoot, board, player);
    mcts.runMCTS(1000); // iterations => can adjust as needed

    // Get the best move from MCTS
    Move res = mcts.getBestMove();

    // Apply that move to our board
    board.makeMove(res, player);

    // Now re-root the tree to that move
    Node* newRoot = MCTS::findChildNode(MCTSRoot, res);
    if (newRoot == nullptr) {
        // If for some reason can't find the new root, discard old tree
        MCTS::deleteTree(MCTSRoot);
        MCTSRoot = nullptr;
    } 
    else {
        // detach
        if (newRoot->parent != nullptr && newRoot != MCTSRoot) {
            auto & siblings = newRoot->parent->children;
            auto it = find(siblings.begin(), siblings.end(), newRoot);
            if (it != siblings.end()) {
                siblings.erase(it);
            }
            newRoot->parent = nullptr;
            MCTS::deleteTree(MCTSRoot);
        }
        MCTSRoot = newRoot;
    }

    return res;
}

StudentAI::~StudentAI()
{
    if (MCTSRoot != nullptr) {
        MCTS::deleteTree(MCTSRoot);
    }
}

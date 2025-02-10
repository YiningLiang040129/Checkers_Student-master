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

bool Node::isFullyExpanded() { // a node is fully expanded if all children have been visited or the node is a leaf node
    return isLeaf || (unvisitedMoves.size() == 0 && visits > 0);
}

double MCTS::getUCT(Node* node) {
    if (node->visits == 0) {
        return INFINITY;
    }
    // TODO: fine tune the UCT formula?
    return (double)node->wins / node->visits + sqrt(2) * sqrt(log(node->parent->visits) / (node->visits + 1e-6));
}

// check if player's move will cause more captures by opponent
// TODO: have not considered force capture yet
bool MCTS::causeMoreCaptureByOpponent(Board board, Move move, int player) {
    int opponent = player == 1 ? 2 : 1;

    int numCapturesBefore = 0;
    vector<vector<Move>> allMovesBefore = board.getAllPossibleMoves(opponent); // get all possible opponent moves
    for (vector<Move> moves : allMovesBefore) {
        for (Move m : moves) {
            if (m.isCapture()) { // calculate the total number of captures that opponent can make before current player's move
                numCapturesBefore++;
            }
        }
    }

    board.makeMove(move, player); // player make the move

    int numCapturesAfter = 0;
    vector<vector<Move>> allMovesAfter = board.getAllPossibleMoves(opponent);
    for (vector<Move> moves : allMovesAfter) {
        for (Move m : moves) {
            if (m.isCapture()) { // calculate the total number of captures that opponent can make after current player's move
                numCapturesAfter++;
            }
        }
    }

    return numCapturesAfter > numCapturesBefore; // return true if the opponent can capture more peices after the move
}

Node* MCTS::selectNode(Node* node) {
    Node *current = node;
    // repeatedly going down the tree until the current node has no children or is not fully expanded
    while (current->children.size() > 0 && current->isFullyExpanded()) {
        double bestUCTValue = -INFINITY;
        Node *bestChild = nullptr;
        // iterate all the children and find the one with the best UCT value
        for (Node *child : current->children) {
            double uctValue = getUCT(child);
            if (uctValue > bestUCTValue) { // && !child->isLeaf
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
    if (allMoves.size() == 0) { // if the node has no possible moves, then its a leaf node
        node->isLeaf = true;
        return nullptr;
    }

    // if the node has not been visited and has empty unvisitedMoves, then save all possible moves
    if (node->unvisitedMoves.size() == 0 && node->visits == 0) {
        for (vector<Move> moves : allMoves) {
            for (Move move : moves) {
                node->unvisitedMoves.push_back(move);
            }
        }
    }

    if (node->unvisitedMoves.size() == 0) { // if still has no unvisited moves, then it's fully expanded
        return nullptr;
    }

    // randomly select an unvisited move
    int i = rand() % (node->unvisitedMoves.size());
    Move randomMove = node->unvisitedMoves[i];

    // create a new node for the selected move
    Board newBoard = node->board;
    newBoard.makeMove(randomMove, node->player);
    Node *newNode = new Node(node, randomMove, newBoard, node->player == 1 ? 2 : 1);
    node->children.push_back(newNode);

    // remove the selected move from the unvisitedMoves
    node->unvisitedMoves.erase(node->unvisitedMoves.begin() + i);

    return newNode; // returns the expanded node
}

int MCTS::simulation(Node* node) {
    Board board = node->board;
    int player = node->player;
    int noCaptureCount = 0;
    while (true) {
        vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
        if (allMoves.size() == 0) { // stops simulation if a player has no possible moves
            break;
        }
        if (noCaptureCount >= 40) { // stops simulation if no capture moves have been made for 40 turns (prevent infinite loop, also 40 is the tie count)
            return -1;
        }

        // rollouts
        // TODO: need better heurstics?
        double bestScore = -INFINITY;
        Move bestMove;
        for (vector<Move> moves : allMoves) {
            for (Move move : moves) {
                double score = 0.0;
                if (move.isCapture()) { // direct capture
                    score += 2.0;
                }
                if (causeMoreCaptureByOpponent(board, move, player)) { // check if move leads to more captures by opponent
                    score -= 1.0;
                    // if (score < 0.0) {
                    //     score = 0.0;
                    // }
                } else {
                    score += 1.0;
                }

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
            }
        }
        board.makeMove(bestMove, player);
        if (bestMove.isCapture()) { // count the number of non capture moves
            noCaptureCount = 0;
        } else {
            noCaptureCount++;
        }

        // else { // simulate random moves for the opponent
        //     int i = rand() % (allMoves.size());
        //     vector<Move> checker_moves = allMoves[i];
        //     int j = rand() % (checker_moves.size());
        //     Move randomMove = checker_moves[j];
        //     board.makeMove(randomMove, player);
        // }

        player = player == 1 ? 2 : 1;
    }

    int result = board.isWin(player);
    int opponent = player == 1 ? 2 : 1;
    if (result == root->player) { // return 1 if the root player wins
        return 1;
    } else if (result == opponent) { // return 0 if the root player loses
        return 0;
    }
    // else return -1 if it's a tie
    return -1;
}

void MCTS::backPropagation(Node* node, int result) {
    Node *current = node;
    while (current != nullptr) { // repeatedly going up the tree and update wins and visits
        current->visits++;

        // // if result == 1, then +1 win for the root player, +0 win for the other player
        // current->wins += (node->player == root->player ? result : 1 - result);

        // TODO: adjust the backpropagation details
        if (node->player == root->player) {
            if (result == 1) { // +1 if current player is root and wins
                current->wins += 1;
            } else if (result == 0) { // -1 for losing
                current->wins -= 1;
            } // 0 for tie  TODO: though kask said we should consider tie as a win?
        } else {
            if (result == 1) { // -1 for losing to player (as the opponent)
                current->wins -= 1;
            } else if (result == 0) { // +1 for winning (as the opponent)
                current->wins += 1;
            }
        }

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

// picking the best move based the the most visit
Move MCTS::getBestMove() {
    // double bestUCT = -INFINITY;
    // Move bestMove;
    // for (Node *child : root->children) { // calculate the beest UCT value among the children of the root node
    //     double UCT = getUCT(child);
    //     if (UCT > bestUCT) {
    //         bestUCT = UCT;
    //         bestMove = child->move;
    //     }
    // }

    int maxVisits = -1;
    Move bestMove;
    for (Node *child : root->children) { // find the child with the most visits
        if (child->visits > maxVisits) {
            maxVisits = child->visits;
            bestMove = child->move;
        }
    }

    return bestMove;
}

void MCTS::deleteTree(Node* node) { // TODO: check memory leak?
    for (Node *child : node->children) {
        deleteTree(child);
    }
    delete node;
}

MCTS::~MCTS() {
    deleteTree(root);
}


StudentAI::StudentAI(int col,int row,int p) : AI(col, row, p) {
    board = Board(col,row,p);
    board.initializeGame();
    player = 2;
}

Move StudentAI::GetMove(Move move) {
    if (move.seq.empty())
    {
        player = 1;
    } else{
        board.makeMove(move,player == 1?2:1);
    }

    // TODO: persist and reuse the previous tree?
    Node* root = new Node(NULL, Move(), board, player);
    MCTS mcts = MCTS(root, board, player);
    mcts.runMCTS(1000); // TODO: adjust the number of MCTS iterations
    Move res = mcts.getBestMove();

    board.makeMove(res, player);
    return res;
}


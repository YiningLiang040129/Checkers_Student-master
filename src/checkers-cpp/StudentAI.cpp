#include "StudentAI.h"

//The following part should be completed by students.
//The students can modify anything except the class name and exisiting functions and varibles.

// interactive MCTS website: https://vgarciasc.github.io/mcts-viz/
// MCTS algorithm explained: https://gibberblot.github.io/rl-notes/single-agent/mcts.html

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

// reuse the same tree by re-rooting the tree to the new root and delete the old parts of the tree
Node* MCTS::reRoot(Node *root, const Move &move) {
    Node *newRoot = MCTS::findChildNode(root, move);
    if (newRoot == nullptr) { // delete the tree if for some reason the new root is not found
        MCTS::deleteTree(root);
        return nullptr;
    } else {
        if (newRoot->parent != nullptr && newRoot != root) { // detach the new root from the tree so that deleteTree won't delete newRoot
            vector<Node*>& childrenVector = newRoot->parent->children;
            childrenVector.erase(remove(childrenVector.begin(), childrenVector.end(), newRoot), childrenVector.end());
            newRoot->parent = nullptr;
            MCTS::deleteTree(root);
        }
        return newRoot;
    }
}

bool Node::isFullyExpanded() { // a node is fully expanded if all children have been visited or the node is a leaf node (or is it called terminal node?)
    return isLeaf || (unvisitedMoves.size() == 0); //  && visits > 0
}

double MCTS::getUCT(Node* node) {
    if (node->visits == 0) {
        return INFINITY;
    }
    // TODO: fine tune the c constant value
    return (double)node->wins / node->visits + 1.5 * sqrt(log(node->parent->visits) / (node->visits + 1e-6));
}

// force capture that can capture multiple pieces
bool MCTS::isMultipleCapture(const Move &move) {
    if (move.seq.size() > 2) { // if move has 3 positions, then it's a 2+ capture move
        return true;
    }

    return false;
}

// check if player's move can be captured by opponent
double MCTS::isVulnerableMove(Board &board, const Move &move, int player) {
    int opponent = player == 1 ? 2 : 1;

    int numCapturesBefore = 0;
    vector<vector<Move>> allMovesBefore = board.getAllPossibleMoves(opponent);
    for (vector<Move> moves : allMovesBefore) { // count the number of vulnerable pieces before the move
        for (Move m : moves) {
            if (m.isCapture()) {
                numCapturesBefore++;
            }
        }
    }

    board.makeMove(move, player);

    int numCapturesAfter = 0;
    vector<vector<Move>> allMoves = board.getAllPossibleMoves(opponent);
    for (vector<Move> moves : allMoves) {
        for (Move m : moves) {
            if (isMultipleCapture(m)) { // it's a REALLY bad move if opponent can capture multiple pieces
                board.Undo();
                return -5.0;
            }
            numCapturesAfter++;
        }
    }

    board.Undo();

    if (numCapturesAfter > numCapturesBefore) {
        return -2.0; // penalty if move pieces are vulnerable after the move
    }

    return 1.0;
}

// check if a move will cause promote
bool MCTS::isPromoting(const Board &board, const Move &move, int player) {
    Position next_pos = move.seq[move.seq.size() - 1];
    if (player == 1 && next_pos[0] == board.row - 1) { // player 1 promotes if reaches the last row
        return true;
    } else if (player == 2 && next_pos[0] == 0) { // player 2 promotes if reaches the first row
        return true;
    }
    return false;
}

// evaluate the board position after the momve and returns the score
double MCTS::generalBoardPositionEvaluation(Board &board, const Move &move, int player) {
    double score = 0.0;
    board.makeMove(move, player);

    // TODO: fine tune the scores
    string playerColor = player == 1 ? "B" : "W";
    double kingScore = 0.7;
    double centerScore = 0.5;
    double edgeScore = 0.3;
    double defensiveScore = 0.2;
    double scoreMultiplier = 0.5 * (board.col / 7);

    for (int i = 0; i < board.row; i++) { // iterating the board and give points based on player and opponents position
        for (int j = 0; j < board.col; j++) {
            Checker checker = board.board[i][j];
            if (checker.color != "B" && checker.color != "W") { // skip empty positions
                continue;
            }

            double currentCheckerScore = 0.0;
            
            if (checker.isKing) { // give extra score for king
                currentCheckerScore += kingScore;
            }

            double distanceToCenter = sqrt(pow(i - board.row / 2.0, 2) + pow(j - board.col / 2.0, 2));
            currentCheckerScore += centerScore / (distanceToCenter + 1.0); // give score for being closer to center

            if (j == 0 || j == board.col - 1) { // give score for being on the edge
                currentCheckerScore += edgeScore;
            }
            
            if (checker.color == "B" && i == 0) { // give score for being in denfensive position
                currentCheckerScore += defensiveScore;
            } else if (checker.color == "W" && i == board.row - 1) {
                currentCheckerScore += defensiveScore;
            }

            if (checker.color == playerColor) { // add/subtract score based on player/opponent
                score += currentCheckerScore;
            } else {
                score -= currentCheckerScore;
            }
        }
    }

    board.Undo();
    return scoreMultiplier * score;
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
    if (allMoves.size() == 0) { // if the node has no possible moves, then its a leaf(terminal?) node
        node->isLeaf = true;

        // directly run backpropagation if the selected node is a leaf node
        int result = -1;
        int opponent = node->player == 1 ? 2 : 1; // if current player has no move, then the last moved player is the opponent
        int winning_player = 3 - node->board.isWin(opponent); // flip the result since isWin might be flipped
        if (winning_player == root->player) { // return 1 if the root player wins
            result = 1;
        } else if (winning_player == opponent) { // return 0 if the root player loses
            result = 0;
        } else { // else return -1 if it's a tie
            result = -1;
        }

        backPropagation(node, result);
        return nullptr;
    }

    // if the node has not been initialized, then save all possible moves
    if (!node->initialized) { 
        for (vector<Move> moves : allMoves) {
            for (Move move : moves) {
                node->unvisitedMoves.push_back(move);
            }
        }
        node->initialized = true;
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
    int lastMovedPlayer = player;
    int noCaptureCount = 0;
    while (true) {
        vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
        if (noCaptureCount >= 40) { // stops simulation if no capture moves have been made for 40 turns (prevent infinite loop, also 40 is the tie count)
            return -1;
        }
        if (allMoves.size() == 0) { // stops simulation if a player has no possible moves
            break;
        }

        // TODO: need better heurstics
        double bestScore = -INFINITY;
        Move bestMove;
        for (vector<Move> moves : allMoves) {
            for (Move move : moves) {
                double score = 0.0;
                if (move.isCapture()) { // direct capture
                    score += 2.0;
                }

                if (isMultipleCapture(move)) { // check if move is a force capture
                    score += 2.0;
                }
                
                // score += isVulnerableMove(board, move, player); // check if move leads to more captures by opponent

                if (isPromoting(board, move, player)) { // check if next move will promote
                    score += 1.0;
                }

                score += generalBoardPositionEvaluation(board, move, player); // evaluate the board position after the move

                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }   
            }
        }

        board.makeMove(bestMove, player);
        lastMovedPlayer = player;

        if (bestMove.isCapture()) { // count the number of non capture moves
            noCaptureCount = 0;
        } else {
            noCaptureCount++;
        }

        // simulate random moves for the opponent
        // int i = rand() % (allMoves.size());
        // vector<Move> checker_moves = allMoves[i];
        // int j = rand() % (checker_moves.size());
        // Move randomMove = checker_moves[j];
        // board.makeMove(randomMove, player);
        // lastMovedPlayer = player;
        

        player = player == 1 ? 2 : 1;
        
    }

    int winning_player = 3 - board.isWin(lastMovedPlayer); // flip the result since isWin might be flipped
    int opponent = root->player == 1 ? 2 : 1;
    if (winning_player == root->player) { // return 1 if the root player wins
        return 1;
    } else if (winning_player == opponent) { // return 0 if the root player loses
        return 0;
    } else { // else return -1 if it's a tie
        return -1;
    }
}

void MCTS::backPropagation(Node* node, int result) {
    Node *current = node;
    while (current != nullptr) { // repeatedly going up the tree and update wins and visits
        current->visits++;

        // // if result == 1, then +1 win for the root player, +0 win for the other player
        // current->wins += (node->player == root->player ? result : 1 - result);

        // TODO: adjust the backpropagation details
        if (current->player == root->player) {
            if (result == 1) { // +1 if current player is root and wins
                current->wins += 1; 
            } else if (result == 1) { // -1 for losing
                current->wins -= 1; 
            } else { // 0 for tie  though kask said we consider tie as a win?
                // current->wins += 0.5;
            } 
        } else {
            if (result == 1) { // -1 for losing to player (as the opponent)
                current->wins -= 1; 
            } else if (result == 1) { // +1 for winning (as the opponent)
                current->wins += 1;
            } else {
                // current->wins += 0.5;
            }
        }

        current = current->parent;
    }
}


void MCTS::runMCTS(int time) {
    for (int i = 0; i < time; i++) {
        Node* selectedNode = selectNode(root); 
        Node* expandedNode = expandNode(selectedNode);
        if (expandedNode == nullptr) { // selected node is a leaf node, backpropagation already ran in expandNode
            continue;
        }
        int result = simulation(expandedNode);
        backPropagation(expandedNode, result);
    }
    
}

// picking the best move based the the most wins per visit
Move MCTS::getBestMove() { 
    double mostWinsPerVisit = -INFINITY;
    Move bestMove;
    for (Node *child : root->children) { // find the child with the most visits
        // cout << "Child move: " << child->move.toString() << ", visits: " << child->visits << ", wins: " << child->wins << endl;
        double currentWinsPerVisit = child->wins / child->visits;
        if (currentWinsPerVisit > mostWinsPerVisit) {
            mostWinsPerVisit = currentWinsPerVisit;
            bestMove = child->move;
        }
    }

    return bestMove;
}

void MCTS::deleteTree(Node* node) { // TODO: check memory leak?
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

// make random move
Move StudentAI::GetRandomMove(Move move) {
    if (move.seq.empty())
    {
        player = 1;
    } else {
        board.makeMove(move,player == 1?2:1);
    }

    vector<vector<Move>> allMoves = board.getAllPossibleMoves(player);
    int i = rand() % (allMoves.size());
    vector<Move> checker_moves = allMoves[i];
    int j = rand() % (checker_moves.size());

    board.makeMove(checker_moves[j], player);

    return checker_moves[j];
}


Move StudentAI::GetMove(Move move) {
    auto start = high_resolution_clock::now();
    auto remainingTime = timeLimit - timeElapsed;
    if (remainingTime < seconds(4)) { // return random move if only has 4 seconds left
        return GetRandomMove(move); // no need to keep track of the remaining time if started using random moves
    }

    if (move.seq.empty())
    {
        player = 1;
    } else {
        // re-root to the opponent's move if it's in the tree, otherwise start a new tree
        board.makeMove(move,player == 1?2:1);
        if (MCTSRoot) { // re root if MCTSRoot is not nullptr
            MCTSRoot = MCTS::reRoot(MCTSRoot, move);
        }
    }

    if (MCTSRoot == nullptr) { // start a new tree if root is nullptr
        MCTSRoot = new Node(nullptr, Move(), board, player);
    }
    MCTS mcts = MCTS(MCTSRoot, board, player);
    mcts.runMCTS(1000); // TODO: adjust the number of MCTS iterations
    Move res = mcts.getBestMove();

    board.makeMove(res, player);

    // re-root to the player's next move
    MCTSRoot = MCTS::reRoot(MCTSRoot, res);

    auto stop = high_resolution_clock::now();
    timeElapsed += duration_cast<milliseconds>(stop - start);
    // cout << "Move took: " << duration_cast<seconds>(stop - start).count() << " seconds" << endl;
    // cout << "Time elapsed: " << duration_cast<seconds>(timeElapsed).count() << " seconds" << endl;
    return res;
}


StudentAI::~StudentAI() {
    if (MCTSRoot != nullptr) {
        MCTS::deleteTree(MCTSRoot);
    }
}

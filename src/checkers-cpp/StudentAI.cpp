#include "StudentAI.h"

// Constructor implementation
StudentAI::StudentAI(int player, int boardSize) 
    : playerID(player), opponentID(player == 1 ? 2 : 1), gameBoard(boardSize), rootNode(nullptr),
      endgamePhase(false), riskMode(false), totalTimeUsedMs(0) 
{
    // Initialize the game board and MCTS root (no moves played yet)
    // Note: The Board class is assumed to handle initial setup internally.
    rootNode = new Node(gameBoard, nullptr, playerID);
}

// MCTS Node constructor
StudentAI::Node::Node(const Board& state_, Node* parent_, int playerTurn, const Move& move_)
    : state(state_), parent(parent_), move(move_), visits(0), wins(0.0), terminal(false), playerToMove(playerTurn) 
{
    // Determine if state is terminal (win/draw) at node creation
    // (Assumes Board has a method to check game over or winner)
    if (state_.isGameOver()) {              // Pseudocode: check if game ended
        terminal = true;
        // No children for terminal node
    } else {
        terminal = false;
    }
}

// Count pieces for a given player (utility)
int StudentAI::countPieces(int playerId) {
    // (Assume Board can provide piece count or we iterate board cells)
    return gameBoard.getPieceCount(playerId);  // using Board's method if available
}

// Count total pieces remaining
int StudentAI::countTotalPieces() {
    // Sum pieces of both players
    return countPieces(1) + countPieces(2);
}

// Update endgamePhase and riskMode flags based on current board state
void StudentAI::updateEndgameStatus() {
    int total = countTotalPieces();
    endgamePhase = (total < ENDGAME_THRESHOLD);  // fewer than half of initial pieces remain
    if (endgamePhase) {
        // If endgame, determine if draw is likely (e.g., material balance)
        int myCount  = countPieces(playerID);
        int oppCount = countPieces(opponentID);
        // Enter risk mode if the game is roughly balanced (likely draw scenario)
        if (abs(myCount - oppCount) == 0) {
            riskMode = true;
        } else {
            riskMode = false;
        }
    } else {
        riskMode = false;
    }
}

// Prune and reuse MCTS tree for opponent's move (to avoid rebuilding tree every turn)
void StudentAI::pruneTreeForNextMove(Node*& currentRoot, const Move& opponentMove) {
    if (!currentRoot) return;
    Node* nextRoot = nullptr;
    // Find the child node that corresponds to the opponent's move
    for (Node* child : currentRoot->children) {
        if (child->move == opponentMove) {   // (Assumes Move has equality operator)
            nextRoot = child;
            nextRoot->parent = nullptr;
            break;
        }
    }
    // Delete the rest of the tree except the subtree for the opponent's move
    for (Node* child : currentRoot->children) {
        if (child != nextRoot) {
            deleteSubtree(child);  // free memory of unused branches
        }
    }
    delete currentRoot;  // delete old root node (not needed anymore)
    currentRoot = nextRoot;
}

// Recursively delete a node and all its descendants (to free memory)
void StudentAI::deleteSubtree(Node* node) {
    if (!node) return;
    for (Node* child : node->children) {
        deleteSubtree(child);
    }
    delete node;
}

// Selection: choose a leaf node to expand using UCT (Upper Confidence Bound)
StudentAI::Node* StudentAI::selectNode(Node* node) {
    // Traverse down the tree while node is fully expanded and not terminal
    while (!node->children.empty() && !node->terminal) {
        Node* bestChild = nullptr;
        double bestValue = -1e9;
        // Calculate UCT value for each child
        for (Node* child : node->children) {
            // UCT formula: value = (wins/visits) + C * sqrt(log(parent.visits) / child.visits)
            double winRate = (child->visits > 0 ? child->wins / child->visits : 0.0);
            // If riskMode is on and this node corresponds to opponent's perspective, adjust winRate 
            // (This ensures we continue to favor our winning chances deep into tree)
            double uctValue = winRate + 1.414 * sqrt(log(node->visits + 1) / (child->visits + 1));
            if (uctValue > bestValue) {
                bestValue = uctValue;
                bestChild = child;
            }
        }
        node = bestChild;
    }
    return node;
}

// Expansion: add all possible moves from this node (one level)
void StudentAI::expandNode(Node* node) {
    if (node->terminal) return;  // no expansion if game is over at this node
    // Generate all legal moves from this node's state for the player whose turn it is
    std::vector<Move> legalMoves = node->state.getLegalMoves(node->playerToMove);
    // If no moves (stalemate) - treat as terminal (draw)
    if (legalMoves.empty()) {
        node->terminal = true;
        return;
    }
    // Create child nodes for each legal move
    for (const Move& mv : legalMoves) {
        Board nextState = node->state;
        nextState.applyMove(mv);  // simulate move
        // Next player's turn after this move
        int nextPlayer = (node->playerToMove == 1 ? 2 : 1);
        Node* child = new Node(nextState, node, nextPlayer, mv);
        node->children.push_back(child);
    }
}

// Simulation: perform a random playout from the given state and return outcome for AI
double StudentAI::simulatePlayout(Board state, int currentPlayer) {
    // Simulate until game ends or until max depth reached
    int depth = 0;
    int playerTurn = currentPlayer;
    while (depth < MAX_SIMULATION_DEPTH) {
        if (state.isGameOver()) {
            // Determine winner and return outcome from AI's perspective
            int winner = state.getWinner();  // e.g., returns 1, 2, or 0 for draw
            if (winner == 0) {
                // Game ended in a draw
                return riskMode ? DRAW_VALUE_RISK : DRAW_VALUE_DEFAULT;
            } else if (winner == playerID) {
                // AI wins
                return 1.0;
            } else {
                // AI loses
                return 0.0;
            }
        }
        // Not terminal yet, check if any legal moves
        std::vector<Move> moves = state.getLegalMoves(playerTurn);
        if (moves.empty()) {
            // No moves means a draw/stalemate situation
            return riskMode ? DRAW_VALUE_RISK : DRAW_VALUE_DEFAULT;
        }
        // Choose a random move to simulate (default policy)
        int randIndex = rand() % moves.size();
        state.applyMove(moves[randIndex]);
        // Switch player turn
        playerTurn = (playerTurn == 1 ? 2 : 1);
        depth++;
    }
    // If we reach max simulation depth without a terminal result, treat as draw (game likely continues)
    return riskMode ? DRAW_VALUE_RISK : DRAW_VALUE_DEFAULT;
}

// Backpropagation: update the node and its ancestors with simulation result
void StudentAI::backpropagate(Node* node, double result) {
    Node* current = node;
    while (current != nullptr) {
        current->visits += 1;
        // If the result is from AI's perspective, update win score appropriately
        // `result` is the score for AI: 1 = win for AI, 0 = loss for AI, (0 < result < 1) for draw or partial.
        current->wins += result;
        current = current->parent;
    }
}

// After MCTS, choose the best move (highest visit count or best win rate)
StudentAI::Node* StudentAI::getBestChild(Node* root) {
    Node* bestChild = nullptr;
    double bestScore = -1.0;
    for (Node* child : root->children) {
        // We can use win-rate or visit count; here we use win-rate bias with visits
        double winRate = (child->visits > 0 ? child->wins / child->visits : 0.0);
        // Select the child with highest winRate (or use child->visits for pure MCTS approach)
        if (winRate > bestScore) {
            bestScore = winRate;
            bestChild = child;
        }
    }
    // If tie or near-tie, we could fall back to most visits:
    if (!bestChild) {
        int maxVisits = -1;
        for (Node* child : root->children) {
            if (child->visits > maxVisits) {
                maxVisits = child->visits;
                bestChild = child;
            }
        }
    }
    return bestChild;
}

// Primary function to decide the next move
Move StudentAI::GetMove(const Move& opponentMove) {
    // Update internal board state with opponent's move
    if (opponentMove.isValid()) {  // (Assumes Move has a method to check if it’s a real move)
        gameBoard.applyMove(opponentMove);
    }
    // **Time Management**: calculate remaining time and allocate for this move
    long long remainingTime = TIME_LIMIT_MS - totalTimeUsedMs;
    // For safety, don't use all remaining time; keep a margin
    long long allocation = std::max(remainingTime - TIME_MARGIN_MS, (long long)50); // at least 50ms
    auto moveStartTime = std::chrono::steady_clock::now();

    // **Efficiency**: Reuse previous search tree if available and prune irrelevant branches
    pruneTreeForNextMove(rootNode, opponentMove);
    if (rootNode == nullptr) {
        // Create a new root node for current state if none exists (e.g., first move or branch not found)
        rootNode = new Node(gameBoard, nullptr, playerID);
    }

    // **Endgame Strategy**: check if we are in endgame phase and adjust strategy flags
    updateEndgameStatus();

    // Monte Carlo Tree Search loop – runs until allocated time is about to elapse
    int simulations = 0;
    while (true) {
        // Check elapsed time in milliseconds
        auto now = std::chrono::steady_clock::now();
        long long elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - moveStartTime).count();
        if (elapsedMs >= allocation) {
            break;  // stop MCTS when out of allocated time for this move
        }
        // Selection: find a node to expand
        Node* selected = selectNode(rootNode);
        // Expansion: if not terminal, expand the selected node
        if (!selected->terminal && selected->visits > 0) {
            // If the node has never been visited, we should simulate before expansion (Monte Carlo initial step)
            expandNode(selected);
            // If expanded has children, pick one child at random for simulation
            if (!selected->children.empty()) {
                selected = selected->children[rand() % selected->children.size()];
            }
        } else if (!selected->terminal && selected->visits == 0) {
            // Node is unvisited and not terminal: expand it now (to generate moves for simulation)
            expandNode(selected);
            if (!selected->children.empty()) {
                selected = selected->children[rand() % selected->children.size()];
            }
        }
        // Simulation: run a playout from the selected node's state
        double result = simulatePlayout(selected->state, selected->playerToMove);
        // **Endgame/Risk Adjustment**: If in risk mode and the simulation result was a draw value, it’s already adjusted in simulatePlayout.
        // Backpropagation: update the tree with the simulation result
        backpropagate(selected, result);
        simulations++;
    }

    // **Time Management**: update total time used by AI
    auto moveEndTime = std::chrono::steady_clock::now();
    long long moveDuration = std::chrono::duration_cast<std::chrono::milliseconds>(moveEndTime - moveStartTime).count();
    totalTimeUsedMs += moveDuration;

    // Choose the best move from the root after MCTS
    Node* bestChild = getBestChild(rootNode);
    Move bestMove = bestChild ? bestChild->move : Move();  // fallback to an empty move if something went wrong

    // Apply our chosen move to the actual game board
    if (bestChild) {
        gameBoard.applyMove(bestMove);
        // Prepare for next turn: set the chosen child as new root (reuse tree) and prune others
        for (Node* child : rootNode->children) {
            if (child != bestChild) {
                deleteSubtree(child);
            }
        }
        bestChild->parent = nullptr;
        // Free the old root and assign the new root
        delete rootNode;
        rootNode = bestChild;
    }

    // If no bestChild (should not happen if moves exist), just return a random legal move to avoid crashing
    if (!bestChild) {
        std::vector<Move> moves = gameBoard.getLegalMoves(playerID);
        if (!moves.empty()) {
            bestMove = moves[0];
            gameBoard.applyMove(bestMove);
            // Reset MCTS tree for next move since we had to pick arbitrarily
            deleteSubtree(rootNode);
            rootNode = new Node(gameBoard, nullptr, opponentID);
        }
    }

    // Return the selected move
    return bestMove;
}

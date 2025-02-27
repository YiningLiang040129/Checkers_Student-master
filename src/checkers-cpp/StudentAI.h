#ifndef STUDENTAI_H
#define STUDENTAI_H

#include <bits/stdc++.h>   // for standard library (vector, etc.)
#include <chrono>          // for high-resolution timing

// Forward declarations (if needed)
struct Move;
class Board;

// StudentAI class definition
class StudentAI {
public:
    // Constructor: initializes player ID and board size, and sets up AI state
    StudentAI(int player, int boardSize);

    // Function to get the next move given the opponent's last move
    Move GetMove(const Move& opponentMove);

private:
    // Monte Carlo Tree Search Node structure
    struct Node {
        Board state;              // game state at this node
        Move move;                // move that led to this state (undefined for root)
        Node* parent;             // parent node in the search tree
        std::vector<Node*> children;  // children nodes (possible next states)
        int visits;               // number of simulations through this node
        double wins;              // aggregate win score (for AI) from this node
        bool terminal;            // true if this state is a game-ending state
        int playerToMove;         // which player's turn it is from this state

        Node(const Board& state_, Node* parent_, int playerTurn, const Move& move_ = Move());
    };

    // Internal helper functions for MCTS
    Node* selectNode(Node* root);               // selection: choose node to expand (UCT)
    void expandNode(Node* node);                // expansion: add children to the node
    double simulatePlayout(Board state, int currentPlayer); // simulation: random (or guided) playout
    void backpropagate(Node* node, double result);          // backpropagation: update stats up the tree

    // Utility functions
    Node* getBestChild(Node* root);             // choose the best child from root after search
    void pruneTreeForNextMove(Node*& root, const Move& opponentMove); // reuse tree: keep subtree matching opponent's move
    void deleteSubtree(Node* node);            // delete a node and all descendants (to free memory)

    // Time and game management
    void updateEndgameStatus();                // check if we are in endgame phase and set risk mode
    int countTotalPieces();                    // count total remaining pieces on the board
    int countPieces(int playerId);             // count remaining pieces for a specific player

    // Member variables
    int playerID;               // AI's player ID (e.g., 1 or 2)
    int opponentID;             // opponent's player ID
    Board gameBoard;            // current game board state
    Node* rootNode;             // root of the MCTS tree for the current turn
    bool endgamePhase;          // flag: true if fewer than half the pieces remain (endgame)
    bool riskMode;              // flag: true if AI should take risks (endgame and likely draw scenario)
    long long totalTimeUsedMs;  // total time used by this AI (in milliseconds) so far

    // Constants for time and strategy
    static const int TIME_LIMIT_MS    = 480000;  // 8 minutes total per game (in milliseconds)
    static const int TIME_MARGIN_MS   = 2000;    // safety margin to avoid timeout (in milliseconds)
    static const int ENDGAME_THRESHOLD = 12;     // threshold for pieces remaining to consider endgame
    static const int MAX_SIMULATION_DEPTH = 60;  // max moves in a playout simulation to avoid long loops
    static constexpr double DRAW_VALUE_DEFAULT = 0.5; // default value for a draw outcome
    static constexpr double DRAW_VALUE_RISK    = 0.25; // devalued draw outcome in risk mode (prioritize win)
};

#endif // STUDENTAI_H

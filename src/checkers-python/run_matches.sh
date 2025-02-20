#!/usr/bin/env bash
# Run matches with various board sizes

NUM_RUNS=100  # number of runs for each board size
k=2  # keep the k parameter constant; adjust if needed

# Define arrays for board dimensions.
# For example, board sizes 7x7, 8x8, and 9x9:
rows=(5 6 7 8 9 10)
cols=(5 6 7 8 9 10)

# Loop over each board size
for idx in "${!rows[@]}"; do
    row="${rows[$idx]}"
    col="${cols[$idx]}"
    
    winsAI1=0
    winsAI2=0
    ties=0

    echo "===================================="
    echo "Running matches for board size: ${col} x ${row}"
    
    for i in $(seq 1 $NUM_RUNS); do
        # Run the game and capture output.
        OUTPUT=$(python3 main.py $col $row $k l ../checkers-cpp/main main.py)
        
        # Check which player won (or if it was a tie)
        if echo "$OUTPUT" | grep -iq "player 1 wins"; then
            winsAI1=$((winsAI1+1))
        elif echo "$OUTPUT" | grep -iq "player 2 wins"; then
            winsAI2=$((winsAI2+1))
        elif echo "$OUTPUT" | grep -iq "Tie"; then
            ties=$((ties+1))
        fi

        echo "Round $i finished: P1: $winsAI1, P2: $winsAI2, Ties: $ties"
    done

    echo "-------------------------------------"
    echo "Results for board size ${col} x ${row}:"
    echo "Player 1 (../checkers-cpp/main) wins: $winsAI1"
    echo "Player 2 (main.py) wins: $winsAI2"
    echo "Ties: $ties"
done

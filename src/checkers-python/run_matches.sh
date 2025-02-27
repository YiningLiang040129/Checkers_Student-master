#!/usr/bin/env bash
# Run matches with various board sizes

NUM_RUNS=100  # number of runs for each board size
k=2  # keep the k parameter constant; adjust if needed

# Define arrays for board dimensions.
# For example, board sizes 5x5, 6x6, ... 10x10:
rows=(6 7 8)
cols=(6 7 8)

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
        # Swap the order so our AI is Player 2
        OUTPUT=$(python3 main.py $col $row $k l main.py ../checkers-cpp/main)
        
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
    echo "Player 1 (main.py) wins: $winsAI1"
    echo "Player 2 (../checkers-cpp/main) wins: $winsAI2"
    echo "Ties: $ties"
done

#!/usr/bin/env bash
# 运行不同棋盘大小和双方先后手的所有可能情况

NUM_RUNS=10      # 每种情况的比赛局数
k=2              # k 参数常量，可以根据需要调整

# 棋盘行和列尺寸数组 (例如：5x5, 6x6, …, 10x10)
rows=(5 6 7 8 9 10)
cols=(5 6 7 8 9 10)

# 循环每一种棋盘大小
for idx in "${!rows[@]}"; do
    row="${rows[$idx]}"
    col="${cols[$idx]}"

    # 对两种先手情况分别测试
    # order=1 表示 main.py 为玩家1，checkers-cpp/main 为玩家2
    # order=2 表示 main.py 为玩家2，checkers-cpp/main 为玩家1
    for order in 1 2; do
        winsMain=0
        winsOpp=0
        ties=0

        echo "===================================="
        echo "棋盘尺寸: ${col} x ${row} ，main.py 作为玩家 ${order}"
        
        for i in $(seq 1 $NUM_RUNS); do
            if [ "$order" -eq 1 ]; then
                # main.py 为玩家1
                OUTPUT=$(python3 main.py $col $row $k l ../checkers-cpp/main main.py)
            else
                # main.py 为玩家2
                OUTPUT=$(python3 main.py $col $row $k l main.py ../checkers-cpp/main)
            fi

            # 根据输出判断获胜情况
            if echo "$OUTPUT" | grep -iq "player 1 wins"; then
                if [ "$order" -eq 1 ]; then
                    winsMain=$((winsMain+1))
                else
                    winsOpp=$((winsOpp+1))
                fi
            elif echo "$OUTPUT" | grep -iq "player 2 wins"; then
                if [ "$order" -eq 1 ]; then
                    winsOpp=$((winsOpp+1))
                else
                    winsMain=$((winsMain+1))
                fi
            elif echo "$OUTPUT" | grep -iq "Tie"; then
                ties=$((ties+1))
            fi

            echo "第 $i 局结束: main.py 胜: $winsMain, 对手胜: $winsOpp, 平局: $ties"
        done

        echo "-------------------------------------"
        echo "棋盘尺寸 ${col} x ${row} ，main.py 作为玩家 ${order} 的最终结果："
        echo "main.py 胜局: $winsMain"
        echo "对手胜局: $winsOpp"
        echo "平局: $ties"
        echo ""
    done
done

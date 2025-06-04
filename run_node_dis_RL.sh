#!/bin/bash

# 定义颜色格式
RED='\033[0;31m'
NC='\033[0m' # No Color
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'

# 配置参数
DISTRIBUTION_TYPES=("dispersed" "clustered")  # 两种分布类型
NODE_COUNTS=(100 300 500 700 1000 1300)  # 测试节点数量
TRAIN_DIR="/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface"
RL_ALGORITHM="DQN"  # 可改为 AVERAGE, MAXIMUM, MINIMUM 等

# 创建主结果目录
MAIN_OUTPUT_DIR="/tmp/lora_simulation_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$MAIN_OUTPUT_DIR"

# 清理并重建临时目录
cleanup() {

    rm -rf "$TRAIN_DIR/workbench"
    mkdir -p "$TRAIN_DIR/workbench"
}
cleanup




# 仿真测试函数
simulation() {
     rladr_algorithm=$1
     rladr_mode=$2
     modality=$3
     title=$4
     crdsa_enabled=$5
    # local radius=$6
     distribution=$6
     node_count=$7

    echo -e "${GREEN}Running $title (${distribution}, ${node_count} nodes)...${NC}"

    # 准备模型文件
    cp $TRAIN_DIR/temp-array-store/max-array.txt $TRAIN_DIR/array.txt 2>/dev/null || true
    cp $TRAIN_DIR/temp-model-store/* $TRAIN_DIR/ 2>/dev/null || true

    # 初始化结果文件
    echo "$title" > "$TRAIN_DIR/workbench/${distribution}_${modality}-energy-plot.csv"
    echo "$title" > "$TRAIN_DIR/workbench/${distribution}_${modality}-goodput-plot.csv"

    # 修改ADR算法配置
    sed -i "s/PLACEHOLDER/$rladr_algorithm/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc

    # 运行仿真
    local i=0
    while [ $i -lt 1 ]; do  # 每个配置运行5次取平均
        cd /home/zhou/tarballs/ns-3-allinone/ns-3.37 || exit 1
        
        ./ns3 run "contrib/lorawan/examples/my-complete-network-example \
            --rlagent=$rladr_mode \
            --radius=6000 \
            --movementMode=$distribution \
            --nDevices=$node_count \
            --simulationTime=7200 \
            --training=false \
            --gamma=0.7 \
            --epsilon=0.9 \
            --alpha=0.8 \
            --pkgbasesize=14 \
            --enableCRDSA=true" | grep Result > "$TRAIN_DIR/workbench/round-$i"
            
        goodput=$(awk '{print $6}' "$TRAIN_DIR/workbench/round-$i")
        energy=$(awk '{print $5}' "$TRAIN_DIR/workbench/round-$i")
        
        echo "$goodput" >> "$TRAIN_DIR/workbench/${distribution}_${modality}-goodput-plot.csv"
        echo "$energy" >> "$TRAIN_DIR/workbench/${distribution}_${modality}-energy-plot.csv"
        
        let "i=i+1"
    done

    # 计算平均值
    calculate_average() {
        local file=$1
        local values=$(tail -n +2 "$file")
        local sum=0
        local count=0
        
        for val in $values; do
            sum=$(echo "$sum + $val" | bc)
            ((count++))
        done
        
        if [ $count -gt 0 ]; then
            average=$(echo "scale=4; $sum / $count" | bc)
            echo "$title" > "$file"
            echo "$average" >> "$file"
        fi
    }
    
    calculate_average "$TRAIN_DIR/workbench/${distribution}_${modality}-goodput-plot.csv"
    calculate_average "$TRAIN_DIR/workbench/${distribution}_${modality}-energy-plot.csv"

    # 恢复ADR配置
    sed -i "s/EnumValue (myAdrComponent::$rladr_algorithm)/EnumValue (myAdrComponent::PLACEHOLDER)/g" \
        /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc
}

# 主测试循环
for distribution in "${DISTRIBUTION_TYPES[@]}"; do
    echo -e "\n${YELLOW}=== Testing $distribution distribution ===${NC}"
    
    # # 根据分布类型设置参数
    # case "$distribution" in
    #     "dispersed")
    #         radius=6000  # 大半径实现分散分布
    #         ;;
    #     "clustered")
    #         radius=2000  # 小半径实现集群分布
    #         ;;
    # esac
    
    # 节点数量测试循环
    for node_count in "${NODE_COUNTS[@]}"; do
        echo -e "\n${BLUE}Testing $node_count nodes in $distribution...${NC}"
        
        OUTPUT_DIR="$MAIN_OUTPUT_DIR/$distribution/nodes_$node_count"
        mkdir -p "$OUTPUT_DIR"
        
        # 运行所有ADR策略测试
        simulation "AVERAGE" 1 "adrrl" "ADR-RL" true "$distribution" "$node_count"
        simulation "MAXIMUM" 4 "adrdqn" "ADR-DQN" true "$distribution" "$node_count"
        simulation "AVERAGE" 0 "adravg" "ADR-AVG" true "$distribution" "$node_count"
        simulation "MAXIMUM" 0 "adrmax" "ADR-MAX" true "$distribution" "$node_count"
        simulation "MINIMUM" 0 "adrmin" "ADR-MIN" true "$distribution" "$node_count"
        
        # 合并结果
        paste "$TRAIN_DIR"/workbench/"${distribution}"_*-goodput-plot.csv | tr "\t" "," > "$OUTPUT_DIR/goodput_results.csv"
        paste "$TRAIN_DIR"/workbench/"${distribution}"_*-energy-plot.csv | tr "\t" "," > "$OUTPUT_DIR/energy_results.csv"
        
        # 实时绘图
        python3 /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/real_time_plot2.py \
            --output_dir "$MAIN_OUTPUT_DIR" \
            --distribution "$distribution" \
            --current_nodes "$node_count" \
            --plot_dir "$MAIN_OUTPUT_DIR/live_plot_${distribution}.png" 2>/dev/null || echo -e "${YELLOW}绘图更新跳过${NC}"
    done
done

# # 生成最终对比报告
# echo -e "\n${GREEN}Generating final comparison report...${NC}"
# python3 /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/RL_vs_noRL.py \
#     --output_dir "$MAIN_OUTPUT_DIR" \
#     --plot_dir "$MAIN_OUTPUT_DIR/final_comparison.png"

# 完成提示
echo -e "\n${YELLOW}=== 测试完成 ===${NC}"
echo -e "结果目录: ${BLUE}$MAIN_OUTPUT_DIR${NC}"
echo -e "最终对比图: ${BLUE}$MAIN_OUTPUT_DIR/final_comparison.png${NC}"
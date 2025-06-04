#!/bin/bash

# 添加节点数量测试功能
# NODE_COUNTS=(100 200 300 400 500 600 700)  # 要测试的不同节点数量
NODE_COUNTS=(300 600 900 1200 1500 1800 2100 2400 2700 3000)  # 要测试的不同节点数量
source /etc/profile 

##### Defining some style formats
RED='\033[0;31m'
NC='\033[0m' # No Color
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'

spin[0]="-"
spin[1]="\\"
spin[2]="|"
spin[3]="/"

# 解析YAML配置
parse_yaml() {
   local prefix=$2
   local s='[[:space:]]*' w='[a-zA-Z0-9_]*' fs=$(echo @|tr @ '\034')
   sed -ne "s|^\($s\)\($w\)$s:$s\"\(.*\)\"$s\$|\1$fs\2$fs\3|p" \
        -e "s|^\($s\)\($w\)$s:$s\(.*\)$s\$|\1$fs\2$fs\3|p"  $1 |
   awk -F$fs '{
      indent = length($1)/2;
      vname[indent] = $2;
      for (i in vname) {if (i > indent) {delete vname[i]}}
      if (length($3) > 0) {
         vn=""; for (i=0; i<indent; i++) {vn=(vn)(vname[i])("_")}
         printf("%s%s%s=\"%s\"\n", "'$prefix'",vn, $2, $3);
      }
   }'
}

# 检查输入文件
arguments_file="/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/arguments.yml"
if [ ! -f "$arguments_file" ]; then
   echo -e "${RED}\033[5m No arguments file provided... Exiting :-(\033[0m${NC}"
   kill -INT $$
else
   eval $(parse_yaml "$arguments_file" "")
fi

# 创建主结果目录
MAIN_OUTPUT_DIR="/tmp/lora_simulation_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$MAIN_OUTPUT_DIR"

TRAIN_DIR="/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface"

# rm -f $TRAIN_DIR/array.txt
rm -f $TRAIN_DIR/model-store  #目标网络
rm -f $TRAIN_DIR/label-store  #label store 价值网络
rm -f $TRAIN_DIR/memory-store

# rm -rf "$TRAIN_DIR/performance-records"
rm -rf "$TRAIN_DIR/temp-model-store"
rm -rf "$TRAIN_DIR/performance-dqn-records"
# rm -rf "$TRAIN_DIR/temp-array-store"
rm -rf "$TRAIN_DIR/workbench"

## Create temporary folder for holding   创建新的临时目录
## QTable values, experiences and handlers

# mkdir "$TRAIN_DIR/temp-array-store"
# mkdir "$TRAIN_DIR/performance-records"
mkdir "$TRAIN_DIR/temp-model-store"
mkdir "$TRAIN_DIR/performance-dqn-records"
mkdir "$TRAIN_DIR/workbench"


# # RL训练
# perf=0
# maxperf=0
# sed -i "s/PLACEHOLDER/$rladr_algorithm/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc
    
# echo -e "${BLUE}Starting RL training for $node_count nodes...${NC}"
# i=0
# while [ $i -lt $training_max_rounds ]; do
#     cd /home/zhou/tarballs/ns-3-allinone/ns-3.37 || exit 1
#     ./ns3 run "contrib/lorawan/examples/my-complete-network-example --rlagent=1 --radius=$context_buildings_radius --nDevices=200 --simulationTime=$training_round_time --training=true --gamma=$training_gamma --epsilon=$training_epsilon --alpha=$training_alpha --pkgbasesize=$context_pkg_size --enableCRDSA=true" | grep Result | tr '\n' '\0' | awk -F" " {'print $6'} > $TRAIN_DIR/performance-records/perf-$i; perf=`cat $TRAIN_DIR/performance-records/perf-$i`;
        
#     if [ $(echo "$perf >= $maxperf" | bc -l) ]; then
#         maxperf=$perf
#         cp $TRAIN_DIR/array.txt $TRAIN_DIR/temp-array-store/max-array.txt
#     fi
#         let "i = $i + 1"
# done
    
# sed -i "s/EnumValue (myAdrComponent::$rladr_algorithm)/EnumValue (myAdrComponent::PLACEHOLDER)/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc

    # DQN训练
perf=0
maxperf=0
sed -i "s/PLACEHOLDER/$rladr_algorithm/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc

echo -e "${BLUE}Starting DQN training for $node_count nodes...${NC}"
i=0
while [ $i -lt $training_dqn_rounds ]; do
    cd /home/zhou/tarballs/ns-3-allinone/ns-3.37 || exit 1
    ./ns3 run "my-complete-network-example --training=true --rlagent=4 --nDevices=200 enableCRDSA=true --gamma=0.7 --epsilon=0.9 --alpha=0.8" | grep Result | tr '\n' '\0' | awk -F" " {'print $6'} > $TRAIN_DIR/performance-dqn-records/perf-$i; perf=`cat $TRAIN_DIR/performance-dqn-records/perf-$i`;
        
    if [ $(echo "$perf >= $maxperf" | bc -l) ]; then
        maxperf=$perf
        cp $TRAIN_DIR/model-store $TRAIN_DIR/label-store $TRAIN_DIR/memory-store $TRAIN_DIR/temp-model-store/
    fi
        let "i = $i + 1"
done
    
    # 恢复ADR算法占位符
sed -i "s/EnumValue (myAdrComponent::$rladr_algorithm)/EnumValue (myAdrComponent::PLACEHOLDER)/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc

# 节点数量测试循环
for node_count in "${NODE_COUNTS[@]}"; do
    echo -e "\n${YELLOW}=== Testing with $node_count nodes ===${NC}"
    
    # 为当前节点数量创建独立目录
    OUTPUT_DIR="$MAIN_OUTPUT_DIR/nodes_$node_count"
    mkdir -p "$OUTPUT_DIR"
    

    # 仿真测试函数
    simulation() {
        rladr_algorithm=$1
        rladr_mode=$2
        modality=$3
        title=$4
        crdsa_enabled=$5

        cp $TRAIN_DIR/temp-array-store/max-array.txt array.txt
        cp $TRAIN_DIR/temp-model-store/* .  

        echo "$title" > $TRAIN_DIR/workbench/$modality-$crdsa_enabled-energy-plot.csv
        echo "$title" > $TRAIN_DIR/workbench/$modality-$crdsa_enabled-goodput-plot.csv
        
        sed -i "s/PLACEHOLDER/$rladr_algorithm/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc
        
        i=0
        while [ $i -lt $context_max_rounds ]; do
            cd /home/zhou/tarballs/ns-3-allinone/ns-3.37 || exit 1
            ./ns3 run "contrib/lorawan/examples/my-complete-network-example --rlagent=$rladr_mode --radius=$context_buildings_radius --nDevices=$node_count --simulationTime=$training_round_time --training=false --gamma=$rladr_gamma --epsilon=$rladr_epsilon --alpha=$rladr_alpha --pkgbasesize=$context_pkg_size --enableCRDSA=$crdsa_enabled" | grep Result > $TRAIN_DIR/workbench/round-$i
            goodput=`cat $TRAIN_DIR/workbench/round-$i | cut -d" " -f6`
            energy=`cat $TRAIN_DIR/workbench/round-$i | cut -d" " -f5`
            
            echo "$goodput" >> $TRAIN_DIR/workbench/$modality-$crdsa_enabled-goodput-plot.csv
            echo "$energy" >> $TRAIN_DIR/workbench/$modality-$crdsa_enabled-energy-plot.csv
            let "i = $i + 1"
        done
        
# 计算平均值并替换文件内容
    calculate_average() {
        local file=$1
        local values=$(tail -n +2 "$file")  # 跳过标题行
        local sum=0
        local count=0
        
        for val in $values; do
            sum=$(echo "$sum + $val" | bc)
            ((count++))
        done
        
        if [ $count -gt 0 ]; then
            average=$(echo "scale=4; $sum / $count" | bc)
            # 用平均值替换文件内容（保留标题行）
            echo "$title" > "$file"
            echo "$average" >> "$file"
        fi
    }
    
    calculate_average "$TRAIN_DIR/workbench/$modality-$crdsa_enabled-goodput-plot.csv"
    calculate_average "$TRAIN_DIR/workbench/$modality-$crdsa_enabled-energy-plot.csv"

    sed -i "s/EnumValue (myAdrComponent::$rladr_algorithm)/EnumValue (myAdrComponent::PLACEHOLDER)/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc
    }

    # 运行所有ADR策略测试
    simulation AVERAGE 1 adrrl ADR-RL 1
    simulation AVERAGE 4 adrdqn ADR-DQN 1
    simulation AVERAGE 0 adravg ADR-AVG 1
    simulation MAXIMUM 0 adrmax ADR-MAX 1
    simulation MINIMUM 0 adrmin ADR-MIN 1

     # 保存当前节点数量的结果
    paste $TRAIN_DIR/workbench/adrrl-$crdsa_enabled-energy-plot.csv $TRAIN_DIR/workbench/adrdqn-$crdsa_enabled-energy-plot.csv $TRAIN_DIR/workbench/adravg-$crdsa_enabled-energy-plot.csv $TRAIN_DIR/workbench/adrmax-$crdsa_enabled-energy-plot.csv $TRAIN_DIR/workbench/adrmin-$crdsa_enabled-energy-plot.csv | tr "\t" "," > $OUTPUT_DIR/$crdsa_enabled-energy_results.csv 

    paste $TRAIN_DIR/workbench/adrrl-$crdsa_enabled-goodput-plot.csv $TRAIN_DIR/workbench/adrdqn-$crdsa_enabled-goodput-plot.csv $TRAIN_DIR/workbench/adravg-$crdsa_enabled-goodput-plot.csv $TRAIN_DIR/workbench/adrmax-$crdsa_enabled-goodput-plot.csv $TRAIN_DIR/workbench/adrmin-$crdsa_enabled-goodput-plot.csv | tr "\t" "," > $OUTPUT_DIR/$crdsa_enabled-goodput_results.csv


    # ============== 添加实时绘图调用 ==============
    python3 /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/real_time_plot1.py \
        --output_dir "$MAIN_OUTPUT_DIR" \
        --current_nodes "$node_count" \
        --plot_dir "$MAIN_OUTPUT_DIR/live_plot1.png" 2>&1 | tee -a "$MAIN_OUTPUT_DIR/plot_log.txt" || echo -e "${YELLOW}绘图更新跳过${NC}"

    # 复制配置文件
    cp $TRAIN_DIR/buildings.txt $TRAIN_DIR/arguments.yml /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/movingGateway/CPLEX/nodes.csv $OUTPUT_DIR/
done

# 生成综合比较图表
echo -e "${GREEN}Generating comparative plots (ADR-RL vs ADR-DQN vs ADR-AVG vs ADR-MAX vs ADR-MIN)...${NC}"

python3 /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/RL_vs_noRL.py \
    --output_dir "$MAIN_OUTPUT_DIR" \
    --plot_dir "$MAIN_OUTPUT_DIR/goodput_comparison.png"

# if [ $? -ne 0 ]; then
#     echo -e "${RED}图表生成失败!${NC}"
# else
#     echo -e "Comparative plot: ${BLUE}$MAIN_OUTPUT_DIR/goodput_comparison.png${NC}"
# fi

clear
echo -e "${YELLOW}#####################################################${NC}"
echo -e "${GREEN}All simulations completed!${NC}"
echo -e "Results saved to: ${BLUE}$MAIN_OUTPUT_DIR${NC}"
echo -e "Comparative plot: ${BLUE}$MAIN_OUTPUT_DIR/goodput_comparison.png${NC}"
echo -e "${YELLOW}#####################################################${NC}"
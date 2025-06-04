#!/bin/bash

# 添加节点数量测试功能
NODE_COUNTS=(1000 1500 2000 2500 3000)  # 要测试的不同节点数量

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

# 节点数量测试循环
for node_count in "${NODE_COUNTS[@]}"; do
    echo -e "\n${YELLOW}=== Testing with $node_count nodes ===${NC}"
    
    # 为当前节点数量创建独立目录
    OUTPUT_DIR="$MAIN_OUTPUT_DIR/nodes_$node_count"
    mkdir -p "$OUTPUT_DIR"
    
    # 清理临时文件
    TRAIN_DIR="/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface"
    # rm -f $TRAIN_DIR/array.txt $TRAIN_DIR/model-store $TRAIN_DIR/label-store $TRAIN_DIR/memory-store
    # rm -rf "$TRAIN_DIR/performance-records" "$TRAIN_DIR/temp-model-store" 
    # rm -rf "$TRAIN_DIR/performance-dqn-records" "$TRAIN_DIR/temp-array-store" "$TRAIN_DIR/workbench"
    rm -rf "$TRAIN_DIR/workbench"

    # mkdir "$TRAIN_DIR/temp-array-store" "$TRAIN_DIR/performance-records" 
    # mkdir "$TRAIN_DIR/temp-model-store" "$TRAIN_DIR/performance-dqn-records" "$TRAIN_DIR/workbench"
    mkdir "$TRAIN_DIR/workbench"

    

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
            ./ns3 run "contrib/lorawan/examples/my-complete-network-example --rlagent=$rladr_mode --radius=$context_buildings_radius --nDevices=$node_count --simulationTime=$training_round_time --training=false --gamma=$rladr_gamma --epsilon=$rladr_epsilon --alpha=$rladr_alpha --pkgbasesize=$context_pkg_size --enableCRDSA=$crdsa_enabled --movementMode=dispersed" | grep Result > $TRAIN_DIR/workbench/round-$i
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
    # simulation AVERAGE 1 adrrl ADR-RL
    # simulation AVERAGE 4 adrdqn ADR-DQN
    simulation AVERAGE 0 adravg ADR-AVG 0
    simulation MAXIMUM 0 adrmax ADR-MAX 0
    simulation MINIMUM 0 adrmin ADR-MIN 0

     # 保存当前节点数量的结果
    paste $TRAIN_DIR/workbench/adravg-$crdsa_enabled-energy-plot.csv $TRAIN_DIR/workbench/adrmax-$crdsa_enabled-energy-plot.csv $TRAIN_DIR/workbench/adrmin-$crdsa_enabled-energy-plot.csv | tr "\t" "," > $OUTPUT_DIR/$crdsa_enabled-energy_results.csv 
    paste $TRAIN_DIR/workbench/adravg-$crdsa_enabled-goodput-plot.csv $TRAIN_DIR/workbench/adrmax-$crdsa_enabled-goodput-plot.csv $TRAIN_DIR/workbench/adrmin-$crdsa_enabled-goodput-plot.csv | tr "\t" "," > $OUTPUT_DIR/$crdsa_enabled-goodput_results.csv

    simulation AVERAGE 0 adravg ADR-AVG 1
    simulation MAXIMUM 0 adrmax ADR-MAX 1
    simulation MINIMUM 0 adrmin ADR-MIN 1
    

    # 保存当前节点数量的结果
    paste $TRAIN_DIR/workbench/adravg-$crdsa_enabled-energy-plot.csv $TRAIN_DIR/workbench/adrmax-$crdsa_enabled-energy-plot.csv $TRAIN_DIR/workbench/adrmin-$crdsa_enabled-energy-plot.csv | tr "\t" "," > $OUTPUT_DIR/$crdsa_enabled-energy_results.csv 
    paste $TRAIN_DIR/workbench/adravg-$crdsa_enabled-goodput-plot.csv $TRAIN_DIR/workbench/adrmax-$crdsa_enabled-goodput-plot.csv $TRAIN_DIR/workbench/adrmin-$crdsa_enabled-goodput-plot.csv | tr "\t" "," > $OUTPUT_DIR/$crdsa_enabled-goodput_results.csv

    python3 /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/real_time_plot.py \
        --output_dir "$MAIN_OUTPUT_DIR" \
        --current_nodes "$node_count" \
        --plot_dir "$MAIN_OUTPUT_DIR/real_time_plot.png" 2>/dev/null || echo -e "${YELLOW}绘图更新跳过${NC}"

    # 复制配置文件
    # cp $TRAIN_DIR/buildings.txt $TRAIN_DIR/arguments.yml /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/movingGateway/CPLEX/nodes.csv $OUTPUT_DIR/
done

# 生成综合比较图表
echo -e "${GREEN}Generating comparative plots (ADR-AVG vs ADR-MAX vs ADR-MIN)...${NC}"

python3 /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/aloha_vs_crdsa_noRL3.py \
    --output_dir "$MAIN_OUTPUT_DIR" \
    --plot_dir "$MAIN_OUTPUT_DIR/goodput_comparison.png"

if [ $? -ne 0 ]; then
    echo -e "${RED}图表生成失败!${NC}"
else
    echo -e "Comparative plot: ${BLUE}$MAIN_OUTPUT_DIR/goodput_comparison.png${NC}"
fi

clear
echo -e "${YELLOW}#####################################################${NC}"
echo -e "${GREEN}All simulations completed!${NC}"
echo -e "Results saved to: ${BLUE}$MAIN_OUTPUT_DIR${NC}"
echo -e "Comparative plot: ${BLUE}$MAIN_OUTPUT_DIR/goodput_comparison.png${NC}"
echo -e "${YELLOW}#####################################################${NC}"
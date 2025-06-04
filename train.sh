# #!/bin/bash

 source /etc/profile 

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


TRAIN_DIR="/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface"

rm -f $TRAIN_DIR/array.txt
rm -f $TRAIN_DIR/model-store  #目标网络
rm -f $TRAIN_DIR/label-store  #label store 价值网络
rm -f $TRAIN_DIR/memory-store

rm -rf "$TRAIN_DIR/performance-records"
rm -rf "$TRAIN_DIR/temp-model-store"
rm -rf "$TRAIN_DIR/performance-dqn-records"
rm -rf "$TRAIN_DIR/temp-array-store"


# ## Create temporary folder for holding   创建新的临时目录
# ## QTable values, experiences and handlers

mkdir "$TRAIN_DIR/temp-array-store"
mkdir "$TRAIN_DIR/performance-records"
mkdir "$TRAIN_DIR/temp-model-store"
mkdir "$TRAIN_DIR/performance-dqn-records"


# RL训练
perf=0
maxperf=0
sed -i "s/PLACEHOLDER/$rladr_algorithm/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc
    
echo -e "${BLUE}Starting RL training for $node_count nodes...${NC}"
i=0
while [ $i -lt $training_max_rounds ]; do
    cd /home/zhou/tarballs/ns-3-allinone/ns-3.37 || exit 1
    ./ns3 run "contrib/lorawan/examples/my-complete-network-example --rlagent=1 --radius=$context_buildings_radius --nDevices=200 --simulationTime=$training_round_time --training=true --gamma=$training_gamma --epsilon=$training_epsilon --alpha=$training_alpha --pkgbasesize=$context_pkg_size --enableCRDSA=true" | grep Result | tr '\n' '\0' | awk -F" " {'print $6'} > $TRAIN_DIR/performance-records/perf-$i; perf=`cat $TRAIN_DIR/performance-records/perf-$i`;
        
    if [ $(echo "$perf >= $maxperf" | bc -l) ]; then
        maxperf=$perf
        cp $TRAIN_DIR/array.txt $TRAIN_DIR/temp-array-store/max-array.txt
    fi
        let "i = $i + 1"
done
    
 sed -i "s/EnumValue (myAdrComponent::$rladr_algorithm)/EnumValue (myAdrComponent::PLACEHOLDER)/g" /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc

    # DQN训练
# DQN训练 - 分批次版本
# 训练配置


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
sed -i "s/EnumValue (myAdrComponent::$rladr_algorithm)/EnumValue (myAdrComponent::PLACEHOLDER)/g" \
    /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/my-adr-component.cc
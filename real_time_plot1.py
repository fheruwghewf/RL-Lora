#!/usr/bin/env python3
import os
import argparse
import pandas as pd
import matplotlib
matplotlib.use('Agg')  # 必须在其他matplotlib导入前设置
import matplotlib.pyplot as plt

def update_plot(output_dir, current_node, plot_path):
    """动态更新对比图表"""
    plt.figure(figsize=(15, 6))
    plt.suptitle(f"Live Results (Current: {current_node} nodes)", y=1.05)
    
    # 准备数据容器
    methods = ['ADR-RL', 'ADR-DQN', 'ADR-AVG', 'ADR-MAX', 'ADR-MIN']
    data = {'goodput': {m: [] for m in methods}, 
            'energy': {m: [] for m in methods}}
    nodes = []

    # 收集已完成节点数据
    for n in sorted([int(d.split('_')[1]) for d in os.listdir(output_dir) 
                   if d.startswith('nodes_') and int(d.split('_')[1]) <= current_node]):
        node_dir = f"{output_dir}/nodes_{n}"
        
        # 读取吞吐量
        goodput_file = f"{node_dir}/1-goodput_results.csv"
        if os.path.exists(goodput_file):
            with open(goodput_file) as f:
                lines = f.readlines()
                if len(lines) >= 2:
                    values = list(map(float, lines[1].strip().split(',')))
                    nodes.append(n)
                    for i, m in enumerate(methods):
                        data['goodput'][m].append(values[i])
        
        # 读取能耗
        energy_file = f"{node_dir}/1-energy_results.csv"
        if os.path.exists(energy_file):
            with open(energy_file) as f:
                lines = f.readlines()
                if len(lines) >= 2:
                    values = list(map(float, lines[1].strip().split(',')))
                    for i, m in enumerate(methods):
                        data['energy'][m].append(values[i])

    # 绘制吞吐量对比
    plt.subplot(1, 2, 1)
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd']
    for i, method in enumerate(methods):
        if nodes and data['goodput'][method]:
            plt.plot(nodes, data['goodput'][method], 
                    color=colors[i], marker='o', label=method)
    plt.xlabel('Number of Nodes')
    plt.ylabel('Goodput')
    plt.title('Goodput Comparison')
    plt.grid(True, alpha=0.3)
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')

    # 绘制能耗对比
    plt.subplot(1, 2, 2)
    for i, method in enumerate(methods):
        if nodes and data['energy'][method]:
            plt.plot(nodes, data['energy'][method], 
                    color=colors[i], marker='o', label=method)
    plt.xlabel('Number of Nodes')
    plt.ylabel('Energy Consumption')
    plt.title('Energy Comparison')
    plt.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(plot_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"Updated live plot at {plot_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output_dir", required=True)
    parser.add_argument("--current_nodes", type=int, required=True)
    parser.add_argument("--plot_dir", required=True)
    args = parser.parse_args()
    
    update_plot(args.output_dir, args.current_nodes, args.plot_dir)
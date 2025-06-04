#!/usr/bin/env python3
import pandas as pd
import matplotlib
matplotlib.use('Agg')  # 必须在其他matplotlib导入前设置
import matplotlib.pyplot as plt
import os
import argparse
import numpy as np

def load_data(output_dir, node_counts, metrics):
    """
    加载所有节点的CSV数据
    :param output_dir: 结果主目录
    :param node_counts: 节点数量列表
    :param metrics: 指标类型 ('goodput' 或 'energy')
    :return: 结构化数据字典
    """
    methods = ['ADR-RL', 'ADR-DQN', 'ADR-AVG', 'ADR-MAX', 'ADR-MIN']
    data = {}  # 初始化数据结构
    for method in methods:
        values = []
        for n in node_counts:
            csv_file = os.path.join(
                output_dir,
                f"nodes_{n}",
                f"1-{metrics}_results.csv"
            )
            
            if not os.path.exists(csv_file):
                raise FileNotFoundError(f"文件不存在: {csv_file}")
            df = pd.read_csv(csv_file, header=0)  # 第一行为列名
                # 获取对应ADR方法的值
            value = df[method].values[0]
            values.append(float(value))

        key = f"{method}"
        data[key] = values
            # # 读取CSV文件
            # with open(csv_file, 'r') as f:
            #     lines = f.readlines()
            #     if len(lines) < 2:  # 至少应该有标题行和数据行
            #         raise ValueError(f"文件格式错误: {csv_file}")
                
            #     # 第二行是数据行
            #     values = lines[1].strip().split(',')
            #     if len(values) != 5:
            #         raise ValueError(f"数据列数不正确，应为5列: {csv_file}")
                
                # 将值对应到各方法
                # for i, method in enumerate(methods):
                #     try:
                #         data[method].append(float(values[i]))
                #     except ValueError:
                #         raise ValueError(f"无法解析数据: {values[i]} in {csv_file}")
    
    return data

def plot_comparison(data, node_counts, ylabel, title, output_path):
    """
    生成对比图表
    :param data: 数据字典
    :param node_counts: X轴节点数量
    :param ylabel: Y轴标签
    :param title: 图表标题
    :param output_path: 输出文件路径
    """
    plt.figure(figsize=(12, 6))
    
    colors = {
        'ADR-RL': '#1f77b4',    # 蓝色
        'ADR-DQN': '#ff7f0e',   # 橙色
        'ADR-AVG': '#2ca02c',    # 绿色
        'ADR-MAX': '#d62728',    # 红色
        'ADR-MIN': '#9467bd'     # 紫色
    }

    line_styles = {
        'ADR-RL': '-',          # 实线
        'ADR-DQN': '-',         # 实线
        'ADR-AVG': '--',        # 虚线
        'ADR-MIN': '--',        # 虚线
        'ADR-MAX': '--'         # 虚线
    }

    # 绘制每条曲线
    for method, values in data.items():
        plt.plot(
            node_counts, values,
            linestyle=line_styles[method],
            color=colors[method],
            marker='o',
            label=method
        )
    
    # 图表装饰
    plt.xlabel('Number of Nodes', fontsize=12)
    plt.ylabel(ylabel, fontsize=12)
    plt.title(title, fontsize=14)
    plt.xticks(node_counts)
    plt.grid(True, linestyle='--', alpha=0.3)
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"图表已保存至: {os.path.abspath(output_path)}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='生成RL性能对比图')
    parser.add_argument('--output_dir', required=True, help='结果目录路径')
    parser.add_argument('--plot_dir', default='./plots', help='图表输出目录')
    args = parser.parse_args()

    
    # 配置参数
    # node_counts = [1000,2000,3000,4000,5000,6000,7000,8000,9000,10000]  # 节点数量列表
    node_counts = [30,60,90,120,150,180,210,240,270,300]  # 节点数量列表

    # NODE_COUNTS=(30 60 90 120 150 180 210 240 270 300)
    os.makedirs(args.plot_dir, exist_ok=True)
    
    try:
        # 处理吞吐量数据
        goodput_data = load_data(args.output_dir, node_counts, 'goodput')
        plot_comparison(
            goodput_data, node_counts,
            ylabel='Goodput',
            title='Throughput Comparison: Different ADR Methods',
            output_path=os.path.join(args.plot_dir, 'goodput__comparison.png')
        )

        # 处理能耗数据
        energy_data = load_data(args.output_dir, node_counts, 'energy')
        plot_comparison(
            energy_data, node_counts,
            ylabel='Energy Consumption',
            title='Energy Comparison: Different ADR Methods',
            output_path=os.path.join(args.plot_dir, 'energy__comparison.png')
        )
        
    except Exception as e:
        print(f"错误: {str(e)}")
        exit(1)
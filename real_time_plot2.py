#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import argparse
import time
import sys

# 配置参数
plt.style.use('default')
colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd']  # 五种方法对应颜色
line_styles = ['-', '--', '-.', ':', '-']  # 线型
markers = ['o', 's', '^', 'D', 'v']  # 数据点标记
ADRMETHODS = ['ADR-RL', 'ADR-DQN', 'ADR-AVG', 'ADR-MAX', 'ADR-MIN']  # 五种ADR方法

def parse_args():
    parser = argparse.ArgumentParser(description='LoRaWAN ADR实时性能监控')
    parser.add_argument('--output_dir', required=True, help='结果主目录')
    parser.add_argument('--distribution', required=True, choices=['dispersed', 'clustered'], 
                       help='节点分布类型')
    parser.add_argument('--current_nodes', type=int, required=True, help='当前测试的节点数量')
    parser.add_argument('--plot_dir', required=True, help='绘图输出路径')
    return parser.parse_args()

    # 额外验证目录是否存在
    if not os.path.exists(args.output_dir):
        raise FileNotFoundError(f"Output directory not found: {args.output_dir}")
    return args

def load_data(output_dir, distribution):
    data = {'goodput': {}, 'energy': {}}
    base_path = f"{output_dir}/{distribution}"
    if not os.path.exists(base_path):
        raise FileNotFoundError(f"Directory not found: {base_path}")

    node_dirs = [d for d in os.listdir(base_path) if d.startswith('nodes_')]
    
    for node_dir in node_dirs:
        try:
            node_count = int(node_dir.split('_')[1])
            # 处理 goodput 数据
            goodput_path = f"{base_path}/{node_dir}/goodput_results.csv"
            if os.path.exists(goodput_path):
                df = pd.read_csv(goodput_path, header=0)
                data['goodput'][node_count] = df.mean()
            
            # 处理 energy 数据
            energy_path = f"{base_path}/{node_dir}/energy_results.csv"
            if os.path.exists(energy_path):
                df = pd.read_csv(energy_path, header=0)
                data['energy'][node_count] = df.mean()
                
        except Exception as e:
            print(f"Error processing {node_dir}: {str(e)}", file=sys.stderr)
    
    return data

def plot_comparison(data, metric, distribution, output_path):
    """绘制指定指标的对比曲线"""
    plt.figure(figsize=(12, 6))
    ax = plt.gca()
    
    # 提取有效节点数（按升序排序）
    valid_nodes = sorted([n for n in data[metric].keys() if not data[metric][n].empty])
    
    # 绘制每种ADR方法的曲线
    for idx, method in enumerate(ADRMETHODS):
        values = []
        for node in valid_nodes:
            try:
                values.append(data[metric][node][method])
            except KeyError:
                values.append(float('nan'))
        
        plt.plot(valid_nodes, values, 
                color=colors[idx],
                linestyle=line_styles[idx],
                marker=markers[idx],
                markersize=8,
                linewidth=2,
                label=method)
    
    # 图表装饰
    plt.title(f"{metric.capitalize()} Comparison ({distribution.capitalize()} Distribution)", fontsize=14)
    plt.xlabel("Number of Nodes", fontsize=12)
    plt.ylabel("Goodput (bps)" if metric == 'goodput' else "Energy Consumption (J)", fontsize=12)
    plt.grid(True, alpha=0.3)
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))  # X轴只显示整数
    
    # 自动调整图例位置
    plt.legend(loc='upper center', bbox_to_anchor=(0.5, -0.15),
              fancybox=True, shadow=True, ncol=3)
    
    plt.tight_layout()
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    plt.close()

def generate_live_plot(output_dir, distribution, current_nodes, plot_dir):
    """生成实时监控图表"""
    try:
        data = load_data(output_dir, distribution)
        if not data['goodput'] or not data['energy']:
            raise ValueError("No valid data found in output directory!")
        
        # 调试输出
        print(f"Data loaded successfully: goodput={len(data['goodput'])}, energy={len(data['energy'])}")
        
        # 绘制图表（确保目录存在）
        os.makedirs(os.path.dirname(plot_dir), exist_ok=True)
        goodput_plot = plot_dir.replace('.png', '_goodput.png')
        energy_plot = plot_dir.replace('.png', '_energy.png')
        
        plot_comparison(data, 'goodput', distribution, goodput_plot)
        plot_comparison(data, 'energy', distribution, energy_plot)
        
        print(f"Plots saved to: {goodput_plot}, {energy_plot}")
    except Exception as e:
        print(f"Error in generate_live_plot: {str(e)}", file=sys.stderr)
        raise  # 终止并显示错误


def merge_plots(plot1, plot2, output_path):
    """合并两个图表到一个文件（横向排列）"""
    from PIL import Image
    
    img1 = Image.open(plot1)
    img2 = Image.open(plot2)
    
    total_width = img1.width + img2.width
    max_height = max(img1.height, img2.height)
    
    new_img = Image.new('RGB', (total_width, max_height))
    new_img.paste(img1, (0, 0))
    new_img.paste(img2, (img1.width, 0))
    new_img.save(output_path)

if __name__ == "__main__":
    args = parse_args()
    
    # 确保输出目录存在
    os.makedirs(os.path.dirname(args.plot_dir), exist_ok=True)
    
    # 生成实时图表
    start_time = time.time()
    try:
        generate_live_plot(args.output_dir, args.distribution, args.current_nodes, args.plot_dir)
        print(f"Plot generation completed in {time.time()-start_time:.2f}s")
    except Exception as e:
        print(f"Error generating plot: {str(e)}")
        exit(1)
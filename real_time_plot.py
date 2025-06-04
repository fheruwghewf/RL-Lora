import os
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import argparse
from glob import glob

def update_plot(output_dir, current_node):
    """动态更新对比图表"""
    plt.figure(figsize=(12, 6))
    
    # 收集所有已完成的节点数据
    goodput_data = []
    energy_data = []
    
    # 获取所有节点目录并按节点数排序
    node_dirs = sorted(
        [d for d in glob(f"{output_dir}/nodes_*") if os.path.isdir(d)],
        key=lambda x: int(os.path.basename(x).split('_')[1]))
    
    for dir_path in node_dirs:
        node_num = int(os.path.basename(dir_path).split('_')[1])
        if node_num > current_node:
            continue  # 跳过未完成的节点
        
        # 检查CRDSA和非CRDSA结果文件
        for crdsa_mode in ['0', '1']:
            try:
                # 读取吞吐量 (跳过标题行)
                goodput_file = f"{dir_path}/{crdsa_mode}-goodput_results.csv"
                if os.path.exists(goodput_file):
                    df = pd.read_csv(goodput_file, header=0)  # 第一行作为列名
                    if len(df) >= 1:  # 至少有一行数据(标题行不算)
                        goodput_data.append({
                            'nodes': node_num,
                            'crdsa': crdsa_mode,
                            'values': df.iloc[0].tolist()  # 获取第一行数据
                        })
                
                # 读取能耗 (跳过标题行)
                energy_file = f"{dir_path}/{crdsa_mode}-energy_results.csv"
                if os.path.exists(energy_file):
                    df = pd.read_csv(energy_file, header=0)  # 第一行作为列名
                    if len(df) >= 1:
                        energy_data.append({
                            'nodes': node_num,
                            'crdsa': crdsa_mode,
                            'values': df.iloc[0].tolist()
                        })
            except Exception as e:
                print(f"Error processing {dir_path}: {e}")
                continue

    if not goodput_data or not energy_data:
        print("No valid data found for plotting")
        return

    # 绘制吞吐量对比
    plt.subplot(1, 2, 1)
    for crdsa_mode in ['0', '1']:
        crdsa_label = "Without CRDSA" if crdsa_mode == '0' else "With CRDSA"
        for method_idx, method in enumerate(['ADR-AVG', 'ADR-MAX', 'ADR-MIN']):
            x = [d['nodes'] for d in goodput_data if d['crdsa'] == crdsa_mode]
            y = [d['values'][method_idx] for d in goodput_data if d['crdsa'] == crdsa_mode]
            if x and y:
                plt.plot(x, y, marker='o', linestyle='-' if crdsa_mode == '0' else '--',
                        label=f"{method} ({crdsa_label})")
    plt.title("Goodput Comparison")
    plt.xlabel("Number of Nodes")
    plt.ylabel("Goodput")
    plt.grid(True, linestyle='--', alpha=0.6)
    
    # 绘制能耗对比
    plt.subplot(1, 2, 2)
    for crdsa_mode in ['0', '1']:
        crdsa_label = "Without CRDSA" if crdsa_mode == '0' else "With CRDSA"
        for method_idx, method in enumerate(['ADR-AVG', 'ADR-MAX', 'ADR-MIN']):
            x = [d['nodes'] for d in energy_data if d['crdsa'] == crdsa_mode]
            y = [d['values'][method_idx] for d in energy_data if d['crdsa'] == crdsa_mode]
            if x and y:
                plt.plot(x, y, marker='o', linestyle='-' if crdsa_mode == '0' else '--',
                        label=f"{method} ({crdsa_label})")
    plt.title("Energy Consumption")
    plt.xlabel("Number of Nodes")
    plt.ylabel("Energy")
    plt.grid(True, linestyle='--', alpha=0.6)
    
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.tight_layout()
    
    plot_path = f"{output_dir}/live_comparison.png"
    plt.savefig(plot_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"Live plot updated: {plot_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output_dir", required=True)
    parser.add_argument("--current_nodes", type=int, required=True)
    args = parser.parse_args()
    
    update_plot(args.output_dir, args.current_nodes)
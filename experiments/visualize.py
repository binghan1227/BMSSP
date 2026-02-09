#!/usr/bin/env python3
"""BMSSP experiment result visualizer.

Reads CSV files produced by run_experiments.py and generates plots.
"""

import argparse
import csv
import os
import statistics
import sys

import matplotlib.pyplot as plt
import numpy as np


def read_csv(path):
    """Read a CSV file and return list of dicts with numeric conversion."""
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        rows = []
        for row in reader:
            converted = {}
            for k, v in row.items():
                try:
                    converted[k] = int(v)
                except ValueError:
                    try:
                        converted[k] = float(v)
                    except ValueError:
                        converted[k] = v
            rows.append(converted)
    return rows


def aggregate(rows, group_key):
    """Group rows by a key and compute stats on time_ms."""
    groups = {}
    for row in rows:
        key = row[group_key]
        groups.setdefault(key, []).append(row["time_ms"])
    result = []
    for key in sorted(groups):
        timings = groups[key]
        result.append({
            group_key: key,
            "timings": timings,
            "mean": statistics.mean(timings),
            "std": statistics.stdev(timings) if len(timings) > 1 else 0,
            "min": min(timings),
            "max": max(timings),
        })
    return result


def plot_node_scaling(rows, output_dir, show):
    """Plot runtime vs. graph size (node scaling experiment)."""
    stats = aggregate(rows, "nodes")
    if not stats:
        return

    # Infer edge multiplier from first row
    first = rows[0]
    edge_mult = first["edges"] // first["nodes"]

    nodes = [s["nodes"] for s in stats]
    means = [s["mean"] for s in stats]
    stds = [s["std"] for s in stats]

    fig, ax = plt.subplots(figsize=(10, 7))

    ax.errorbar(
        nodes, means, yerr=stds,
        fmt="o-", capsize=5, capthick=2,
        markersize=8, linewidth=2,
        color="#2E86AB", ecolor="#A23B72",
        label="BMSSP Runtime",
    )

    # Individual data points
    for s in stats:
        xs = [s["nodes"]] * len(s["timings"])
        ax.scatter(xs, s["timings"], alpha=0.3, s=20, color="#2E86AB")

    ax.set_xlabel("Number of Nodes (n)", fontsize=14)
    ax.set_ylabel("Runtime (ms)", fontsize=14)
    ax.set_title(f"BMSSP Runtime vs. Graph Size\n(Edge Density: m = {edge_mult}n)", fontsize=16)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xticks(nodes)
    ax.set_xticklabels([f"{n // 1000}K" for n in nodes])
    ax.legend(fontsize=12)
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    path = os.path.join(output_dir, "node_scaling.png")
    fig.savefig(path, dpi=150)
    print(f"Saved {path}")
    if show:
        plt.show()
    plt.close(fig)


def plot_edge_density(rows, output_dir, show):
    """Plot runtime vs. edge multiplier (edge density experiment)."""
    stats = aggregate(rows, "multiplier")
    if not stats:
        return

    fixed_nodes = rows[0]["nodes"]
    multipliers = [s["multiplier"] for s in stats]
    means = [s["mean"] for s in stats]
    stds = [s["std"] for s in stats]

    fig, ax = plt.subplots(figsize=(10, 7))

    ax.errorbar(
        multipliers, means, yerr=stds,
        fmt="s-", capsize=5, capthick=2,
        markersize=8, linewidth=2,
        color="#E94F37", ecolor="#393E41",
        label="BMSSP Runtime",
    )

    for s in stats:
        xs = [s["multiplier"]] * len(s["timings"])
        ax.scatter(xs, s["timings"], alpha=0.3, s=20, color="#E94F37")

    ax.set_xlabel("Edge Multiplier (m / n)", fontsize=14)
    ax.set_ylabel("Runtime (ms)", fontsize=14)
    ax.set_title(f"BMSSP Runtime vs. Edge Density\n(Fixed n = {fixed_nodes:,})", fontsize=16)
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks(multipliers)
    ax.set_xticklabels([str(m) for m in multipliers])
    ax.legend(fontsize=12)
    ax.grid(True, alpha=0.3)

    # Secondary x-axis with actual edge counts
    ax2 = ax.twiny()
    ax2.set_xscale("log", base=2)
    ax2.set_xlim(ax.get_xlim())
    ax2.set_xticks(multipliers)
    ax2.set_xticklabels([f"{m * fixed_nodes // 1000}K" for m in multipliers])
    ax2.set_xlabel("Number of Edges (m)", fontsize=12)

    plt.tight_layout()
    path = os.path.join(output_dir, "edge_density.png")
    fig.savefig(path, dpi=150)
    print(f"Saved {path}")
    if show:
        plt.show()
    plt.close(fig)


def plot_combined(node_rows, edge_rows, output_dir, show):
    """Side-by-side combined analysis plot."""
    node_stats = aggregate(node_rows, "nodes")
    edge_stats = aggregate(edge_rows, "multiplier")
    if not node_stats or not edge_stats:
        return

    edge_mult = node_rows[0]["edges"] // node_rows[0]["nodes"]
    fixed_nodes = edge_rows[0]["nodes"]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # Left: node scaling
    nodes = [s["nodes"] for s in node_stats]
    ax1.errorbar(
        nodes, [s["mean"] for s in node_stats], yerr=[s["std"] for s in node_stats],
        fmt="o-", capsize=5, capthick=2,
        markersize=8, linewidth=2,
        color="#2E86AB", ecolor="#A23B72",
    )
    ax1.set_xlabel("Number of Nodes (n)", fontsize=12)
    ax1.set_ylabel("Runtime (ms)", fontsize=12)
    ax1.set_title(f"Runtime vs. Graph Size (m = {edge_mult}n)", fontsize=14)
    ax1.set_xscale("log")
    ax1.set_yscale("log")
    ax1.set_xticks(nodes)
    ax1.set_xticklabels([f"{n // 1000}K" for n in nodes])
    ax1.grid(True, alpha=0.3)

    # Right: edge density
    multipliers = [s["multiplier"] for s in edge_stats]
    ax2.errorbar(
        multipliers, [s["mean"] for s in edge_stats], yerr=[s["std"] for s in edge_stats],
        fmt="s-", capsize=5, capthick=2,
        markersize=8, linewidth=2,
        color="#E94F37", ecolor="#393E41",
    )
    ax2.set_xlabel("Edge Multiplier (m / n)", fontsize=12)
    ax2.set_ylabel("Runtime (ms)", fontsize=12)
    ax2.set_title(f"Runtime vs. Edge Density (n = {fixed_nodes:,})", fontsize=14)
    ax2.set_xscale("log", base=2)
    ax2.set_yscale("log")
    ax2.set_xticks(multipliers)
    ax2.set_xticklabels([str(m) for m in multipliers])
    ax2.grid(True, alpha=0.3)

    plt.suptitle("BMSSP Solver Performance Analysis", fontsize=16, y=1.02)
    plt.tight_layout()
    path = os.path.join(output_dir, "combined.png")
    fig.savefig(path, dpi=150, bbox_inches="tight")
    print(f"Saved {path}")
    if show:
        plt.show()
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Visualize BMSSP experiment results"
    )
    parser.add_argument(
        "--results-dir",
        default="results",
        help="Directory containing CSV files (default: results/)",
    )
    parser.add_argument(
        "--output-dir",
        default="plots",
        help="Directory for plot images (default: plots/)",
    )
    parser.add_argument(
        "--no-show",
        action="store_true",
        help="Don't display plots interactively (just save)",
    )
    args = parser.parse_args()

    plt.style.use("seaborn-v0_8-whitegrid")
    os.makedirs(args.output_dir, exist_ok=True)
    show = not args.no_show

    node_csv = os.path.join(args.results_dir, "node_scaling.csv")
    edge_csv = os.path.join(args.results_dir, "edge_density.csv")

    node_rows = None
    edge_rows = None

    if os.path.isfile(node_csv):
        node_rows = read_csv(node_csv)
        print(f"Loaded {len(node_rows)} rows from {node_csv}")
        plot_node_scaling(node_rows, args.output_dir, show)
    else:
        print(f"Skipping node scaling plot ({node_csv} not found)")

    if os.path.isfile(edge_csv):
        edge_rows = read_csv(edge_csv)
        print(f"Loaded {len(edge_rows)} rows from {edge_csv}")
        plot_edge_density(edge_rows, args.output_dir, show)
    else:
        print(f"Skipping edge density plot ({edge_csv} not found)")

    if node_rows and edge_rows:
        plot_combined(node_rows, edge_rows, args.output_dir, show)
    else:
        print("Skipping combined plot (need both node_scaling.csv and edge_density.csv)")

    print("\nDone.")


if __name__ == "__main__":
    main()

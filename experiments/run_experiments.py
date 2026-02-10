#!/usr/bin/env python3
"""BMSSP solver experiment runner.

Generates random graphs, runs the solver, and logs per-trial results to CSV files.
"""

import argparse
import csv
import os
import random
import re
import subprocess
import sys
import tempfile

WEIGHT_MIN = 0.1
WEIGHT_MAX = 100.0


def generate_connected_graph(n, m, seed=None):
    """Generate a random connected directed graph.

    Ensures connectivity by first creating a spanning tree, then adding
    random edges until the target edge count is reached.

    Returns list of (u, v, w) tuples.
    """
    if seed is not None:
        random.seed(seed)

    if m < n - 1:
        raise ValueError(f"Need at least {n-1} edges for {n} nodes to be connected")

    edges = []
    edge_set = set()

    # Step 1: spanning tree for connectivity
    nodes = list(range(n))
    random.shuffle(nodes)
    for i in range(1, n):
        parent = random.randint(0, i - 1)
        u, v = nodes[parent], nodes[i]
        w = random.uniform(WEIGHT_MIN, WEIGHT_MAX)
        edges.append((u, v, w))
        edge_set.add((u, v))

    # Step 2: remaining random edges
    remaining = m - (n - 1)
    attempts = 0
    max_attempts = remaining * 100
    while len(edges) < m and attempts < max_attempts:
        u = random.randint(0, n - 1)
        v = random.randint(0, n - 1)
        if u != v and (u, v) not in edge_set:
            w = random.uniform(WEIGHT_MIN, WEIGHT_MAX)
            edges.append((u, v, w))
            edge_set.add((u, v))
        attempts += 1

    random.shuffle(edges)
    return edges


def write_graph_to_file(filepath, n, edges, source=0):
    """Write graph in the solver's stdin format."""
    with open(filepath, "w") as f:
        f.write(f"{n} {len(edges)}\n")
        for u, v, w in edges:
            f.write(f"{u} {v} {w:.4f}\n")
        f.write(f"{source}\n")


def extract_timing(output, label="BMSSP"):
    """Extract timing in milliseconds from solver output."""
    match = re.search(rf"{label} Time:\s*([\d.]+)\s*ms", output)
    if match:
        return float(match.group(1))
    raise ValueError(f"Could not extract timing from output: {output[:200]}")


def run_solver(solver_path, input_file, label="BMSSP", timeout=300):
    """Run the solver on an input file. Returns (time_ms, success)."""
    try:
        with open(input_file, "r") as f:
            result = subprocess.run(
                [solver_path],
                stdin=f,
                capture_output=True,
                text=True,
                timeout=timeout,
            )
        if result.returncode != 0:
            print(f"  Warning: Non-zero exit code: {result.returncode}")
            return 0.0, False
        return extract_timing(result.stdout, label), True
    except subprocess.TimeoutExpired:
        print("  Warning: Solver timed out")
        return 0.0, False
    except Exception as e:
        print(f"  Error: {e}")
        return 0.0, False


def make_seed(n, m, trial):
    """Deterministic seed derived from experiment configuration."""
    return hash((n, m, trial)) & 0x7FFFFFFF


def run_node_scaling(solver_path, node_counts, edge_multiplier, trials, output_dir,
                     dijkstra_path=None):
    """Run node-scaling experiment and write results to CSV."""
    csv_path = os.path.join(output_dir, "node_scaling.csv")
    print("=" * 60)
    print("NODE SCALING EXPERIMENT")
    print(f"Edge density: m = {edge_multiplier} * n")
    print(f"Node counts: {node_counts}")
    print(f"Trials per configuration: {trials}")
    if dijkstra_path:
        print("Dijkstra baseline: enabled")
    print("=" * 60)

    solvers = [("bmssp", solver_path, "BMSSP")]
    if dijkstra_path:
        solvers.append(("dijkstra", dijkstra_path, "Dijkstra"))

    with open(csv_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["nodes", "edges", "trial", "seed", "solver", "time_ms"])

        with tempfile.TemporaryDirectory() as tmpdir:
            for n in node_counts:
                m = edge_multiplier * n
                print(f"\nn={n:,}, m={m:,}")
                for trial in range(trials):
                    seed = make_seed(n, m, trial)
                    graph_file = os.path.join(tmpdir, "graph.in")
                    edges = generate_connected_graph(n, m, seed=seed)
                    write_graph_to_file(graph_file, n, edges)
                    for solver_name, spath, label in solvers:
                        timing, success = run_solver(spath, graph_file, label)
                        if success:
                            writer.writerow([n, m, trial, seed, solver_name, f"{timing:.4f}"])
                            print(f"  Trial {trial+1}/{trials} [{solver_name}]: {timing:.2f} ms")
                        else:
                            print(f"  Trial {trial+1}/{trials} [{solver_name}]: FAILED")

    print(f"\nResults saved to {csv_path}")


def run_edge_density(solver_path, fixed_nodes, edge_multipliers, trials, output_dir,
                     dijkstra_path=None):
    """Run edge-density experiment and write results to CSV."""
    csv_path = os.path.join(output_dir, "edge_density.csv")
    print("\n" + "=" * 60)
    print("EDGE DENSITY EXPERIMENT")
    print(f"Fixed nodes: n = {fixed_nodes:,}")
    print(f"Edge multipliers: {edge_multipliers}")
    print(f"Trials per configuration: {trials}")
    if dijkstra_path:
        print("Dijkstra baseline: enabled")
    print("=" * 60)

    solvers = [("bmssp", solver_path, "BMSSP")]
    if dijkstra_path:
        solvers.append(("dijkstra", dijkstra_path, "Dijkstra"))

    with open(csv_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["nodes", "edges", "multiplier", "trial", "seed", "solver", "time_ms"])

        with tempfile.TemporaryDirectory() as tmpdir:
            for multiplier in edge_multipliers:
                n = fixed_nodes
                m = multiplier * n
                print(f"\nn={n:,}, m={m:,} (multiplier={multiplier})")
                for trial in range(trials):
                    seed = make_seed(n, m, trial)
                    graph_file = os.path.join(tmpdir, "graph.in")
                    edges = generate_connected_graph(n, m, seed=seed)
                    write_graph_to_file(graph_file, n, edges)
                    for solver_name, spath, label in solvers:
                        timing, success = run_solver(spath, graph_file, label)
                        if success:
                            writer.writerow([n, m, multiplier, trial, seed, solver_name, f"{timing:.4f}"])
                            print(f"  Trial {trial+1}/{trials} [{solver_name}]: {timing:.2f} ms")
                        else:
                            print(f"  Trial {trial+1}/{trials} [{solver_name}]: FAILED")

    print(f"\nResults saved to {csv_path}")


def parse_int_list(s):
    """Parse a comma-separated list of integers."""
    return [int(x.strip()) for x in s.split(",")]


def main():
    parser = argparse.ArgumentParser(
        description="Run BMSSP solver performance experiments"
    )
    parser.add_argument(
        "--solver",
        default="../build/bmssp_solver",
        help="Path to solver binary (default: ../build/bmssp_solver)",
    )
    parser.add_argument(
        "--output-dir",
        default="results",
        help="Output directory for CSV results (default: results/)",
    )
    parser.add_argument(
        "--trials",
        type=int,
        default=5,
        help="Number of trials per configuration (default: 5)",
    )
    parser.add_argument(
        "--node-counts",
        type=parse_int_list,
        default=[10000, 20000, 40000, 80000, 160000],
        help="Comma-separated node counts (default: 10000,20000,40000,80000,160000)",
    )
    parser.add_argument(
        "--edge-multiplier",
        type=int,
        default=4,
        help="Edge multiplier for node scaling experiment (default: 4)",
    )
    parser.add_argument(
        "--fixed-nodes",
        type=int,
        default=20000,
        help="Fixed node count for edge density experiment (default: 20000)",
    )
    parser.add_argument(
        "--edge-multipliers",
        type=parse_int_list,
        default=[2, 4, 8, 16, 32, 64],
        help="Comma-separated multipliers for edge density (default: 2,4,8,16,32,64)",
    )
    parser.add_argument(
        "--skip-node-scaling",
        action="store_true",
        help="Skip the node scaling experiment",
    )
    parser.add_argument(
        "--skip-edge-density",
        action="store_true",
        help="Skip the edge density experiment",
    )
    parser.add_argument(
        "--dijkstra-solver",
        default="../build/dijkstra_solver",
        help="Path to Dijkstra baseline binary (default: ../build/dijkstra_solver)",
    )
    parser.add_argument(
        "--skip-dijkstra",
        action="store_true",
        help="Skip the Dijkstra baseline comparison",
    )

    args = parser.parse_args()

    solver_path = os.path.abspath(args.solver)
    if not os.path.isfile(solver_path):
        print(f"Error: Solver not found at {solver_path}", file=sys.stderr)
        print("Build it first: cd .. && cmake -B build && cmake --build build", file=sys.stderr)
        sys.exit(1)

    dijkstra_path = None
    if not args.skip_dijkstra:
        dp = os.path.abspath(args.dijkstra_solver)
        if os.path.isfile(dp):
            dijkstra_path = dp
            print(f"Dijkstra baseline: {dijkstra_path}")
        else:
            print(f"Warning: Dijkstra solver not found at {dp}, skipping baseline")

    os.makedirs(args.output_dir, exist_ok=True)

    if not args.skip_node_scaling:
        run_node_scaling(
            solver_path, args.node_counts, args.edge_multiplier, args.trials, args.output_dir,
            dijkstra_path,
        )

    if not args.skip_edge_density:
        run_edge_density(
            solver_path, args.fixed_nodes, args.edge_multipliers, args.trials, args.output_dir,
            dijkstra_path,
        )

    print("\nAll experiments complete.")


if __name__ == "__main__":
    main()

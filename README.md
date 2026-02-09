# BMSSP Solver

Implementation of the BMSSP single-source shortest path algorithm with a block-list data structure and an optional trace visualizer. This repository includes a CLI solver, BlockList correctness tests, and a browser-based visualizer for trace output.

## Features
- C++17 implementation of BMSSP for directed weighted graphs.
- BlockList data structure with unit tests.
- Optional JSONL tracing to drive the visualizer.
- Sample inputs for quick validation.

## Build

```bash
cmake -S . -B build
cmake --build build
```

This produces the following binaries in `build/`:
- `bmssp_solver`
- `test_block_list`

## Run

Input format:
- First line: `n m` (number of nodes and directed edges)
- Next `m` lines: `u v w` (edge from `u` to `v` with weight `w`)
- Final line: `source` (start node index)

Example:

```bash
./build/bmssp_solver < sample.in
```

You can also try a minimal case:

```bash
./build/bmssp_solver < test_input.txt
```

## Output

The solver prints a timing line followed by distances from the source.
Unreachable nodes are shown as `INF`.

Example snippet:

```
BMSSP Time: 0.123 ms
--------------------
Node 0: 0
Node 1: 2
Node 2: INF
```

## Tracing and Visualizer

Enable trace output at build time:

```bash
cmake -S . -B build -DBMSSP_ENABLE_TRACE=ON
cmake --build build
```

Running the solver with tracing writes `bmssp_trace.jsonl` in the current working directory. Open the visualizer in `bmssp-viz/index.html`, then load:

- Graph file: `.in` or `.txt` (for example `sample.in`)
- Trace file: `bmssp_trace.jsonl`

The visualizer UI is static and can be opened directly in a browser.

## Tests

```bash
ctest --test-dir build
```

Or run the test binary directly:

```bash
./build/test_block_list
```

## Experiments

The `experiments/` directory contains scripts for benchmarking the solver on randomly generated graphs. The workflow has two steps: run experiments, then visualize.

### Running experiments

```bash
cd experiments
python3 run_experiments.py
```

This generates random connected directed graphs, runs the solver, and logs per-trial results to CSV files in `experiments/results/`. Seeds are deterministic so results are reproducible.

Two experiment types are included:
- **Node scaling** -- varies graph size with a fixed edge density (`m = k * n`).
- **Edge density** -- varies the number of edges with a fixed node count.

Key flags:

| Flag | Default | Description |
|------|---------|-------------|
| `--solver` | `../build/bmssp_solver` | Path to solver binary |
| `--trials` | `5` | Trials per configuration |
| `--node-counts` | `10000,20000,40000,80000,160000` | Comma-separated node counts |
| `--edge-multiplier` | `4` | Edge multiplier for node scaling (`m = k * n`) |
| `--fixed-nodes` | `20000` | Fixed node count for edge density |
| `--edge-multipliers` | `2,4,8,16,32,64` | Comma-separated multipliers for edge density |
| `--skip-node-scaling` | | Skip the node scaling experiment |
| `--skip-edge-density` | | Skip the edge density experiment |

### Visualizing results

```bash
cd experiments
python3 visualize.py --no-show
```

Reads the CSV files and produces plots (node scaling, edge density, and a combined side-by-side view). Plots are saved to `experiments/plots/`.

Requires `matplotlib` and `numpy`.

Key flags:

| Flag | Default | Description |
|------|---------|-------------|
| `--results-dir` | `results` | Directory containing CSV files |
| `--output-dir` | `plots` | Directory for plot images |
| `--no-show` | | Save plots without displaying interactively |

## Project Layout

- `main.cpp`: CLI entrypoint for the BMSSP solver.
- `bmssp.cpp`, `bmssp.h`: BMSSP algorithm implementation.
- `block_list.cpp`, `block_list.h`: BlockList data structure.
- `test_block_list.cpp`: BlockList correctness tests.
- `trace.h`: JSONL trace helpers (enabled via `BMSSP_ENABLE_TRACE`).
- `bmssp-viz/`: browser visualizer for trace playback.
- `experiments/`: performance benchmarking scripts and results.
- `sample.in`, `test_input.txt`: example inputs.

## Reference

BMSSP is described in the paper: arXiv:2504.17033v2.

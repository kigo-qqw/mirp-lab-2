#!/usr/bin/env python3
# /// script
# requires-python = ">=3.8"
# dependencies = [
#     "pandas>=1.3.0",
#     "matplotlib>=3.4.0",
# ]
# ///

import argparse
import csv
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, List

try:
    import matplotlib.pyplot as plt
    import pandas as pd

    HAS_PLOTTING = True
except ImportError:
    HAS_PLOTTING = False


def run_benchmark(
    binary: str,
    n: int,
    trace: str,
    methods: str,
    threads: int = 1,
    verbose: bool = False,
) -> List[Dict]:
    pattern = re.compile(r"\[([^\]]+)\] Time : ([0-9.]+)s")
    max_number_pattern = re.compile(r"\[([^\]]+)\] Maximum Number: (\d+)")

    cmd = [binary, f"--trace={trace}", f"--run={methods}", str(n)]

    env = os.environ.copy()
    env["OMP_NUM_THREADS"] = str(threads)

    if verbose:
        print(f"[DEBUG] Running: {' '.join(cmd)}", file=sys.stderr)

    try:
        process = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=True,
            timeout=600,
            env=env,
        )
        output = process.stdout

        results = []
        times = {}
        max_numbers = {}

        for method, time_val in pattern.findall(output):
            times[method] = float(time_val)

        for method, max_num in max_number_pattern.findall(output):
            max_numbers[method] = int(max_num)

        for method in times:
            results.append(
                {
                    "method": method,
                    "N": n,
                    "threads": threads,
                    "time": times.get(method, 0),
                    "max_number": max_numbers.get(method, 0),
                }
            )

        return results

    except subprocess.TimeoutExpired:
        print(f"[ERROR] Timeout at N={n}", file=sys.stderr)
        return []
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Failed to run {binary}: {e.stderr}", file=sys.stderr)
        return []
    except FileNotFoundError:
        print(f"[ERROR] Binary not found: {binary}", file=sys.stderr)
        sys.exit(1)


def append_result(output_path: Path, result: Dict) -> None:
    file_exists = output_path.exists()
    fieldnames = ["method", "N", "threads", "time", "max_number"]

    with open(output_path, "a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow(result)


def run_benchmarks(
    binary: str,
    n_values: List[int],
    trace: str,
    methods: str,
    threads: List[int],
    output_path: Path,
    verbose: bool,
    runs: int = 1,
) -> tuple[bool, bool]:
    methods_set = {m.strip() for m in methods.split(",")}
    run_sequential = any(
        m in methods_set for m in ["sequential", "sequential_cached"]
    )
    run_parallel = any(
        m in methods_set for m in ["parallel", "parallel_cached"]
    )
    seq_methods = ",".join(
        m for m in ["sequential", "sequential_cached"] if m in methods_set
    )
    par_methods = ",".join(
        m for m in ["parallel", "parallel_cached"] if m in methods_set
    )

    for n in n_values:
        if run_sequential and seq_methods:
            if verbose:
                print(
                    f"[INFO] Running N={n:,} (sequential, {runs} runs)...",
                    file=sys.stderr,
                )

            all_results = []
            for run_num in range(runs):
                results = run_benchmark(
                    binary, n, trace, seq_methods, threads[0], verbose
                )
                all_results.extend(results)

            avg_results = _average_results(all_results)
            if avg_results and verbose:
                for r in avg_results:
                    print(
                        f"  [{r['method']}] {r['time']:.6f}s (avg of {runs})",
                        file=sys.stderr,
                    )

            for result in avg_results:
                append_result(output_path, result)

        if run_parallel and par_methods:
            for t in threads:
                if verbose:
                    print(
                        f"[INFO] Running N={n:,} with {t} threads ({runs} runs)...",
                        file=sys.stderr,
                    )

                all_results = []
                for run_num in range(runs):
                    results = run_benchmark(
                        binary, n, trace, par_methods, t, verbose
                    )
                    all_results.extend(results)

                avg_results = _average_results(all_results)
                if avg_results and verbose:
                    for r in avg_results:
                        print(
                            f"  [{r['method']}] {r['time']:.6f}s (avg of {runs})",
                            file=sys.stderr,
                        )

                for result in avg_results:
                    append_result(output_path, result)

    return run_sequential, run_parallel


def _average_results(results: List[Dict]) -> List[Dict]:
    if not results:
        return []

    grouped = {}
    for result in results:
        key = (result["method"], result["N"], result["threads"])
        if key not in grouped:
            grouped[key] = {
                "times": [],
                "max_numbers": [],
                "method": result["method"],
                "N": result["N"],
                "threads": result["threads"],
            }
        grouped[key]["times"].append(result["time"])
        grouped[key]["max_numbers"].append(result["max_number"])

    avg_results = []
    for key, data in grouped.items():
        avg_results.append(
            {
                "method": data["method"],
                "N": data["N"],
                "threads": data["threads"],
                "time": sum(data["times"]) / len(data["times"]),
                "max_number": data["max_numbers"][0],
            }
        )

    return avg_results


def generate_time_vs_n_graph(
    df: pd.DataFrame,
    methods: set,
    plots_path: Path,
    graph_format: str,
    run_sequential: bool,
    run_parallel: bool,
) -> None:
    if run_sequential and run_parallel:
        max_threads = df["threads"].max()
        plt.figure(figsize=(10, 6))

        for method in sorted(
            [
                m.upper()
                for m in ["sequential", "sequential_cached"]
                if m in methods
            ]
        ):
            subset = df[df["method"] == method].sort_values("N")
            if not subset.empty:
                plt.plot(
                    subset["N"],
                    subset["time"],
                    marker="o",
                    label=method,
                    linewidth=2,
                )

        for method in sorted(
            [m.upper() for m in ["parallel", "parallel_cached"] if m in methods]
        ):
            subset = df[
                (df["method"] == method) & (df["threads"] == max_threads)
            ].sort_values("N")
            if not subset.empty:
                plt.plot(
                    subset["N"],
                    subset["time"],
                    marker="o",
                    label=f"{method} ({max_threads} threads)",
                    linewidth=2,
                )

        plt.xlabel("N", fontsize=12)
        plt.ylabel("Time (seconds)", fontsize=12)
        plt.title(
            "Execution Time vs Input Size", fontsize=14, fontweight="bold"
        )
        plt.legend(fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()

        file_path = plots_path / f"time_vs_N.{graph_format}"
        plt.savefig(str(file_path), dpi=300, format=graph_format)
        print(f"✓ Graph saved: {file_path}")
        plt.close()

    elif run_sequential:
        plt.figure(figsize=(10, 6))
        for method in sorted(
            [
                m.upper()
                for m in ["sequential", "sequential_cached"]
                if m in methods
            ]
        ):
            subset = df[df["method"] == method].sort_values("N")
            if not subset.empty:
                plt.plot(
                    subset["N"],
                    subset["time"],
                    marker="o",
                    label=method,
                    linewidth=2,
                )

        plt.xlabel("N", fontsize=12)
        plt.ylabel("Time (seconds)", fontsize=12)
        plt.title(
            "Sequential Methods - Time vs Input Size",
            fontsize=14,
            fontweight="bold",
        )
        plt.legend(fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()

        file_path = plots_path / f"time_vs_N.{graph_format}"
        plt.savefig(str(file_path), dpi=300, format=graph_format)
        print(f"✓ Graph saved: {file_path}")
        plt.close()


def generate_scaling_graphs(
    df: pd.DataFrame,
    methods: set,
    plots_path: Path,
    graph_format: str,
) -> None:
    for method in sorted(
        [m.upper() for m in ["parallel", "parallel_cached"] if m in methods]
    ):
        plt.figure(figsize=(10, 6))
        for threads in sorted(df["threads"].unique()):
            subset = df[
                (df["method"] == method) & (df["threads"] == threads)
            ].sort_values("N")
            if not subset.empty:
                plt.plot(
                    subset["N"],
                    subset["time"],
                    marker="o",
                    label=f"{threads} threads",
                    linewidth=2,
                )

        plt.xlabel("N", fontsize=12)
        plt.ylabel("Time (seconds)", fontsize=12)
        plt.title(
            f"{method.replace('_', ' ').title()} - Time vs Input Size",
            fontsize=14,
            fontweight="bold",
        )
        plt.legend(fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()

        file_path = plots_path / f"{method.lower()}_scaling.{graph_format}"
        plt.savefig(str(file_path), dpi=300, format=graph_format)
        print(f"✓ Graph saved: {file_path}")
        plt.close()


def generate_speedup_graphs(
    df: pd.DataFrame,
    methods: set,
    plots_path: Path,
    graph_format: str,
) -> None:
    for method in sorted(
        [m.upper() for m in ["parallel", "parallel_cached"] if m in methods]
    ):
        if method == "PARALLEL":
            seq_method = "SEQUENTIAL"
        else:
            seq_method = "SEQUENTIAL_CACHED"
            if seq_method not in df["method"].values:
                continue

        if seq_method not in df["method"].values:
            continue

        plt.figure(figsize=(10, 6))
        for threads in sorted(df["threads"].unique()):
            par_subset = df[
                (df["method"] == method) & (df["threads"] == threads)
            ].sort_values("N")

            if not par_subset.empty:
                seq_subset = df[
                    (df["method"] == seq_method)
                    & (df["N"].isin(par_subset["N"]))
                ].sort_values("N")

                if len(seq_subset) > 0 and len(par_subset) > 0:
                    speedup = (
                        seq_subset["time"].values / par_subset["time"].values
                    )
                    plt.plot(
                        par_subset["N"],
                        speedup,
                        marker="o",
                        label=f"{threads} threads",
                        linewidth=2,
                    )

        plt.xlabel("N", fontsize=12)
        plt.ylabel("Speedup vs Sequential", fontsize=12)
        plt.title(
            f"{method.replace('_', ' ').title()} - Speedup",
            fontsize=14,
            fontweight="bold",
        )
        plt.legend(fontsize=10)
        plt.grid(True, alpha=0.3)
        plt.tight_layout()

        file_path = plots_path / f"{method.lower()}_speedup.{graph_format}"
        plt.savefig(str(file_path), dpi=300, format=graph_format)
        print(f"✓ Graph saved: {file_path}")
        plt.close()


def main():
    parser = argparse.ArgumentParser(description="Collatz benchmark analysis")

    parser.add_argument(
        "--binary", default="./build/mirp-lab-2", help="Path to executable"
    )
    parser.add_argument(
        "--output", "-o", default="results.csv", help="Output CSV file"
    )
    parser.add_argument(
        "--plots", "-p", default="plots", help="Directory for graphs"
    )
    parser.add_argument(
        "--trace",
        choices=["none", "result", "all"],
        default="none",
        help="Trace level",
    )
    parser.add_argument(
        "--methods",
        "-m",
        default="sequential,sequential_cached,parallel,parallel_cached",
        help="Methods to benchmark (default: all)",
    )
    parser.add_argument(
        "--format",
        choices=["png", "pdf", "svg"],
        default="png",
        help="Graph format",
    )
    parser.add_argument(
        "--verbose", "-v", action="store_true", help="Verbose output"
    )
    parser.add_argument(
        "--clear", action="store_true", help="Clear output file before running"
    )
    parser.add_argument(
        "--threads",
        type=int,
        nargs="+",
        default=list(range(1, 17)),
        help="Thread counts for parallel methods (default: 1-16)",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=1,
        help="Number of runs to average (default: 1)",
    )
    parser.add_argument("N_values", nargs="+", type=int, help="Values of N")

    args = parser.parse_args()

    if not Path(args.binary).exists():
        print(f"[ERROR] Binary not found: {args.binary}", file=sys.stderr)
        sys.exit(1)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    plots_path = Path(args.plots)
    plots_path.mkdir(parents=True, exist_ok=True)

    if args.clear and output_path.exists():
        output_path.unlink()

    run_sequential, run_parallel = run_benchmarks(
        args.binary,
        args.N_values,
        args.trace,
        args.methods,
        args.threads,
        output_path,
        args.verbose,
        args.runs,
    )

    print(f"✓ Results saved: {output_path}")

    if not HAS_PLOTTING:
        return

    try:
        df = pd.read_csv(output_path)
        methods = {m.strip() for m in args.methods.split(",")}

        generate_time_vs_n_graph(
            df, methods, plots_path, args.format, run_sequential, run_parallel
        )

        if run_parallel:
            generate_scaling_graphs(df, methods, plots_path, args.format)

            if run_sequential:
                generate_speedup_graphs(df, methods, plots_path, args.format)

    except Exception as e:
        print(f"[ERROR] Failed to generate graphs: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()

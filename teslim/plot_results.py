#!/usr/bin/env python3
"""
Take the raw numbers from results_raw.csv and turn them into the four
figures and three tables I use in the report.

For each (load, MAC, STA count) combination there are 5 numbers from
the 5 seeds. We average them and use the standard deviation as the
error bar.

Run:  python3 plot_results.py results_raw.csv ./figures
"""

from __future__ import annotations

import csv
import os
import statistics
import sys
from collections import defaultdict

import matplotlib.pyplot as plt

STA_AXIS  = [4, 8, 12, 20]
LOADS     = [50, 80, 90]
MACS      = [("EDCA",   "EDCA"),
             ("RTSCTS", "EDCA+RTS/CTS")]

# Max Wi-Fi speed at MCS7 with short GI. Loads are a % of this.
RAW_PHY_MBPS = 72.2


# Read the CSV and group the 5 seeds for each scenario together,
# then compute the average and the standard deviation.
def read_results(path: str):
    samples: dict[tuple[int, str, int], list[float]] = defaultdict(list)
    with open(path, newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            n    = int(row["numSTA"])
            mac  = row["macMechanism"].strip()
            load = int(float(row["totalLoadPercent"]))
            thr  = float(row["throughputMbps"])
            samples[(load, mac, n)].append(thr)

    data: dict[tuple[int, str], dict[int, tuple[float, float, int]]] = \
        defaultdict(dict)
    for (load, mac, n), values in samples.items():
        mean = statistics.fmean(values)
        std  = statistics.stdev(values) if len(values) > 1 else 0.0
        data[(load, mac)][n] = (mean, std, len(values))
    return data


# Draws one of the three figures the project asks for:
# throughput vs number of STAs, for a single load value.
def plot_one(load: int, data, out_dir: str) -> str:
    fig, ax = plt.subplots(figsize=(7.5, 5.0))
    for mac, label in MACS:
        marker, color = ("o", "#1f77b4") if mac == "EDCA" \
                                          else ("s", "#d62728")
        per_n = data.get((load, mac), {})
        y    = [per_n.get(n, (float("nan"), 0.0, 0))[0] for n in STA_AXIS]
        yerr = [per_n.get(n, (float("nan"), 0.0, 0))[1] for n in STA_AXIS]
        ax.errorbar(STA_AXIS, y, yerr=yerr, marker=marker, color=color,
                    linewidth=2.0, markersize=8, capsize=4, label=label)

    ax.axhline(RAW_PHY_MBPS * load / 100.0, color="gray",
               linestyle=":", linewidth=1.0,
               label=f"Offered load ({RAW_PHY_MBPS*load/100:.1f} Mbit/s)")
    ax.set_title(f"Aggregate uplink throughput vs STA count "
                 f"(offered load = {load}%)")
    ax.set_xlabel("Number of STAs")
    ax.set_ylabel("Total throughput (Mbit/s)")
    ax.set_xticks(STA_AXIS)
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend()
    fig.tight_layout()

    out = os.path.join(out_dir, f"throughput_load{load}.png")
    fig.savefig(out, dpi=150)
    plt.close(fig)
    return out


# Bonus 4th figure: all 6 lines on one plot, so you can compare
# everything at once.
def plot_overview(data, out_dir: str) -> str:
    fig, ax = plt.subplots(figsize=(10.0, 5.5))
    linestyles = {50: ":", 80: "--", 90: "-"}
    colors     = {"EDCA": "#1f77b4", "RTSCTS": "#d62728"}
    markers    = {"EDCA": "o",       "RTSCTS": "s"}

    for load in LOADS:
        for mac, label in MACS:
            per_n = data.get((load, mac), {})
            y     = [per_n.get(n, (float("nan"), 0, 0))[0] for n in STA_AXIS]
            ax.plot(STA_AXIS, y,
                    color=colors[mac],
                    linestyle=linestyles[load],
                    marker=markers[mac],
                    markersize=7,
                    linewidth=1.8,
                    label=f"{label}, load={load}%")

    ax.set_title("Aggregate uplink throughput - all 6 scenarios")
    ax.set_xlabel("Number of STAs")
    ax.set_ylabel("Total throughput (Mbit/s)")
    ax.set_xticks(STA_AXIS)
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(loc="center left", bbox_to_anchor=(1.02, 0.5),
              fontsize=9, frameon=True)
    fig.tight_layout()

    out = os.path.join(out_dir, "throughput_overview.png")
    fig.savefig(out, dpi=150, bbox_inches="tight")
    plt.close(fig)
    return out


# Save the averaged numbers as their own CSV so anyone can open
# them in Excel without re-running the simulation.
def write_averaged_csv(data, path: str) -> None:
    with open(path, "w", newline="", encoding="utf-8") as fh:
        w = csv.writer(fh)
        w.writerow(["numSTA", "macMechanism", "totalLoadPercent",
                    "throughputMbps_mean", "throughputMbps_stdev",
                    "numRuns"])
        for (load, mac), per_n in sorted(data.items()):
            for n in sorted(per_n):
                mean, std, runs = per_n[n]
                w.writerow([n, mac, load,
                            f"{mean:.3f}", f"{std:.3f}", runs])


# Print three Markdown tables. I copy these straight into the report.
def print_markdown_tables(data) -> None:
    # Table 1: average throughput with the noise (stdev).
    print()
    print("### Numerical results (Mbit/s, mean ± sample stdev across seeds)")
    print()
    print("| Load (%) | MAC          | 4 STA           | 8 STA           "
          "| 12 STA          | 20 STA          |")
    print("|---------:|--------------|:---------------:|:---------------:"
          "|:---------------:|:---------------:|")
    for load in LOADS:
        for mac, label in MACS:
            row = data.get((load, mac), {})
            cells = " | ".join(
                f"{row.get(n, (float('nan'), 0, 0))[0]:>5.2f} "
                f"± {row.get(n, (0, 0, 0))[1]:.2f}"
                for n in STA_AXIS)
            print(f"| {load:>7} | {label:<12} | {cells} |")

    # Table 2: how much throughput we lose going from 4 to 20 STAs.
    # This is the "Bianchi effect" in one number.
    print()
    print("### Throughput drop from 4 STAs to 20 STAs (Bianchi-style effect)")
    print()
    print("| Load (%) | MAC          | 4 STA | 20 STA | Absolute drop | "
          "Relative drop |")
    print("|---------:|--------------|------:|-------:|--------------:|"
          "--------------:|")
    for load in LOADS:
        for mac, label in MACS:
            row = data.get((load, mac), {})
            mean4  = row.get(4,  (float("nan"), 0, 0))[0]
            mean20 = row.get(20, (float("nan"), 0, 0))[0]
            absdrop = mean4 - mean20
            reldrop = 100.0 * absdrop / mean4 if mean4 else float("nan")
            print(f"| {load:>7} | {label:<12} | "
                  f"{mean4:>5.2f} | {mean20:>5.2f} | "
                  f"{absdrop:>10.2f} Mb/s | "
                  f"{reldrop:>10.1f} % |")

    # Table 3: how much of the offered load actually got through.
    # 1.0 = perfect, anything lower = the Wi-Fi dropped some traffic.
    print()
    print("### Efficiency = measured throughput / offered load "
          "(closer to 1.0 = better)")
    print()
    print("| Load (%) | MAC          |  4 STA |  8 STA | 12 STA | 20 STA |")
    print("|---------:|--------------|-------:|-------:|-------:|-------:|")
    for load in LOADS:
        offered = RAW_PHY_MBPS * load / 100.0
        for mac, label in MACS:
            row = data.get((load, mac), {})
            cells = " | ".join(
                f"{(row.get(n, (float('nan'), 0, 0))[0] / offered):>5.2f}"
                for n in STA_AXIS)
            print(f"| {load:>7} | {label:<12} | {cells} |")


def main() -> int:
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "results_raw.csv"
    out_dir  = sys.argv[2] if len(sys.argv) > 2 else "."
    if not os.path.exists(csv_path):
        print(f"error: {csv_path} not found", file=sys.stderr)
        return 1
    os.makedirs(out_dir, exist_ok=True)

    data = read_results(csv_path)

    for load in LOADS:
        path = plot_one(load, data, out_dir)
        print(f"wrote {path}")

    overview = plot_overview(data, out_dir)
    print(f"wrote {overview}")

    avg_csv = os.path.join(out_dir, "results_avg.csv")
    write_averaged_csv(data, avg_csv)
    print(f"wrote {avg_csv}")

    print_markdown_tables(data)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

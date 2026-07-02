# Wi-Fi Performance Analysis — IEEE 802.11n (EDCA vs. EDCA + RTS/CTS)

An **ns-3** simulation study of how Wi-Fi throughput scales as more stations
contend for the channel. The project reproduces and re-examines G. Bianchi's
classic 2000 result — *does aggregate throughput fall as the number of stations
grows?* — but with modern **IEEE 802.11n** MAC features (EDCA, TXOP, A-MPDU,
Block-ACK) enabled, and compares plain **EDCA** against **EDCA + RTS/CTS**.

> Course: **BLM 4140 — Wireless & Mobile Networks**, Yıldız Technical University
> · Student: **Ozan Orhan (21011077)** · Instructor: Asst. Prof. Mehmet Şükrü Kuran

---

## 🎯 Purpose

Measure the **aggregate uplink TCP throughput** of a single-AP BSS as the number
of stations grows, under two MAC mechanisms and three traffic loads, and answer:

1. Does Bianchi's throughput-drop still occur with 802.11n's modern MAC?
2. When (if ever) does **RTS/CTS** help, given there are **no hidden nodes**?

## 🧪 Method

- **Simulator:** ns-3.40, a single parameterised C++ file (`wifi-project.cc`).
- **PHY:** IEEE 802.11n, 2.4 GHz, 20 MHz, 1×1, short GI → MCS 7, **72.2 Mbit/s** reference.
- **Topology:** 1 AP + N STAs (all 1 m from AP → no hidden-node problem).
- **Traffic:** uplink **TCP** (NewReno), offered load as a % of the PHY rate.
- **Sweep:** `numSTA ∈ {4, 8, 12, 20}` × `MAC ∈ {EDCA, EDCA+RTS/CTS}` ×
  `load ∈ {50 %, 80 %, 90 %}` = **24 scenarios**, each run with multiple RNG
  seeds and averaged (error bars = sample stdev).
- **MAC features:** EDCA (automatic with 802.11n QoS MAC), A-MPDU (64 KB) →
  Block-ACK (automatic), explicit **TXOP** (3.2 ms), RTS/CTS toggled via the
  RTS/CTS threshold.

## 📊 Key Findings

- **Bianchi's effect still holds:** throughput drops monotonically with N under
  every (load, MAC) combination — plain EDCA loses ~31 % / 40 % / 40 % of its
  throughput from 4→20 STAs at 50 % / 80 % / 90 % load.
- **RTS/CTS wins under heavy uplink TCP:** despite no hidden nodes, RTS/CTS beats
  EDCA at *every* station count once load ≥ 80 %, up to **+9.65 Mbit/s (~+34 %)**
  at 20 STAs / 90 % load — because long A-MPDU collisions are far more expensive
  than short RTS collisions, and RTS/CTS also helps the AP win the medium for
  downstream TCP ACKs.
- **At 50 % load the MAC choice barely matters** — the two curves sit inside each
  other's error bars for 4–12 STAs.

See [`project/report.md`](project/report.md) (English) /
[`project/report-tr.md`](project/report-tr.md) (Turkish) for the full write-up,
tables and figure-by-figure analysis.

## 📁 Repository Layout

```
project/                 → working directory (source + scripts + reports)
  wifi-project.cc        → ns-3 simulation source (parameterised)
  run_all.sh             → runs the full scenario sweep into a CSV
  plot_results.py        → builds the figures + averaged CSV + summary tables
  report.md / report-tr.md          → full report (EN / TR)
  video_script_en.md / _tr.md       → presentation video scripts
  output/                → generated figures, CSVs, exported PDFs
teslim/                  → final submission bundle
  wifi-project.cc, run_all.sh, plot_results.py
  results_raw.csv, results_raw_3seeds.csv
  figures/               → throughput_load{50,80,90}.png, overview, avg CSV
  report.pdf             → exported report
  video/                 → presentation video
```

## 🚀 How to Reproduce

**1. ns-3.40 (inside WSL 2 / Ubuntu — ns-3 does not build natively on Windows):**
```bash
sudo apt update && sudo apt install -y g++ python3 python3-dev cmake ninja-build \
     git pkg-config sqlite3 libsqlite3-dev libgsl-dev libxml2-dev libboost-all-dev
cd ~ && wget https://www.nsnam.org/releases/ns-allinone-3.40.tar.bz2
tar xjf ns-allinone-3.40.tar.bz2 && cd ns-allinone-3.40/ns-3.40
./ns3 configure --build-profile=optimized && ./ns3 build
```

**2. Build & smoke-test the project:**
```bash
cp path/to/project/wifi-project.cc scratch/
./ns3 build
./ns3 run "wifi-project --numSTA=4 --macMechanism=EDCA --totalLoadPercent=50"
# → prints one CSV line, e.g. 4,EDCA,50,29.812
```

**3. Full sweep + figures:**
```bash
cp path/to/project/run_all.sh . && chmod +x run_all.sh
./run_all.sh results_raw.csv
pip install matplotlib
python3 plot_results.py results_raw.csv ./figures
```

> **Target ns-3 version: 3.40 / 3.41.** Older releases use a different Wi-Fi
> attribute API and will not compile this file as-is.

## 📚 References

1. **ns-3** network simulator — <https://www.nsnam.org/>
2. G. Bianchi, *"Performance analysis of the IEEE 802.11 distributed coordination
   function,"* IEEE J. Sel. Areas Commun., vol. 18, no. 3, pp. 535–547, Mar. 2000.
3. IEEE 802.11-2020, *Part 11: Wireless LAN MAC and PHY Specifications.*

# BLM 4140 – Wi-Fi Performance Analysis Project

Files in this folder:

| File | What it is |
|---|---|
| `wifi-project.cc` | NS-3 simulation source (single CC file, parameterised) |
| `run_all.sh`      | Bash script that runs all 24 scenarios into `results.csv` |
| `plot_results.py` | Python script that draws the three required figures |
| `report.md`       | Results documentation (paste numbers in once you have them) |
| `README.md`       | This file |

> **Target version:** NS-3.40 / 3.41. Older NS-3 releases use a slightly
> different Wi-Fi attribute API and will not compile this file as-is.

---

## 1 · Install WSL + Ubuntu (only once)

NS-3 does not build natively on Windows. We use WSL 2 with Ubuntu.

```powershell
# In an *elevated* PowerShell
wsl --install -d Ubuntu-22.04
```

Reboot if prompted, finish the Ubuntu first-run setup (pick a username
and password), then `wsl` opens an Ubuntu shell.

## 2 · Install NS-3.40 inside Ubuntu (only once)

```bash
sudo apt update
sudo apt install -y g++ python3 python3-dev python3-pip cmake ninja-build \
                    git pkg-config sqlite3 libsqlite3-dev libgsl-dev \
                    libxml2 libxml2-dev libboost-all-dev

cd ~
wget https://www.nsnam.org/releases/ns-allinone-3.40.tar.bz2
tar xjf ns-allinone-3.40.tar.bz2
cd ns-allinone-3.40/ns-3.40

./ns3 configure --enable-examples --enable-tests --build-profile=optimized
./ns3 build
```

A full build takes 10–20 minutes on a modern laptop. If the build fails,
the most common cause is a missing apt package – read the first error
line and `sudo apt install` what it asks for.

## 3 · Drop in the project file and build it

From inside `ns-allinone-3.40/ns-3.40`:

```bash
# Copy wifi-project.cc to the scratch folder.
# Adjust the source path to wherever you cloned this repo.
cp /mnt/c/Users/Ozan\ Orhan/OneDrive/Masaüstü/network/project/wifi-project.cc \
   scratch/

./ns3 build
```

A successful build prints something like
`Build complete.  Executing wifi-project ...` and `[0/0] No work to do.`

Smoke test (one scenario):

```bash
./ns3 run "wifi-project --numSTA=4 --macMechanism=EDCA --totalLoadPercent=50"
```

You should see a single CSV line on stdout, e.g.

```
4,EDCA,50,29.812
```

## 4 · Run the full sweep (72 runs = 24 scenarios × 3 seeds)

Copy `run_all.sh` next to the `./ns3` wrapper, make it executable, and
launch it:

```bash
cp /mnt/c/Users/Ozan\ Orhan/OneDrive/Masaüstü/network/project/run_all.sh .
chmod +x run_all.sh
./run_all.sh results_raw.csv
```

The script runs every scenario with three different RNG seeds so the
plot script can average out the run-to-run jitter. A complete sweep is
roughly 72 × 30–60 s ≈ 30–60 min depending on your CPU.
`results_raw.csv` will look like

```
seed,numSTA,macMechanism,totalLoadPercent,throughputMbps
1,4,EDCA,50,29.812
1,4,EDCA,80,...
...
3,20,RTSCTS,90,...
```

## 5 · Plot the figures and update the report

Back in Windows (or still inside WSL):

```bash
pip install matplotlib
python3 plot_results.py results_raw.csv ./figures
```

You will get `figures/throughput_load50.png`, `..._load80.png`,
`..._load90.png` (each curve is the mean over the 3 seeds, with
±1 σ error bars), the averaged file `figures/results_avg.csv`, **and** a
Markdown summary table on stdout. Paste that table into `report.md`
over the `TODO` cells, rewrite the parenthesised "comment on your
numbers" sentences with what you actually observed, and you are done
with the document.

## 6 · 10-minute video

The project also requires a video presentation. A workable outline:

1. **0:00 – 1:30** Problem statement: Bianchi result, what 802.11n adds.
2. **1:30 – 4:00** Walk through `wifi-project.cc`: parameters, MAC
   feature toggles, traffic generation, throughput measurement.
3. **4:00 – 5:30** Show `run_all.sh` running one scenario live, point
   at `results.csv`.
4. **5:30 – 8:30** Open the three figures one by one, read the relevant
   paragraph from `report.md`, point at the crossover behaviour at 80 %
   and 90 %.
5. **8:30 – 10:00** Conclusion: Bianchi-style drop is still there with
   modern MAC, but A-MPDU softens it; RTS/CTS only helps under heavy
   contention.

Record with **OBS Studio** (free, Windows). Submit the resulting MP4
along with the project ZIP.

## 7 · What to submit

A single ZIP (Google Classroom assignment) containing:

* `wifi-project.cc`
* `run_all.sh`
* `plot_results.py`
* `results_raw.csv` and `figures/results_avg.csv` (your simulation output)
* `figures/throughput_load50.png`, `..._load80.png`, `..._load90.png`
* `report.pdf` (export `report.md` to PDF — Pandoc, VS Code "Markdown
  PDF" extension, or Word will all work)
* The recorded video (or a link to it, if too large)

Deadline: **25 May 23:59**.

---

### Troubleshooting

* **`./ns3 run` reports "Could not find program wifi-project"** – you
  forgot to copy the `.cc` file into `scratch/`, or you forgot to run
  `./ns3 build` after copying.
* **Throughput at 50 % load is much lower than 36 Mbit/s** – check that
  `HtConfiguration/ShortGuardIntervalSupported` is `true` and that the
  rate manager is `MinstrelHtWifiManager`. Both are set in the project
  file.
* **`./ns3 run` is very slow** – make sure you configured with
  `--build-profile=optimized` (or `release`). The default `debug`
  profile is 5×–10× slower.
* **A scenario crashes with "ChannelSettings" parse error** – your
  NS-3 version is older than 3.36. Either upgrade or rewrite the
  channel/PHY setup using the legacy `SetChannelNumber` API.

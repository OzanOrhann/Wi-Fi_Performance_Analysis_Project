#!/usr/bin/env bash
# Runs all 4 x 2 x 3 = 24 scenarios, each one with 3 different RNG seeds,
# for a total of 72 simulator invocations.  Raw results go to
# results_raw.csv (one row per run, including the seed column).  The
# averaging across seeds is done in plot_results.py.
#
# Run from the root of the NS-3 source tree (where ./ns3 lives), AFTER
# copying wifi-project.cc into scratch/ and running ./ns3 build once.

set -euo pipefail

OUT="${1:-results_raw.csv}"
SEEDS=(1 2 3 4 5)

echo "seed,numSTA,macMechanism,totalLoadPercent,throughputMbps" > "${OUT}"

i=0
for SEED in "${SEEDS[@]}"; do
  for STA in 4 8 12 20; do
    for MAC in EDCA RTSCTS; do
      for LOAD in 50 80 90; do
        i=$((i + 1))
        printf ">>> [%d/%d] seed=%d  numSTA=%d  mac=%s  load=%d%%\n" \
               "${i}" "$((${#SEEDS[@]} * 24))" "${SEED}" "${STA}" "${MAC}" "${LOAD}"
        LINE=$(./ns3 run --no-build "wifi-project --numSTA=${STA} \
                                                  --macMechanism=${MAC} \
                                                  --totalLoadPercent=${LOAD} \
                                                  --seed=${SEED}" \
               | tail -n 1)
        echo "${SEED},${LINE}" >> "${OUT}"
      done
    done
  done
done

echo
echo "All 72 runs done.  Raw results saved to ${OUT}."
echo "Now run:  python3 plot_results.py ${OUT} ./figures"

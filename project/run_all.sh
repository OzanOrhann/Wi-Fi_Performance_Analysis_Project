#!/usr/bin/env bash
# Runs all 24 scenarios (4 STA counts x 2 MACs x 3 loads), and does each
# one 5 times with a different random seed. Total: 120 runs.
# Every line of output ends up in results_raw.csv.
#
# Run me from the ns-3.40 folder (where ./ns3 is), after you put
# wifi-project.cc in scratch/ and ran "./ns3 build" once.

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

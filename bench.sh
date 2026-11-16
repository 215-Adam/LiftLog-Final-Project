#!/usr/bin/env bash
# bench.sh - build LiftLog, generate logs, benchmark baseline vs optimized.
#
# For each input size: run each program RUNS times on the same file, throw
# out the first run as a warmup, report the median of the rest, and check
# that both programs produced byte-identical reports.
#
# Environment overrides:
#   SIZES="100000 500000 1000000 5000000"   number of sets per log
#   USERS=10000                              number of lifters
#   RUNS=10                                  runs per program per size
#   CFLAGS="-O2 -Wall"                       flags used for BOTH programs
#
# Works in Linux/macOS shells and Git Bash on Windows.
set -euo pipefail
cd "$(dirname "$0")"

SIZES=${SIZES:-"100000 500000 1000000 5000000"}
USERS=${USERS:-10000}
RUNS=${RUNS:-10}
CFLAGS=${CFLAGS:-"-O2 -Wall"}

echo "Building with: gcc $CFLAGS"
gcc $CFLAGS gen_log.c   -o gen_log
gcc $CFLAGS baseline.c  -o baseline
gcc $CFLAGS optimized.c -o optimized

mkdir -p data results
: > results/phases.txt

# median of the numbers on stdin
median() {
    sort -g | awk '{a[NR]=$1} END {
        if (NR % 2) printf "%.2f", a[(NR+1)/2];
        else printf "%.2f", (a[NR/2] + a[NR/2+1]) / 2 }'
}

# run_prog PROG LOG -> prints median total_ms; also stores median phases
run_prog() {
    local prog=$1 log=$2 tag=$3
    local times="results/${prog}_${tag}.times"
    : > "$times"
    for ((r = 1; r <= RUNS; r++)); do
        ./"$prog" "$log" > "results/${prog}_${tag}.txt" 2> "results/${prog}_${tag}.err"
        if ((r > 1)); then   # run 1 is the warmup
            sed -E 's/.*load_ms=([0-9.]+) process_ms=([0-9.]+) cleanup_ms=([0-9.]+) total_ms=([0-9.]+).*/\1 \2 \3 \4/' \
                "results/${prog}_${tag}.err" >> "$times"
        fi
    done
}

col_median() { awk -v c="$1" '{print $c}' "$2" | median; }

printf "\n%-10s | %-12s | %-12s | %-8s | %s\n" "Sets" "Baseline ms" "Optimized ms" "Speedup" "Output"
printf -- "-----------+--------------+--------------+----------+--------\n"
for n in $SIZES; do
    log="data/log_${n}.csv"
    [[ -f "$log" ]] || ./gen_log "$n" "$USERS" "$log"
    run_prog baseline  "$log" "$n"
    run_prog optimized "$log" "$n"

    b=$(col_median 4 "results/baseline_${n}.times")
    o=$(col_median 4 "results/optimized_${n}.times")
    speed=$(awk -v b="$b" -v o="$o" 'BEGIN { printf "%.2fx", b / o }')
    if cmp -s "results/baseline_${n}.txt" "results/optimized_${n}.txt"; then same=identical; else same=DIFFERENT; fi
    printf "%-10s | %12s | %12s | %8s | %s\n" "$n" "$b" "$o" "$speed" "$same"

    {
        echo "sets=$n"
        for p in baseline optimized; do
            echo "  $p median load=$(col_median 1 results/${p}_${n}.times) process=$(col_median 2 results/${p}_${n}.times) cleanup=$(col_median 3 results/${p}_${n}.times) total=$(col_median 4 results/${p}_${n}.times)  [$(sed -E 's/.*(mallocs=[0-9]+ frees=[0-9]+).*/\1/' results/${p}_${n}.err)]"
        done
    } >> results/phases.txt
done
echo
echo "Per-phase medians: results/phases.txt"

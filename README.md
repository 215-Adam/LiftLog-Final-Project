# LiftLog — Workout Log Analytics in C

LiftLog reads a large gym workout log (one line per set) and computes training
stats for every lifter: best estimated one-rep max per exercise (Epley formula,
`weight × (1 + reps / 30)`), personal record (heaviest weight), total and weekly
training volume (`weight × reps`), and set counts.

The project compares two implementations that produce **byte-identical output**:

| File | What it is |
|---|---|
| `baseline.c` | Naive version: `fgets` + `strtok`, one `malloc` per set plus a `strdup` per exercise name, sets in a linked list, per-user exercise stats in a linked list searched with `strcmp`, freed node by node. |
| `optimized.c` | MVP optimization — contiguous memory layout: whole file read with one `fread`, all sets in one pre-sized array of 16-byte `struct Set`, exercise names interned to small integer IDs in an arena, stats in a flat `[user][exercise]` table. About 6 `malloc` calls total instead of millions. |
| `gen_log.c` | Deterministic generator for synthetic logs (default 10,000 users). |
| `liftlog_common.h` | Shared timer (`clock_gettime(CLOCK_MONOTONIC)`, or `QueryPerformanceCounter` on Windows) and date helpers. |
| `bench.sh` | Builds everything, generates logs, runs the benchmark and checks outputs match. |
| `tests/` | A tiny hand-checked log (`tiny.csv`) and its expected report. |
| `asm/` | Self-contained copies of each version's hot loop for Compiler Explorer. |

## Input format

```
user_id,YYYY-MM-DD,exercise,weight,reps
4821,2024-03-17,Bench Press,185.0,5
```

Blank or malformed lines are skipped. Weeks are counted from 2024-01-01.

## Compiling

Both versions compile with no warnings:

```bash
gcc -Wall baseline.c -o baseline
gcc -Wall optimized.c -o optimized
gcc -Wall gen_log.c -o gen_log
```

For benchmarking, build both with the same optimization level, e.g.:

```bash
gcc -O2 -Wall baseline.c -o baseline
gcc -O2 -Wall optimized.c -o optimized
```

Works with GCC on Linux/macOS and with MinGW/TDM-GCC on Windows.

## Running

Generate a log (`NUM_SETS NUM_USERS OUTPUT [SEED]`), then run either program:

```bash
./gen_log 1000000 10000 data/log_1000000.csv
./baseline  data/log_1000000.csv > baseline_report.txt
./optimized data/log_1000000.csv > optimized_report.txt
cmp baseline_report.txt optimized_report.txt && echo "outputs identical"
```

Correctness check against the hand-verified test:

```bash
./baseline tests/tiny.csv | diff - tests/tiny_expected.txt
./optimized tests/tiny.csv | diff - tests/tiny_expected.txt
```

## Running the benchmarks

```bash
./bench.sh
```

`bench.sh` builds all three programs with `-O2 -Wall`, generates logs of
100K, 500K, 1M and 5M sets (10,000 users), runs each program 10 times per size,
drops the first run as a warmup, reports the **median** total time and the
speedup, and confirms both reports are identical. Per-phase medians
(load / process / cleanup) and malloc/free counts go to `results/phases.txt`.

Options are environment variables:

```bash
SIZES="100000 1000000" RUNS=5 CFLAGS="-O0 -Wall" ./bench.sh
```

On Windows, run it from Git Bash. The 5M-set log is about 170 MB; `data/` and
`results/` are git-ignored.

## Expected output format

The report goes to **stdout**, one block per lifter (in user-ID order), with
exercises sorted by name:

```
user 1: total_volume=3440.0 best_week=1 week_volume=1360.0
  Back Squat: e1rm=233.33 pr=200.0 volume=1000.0 sets=1
  Bench Press: e1rm=133.33 pr=120.0 volume=2440.0 sets=3
user 2: total_volume=2300.0 best_week=1 week_volume=2300.0
  Deadlift: e1rm=316.67 pr=300.0 volume=2300.0 sets=2
```

Timing and memory instrumentation go to **stderr** as one line, so the report
can be diffed:

```
optimized: sets=1000000 load_ms=... process_ms=... cleanup_ms=... total_ms=... mallocs=6 frees=6
```

- `load_ms` — reading and parsing the file into memory
- `process_ms` — the stats loop
- `cleanup_ms` — freeing everything
- `total_ms` — load + process + cleanup (report printing is not timed)
- `mallocs` / `frees` — every allocation is counted; equal numbers mean nothing leaked

## Results (MVP)

RESULTS_PLACEHOLDER

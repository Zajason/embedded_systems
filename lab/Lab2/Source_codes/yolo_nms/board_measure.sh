#!/bin/bash
# Builds and measures every version of the exercise on the AUP-ZU3. Run it from yolo_nms/:
#
#   ./board_measure.sh            everything: Q1, Q2, Q3, Q4
#   ./board_measure.sh q1 q2      only these parts
#   ./board_measure.sh --no-build q2   measure binaries that are already in bin/
#   ./board_measure.sh --skip-validate  build without the -DVALIDATE check (half the compiles)
#
#   q1  original nms()                         bin/nms-orig
#   q2  9 DDTs, glibc                          bin/nms-<ddt>-glibc
#   q3  9 DDTs x arena/first-fit/best-fit/worst-fit   bin/nms-<ddt>-<alloc>
#   q4  -DNMS_FLAG, 9 DDTs x 5 allocators      bin/nms-flag-<ddt>-<alloc>
#
# For every binary:
#   time : 3 repetitions of runs=20; the mean of the 20 ms= lines of each repetition, then the
#          mean and (sample) variance of the 3 means
#   perf : perf stat -r 20 -e l2d_cache_refill, with runs=1 (one pass over the frames) and with
#          runs=0 (only loading the data), so that nms() itself is about the difference
# Results go to board_results/<part>/: summary.csv, plus the raw output in raw/.
# Before measuring, every binary is checked once with its -DVALIDATE build (all 90 passed on the
# PC already, so --skip-validate is safe if compiling on the board takes too long).
# Memory footprint is not measured here: Massif (glibc) and -DALLOC_STATS were done on the PC.
set -u
cd "$(dirname "$0")"

INPUT=data/detrac_y5s_640.bin
REPS=${REPS:-3}
RUNS=${RUNS:-20}
PERF_REPS=${PERF_REPS:-20}
DDTS="vector single_linked_list double_linked_list roving_single_linked_list roving_double_linked_list
single_linked_list_no_sentinel double_linked_list_no_sentinel roving_single_linked_list_no_sentinel
roving_double_linked_list_no_sentinel"

BUILD=1
VALIDATE=1
PARTS=""
for a in "$@"; do
  case $a in
    --no-build) BUILD=0 ;;
    --skip-validate) VALIDATE=0 ;;
    q1|q2|q3|q4) PARTS="$PARTS $a" ;;
    *) echo "unknown argument: $a"; exit 1 ;;
  esac
done
[ -z "$PARTS" ] && PARTS="q1 q2 q3 q4"

PERF=$(command -v perf || echo "$HOME/.local/bin/perf")
if [ ! -x "$PERF" ]; then
  echo "perf not found: run ../perf_setup/install_perf.sh and 'source ~/.profile' first"
  exit 1
fi

mkdir -p board_results
{ date; uname -a; g++ --version | head -1; "$PERF" --version; } > board_results/environment.txt

# build <flags...>: runs build.sh, prints the binary it built (empty on failure)
build() {
  local out
  if ! out=$(./build.sh "$@" 2>&1); then
    echo "BUILD FAILED: ./build.sh $*" >&2
    echo "$out" >&2
    return 1
  fi
  echo "$out" | sed -n 's/^built //p'
}

# measure <part> <label> <ddt> <allocator> <binary>
measure() {
  local part=$1 label=$2 ddt=$3 alloc=$4 bin=$5
  local dir=board_results/$part raw=board_results/$part/raw
  mkdir -p "$raw"
  local means=""
  for r in $(seq 1 $REPS); do
    ./"$bin" input=$INPUT runs=$RUNS > "$raw/$label.time$r.txt"
    means="$means $(awk -F= '/^ms=/ { s += $2; n++ } END { printf "%.4f", s / n }' "$raw/$label.time$r.txt")"
  done
  local stats
  stats=$(echo $means | awk '{ for (i = 1; i <= NF; i++) { s += $i; v[i] = $i }
                               m = s / NF; for (i = 1; i <= NF; i++) q += (v[i] - m) ^ 2
                               printf "%.4f,%.6f", m, (NF > 1 ? q / (NF - 1) : 0) }')
  local l2=() runs
  for runs in 1 0; do
    "$PERF" stat -x, -r $PERF_REPS -e l2d_cache_refill -o "$raw/$label.perf_runs$runs.txt" \
      ./"$bin" input=$INPUT runs=$runs > /dev/null
    l2+=("$(grep l2d_cache_refill "$raw/$label.perf_runs$runs.txt" | head -1 | cut -d, -f1)")
  done
  echo "$label,$ddt,$alloc,$(echo $means | tr ' ' ','),$stats,${l2[0]},${l2[1]}" >> "$dir/summary.csv"
  printf '  %-50s %s ms  l2d_cache_refill=%s (load only %s)\n' "$label" "${stats%%,*}" "${l2[0]}" "${l2[1]}"
}

# run_part <part> <label> <ddt> <alloc> <build args...>
run_part() {
  local part=$1 label=$2 ddt=$3 alloc=$4; shift 4
  local bin
  if [ $BUILD = 1 ]; then
    if [ "$1" != orig ] && [ $VALIDATE = 1 ]; then
      local vbin
      vbin=$(build "$@" -DVALIDATE) || return
      if ! ./"$vbin" input=$INPUT | grep -q "validate: OK"; then
        echo "  $label: VALIDATE FAILED, not measured" | tee -a board_results/$part/errors.txt
        return
      fi
    fi
    bin=$(build "$@") || return
  else
    bin=bin/$label
    [ -x "$bin" ] || { echo "  $bin missing (run without --no-build)"; return; }
  fi
  measure "$part" "$label" "$ddt" "$alloc" "$bin"
}

header() {
  mkdir -p board_results/$1
  echo "binary,ddt,allocator,rep1_ms,rep2_ms,rep3_ms,mean_ms,var_ms2,l2d_refill_runs1,l2d_refill_runs0" \
    > board_results/$1/summary.csv
  echo "== $1"
}

for part in $PARTS; do
  header $part
  case $part in
    q1) run_part q1 nms-orig orig glibc orig ;;
    q2) for d in $DDTS; do run_part q2 nms-$d-glibc $d glibc $d glibc; done ;;
    q3) for a in arena first-fit best-fit worst-fit; do
          for d in $DDTS; do run_part q3 nms-$d-$a $d $a $d $a; done
        done ;;
    q4) for a in glibc arena first-fit best-fit worst-fit; do
          for d in $DDTS; do run_part q4 nms-flag-$d-$a $d $a $d $a -DNMS_FLAG; done
        done ;;
  esac
done
echo "done: board_results/*/summary.csv"

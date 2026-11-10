#!/usr/bin/env bash
#
# Checks that perf works on the board by counting a small sorting job.
# Run install_perf.sh first.

set -u

if ! command -v perf >/dev/null 2>&1; then
    if [ -x "$HOME/.local/bin/perf" ]; then
        echo "FAILED: this terminal cannot find perf yet. Run: source ~/.profile" >&2
    else
        echo "FAILED: perf is not installed. Run ./install_perf.sh first." >&2
    fi
    exit 1
fi

OUT="$(mktemp)" || exit 1
trap 'rm -f "$OUT"' EXIT

EVENTS="task-clock,cycles,instructions,l2d_cache_refill,LLC-load-misses,LLC-store-misses"

if ! perf stat -o "$OUT" -e "$EVENTS" -- sh -c 'seq 1 200000 | sort -rn > /dev/null'; then
    cat "$OUT"
    echo "FAILED: perf could not run the test job." >&2
    exit 1
fi
cat "$OUT"

# True when the event has a real, non-zero count in the perf output.
counted() {
    grep -E "[[:space:]]$1(:u)?([[:space:]]|$)" "$OUT" | grep -qE '^[[:space:]]*[0-9,.]*[1-9]'
}

STATUS=0
for event in task-clock cycles instructions l2d_cache_refill; do
    if ! counted "$event"; then
        echo "FAILED: $event was not counted." >&2
        STATUS=1
    fi
done
[ "$STATUS" = 0 ] || exit 1

echo "OK: perf works on this board."
if ! counted LLC-load-misses || ! counted LLC-store-misses; then
    echo
    echo "LLC-load-misses and LLC-store-misses are not supported on this CPU (Cortex-A53)."
    echo "The L2 is the last-level cache here, so count its misses with:"
    echo "  perf stat -e l2d_cache_refill ./exec"
    echo "Loads and stores cannot be counted separately."
fi

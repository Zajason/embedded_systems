#!/bin/sh
# Rebuild the estimates: needs clang (Xcode), python3 with unicorn capstone pyelftools numpy.
set -e
cd "$(dirname "$0")"
R=https://raw.githubusercontent.com/jimmichalako/embedded-systems-ntua/main/lab-1/src/ask1
mkdir -p theirs obj
[ -f theirs/base_phods.c ] || curl -sfL -o theirs/base_phods.c $R/base_phods.c
[ -f theirs/rect_phods.c ] || curl -sfL -o theirs/rect_phods.c $R/rect_phods.c
rm -rf ours_src && cp -r ../../src ours_src
printf 'int abs(int);\n' > stub/stdlib.h; : > stub/stdio.h; : > stub/string.h
CF="--target=thumbv7em-none-eabihf -mcpu=cortex-m4 -mthumb -O0 -ffreestanding -nostdlibinc -fno-common -w"
clang $CF -I stub -I ours_src -DTHEIR_FILE='"theirs/base_phods.c"' -c theirs_harness.c -o obj/theirs_base.o
for bx in 1 2 5 10; do for by in 1 2 5 10; do
  sed "s/#define Bx 10/#define Bx $bx/; s/#define By 10/#define By $by/" theirs/rect_phods.c > theirs/rect_${bx}_${by}.c
  clang $CF -I stub -I ours_src -I theirs -DTHEIR_FILE="\"theirs/rect_${bx}_${by}.c\"" -c theirs_harness.c -o obj/theirs_rect_${bx}_${by}.o
done; done
clang $CF -I stub -I ours_src -c ours_harness.c -o obj/ours.o
python3 sim.py      # ~4 min -> features.json
python3 fit2.py     # -> pred.json

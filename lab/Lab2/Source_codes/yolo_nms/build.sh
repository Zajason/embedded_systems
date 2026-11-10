#!/bin/bash
# Run it from this folder. It only compiles: the binaries go to bin/ and you run them yourself.
#   ./build.sh orig                 the original nms()
#   ./build.sh <ddt> <allocator>    your port in postprocess.cpp, with the allocator
#                                   glibc, pool, arena, first-fit, best-fit or worst-fit
#
# Extra arguments go to g++. These flags also change the name of the binary, so that builds
# with different flags do not overwrite each other:
#   -DNMS_FLAG      nms-flag-<ddt>-<allocator>
#   -DALLOC_STATS   ...-stats      prints the footprint of the allocator
#   -DVALIDATE      ...-validate   compares the results with the original nms()
set -e
FLAGS="-std=c++17 -O3 -DNDEBUG -w -I. -I../ddtr_student/include"
mkdir -p bin

if [ "$1" = orig ]; then
  g++ $FLAGS -DRUN_ORIG upstream/nms_orig.cpp main.cpp -o bin/nms-orig
  echo "built bin/nms-orig"
  exit
fi

ddt=$1; alloc=$2; shift 2
if [ "$alloc" = glibc ]; then type=ddtr::malloc_allocator; else type=${alloc//-/_}; fi
case "$alloc $*" in glibc*ALLOC_STATS*) echo "-DALLOC_STATS works only with your allocators: for glibc, use Massif"; exit 1;; esac

name=nms; orig=
case "$*" in *NMS_FLAG*) name=nms-flag;; esac
name=$name-$ddt-$alloc
case "$*" in *ALLOC_STATS*) name=$name-stats;; esac
case "$*" in *VALIDATE*) name=$name-validate; orig=upstream/nms_orig.cpp;; esac

g++ $FLAGS -Dsel_dt_dets=$ddt -Dsel_alloc_dets=$type postprocess.cpp main.cpp $orig "$@" -o bin/$name
echo "built bin/$name"

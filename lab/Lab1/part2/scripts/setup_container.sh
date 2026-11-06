#!/bin/bash
# Run INSIDE the aferikoglou/gem5built:v1.0 container (it is started with --rm, so run
# this every time). Does the "Exercise Setup" steps of github.com/aferikoglou/Gem5DSE:
#   1. get Gem5DSE and copy it into /gem5
#   2. compile tables*.c to tables*.exe (x86, -O0)
#   3. let simple.py take the binary as an argument (two_level.py already does)
#   4. install the Python libraries
set -e
cd /gem5
if [ ! -d /gem5/tables_UF ]; then
  git clone -q https://github.com/aferikoglou/Gem5DSE.git /tmp/Gem5DSE
  cp -r /tmp/Gem5DSE/* /gem5/
fi
(cd /gem5/tables_UF && bash compile.sh)

python3 - <<'PY'
p = "/gem5/configs/learning_gem5/part1/simple.py"
s = open(p).read()
if "sys.argv" not in s:
    s = s.replace("binary = os.path.join(", "import sys\nbinary = sys.argv[1] if len(sys.argv) > 1 else os.path.join(", 1)
    open(p, "w").write(s)
PY
grep -n "sys.argv" /gem5/configs/learning_gem5/part1/simple.py

python3 -m pip install -q -r /gem5/requirements.txt -r /tmp/Gem5DSE/requirements.txt 2>&1 | grep -v -i "warning\|notice" || true
python3 -c "import pandas, paretoset, pymoo; print('python libs ok')"
ls /gem5/tables_UF/*.exe

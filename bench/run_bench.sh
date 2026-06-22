#!/usr/bin/env bash
# TortureLang vs Python 性能基准汇总
set -euo pipefail

ROOT="/home/LangLang/code/cpp/Lang"
TBC="/tmp/bench.tbc"
cd "$ROOT"

echo "=== build torturec / torturevm ==="
cmake --build build >/dev/null

echo "=== compile bench.torture ==="
./build/torturec compile bench/bench.torture -o "$TBC"

echo
echo "=== TortureLang (torturevm run) ==="
T_TL_START=$(date +%s%N)
./build/torturevm run "$TBC"
T_TL_END=$(date +%s%N)
T_TL_NS=$((T_TL_END - T_TL_START))
T_TL_MS=$(awk -v n="$T_TL_NS" 'BEGIN{printf "%.3f", n/1000000}')

echo
echo "=== Python (bench.py) ==="
T_PY_START=$(date +%s%N)
python3 bench/bench.py
T_PY_END=$(date +%s%N)
T_PY_NS=$((T_PY_END - T_PY_START))
T_PY_MS=$(awk -v n="$T_PY_NS" 'BEGIN{printf "%.3f", n/1000000}')

echo
echo "=== summary (wall time) ==="
printf "TortureLang (VM 字节码解释执行) : %s ms\n" "$T_TL_MS"
printf "Python    (CPython 3 解释执行)   : %s ms\n" "$T_PY_MS"

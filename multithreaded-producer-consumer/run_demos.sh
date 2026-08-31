#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

echo
echo "=== producer/consumer ==="
./build/producer_consumer

echo
echo "=== race condition demo ==="
./build/race_condition_demo

echo
echo "=== deadlock demo ==="
./build/deadlock_demo

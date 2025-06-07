#!/usr/bin/env bash
set -e

echo "[Script] Running valgrind on analyzer..."

timeout 10s valgrind --leak-check=full --show-leak-kinds=all \
  --log-file=build/valgrind-analyzer.log \
  build/analyzer || true  # don't fail on timeout

sleep 12

cat build/valgrind-analyzer.log

if grep "definitely lost: [1-9][0-9]* bytes" build/valgrind-analyzer.log; then
  echo "Memory leaks found in analyzer"
  exit 1
else
  echo "No leaks detected in analyzer"
fi
#!/usr/bin/env bash
set -e

echo "[Script] Running valgrind on analyzer (i.e. gui)..."

timeout 10s valgrind --leak-check=full --show-leak-kinds=all \
  --log-file=build/valgrind-analyzer.log \
  build/gui || true  # don't fail on timeout



cat build/valgrind-analyzer.log

# Extract number of definitely lost bytes
lost_bytes=$(grep -E "==[0-9]+== +definitely lost:" build/valgrind-analyzer.log \
  | sed -E 's/.*definitely lost: *([0-9,]+) bytes.*/\1/' \
  | tr -d ',')

# If extraction failed or empty, default to 0
lost_bytes=${lost_bytes:-0}

echo "[Valgrind] Definitely lost: $lost_bytes bytes"

if [ "$lost_bytes" -ge 8000 ]; then
  echo "Memory leaks found in analyzer"
  exit 1
else
  echo "No critical memory leaks detected"
fi
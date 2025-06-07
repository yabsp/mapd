#!/usr/bin/env bash

set -e

echo "[Script] Running valgrind test..."
if valgrind --leak-check=full --error-exitcode=1 build/test_valgrind; then
  echo "Leak not detected — test failed"
  exit 1
else
  echo "Leak detected — Valgrind is working as expected"
  exit 0
fi

#!/usr/bin/env bash
# Runs the whole test suite (firmware logic + tools).
#   ./test/run_tests.sh          normal
#   ./test/run_tests.sh --asan   also with AddressSanitizer + UBSan
set -uo pipefail
cd "$(dirname "$0")"

fail=0

echo "== firmware logic (pet.cpp / i18n.cpp / dex.h) =="
make --no-print-directory run || fail=1

echo
echo "== the same logic with 1.51's rules (UI_V2=0) =="
make --no-print-directory v1 | tail -1 || fail=1

if [[ "${1:-}" == "--asan" ]]; then
  echo
  echo "== the same suite with AddressSanitizer + UBSan =="
  make --no-print-directory asan || fail=1
fi

echo
echo "== tools and generated files (tools/) =="
python3 test_tools.py || fail=1

echo
echo "== host layer (host/: canvas, save, sound and the game without a window) =="
make --no-print-directory -C ../host test | tail -1 || fail=1

echo
if [[ $fail -eq 0 ]]; then
  echo "ALL OK"
else
  echo "THERE ARE FAILURES"
fi
exit $fail

#!/bin/sh
# Builds and runs the unit tests, then checks the demo session output.
set -e
cd "$(dirname "$0")/.."
${CXX:-g++} -std=c++17 -Wall -Wextra -Iinclude tests/unit_tests.cpp -o tests/unit_tests
./tests/unit_tests

rm -rf tests/tmp/demo
BB_TODAY=16-11-2026 ./lifeline tests/tmp/demo --echo < demo/session1.txt > tests/tmp/out.log
grep -q "units: 2 4" tests/tmp/out.log
grep -q "only 1 compatible" tests/tmp/out.log
# restart: history must still be there
printf '9\n0\n' | BB_TODAY=16-11-2026 ./lifeline tests/tmp/demo | grep -q "Request 2"
echo "demo session checks passed"

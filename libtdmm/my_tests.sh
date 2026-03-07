#!/bin/bash
set -e

echo "Building tests..."
gcc test.c tdmm.c buddyAlloc.c -o test

echo "Running tests..."
./test

echo "All tests completed."
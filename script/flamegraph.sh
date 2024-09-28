#!/bin/bash

# Usage: ./flamegraph.sh <flamegraph-src> <build_directory> <benchmark_executable>

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
  echo "Usage: $0 <flamegraph-src> <build_directory> <benchmark_executable>"
  echo "This should be run from CMake, e.g. ninja flamegraph"
  exit 1
fi

# Assign command line arguments to variables
FLAMEGRAPH_DIR=$(realpath "$1")
BUILD_DIR=$(realpath "$2")
BENCHMARK_EXECUTABLE=$(realpath "$3")

# Check if FlameGraph directory exists
if [ ! -d "$FLAMEGRAPH_DIR" ]; then
  echo "Error: FlameGraph directory not found at ${FLAMEGRAPH_DIR}."
  echo "Please clone the FlameGraph repository from https://github.com/brendangregg/FlameGraph and place it in your source directory."
  exit 1
fi

# Check if the benchmark executable exists
if [ ! -f "$BENCHMARK_EXECUTABLE" ]; then
  echo "Error: Benchmark executable not found at ${BENCHMARK_EXECUTABLE}."
  exit 1
fi

# Create the flamegraph output directory if it doesn't exist
OUTPUT_DIR="${BUILD_DIR}/flamegraph"
mkdir -p "${OUTPUT_DIR}"

# Run the benchmark with perf
echo "Running perf on ${BENCHMARK_EXECUTABLE}..."
perf record -F max -a --call-graph fp,256 -- "${BENCHMARK_EXECUTABLE}"

# Generate the flame graph
echo "Generating flame graph..."
perf script | "${FLAMEGRAPH_DIR}/stackcollapse-perf.pl" > "${OUTPUT_DIR}/out.perf-folded"
"${FLAMEGRAPH_DIR}/flamegraph.pl" "${OUTPUT_DIR}/out.perf-folded" > "${OUTPUT_DIR}/flamegraph.svg"

echo "Flame graph generated at: ${OUTPUT_DIR}/flamegraph.svg"

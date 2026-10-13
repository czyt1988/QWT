#!/usr/bin/env bash
#
# chartbench - runs the full measurement matrix.
#
# Usage:  ./run-bench.sh [build-dir] [output-dir]
#
# Defaults: build-dir = <example>/build, output-dir = <repo>/docs/assets/bench/data
#
# Every library runs in its own process so that the process memory reported in
# the CSVs can be attributed to that library alone. Qt Charts and QIm only take
# part with the backend they support (see README.md).

set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "${HERE}/../../.." && pwd)"          # <repo>/examples/bench/chartbench
BUILD_DIR="${1:-${REPO}/build}"
OUT_DIR="${2:-${REPO}/docs/assets/bench/data}"

BIN="${BUILD_DIR}/bin/Release"
if [ ! -x "${BIN}/chartbench.exe" ] && [ ! -x "${BIN}/chartbench" ]; then
    BIN="${BUILD_DIR}/bin"
fi

# The canvas suite resizes the window, so it runs last: any leftover geometry
# from it can then not leak into a fixed canvas size suite.
SUITES="points,curves,update,downsample,jitter,memory,fidelity,canvas"
COMMON="--suite=${SUITES} --out=${OUT_DIR} --verbose"

# A leftover run would append to the same CSV files and silently corrupt them.
if tasklist 2>/dev/null | grep -qi chartbench; then
    echo "another chartbench is still running - stop it first" >&2
    exit 1
fi

mkdir -p "${OUT_DIR}"

run() {
    local exe="$1"; shift
    echo "=== ${exe} $* ==="
    "${BIN}/${exe}" "$@"
    echo "--- exit $? ---"
}

# Qwt: raster canvas and OpenGL canvas
run chartbench --library=qwt        --backend=raster ${COMMON}
run chartbench --library=qwt        --backend=opengl ${COMMON}

# QCustomPlot: QCUSTOMPLOT_USE_OPENGL is a compile time switch, hence two binaries
run chartbench --library=qcustomplot --backend=raster ${COMMON}
run chartbench_qcpgl --library=qcustomplot --backend=opengl ${COMMON}

# Qt Charts: raster only, its OpenGL series never render (see README.md)
run chartbench --library=qtcharts   --backend=raster ${COMMON}

# QIm: OpenGL only
run chartbench --library=qim        --backend=opengl ${COMMON}

echo "results in ${OUT_DIR}"

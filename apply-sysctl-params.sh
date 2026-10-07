#!/bin/sh
# Applies the slope/intercept fitted by flush-calibration.sh (run once on
# the baseline kernel) to the nocap kernel's adaptive DONTNEED delay.
#
# Usage: ./apply-sysctl-params.sh [path/to/<hostname>-sysctl-params.csv]
# Run as root, after disable-readahead.sh, on the nocap boot only.

set -eu

PARAMS_FILE="${1:-/root/flush-calibration/parameter-determination/data/$(cat /etc/hostname)-sysctl-params.csv}"

if [ ! -f "$PARAMS_FILE" ]; then
    echo "params file not found: $PARAMS_FILE" >&2
    echo "run flush-calibration.sh on the baseline kernel first." >&2
    exit 1
fi

slope=$(grep '^slope,' "$PARAMS_FILE" | cut -d, -f2)
intercept=$(grep '^intercept,' "$PARAMS_FILE" | cut -d, -f2)

factor_centi=$(python3 -c "print(round(float('$slope') * 100))")
const_centi=$(python3 -c "print(round(float('$intercept') * 100))")

echo "$factor_centi" > /proc/sys/vm/dontneed_wait_factor_centicycles
echo "$const_centi" > /proc/sys/vm/dontneed_wait_constant_centicycles

echo "dontneed_wait_factor_centicycles   = $(cat /proc/sys/vm/dontneed_wait_factor_centicycles)"
echo "dontneed_wait_constant_centicycles = $(cat /proc/sys/vm/dontneed_wait_constant_centicycles)"

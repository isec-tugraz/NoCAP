#!/bin/bash
# Measures fadvise(DONTNEED) flush time vs page count and fits a line to it.
# The venv + its deps (cvxopt/numpy/pandas) are pre-installed in this image 
#
# Output: data/<hostname>.csv          (raw timing samples)
#         data/<hostname>-sysfs-params.csv  (slope,intercept,margin)

set -eu

source venv/bin/activate

BUILD_DIR=./build
DATA_DIR=./data
HOSTNAME=$(cat /etc/hostname)
FILENAME=file.bin

echo "[Flush Calibration] start..."

./file-create.sh $FILENAME
mkdir -p $DATA_DIR

make

$BUILD_DIR/flush-repeat $FILENAME $DATA_DIR/$HOSTNAME.csv

python3 analyse.py $DATA_DIR/$HOSTNAME.csv $DATA_DIR/$HOSTNAME-sysctl-params.csv

echo "done: $DATA_DIR/$HOSTNAME.csv"
echo "sysfs params: $DATA_DIR/$HOSTNAME-sysfs-params.csv"

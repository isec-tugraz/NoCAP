#!/bin/sh
# Disables the block layer's forward read-ahead on the root disk (vda).

echo 0 > /sys/block/vda/queue/read_ahead_kb
echo "read_ahead_kb now: $(cat /sys/block/vda/queue/read_ahead_kb)"

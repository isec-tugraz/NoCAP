#!/bin/bash

dd if=/dev/urandom of=$1 bs=4K count=1024

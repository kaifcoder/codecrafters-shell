#!/bin/sh
set -e

# Compile the C++ shell
make -s

# Run the compiled shell
exec ./shell

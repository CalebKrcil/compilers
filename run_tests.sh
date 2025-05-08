#!/usr/bin/env bash
set -uo pipefail

# Directory containing the .kt files (default: current dir)
SRCDIR="${1:-.}/finaltests"

# Path to your compiler
K0="./k0"

# Check that k0 exists and is executable
if [ ! -x "$K0" ]; then
  echo "Error: compiler '$K0' not found or not executable"
  exit 1
fi

# Loop over all .kt files
shopt -s nullglob
for kt in "$SRCDIR"/*.kt; do
  base="$(basename "$kt" .kt)"
  echo "=== Compiling $kt → $base ==="
  if ! "$K0" "$kt"; then
    echo ">> Compilation failed for $kt"
    exit 1
  fi

  # Now run the produced executable, if it exists
  if [ -x "$SRCDIR/$base" ]; then
    echo "=== Running $base ==="
    "$SRCDIR/$base"
    echo
  else
    echo ">> Warning: executable '$SRCDIR/$base' not found or not executable"
  fi
done
#!/bin/bash

# Common path prefixes and suffixes
SRC_BASE="/home/zhejiang/FireGuard_V2"
DST_BASE="/home/zhejiang/firesim/target-design/chipyard"



# Define source and destination arrays
sources=(
  "${SRC_BASE}/Hardware/big.core/scala"
  "${SRC_BASE}/Hardware/LITTLE.core/scala"
  "${SRC_BASE}/Hardware/Top/scala"
  "${SRC_BASE}/Hardware/Firesim/scala"
  "${SRC_BASE}/Software/bare-metal"
)

destinations=(
  "${DST_BASE}/generators/boom/src/main/scala"
  "${DST_BASE}/generators/rocket-chip/src/main/scala"
  "${DST_BASE}/generators/chipyard/src/main/scala"
  "${DST_BASE}/generators/firechip/src/main/scala"
  "${DST_BASE}/sims/verilator/bare-metal"
)

for i in "${!sources[@]}"; do
  src="${sources[$i]}"
  dst="${destinations[$i]}"

  # Remove existing destination (file, dir, or symlink)
  if [ -e "$dst" ] || [ -L "$dst" ]; then
    rm -rf "$dst"
    echo "Removed existing $dst"
  fi

  # Create parent directory if it doesn't exist
  mkdir -p "$(dirname "$dst")"

  # Create symbolic link
  ln -s "$src" "$dst"
  echo "Linked $dst -> $src"
done
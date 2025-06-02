#!/bin/bash

# Common path prefixes and suffixes
SRC_BASE="/home/zhe/Workspace/FireGuard_V2"
DST_BASE="/home/zhe/Workspace/chipyard"



# Define source and destination arrays
sources=(
  "${SRC_BASE}/Hardware/big.core/scala"
  "${SRC_BASE}/Hardware/LITTLE.core/scala"
  "${SRC_BASE}/Hardware/Top/scala"
)

destinations=(
  "${DST_BASE}/generators/boom/src/main/scala"
  "${DST_BASE}/generators/rocket-chip/src/main/scala"
  "${DST_BASE}/generators/chipyard/src/main/scala"
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
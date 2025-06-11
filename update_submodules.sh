#!/bin/bash

# Simple script to update all submodules in the repository

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Updating all submodules..."
cd "$SCRIPT_DIR"

# Add parsec-benchmark submodule if it doesn't exist
SUBMODULE_PATH="Software/linux/parsecv3/parsec-benchmark"
SUBMODULE_URL="git@github.com:zhejianguk/parsec-benchmark.git"

# Check if submodule is already configured in git
if ! git config --file .gitmodules --get submodule."$SUBMODULE_PATH".url > /dev/null 2>&1; then
    echo "Adding parsec-benchmark submodule..."
    git submodule add "$SUBMODULE_URL" "$SUBMODULE_PATH"
else
    echo "parsec-benchmark submodule already configured, skipping add..."
fi

# Initialize and update all submodules
git submodule update --init --recursive

echo "All submodules updated successfully!" 
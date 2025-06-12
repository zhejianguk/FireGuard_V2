#!/bin/bash

# Simple script to update all submodules in the repository and ensure correct branch

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_BRANCH="RISC-V"

echo "Updating all submodules and ensuring correct branch setup..."
cd "$SCRIPT_DIR"

# Ensure main repository is on RISC-V branch
echo "Checking out $TARGET_BRANCH branch in main repository..."
git checkout "$TARGET_BRANCH" 2>/dev/null || {
    echo "$TARGET_BRANCH branch not found in main repository, creating it..."
    git checkout -b "$TARGET_BRANCH"
}

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
echo "Initializing and updating submodules..."
git submodule update --init --recursive

# Ensure submodules are on the correct branch
echo "Ensuring submodules are on $TARGET_BRANCH branch..."
git submodule foreach --recursive '
    echo "Processing submodule: $name"
    git checkout '"$TARGET_BRANCH"' 2>/dev/null || {
        echo "'"$TARGET_BRANCH"' branch not found in submodule $name, creating it..."
        git checkout -b '"$TARGET_BRANCH"'
    }
'

echo "All submodules updated and checked out to $TARGET_BRANCH branch successfully!" 
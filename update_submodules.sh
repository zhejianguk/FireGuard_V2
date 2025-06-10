#!/bin/bash

# Simple script to update all submodules in the repository

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Updating all submodules..."
cd "$SCRIPT_DIR"

# Initialize and update all submodules
git submodule update --init --recursive

echo "All submodules updated successfully!" 
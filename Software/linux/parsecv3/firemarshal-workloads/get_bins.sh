#!/bin/bash

# Script to copy parsec-benchmark/pkgs to firemarshal-workloads destination
# Using relative paths from the current script location

# Define source and destination paths (relative to script location)
SOURCE_DIR="../parsec-benchmark/pkgs"
DEST_DIR="./parsecv3-workloads/parsec/overlay/root/pkgs"

echo "Copying pkgs directory..."
echo "Source: $SOURCE_DIR"
echo "Destination: $DEST_DIR"

# Check if source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Source directory $SOURCE_DIR does not exist!"
    exit 1
fi

# Check if destination directory exists, and remove it if it does
if [ -d "$DEST_DIR" ]; then
    echo "Destination directory $DEST_DIR already exists. Removing it..."
    rm -rf "$DEST_DIR"
    if [ $? -eq 0 ]; then
        echo "Successfully removed existing destination directory."
    else
        echo "Error: Failed to remove existing destination directory!"
        exit 1
    fi
fi

# Create parent directories if they don't exist
DEST_PARENT_DIR=$(dirname "$DEST_DIR")
if [ ! -d "$DEST_PARENT_DIR" ]; then
    echo "Creating parent directories: $DEST_PARENT_DIR"
    mkdir -p "$DEST_PARENT_DIR"
    if [ $? -ne 0 ]; then
        echo "Error: Failed to create parent directories!"
        exit 1
    fi
fi

# Copy the source directory to destination
echo "Copying $SOURCE_DIR to $DEST_DIR..."
cp -r "$SOURCE_DIR" "$DEST_DIR"

# Check if copy was successful
if [ $? -eq 0 ]; then
    echo "Successfully copied pkgs directory!"
    echo "Verifying destination..."
    if [ -d "$DEST_DIR" ]; then
        echo "Destination directory confirmed to exist: $DEST_DIR"
        ls -la "$DEST_DIR" | head -10
    else
        echo "Warning: Destination directory not found after copy operation!"
    fi
else
    echo "Error: Failed to copy pkgs directory!"
    exit 1
fi

echo "Copy operation completed successfully!"

# Run simplify_parsec.sh script after successful copy
SIMPLIFY_SCRIPT="parsecv3-workloads/parsec/overlay/root/simplify_parsec.sh"
echo "Running simplify_parsec.sh script..."
if [ -x "$SIMPLIFY_SCRIPT" ]; then
    ./"$SIMPLIFY_SCRIPT"
    if [ $? -eq 0 ]; then
        echo "Successfully completed simplify_parsec.sh script!"
    else
        echo "Warning: simplify_parsec.sh script completed with errors"
    fi
else
    echo "Error: simplify_parsec.sh script not found or not executable at $SIMPLIFY_SCRIPT"
    exit 1
fi

echo "All operations completed successfully!"
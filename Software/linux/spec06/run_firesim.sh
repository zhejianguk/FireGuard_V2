#!/bin/bash

# FireSim SPEC06 Benchmark Runner
# This script builds and runs SPEC06 benchmarks on FireSim

set -e  # Exit on any error

# Check for gc_kernel parameter
if [ $# -lt 1 ] || [ -z "$1" ]; then
    echo "Usage: $0 <gc_kernel> [benchmark_name]"
    echo "Example: $0 my_kernel_config"
    echo "Example: $0 my_kernel_config 400.perlbench"
    exit 1
fi

GC_KERNEL="$1"


# Configuration
cd spec06-benchmark && . ./env.sh
cd ..
BENCHMARKS=(400.perlbench 401.bzip2 403.gcc 429.mcf 445.gobmk 456.hmmer 458.sjeng 462.libquantum 464.h264ref 471.omnetpp 473.astar 483.xalancbmk)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SPEC06_DIR="${SCRIPT_DIR}/spec06-benchmark"
FIREMARSHAL_WORKLOADS_DIR="${SCRIPT_DIR}/firemarshal-workloads"
FIREMARSHAL_DIR="/home/zhejiang/firesim/sw/FireMarshal"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if directory exists
check_directory() {
    if [ ! -d "$1" ]; then
        log_error "Directory does not exist: $1"
        exit 1
    fi
}

# Function to run command with error checking
run_cmd() {
    local cmd="$1"
    local desc="$2"
    
    log_info "Running: $desc"
    if eval "$cmd"; then
        log_success "$desc completed successfully"
    else
        log_error "$desc failed"
        exit 1
    fi
}

# Function to build SPEC06 benchmarks
build_spec06() {
    log_info "Building SPEC06 benchmarks with gc_kernel: $GC_KERNEL"
    cd "$SPEC06_DIR"
    run_cmd "./build_spec06.sh $GC_KERNEL" "Building SPEC06 with gc_kernel $GC_KERNEL"
}

# Function to copy SPEC06 binaries
copy_spec06_binaries() {
    log_info "Copying SPEC06 binaries to firemarshal workloads"
    local source_dir="$SPEC06_DIR/riscv-spec-ref"
    local target_dir="$FIREMARSHAL_WORKLOADS_DIR/spec06-workloads/spec06/overlay/root/riscv-spec-ref"
    
    # Check if source directory exists
    if [ ! -d "$source_dir" ]; then
        log_error "Source directory does not exist: $source_dir"
        exit 1
    fi
    
    # Clean target directory contents if it exists
    if [ -d "$target_dir" ]; then
        log_info "Cleaning existing target directory: $target_dir"
        run_cmd "rm -rf $target_dir/*" "Cleaning target directory"
    fi
    
    # Create target directory if it doesn't exist
    mkdir -p "$target_dir"
    
    run_cmd "cp -r $source_dir/* $target_dir/" "Copying SPEC06 binaries"
}

# Function to update JSON configuration
update_json_config() {
    local benchmark="$1"
    local json_file="$FIREMARSHAL_WORKLOADS_DIR/spec06-workloads/spec06.json"
    
    log_info "Updating JSON configuration for benchmark: $benchmark"
    
    # Create the new command - note the corrected format
    local new_command="cd /root/riscv-spec-ref && ./run.sh -w $benchmark && poweroff -f"
    
    # Create a temporary file and rewrite the JSON properly
    local temp_file=$(mktemp)
    
    # Use Python to safely update the JSON
    python3 -c "
import json
import sys

# Read the original JSON
with open('$json_file', 'r') as f:
    data = json.load(f)

# Update the command
data['jobs'][0]['command'] = '$new_command'

# Write back to temp file
with open('$temp_file', 'w') as f:
    json.dump(data, f, indent=4)
"
    
    # Replace the original file with the updated one
    if [ $? -eq 0 ]; then
        mv "$temp_file" "$json_file"
        log_success "Updated JSON configuration"
    else
        log_error "Failed to update JSON configuration"
        rm -f "$temp_file"
        exit 1
    fi
}

# Function to build kernel
build_kernel() {
    log_info "Building kernel"
    cd "$FIREMARSHAL_DIR"
    run_cmd "./build_kernel_spec06.sh" "Building kernel"
}

# Function to run FireSim workflow
run_firesim_workflow() {
    local benchmark="$1"
    log_info "Running FireSim workflow for $benchmark"
    
    cd "$FIREMARSHAL_DIR"
    
    run_cmd "firesim infrasetup" "FireSim infrastructure setup"
    run_cmd "firesim runworkload" "FireSim workload execution"
    run_cmd "firesim terminaterunfarm" "FireSim run farm termination"
}

# Main execution
main() {
    log_info "Starting FireSim SPEC06 benchmark runner"
    log_info "GC Kernel: $GC_KERNEL"
    log_info "Benchmarks to run: ${BENCHMARKS[*]}"
    
    # Verify directories exist
    check_directory "$SPEC06_DIR"
    check_directory "$FIREMARSHAL_WORKLOADS_DIR"
    check_directory "$FIREMARSHAL_DIR"
    
    # Build SPEC06 once for all benchmarks
    build_spec06
    
    # Copy SPEC06 binaries once
    copy_spec06_binaries
    
    local total_benchmarks=${#BENCHMARKS[@]}
    local current=0
    
    for benchmark in "${BENCHMARKS[@]}"; do
        current=$((current + 1))
        echo ""
        echo "=========================================="
        log_info "Processing benchmark $current/$total_benchmarks: $benchmark"
        echo "=========================================="
        
        # Update JSON configuration for this benchmark
        update_json_config "$benchmark"
        
        # Build kernel
        build_kernel
        
        # Run FireSim workflow
        run_firesim_workflow "$benchmark"
        
        log_success "Completed benchmark: $benchmark"
        echo "=========================================="
    done
    
    echo ""
    log_success "All benchmarks completed successfully!"
    log_info "Total benchmarks processed: $total_benchmarks"
}

# Check if running with specific benchmark (second parameter)
if [ $# -eq 2 ]; then
    if [[ " ${BENCHMARKS[*]} " =~ " $2 " ]]; then
        log_info "Running single benchmark: $2"
        BENCHMARKS=("$2")
    else
        log_error "Invalid benchmark: $2"
        log_info "Available benchmarks: ${BENCHMARKS[*]}"
        exit 1
    fi
fi

# Run main function
main

#!/bin/bash

# FireSim PARSEC Benchmark Runner
# This script builds and runs PARSEC benchmarks on FireSim

set -e  # Exit on any error

# Configuration
BENCHMARKS=(swaptions)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARSEC_DIR="${SCRIPT_DIR}/parsec-benchmark/pkgs"
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

# Function to build benchmark
build_benchmark() {
    local benchmark="$1"
    log_info "Building benchmark: $benchmark"
    run_cmd "./build_parsec.sh -k sanitiser -b $benchmark" "Building $benchmark"
}

# Function to get binaries
get_binaries() {
    log_info "Getting binaries"
    cd "$FIREMARSHAL_WORKLOADS_DIR"
    run_cmd "./get_bins.sh" "Getting binaries"
}

# Function to build kernel
build_kernel() {
    log_info "Building kernel"
    cd "$FIREMARSHAL_DIR"
    run_cmd "./build_kernel.sh" "Building kernel"
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
    log_info "Starting FireSim PARSEC benchmark runner"
    log_info "Benchmarks to run: ${BENCHMARKS[*]}"
    
    # Verify directories exist
    check_directory "$PARSEC_DIR"
    check_directory "$FIREMARSHAL_WORKLOADS_DIR"
    check_directory "$FIREMARSHAL_DIR"
    
    # Change to parsec directory
    cd "$PARSEC_DIR"
    local base_dir="$PWD"
    
    local total_benchmarks=${#BENCHMARKS[@]}
    local current=0
    
    for benchmark in "${BENCHMARKS[@]}"; do
        current=$((current + 1))
        echo ""
        echo "=========================================="
        log_info "Processing benchmark $current/$total_benchmarks: $benchmark"
        echo "=========================================="
        
        # Build benchmark
        cd "$base_dir"
        build_benchmark "$benchmark"
        
        # Get binaries
        get_binaries
        
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

# Check if running with specific benchmark
if [ $# -eq 1 ]; then
    if [[ " ${BENCHMARKS[*]} " =~ " $1 " ]]; then
        log_info "Running single benchmark: $1"
        BENCHMARKS=("$1")
    else
        log_error "Invalid benchmark: $1"
        log_info "Available benchmarks: ${BENCHMARKS[*]}"
        exit 1
    fi
fi

# Run main function
main

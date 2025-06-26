# Kernel Build and Configuration Script

This script automates the process of compiling FireGuard kernels and updating the SPEC CPU2006 configuration.

## Prerequisites

**IMPORTANT:** You need the RISC-V cross-compiler toolchain installed before running this script.

### Installing RISC-V Toolchain

The script requires `riscv64-unknown-linux-gnu-gcc`. If you get an error about the compiler not being found:

1. **Check if it's already installed but not in PATH:**
   ```bash
   find /opt /usr -name "riscv64-unknown-linux-gnu-gcc" 2>/dev/null
   ```

2. **Install the toolchain** (method depends on your system):
   ```bash
   # Ubuntu/Debian
   sudo apt-get install gcc-riscv64-linux-gnu
   
   # Or build from source
   # See: https://github.com/riscv/riscv-gnu-toolchain
   ```

3. **Add to PATH** if installed in a custom location:
   ```bash
   export PATH="/path/to/riscv/toolchain/bin:$PATH"
   ```

4. **Verify installation:**
   ```bash
   riscv64-unknown-linux-gnu-gcc --version
   ```

## Usage

```bash
./build_and_configure_kernel.sh <kernel_type>
```

## Available Kernel Types

- `none` - Basic kernel without additional protection
- `pmc` - Performance monitoring counter kernel
- `perf` - Performance-optimized kernel  
- `sanitiser` - Memory sanitizer kernel
- `minesweeper` - Minesweeper protection kernel
- `ss` - Shadow stack kernel
- `ss_mc` - Shadow stack with memory checking kernel

## Example

```bash
# Build and configure the sanitiser kernel
./build_and_configure_kernel.sh sanitiser
```

## What the Script Does

1. **Validates prerequisites:**
   - Checks for RISC-V cross-compiler availability
   - Verifies directory access

2. **Compiles the kernel components:**
   - `initialisation_{kernel}` - Creates an executable (.riscv file)
   - `gc_main_{kernel}` - Creates an object file (.o file)

3. **Copies files:**
   - Copies `initialisation_{kernel}.riscv` to the spec06-benchmark directory

4. **Updates configuration:**
   - Updates `riscv.cfg` to point to the correct path for the kernel object file
   - Creates a backup of the original configuration as `riscv.cfg.backup`

## Files Modified

- `FireGuard_V2/Software/linux/spec06/spec06-benchmark/riscv.cfg` - Updated with correct kernel paths
- `FireGuard_V2/Software/linux/spec06/spec06-benchmark/riscv.cfg.backup` - Backup of original configuration

## Output

The script will show the compilation progress and confirm successful completion. The final output shows the updated optimization flags in the riscv.cfg file.

## Troubleshooting

- **Compiler not found:** Install RISC-V toolchain (see Prerequisites section)
- **Make errors:** Check that all source files exist in the kernels directory
- **Permission denied:** Ensure you have write access to the FireGuard directories
- **Path issues:** Verify all directory paths exist and are accessible 
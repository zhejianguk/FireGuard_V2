#!/bin/bash

# Check if yq is installed
if ! command -v yq &> /dev/null; then
    echo "Error: yq is required to parse YAML. Please install it:"
    echo "  Ubuntu/Debian: sudo snap install yq"
    echo "  Or download from: https://github.com/mikefarah/yq/releases"
    exit 1
fi

# Get the directory of the script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_FILE="$SCRIPT_DIR/link_config.yaml"

# Check if config file exists
if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Configuration file $CONFIG_FILE not found"
    exit 1
fi

# Determine mode
MODE="chipyard"
if [ "$1" = "--firesim" ] || [ "$1" = "-f" ] || [ "$1" = "-firesim" ]; then
  MODE="firesim"
fi

echo "Mode: $MODE"

# Read configuration from YAML
SRC_BASE=$(yq eval '.src_base' "$CONFIG_FILE")
echo "Source base: $SRC_BASE"

# Check if this mode uses multiple destinations
DESTINATIONS=$(yq eval ".${MODE}.destinations // null" "$CONFIG_FILE")
if [ "$DESTINATIONS" != "null" ]; then
    echo "Using multiple destination bases:"
    yq eval ".${MODE}.destinations | to_entries | .[] | \"  \" + .key + \": \" + .value" "$CONFIG_FILE"
else
    DST_BASE=$(yq eval ".${MODE}.dst_base" "$CONFIG_FILE")
    echo "Destination base: $DST_BASE"
fi

# Function to create a symbolic link
create_link() {
  local src="$1"
  local dst="$2"

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
}

# Function to resolve destination path
resolve_destination() {
  local dst_rel="$1"
  
  if [ "$DESTINATIONS" != "null" ]; then
    # Handle destination:path format
    if [[ "$dst_rel" == *":"* ]]; then
      local dest_name="${dst_rel%%:*}"
      local dest_path="${dst_rel#*:}"
      local dest_base=$(yq eval ".${MODE}.destinations.${dest_name}" "$CONFIG_FILE")
      echo "$dest_base/$dest_path"
    else
      # Default to main destination if no prefix
      local dest_base=$(yq eval ".${MODE}.destinations.main" "$CONFIG_FILE")
      echo "$dest_base/$dst_rel"
    fi
  else
    # Traditional single destination base
    echo "$DST_BASE/$dst_rel"
  fi
}

# Process each link from the YAML configuration
echo "Creating symbolic links..."
yq eval ".${MODE}.links | to_entries | .[] | [.key, .value] | @tsv" "$CONFIG_FILE" | while IFS=$'\t' read -r src_rel dst_rel; do
  src_full="$SRC_BASE/$src_rel"
  dst_full=$(resolve_destination "$dst_rel")
  
  create_link "$src_full" "$dst_full"
done

echo "Linking complete!"
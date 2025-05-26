#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025 NXP

# Script to search for and copy the required kernel modules.
# This script searches for the specified kernel modules in the current directory,
# assuming it is the kernel source directory, and copies them into a directory named "cmm_modules".
# Optionally, the user can provide a custom search path as the first argument.

# Space-separated list of modules
modules="nf_defrag_ipv4.ko nf_defrag_ipv6.ko nf_conntrack.ko nf_nat.ko x_tables.ko xt_nat.ko xt_MASQUERADE.ko ip_tables.ko iptable_filter.ko iptable_nat.ko xt_conntrack.ko nf_conntrack_netlink.ko ip6_tables.ko ip6table_filter.ko ip6table_nat.ko"

# Use first argument as Linux tree root or default to current directory
LINUX_TREE="${1:-.}"

# Destination directory
DEST_DIR="cmm_modules"

# Create destination directory if it doesn't exist
mkdir -p "$DEST_DIR"

echo "Searching for modules in: $LINUX_TREE"
echo "Copying found modules to: $DEST_DIR"

# Loop through each module
for module in $modules; do
  module_path=$(find "$LINUX_TREE" -name "$module" 2>/dev/null | head -n 1)

  if [ -z "$module_path" ]; then
    echo "Error: Module $module not found in $LINUX_TREE."
    continue
  fi

  if ! cp "$module_path" "$DEST_DIR/"; then
    echo "Error: Failed to copy $module from $module_path."
  else
    echo "Copied $module to $DEST_DIR."
  fi
done


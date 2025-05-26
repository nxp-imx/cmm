#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025 NXP

# Script to load required kernel modules.
# This script searches for the specified kernel modules in the "/" directory
# and attempts to insert them using insmod.
# Optionally, the user can provide a custom search path as the first argument.

# List of kernel modules to load
modules=(
  nf_defrag_ipv4.ko
  nf_defrag_ipv6.ko
  nf_conntrack.ko
  nf_nat.ko
  x_tables.ko
  xt_nat.ko
  xt_MASQUERADE.ko
  ip_tables.ko
  iptable_filter.ko
  iptable_nat.ko
  xt_conntrack.ko
  nf_conntrack_netlink.ko
  ip6_tables.ko
  ip6table_nat.ko
  ip6table_filter.ko
)

# Use first argument as root path or default to "/"
ROOTFS="${1:-/}"

echo "Searching for modules in: $ROOTFS"

# Loop through each module
for module in "${modules[@]}"; do
  module_path=$(find "$ROOTFS" -name "$module" 2>/dev/null | head -n 1)

  if [ -z "$module_path" ]; then
    echo "Error: Module $module not found in $ROOTFS."
    continue
  fi

  if ! insmod "$module_path"; then
    echo "Error: Failed to insert module $module from $module_path."
  else
    echo "Successfully inserted $module."
  fi
done

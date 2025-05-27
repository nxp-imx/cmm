#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025 NXP

# Usage: ./network_setup.sh [-p] [--no-vlan-offload] [--no-rxhash-off] <iface1> <ip1> <iface2> <ip2> ...
# Example: ./network_setup.sh -p eth0 2001:db8:1::1 eth1 2001:db8:2::1
# Example: ./network_setup.sh -p eth0 1.1.1.2 eth1 2.1.1.2
# default IPv6 subnet is 64 and IPv4 is 24

PROMISC=0
HAS_IPV6=0
DISABLE_VLAN_OFFLOAD=1
DISABLE_RXHASH=1

# Check for -p flag and other options
while [[ "$1" == -* ]]; do
    case "$1" in
        -p) PROMISC=1 ;;  # Enable promiscuous mode
        --no-vlan-offload) DISABLE_VLAN_OFFLOAD=0 ;;  # Keep VLAN offload enabled
        --no-rxhash-off) DISABLE_RXHASH=0 ;;  # Keep RX hash enabled
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
    shift
done

# Validate argument count
if (( $# % 2 != 0 )); then
    echo "Error: Arguments must be in pairs of <interface> <IP address>"
    exit 1
fi

# Set unlimited locked memory
ulimit -l unlimited

# Enable IP forwarding for both IPv4 and IPv6
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 1 > /proc/sys/net/ipv6/conf/all/forwarding

# Loop through interface-IP pairs
while (( "$#" )); do
    IFACE=$1
    IPADDR=$2

    echo "Configuring $IFACE with IP $IPADDR..."

    # Disable general offloading features
    ethtool -K $IFACE gro off gso off tso off

    # Conditionally disable VLAN offload
    if (( DISABLE_VLAN_OFFLOAD )); then
        echo "Disabling VLAN offload on $IFACE"
        ethtool -K $IFACE rxvlan off txvlan off
    fi

    # Conditionally disable RX hash
    if (( DISABLE_RXHASH )); then
        echo "Disabling RX hash on $IFACE"
        ethtool -K $IFACE rxhash off
    fi

    # Bring interface up first
    ip link set $IFACE up

    # Wait briefly to allow IPv6 to initialize
    sleep 1

    # Assign IP address (detect IPv6 vs IPv4)
    if [[ $IPADDR == *:* ]]; then
      # Wait until link-local address is available (max 5 seconds)
      for i in {1..5}; do
          if ip -6 addr show dev $IFACE | grep -q 'fe80::'; then
              break
          fi
          sleep 1
      done

      echo "Assigning IPv6 address ${IPADDR}/64 to $IFACE"
      ip -6 addr add ${IPADDR}/64 dev $IFACE
      HAS_IPV6=1
    else
      ip addr add ${IPADDR}/24 dev $IFACE
    fi

    # Enable promiscuous mode if requested
    if (( PROMISC )); then
        echo "Enabling promiscuous mode on $IFACE"
        ip link set $IFACE promisc on
    fi

    # Shift to next pair
    shift 2
done

# Flush and list NAT table for IPv4 if supported
if iptables -t nat -L &>/dev/null; then
    iptables -t nat -F
    iptables -t nat -L -n

    # Allow forwarding of established connections (IPv4)
    iptables -A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT
else
    echo "Warning: NAT table not supported in iptables."
fi

# IPv6 NAT and forwarding rules only if IPv6 was used and supported
if (( HAS_IPV6 )); then
    if ip6tables -t nat -L &>/dev/null; then
        ip6tables -t nat -F
        ip6tables -t nat -L -n
        ip6tables -A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT
    else
        echo "Warning: NAT table not supported in ip6tables."
    fi
fi

echo "All interfaces configured successfully."

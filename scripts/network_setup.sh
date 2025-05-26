#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025 NXP

# Usage: ./network_setup.sh [-p] <iface1> <ip1> <iface2> <ip2> ...
# Example: ./network_setup.sh -p eth0 2001:db8:1::1 eth1 2001:db8:2::1
# Example: ./network_setup.sh -p eth0 1.1.1.2 eth1 2.1.1.2
# default IPv6 subnet is 64 and IPv4 is 24

PROMISC=0

# Check for -p flag
if [[ "$1" == "-p" ]]; then
    PROMISC=1
    shift
fi

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

    # Disable offloading and VLAN features
    ethtool -K $IFACE gro off gso off tso off rxvlan off txvlan off rxhash off
    ethtool -A $IFACE rx off tx off autoneg off


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

# Flush and list NAT table for IPv4
iptables -t nat -F
iptables -t nat -L -n

# Allow forwarding of established connections (IPv4)
iptables -A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT

echo "All interfaces configured successfully."

#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025 NXP

# Usage: ./network_setup.sh [-p] <iface1> <ip1> <iface2> <ip2> ...
# Example: ./network_setup.sh -p eth0 1.1.1.2 eth1 2.1.1.2

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

# Enable IP forwarding
echo 1 > /proc/sys/net/ipv4/conf/all/forwarding

# Loop through interface-IP pairs
while (( "$#" )); do
    IFACE=$1
    IPADDR=$2

    echo "Configuring $IFACE with IP $IPADDR..."

    # Disable offloading and VLAN features
    ethtool -K $IFACE gro off gso off tso off rxvlan off txvlan off rxhash off
    ethtool -A $IFACE rx off tx off autoneg off

    # Assign IP address
    ip addr add ${IPADDR}/24 dev $IFACE

    # Bring interface up
    ip link set $IFACE up

    # Enable promiscuous mode if requested
    if (( PROMISC )); then
        echo "Enabling promiscuous mode on $IFACE"
        ip link set $IFACE promisc on
    fi

    # Shift to next pair
    shift 2
done

# Flush and list NAT table
iptables -t nat -F
iptables -t nat -L -n

# Allow forwarding of established connections
iptables -A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT

echo "All interfaces configured successfully."

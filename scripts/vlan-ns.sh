#!/bin/bash

# Clean up from previous runs (optional)
ip netns del ns1 2>/dev/null
ip netns del ns2 2>/dev/null
ip netns del ns3 2>/dev/null
ip netns del ns4 2>/dev/null
ip link del br1 2>/dev/null
ip link del br2 2>/dev/null
ip link del br-link1 2>/dev/null

# 1. Create network namespaces
ip netns add ns1
ip netns add ns2
ip netns add ns3
ip netns add ns4

# 2. Create veth pairs
ip link add veth1 type veth peer name veth1-br
ip link add veth2 type veth peer name veth2-br
ip link add veth3 type veth peer name veth3-br
ip link add veth4 type veth peer name veth4-br

# 3. Assign veth interfaces to namespaces
ip link set veth1 netns ns1
ip link set veth2 netns ns2
ip link set veth3 netns ns3
ip link set veth4 netns ns4

# 4. Create bridges
ip link add name br1 type bridge
ip link add name br2 type bridge

# 5. Create inter-bridge trunk link
ip link add br-link1 type veth peer name br-link2

# 6. Attach veths to bridges
ip link set veth1-br master br1
ip link set veth2-br master br1
ip link set br-link1 master br1

ip link set veth3-br master br2
ip link set veth4-br master br2
ip link set br-link2 master br2

# 7. Bring up bridges and interfaces
ip link set br1 up
ip link set br2 up

ip link set veth1-br up
ip link set veth2-br up
ip link set veth3-br up
ip link set veth4-br up
ip link set br-link1 up
ip link set br-link2 up

# 8. Enable VLAN filtering
ip link set br1 type bridge vlan_filtering 1
ip link set br2 type bridge vlan_filtering 1

# 9. Configure VLANs
# br1 side
bridge vlan add dev veth1-br vid 1 pvid untagged
bridge vlan add dev veth2-br vid 2 pvid untagged
bridge vlan add dev br-link1 vid 1
bridge vlan add dev br-link1 vid 2

# br2 side
bridge vlan add dev veth3-br vid 1 pvid untagged
bridge vlan add dev veth4-br vid 2 pvid untagged
bridge vlan add dev br-link2 vid 1
bridge vlan add dev br-link2 vid 2

# 10. Setup interfaces inside namespaces
ip netns exec ns1 ip link set lo up
ip netns exec ns1 ip link set veth1 up
ip netns exec ns1 ip addr add 1.1.1.1/24 dev veth1

ip netns exec ns2 ip link set lo up
ip netns exec ns2 ip link set veth2 up
ip netns exec ns2 ip addr add 1.1.1.2/24 dev veth2

ip netns exec ns3 ip link set lo up
ip netns exec ns3 ip link set veth3 up
ip netns exec ns3 ip addr add 1.1.1.3/24 dev veth3

ip netns exec ns4 ip link set lo up
ip netns exec ns4 ip link set veth4 up
ip netns exec ns4 ip addr add 1.1.1.4/24 dev veth4


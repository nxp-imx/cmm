# Compilation steps:

Set up your Yocto SDK environment using the LF release and run following command to compile cmm:

```bash
bitbake cmm
```

> Ensure the following libraries are available:
```bash
libcmm, libcrypt, libcli, libnfnetlink, libbpf
```
> Additionally, verify that the following binaries and scripts are present on the DUT (Device Under Test)

```bash
cmm
load_cmm_modules.sh
network_setup.sh
```

## 🔧 Setup Instructions

### 1. Configure Network Interfaces and load kernel modules

Connect to the network using DHCP or assign a static IP:

```bash
udhcp
```
OR
```bash
ifconfig <ethx> <ip>
```

Load kernel modules:

```bash
/etc/cmm/load_cmm_modules.sh /
```

Set up interfaces and enable IP forwarding:

For IPv4:
```bash
/etc/cmm/network_setup.sh eth0 1.1.1.2 eth1 2.1.1.2
```
or
For IPv6:
```bash
/etc/cmm/network_setup.sh eth0 2001:db8:1::2 eth1 2001:db8:2::2
```

> Replace `eth0`, `eth1`, and IPs as per your setup.

---

### 2. Packet Processing Framework (XDP)

1. Load XDP program using the recommended `xdp_fp` utility:

```bash
/opt/xdp/xdp_fp -a eth0 eth1              # IP layer Fastpath mode
/opt/xdp/xdp_fp -a eth0 eth1 -b           # Bridge Fastpath mode
/opt/xdp/xdp_fp -h                        # Help
```

2. Alternatively, use `ip link` method:

```bash
ip link set dev eth0 xdpgeneric obj /opt/xdp/imx-xdp-fp.o sec xdp_fp
ip link set dev eth1 xdpgeneric obj /opt/xdp/imx-xdp-fp.o sec xdp_fp
```
> `xdp_fp` section is for IP forwarding fastpath mode, `xdp_fp_bridge` section is for Bridge fastpath mode.

---

### 2. Run CMM

Start the CMM daemon:

```bash
cmm
```

Connect to the CMM CLI via telnet:

```bash
telnet 127.0.0.1 2103
# Login: admin / admin
```

---

## 🧪 Useful CLI Commands

- `?` – Short help  
- `help` – Detailed help  
- `en` – Enable privileged mode  
- `set debug error|warning|info 1` – Enable debug logs  
- `show route`, `show neighbor`, `show connections`, `show rules`, `show debug_level` – Monitoring commands  
- `show fpp_route` – View fastpath routes 
- `kill` - Kill CMM daemon

---

## 📊 Monitoring & Debugging

Check Fastpath statistics in XDP:

```bash
/opt/xdp/xdp_fp -s
```

List connection tracking entries:

```bash
conntrack -L
```

> Note: `conntrack` may not be available in rootfs. Download from [ArchLinux ARM](https://archlinuxarm.org/packages/aarch64/conntrack-tools) and ensure dependencies are present.

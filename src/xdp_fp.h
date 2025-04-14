/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 *
 */

#ifndef _XDP_FP_COMMON_H
#define _XDP_FP_COMMON_H

#include <stdint.h>

#define MAX_IPV4_ENTRIES 512
#define MAX_IPV6_ENTRIES 512
#define MAX_FP_ROUTES	 512

#define MAX_L2_HEADER_SIZE	18

#define FP_INFO_FLAG_FAST_PATH 		(1 << 0)
#define FP_INFO_FLAG_NAT       		(1 << 1)
#define FP_INFO_FLAG_CONNTRACK_ORIG	(1 << 2)

#define ENET_IFACE	0
#define RAWIP_IFACE	1

enum {
	GLOB_ROUTE,
	GLOB_FF_DISABLE,
	GLOB_MAX
};

struct route {
	uint32_t flags;
	int redir_ifindex;
	uint8_t l2_hdr[MAX_L2_HEADER_SIZE];
	uint32_t l2_hdr_size;
	uint16_t mtu;
	uint16_t redir_if_type;
};

struct ipv4_flow {
	uint32_t saddr;
	uint32_t daddr;
	uint16_t sport;
	uint16_t dport;
	uint8_t protocol;
};

struct ipv4_info {
	uint8_t flags;
	uint8_t active;
	uint32_t nat_saddr;
	uint32_t nat_daddr;
	uint16_t nat_sport;
	uint16_t nat_dport;
	unsigned int ip_csum_corr;
	unsigned int trans_csum_corr;
	int route_ifindex;
	unsigned int last_timer;
	uint16_t mtu;
};

struct ipv6_flow {
	uint32_t saddr[4];
	uint32_t daddr[4];
	uint16_t sport;
	uint16_t dport;
	uint8_t protocol;
};

struct ipv6_info {
	uint8_t flags;
	uint8_t active;
	uint32_t nat_saddr[4];
	uint32_t nat_daddr[4];
	uint16_t nat_sport;
	uint16_t nat_dport;
	unsigned int trans_csum_corr;
	int route_ifindex;
	unsigned int last_timer;
	uint16_t mtu;
};

#endif

/*
 *Copyright 2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 *
 *
 */

#ifndef __CMM_FPR_H
#define __CMM_FPR_H

#define MAX_NAME_LENGTH 20
#define MAX_MAC_LENGTH 6
#define FPR_CMM_PATH "/tmp/fpr_cmm.sock"
typedef enum { LINK_ADD, LINK_DELETE } link_action_t;
typedef enum { ADDR_ADD, ADDR_DELETE } addr_action_t;
typedef enum { NEIGHBOR_ADD, NEIGHBOR_DELETE } neighbor_action_t;
typedef enum { ROUTE_ADD, ROUTE_DELETE } route_action_t;

typedef enum {
        LINK_UNKNOWN,
        LINK_NOTPRESENT,
        LINK_DOWN,
        LINK_LOWERLAYERDOWN,
        LINK_TESTING,
        LINK_DORMANT,
        LINK_UP
} oper_state_t;

enum msg_t {
        CMM_ROUTE4 = 1,
        CMM_ROUTE6,
        CMM_NEIGHBOR4,
        CMM_NEIGHBOR6,
        CMM_ADDR4,
        CMM_ADDR6,
        CMM_LINK
};

/* cmm msg for DPDK FPR application */
struct cmm_msg {
	enum msg_t msg_type;
	union {
		/* Route event data */
       		struct {
			uint8_t r_type;
	       		route_action_t r_action;
			union {
       				struct in_addr r_addr;
				struct in6_addr r_addr6;
			};
       			uint8_t r_depth;
			union {
       				struct in_addr r_nexthop;
				struct in6_addr r_nexthop6;
			};
	       	};
		/* Neighbor event data */
		struct {
			neighbor_action_t n_action;
			uint32_t n_portid;
			union {
       				struct in_addr n_addr;
       				struct in6_addr n_addr6;
			};
			uint8_t n_mac[MAX_MAC_LENGTH];
			uint8_t n_flags;
			uint16_t n_vlanid;
		};
		/* address event data */
		struct {
			addr_action_t a_action;
			uint32_t a_portid;
			union {
				struct in_addr a_addr;
				struct in6_addr a_addr6;
			};
			uint8_t a_prefixlen;
		};
		/* Link event data */
		struct {
			link_action_t l_action;
			uint32_t l_portid;
			uint8_t l_mac[MAX_MAC_LENGTH];
			uint32_t l_mtu;
			uint8_t l_name[MAX_NAME_LENGTH];
			uint16_t l_vlanid;
			oper_state_t l_state;
		};
       };
};

/* API to send message to FPR application */
void fpr_send_message(int fd, struct cmm_msg *message);

#endif

/* Copyright 2019,2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 */

#include <stddef.h>
#include <errno.h>
#include <bpf/bpf.h>
#include <linux/bpf.h>
#include <sys/timerfd.h>
#include "cmm.h"
#include "fpp.h"
#include "cpal_bpf.h"
#include "xdp_fp.h"
#include <net/if_arp.h>

#define CPAL_MAX_PAYLOAD 256

#define PINNED_IPV4 "/sys/fs/bpf/fp_ipv4"
#define PINNED_IPV6 "/sys/fs/bpf/fp_ipv6"
#define PINNED_ROUTE "/sys/fs/bpf/fp_route"
#define PINNED_GLOBALS "/sys/fs/bpf/fp_globals"

// This variable needs to be accessed from various threads/handles, so cannot be kept in cpal_handle_t
unsigned int current_timer = 0;
static cpal_timeouts_t timeouts;
uint8_t ff_enable = 1;
static cpal_route_info_t route_info[MAX_FP_ROUTES] = {0};

static inline void ipv6_addr_copy(uint32_t *a, uint32_t *b)
{
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
	a[3] = b[3];
}

static inline unsigned int ipv6_addr_cmp(uint32_t *a, uint32_t *b)
{
	if ((a[0] == b[0]) &&
		(a[1] == b[1]) &&
		(a[2] == b[2]) &&
		(a[3] == b[3]))
		return 1;
	else
		return 0;
}

static inline unsigned int get_timeout_value(uint8_t protocol, int bidir_flag)
{
	unsigned int timeout_value;

	switch (protocol) {
	case IPPROTO_UDP:
		if (bidir_flag)
			timeout_value = timeouts.udp_bidir_timeout;
		else
			timeout_value = timeouts.udp_unidir_timeout;
		break;
	case IPPROTO_TCP:
			timeout_value = timeouts.tcp_timeout;
		break;
	default:
			timeout_value = timeouts.other_proto_timeout;
		break;
	}

	return timeout_value;
}

static void set_ipv4_checksum_correction(struct ipv4_flow *ipv4f,
		struct ipv4_info *ipv4i)
{
	unsigned int saddr_corr = 0;
	unsigned int daddr_corr = 0;
	unsigned int sport_corr = 0;
	unsigned int dport_corr = 0;
	unsigned int corr;

	if (ipv4f->saddr != ipv4i->nat_saddr
			|| ipv4f->daddr != ipv4i->nat_daddr
			|| ipv4f->sport != ipv4i->nat_sport
			|| ipv4f->dport != ipv4i->nat_dport)
		ipv4i->flags |= FP_INFO_FLAG_NAT;

	ipv4i->ip_csum_corr = 0x0001;

	/* Calculate checksum correction */
	saddr_corr = (ipv4f->saddr & 0xffff) + (ipv4f->saddr >> 16)
		+ ((ipv4i->nat_saddr & 0xffff) ^ 0xffff)
		+ ((ipv4i->nat_saddr >> 16) ^ 0xffff);
	daddr_corr = (ipv4f->daddr & 0xffff) + (ipv4f->daddr >> 16)
		+ ((ipv4i->nat_daddr & 0xffff) ^ 0xffff)
		+ ((ipv4i->nat_daddr >> 16) ^ 0xffff);
	sport_corr = ipv4f->sport + (ipv4i->nat_sport ^ 0xffff);
	dport_corr = ipv4f->dport + (ipv4i->nat_dport ^ 0xffff);

	/* IP checksum */
	/* NAT correction */
	corr = saddr_corr + daddr_corr;
	while (corr >> 16)
		corr = (corr & 0xffff) + (corr >> 16);

	if (corr == 0xffff)
		corr = 0;

	/* TTL correction */
	corr += 0x0001;
	if (corr + 1 >= 0x10000)
		corr++;

	ipv4i->ip_csum_corr = corr;

	/* UDP/TCP checksum */
	corr = saddr_corr + daddr_corr + dport_corr + sport_corr;

	 while (corr >> 16)
		corr = (corr & 0xffff) + (corr >> 16);

	 if (corr == 0xffff)
		corr = 0;

	ipv4i->trans_csum_corr = corr;
}

static void set_ipv6_checksum_correction(struct ipv6_flow *ipv6f,
		struct ipv6_info *ipv6i)
{
	unsigned int saddr_corr = 0;
	unsigned int daddr_corr = 0;
	unsigned int sport_corr = 0;
	unsigned int dport_corr = 0;
	unsigned int corr;

	if ((!ipv6_addr_cmp(ipv6f->saddr, ipv6i->nat_saddr)) ||
		(!ipv6_addr_cmp(ipv6f->daddr, ipv6i->nat_daddr)) ||
		(ipv6f->sport != ipv6i->nat_sport) ||
		(ipv6f->dport != ipv6i->nat_dport))
		ipv6i->flags |= FP_INFO_FLAG_NAT;

	/* Calculate checksum correction */
	saddr_corr = (ipv6f->saddr[0] & 0xffff) + (ipv6f->saddr[0] >> 16) +
				((ipv6i->nat_saddr[0] & 0xffff) ^ 0xffff) +
				((ipv6i->nat_saddr[0] >> 16) ^ 0xffff);
	saddr_corr += (ipv6f->saddr[1] & 0xffff) + (ipv6f->saddr[1] >> 16) +
				((ipv6i->nat_saddr[1] & 0xffff) ^ 0xffff) +
				((ipv6i->nat_saddr[1] >> 16) ^ 0xffff);
	saddr_corr += (ipv6f->saddr[2] & 0xffff) + (ipv6f->saddr[2] >> 16) +
				((ipv6i->nat_saddr[2] & 0xffff) ^ 0xffff) +
				((ipv6i->nat_saddr[2] >> 16) ^ 0xffff);
	saddr_corr += (ipv6f->saddr[3] & 0xffff) + (ipv6f->saddr[3] >> 16) +
				((ipv6i->nat_saddr[3] & 0xffff) ^ 0xffff) +
				((ipv6i->nat_saddr[3] >> 16) ^ 0xffff);

	daddr_corr = (ipv6f->daddr[0] & 0xffff) + (ipv6f->daddr[0] >> 16) +
				((ipv6i->nat_daddr[0] & 0xffff) ^ 0xffff) +
				((ipv6i->nat_daddr[0] >> 16) ^ 0xffff);
	daddr_corr += (ipv6f->daddr[1] & 0xffff) + (ipv6f->daddr[1] >> 16) +
				((ipv6i->nat_daddr[1] & 0xffff) ^ 0xffff) +
				((ipv6i->nat_daddr[1] >> 16) ^ 0xffff);
	daddr_corr += (ipv6f->daddr[2] & 0xffff) + (ipv6f->daddr[2] >> 16) +
				((ipv6i->nat_daddr[2] & 0xffff) ^ 0xffff) +
				((ipv6i->nat_daddr[2] >> 16) ^ 0xffff);
	daddr_corr += (ipv6f->daddr[3] & 0xffff) + (ipv6f->daddr[3] >> 16) +
				((ipv6i->nat_daddr[3] & 0xffff) ^ 0xffff) +
				((ipv6i->nat_daddr[3] >> 16) ^ 0xffff);

	sport_corr = ipv6f->sport + (ipv6i->nat_sport ^ 0xffff);
	dport_corr = ipv6f->dport + (ipv6i->nat_dport ^ 0xffff);

	/* UDP/TCP checksum */
	corr = saddr_corr + daddr_corr + dport_corr + sport_corr;

	while (corr >> 16)
		corr = (corr & 0xffff) + (corr >> 16);

	if (corr == 0xffff)
		corr = 0;

	ipv6i->trans_csum_corr = corr;
}

static int hash_route_id(u_int32_t route_id)
{
	int key = route_id % MAX_FP_ROUTES;
	int limit = key ? key - 1 : MAX_FP_ROUTES - 1;

	while (route_info[key].id && (key != limit)) {
		key++;
		if (key >= MAX_FP_ROUTES)
			key = 0;
	}

	return (key != limit) ? key : -1;
}

static int find_route_id(u_int32_t route_id)
{
	int key = route_id % MAX_FP_ROUTES;
	int limit = key ? key - 1 : MAX_FP_ROUTES - 1;

	while ((route_info[key].id != route_id) && (key != limit)) {
		key++;
		if (key >= MAX_FP_ROUTES)
			key = 0;
	}

	return (key != limit) ? key : -1;
}

cpal_handle_t *cpal_ff_open(void)
{
	cpal_handle_t *bpf_handle = NULL;

	bpf_handle = malloc(sizeof(cpal_handle_t));
	if (!bpf_handle) {
		cmm_print(DEBUG_ERROR, "CPAL handle allocation failed: %s(%d)\n",
			strerror(errno), errno);
		goto err_malloc;
	}

	bpf_handle->route_fd = bpf_obj_get(PINNED_ROUTE);
	if (bpf_handle->route_fd < 0) {
		cmm_print(DEBUG_ERROR, "bpf_obj_get(%s): %s(%d)\n",
			PINNED_ROUTE, strerror(errno), errno);
		goto err;
	}
	bpf_handle->ipv4_fd = bpf_obj_get(PINNED_IPV4);
	if (bpf_handle->ipv4_fd < 0) {
		cmm_print(DEBUG_ERROR, "bpf_obj_get(%s): %s(%d)\n",
			PINNED_IPV4, strerror(errno), errno);
		goto err;
	}

	bpf_handle->ipv6_fd = bpf_obj_get(PINNED_IPV6);
	if (bpf_handle->ipv6_fd < 0) {
		cmm_print(DEBUG_ERROR, "bpf_obj_get(%s): %s(%d)\n",
			PINNED_IPV6, strerror(errno), errno);
		goto err;
	}

	bpf_handle->globals_fd = bpf_obj_get(PINNED_GLOBALS);
	if (bpf_handle->globals_fd < 0) {
		cmm_print(DEBUG_ERROR, "bpf_obj_get(%s): %s(%d)\n",
			PINNED_GLOBALS, strerror(errno), errno);
		goto err;
	}

	return bpf_handle;
err:
	free(bpf_handle);
err_malloc:
	return NULL;
}

cpal_handle_t *cpal_ff_catch_open(void)
{
	cpal_handle_t *bpf_handle = cpal_ff_open();
	int rc;
	struct itimerspec timer_val = {
		.it_value = {
			.tv_sec = 0,
			.tv_nsec = CONNECTION_CHECK_INTERVAL
		},
	};

	if (bpf_handle) {
		bpf_handle->ff_catch_fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
		if (bpf_handle->ff_catch_fd < 0) {
			cmm_print(DEBUG_ERROR, "CPAL catch handle creation failed: timerfd_create: %s(%d)\n",
				strerror(errno), errno);
			cpal_close(bpf_handle);
			goto exit;
		}

		timeouts.tcp_timeout = TCP_TIMEOUT * CPAL_TICKS_PER_SECOND;
		timeouts.udp_bidir_timeout = UDP_BIDIR_TIMEOUT * CPAL_TICKS_PER_SECOND;
		timeouts.udp_unidir_timeout = UDP_UNIDIR_TIMEOUT * CPAL_TICKS_PER_SECOND;
		timeouts.other_proto_timeout = OTHER_PROTO_TIMEOUT * CPAL_TICKS_PER_SECOND;

		rc = timerfd_settime(bpf_handle->ff_catch_fd, 0, &timer_val, NULL);
		if (rc < 0) {
			cmm_print(DEBUG_ERROR, "CPAL catch handle creation failed: timerfd_settime: %s(%d)\n",
				strerror(errno), errno);
			cpal_close(bpf_handle);
			bpf_handle = NULL;
			goto exit;
		}
	}

exit:
	return bpf_handle;
}

static int check_ipv4_timeout(cpal_handle_t *handle)
{
	int count = 0, rc = 0, bidir_flag;
	struct ipv4_info ipv4_entry = {}, ipv4_entry_reply = {};
	struct ipv4_flow ipv4_key_reply = {};
	fpp_ct_cmd_t command;

	memset(&handle->timeout_ipv4_next_key, 0, sizeof(struct ipv4_flow));

	while (count < (MAX_IPV4_ENTRIES >> CONNECTION_CHECK_RATIO_LOG)) {
		rc = bpf_map_get_next_key(handle->ipv4_fd, &handle->timeout_ipv4_next_key, &handle->timeout_ipv4_next_key);
		if (rc < 0) {
			// Not an error, we reached the end of the table, so we can exit
			if (errno == ENOENT) {
				rc = 0;
				memset(&handle->timeout_ipv4_next_key, 0, sizeof(struct ipv4_flow));
				goto exit;
			}
			cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
				__func__, __LINE__, errno, strerror(errno));
			count++;
			continue;
		}

		rc = bpf_map_lookup_elem(handle->ipv4_fd, &handle->timeout_ipv4_next_key, &ipv4_entry);
		if (rc < 0) {
			cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
					__func__, __LINE__, errno, strerror(errno));
			count++;
			continue;
		}

		if (ipv4_entry.flags & FP_INFO_FLAG_CONNTRACK_ORIG) {
			ipv4_key_reply.saddr = ipv4_entry.nat_daddr;
			ipv4_key_reply.daddr = ipv4_entry.nat_saddr;
			ipv4_key_reply.sport = ipv4_entry.nat_dport;
			ipv4_key_reply.dport = ipv4_entry.nat_sport;
			ipv4_key_reply.protocol = handle->timeout_ipv4_next_key.protocol;

			rc = bpf_map_lookup_elem(handle->ipv4_fd, &ipv4_key_reply, &ipv4_entry_reply);
			if (rc < 0) {
				cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
					__func__, __LINE__, errno, strerror(errno));
				count++;
				continue;
			}

			count++;
			bidir_flag = ipv4_entry_reply.last_timer != UDP_REPLY_TIMER_INF;

			if (ipv4_entry.active || ipv4_entry_reply.active) {
				// At least one of the directions saw packets, update the required entries and continue
				if (ipv4_entry.active) {
					ipv4_entry.active = 0;
					ipv4_entry.last_timer = current_timer;

					rc = bpf_map_update_elem(handle->ipv4_fd, &handle->timeout_ipv4_next_key, &ipv4_entry, BPF_EXIST);
					if (rc < 0) {
						cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
								__func__, __LINE__, errno, strerror(errno));
						continue;
					}

				}
				if (ipv4_entry_reply.active && bidir_flag) {
					ipv4_entry_reply.active = 0;
					ipv4_entry_reply.last_timer = current_timer;

					rc = bpf_map_update_elem(handle->ipv4_fd, &ipv4_key_reply, &ipv4_entry_reply, BPF_EXIST);
					if (rc < 0) {
						cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
								__func__, __LINE__, errno, strerror(errno));
						continue;
					}

				}
			} else {
				unsigned int timeout;
				// No packets in the last interval, check the timer value
				if (bidir_flag)
					timeout =  current_timer - min(ipv4_entry.last_timer, ipv4_entry_reply.last_timer);
				else
					timeout = current_timer - ipv4_entry.last_timer;

				if (timeout > get_timeout_value(ipv4_key_reply.protocol, bidir_flag)) {
					command.daddr = handle->timeout_ipv4_next_key.daddr;
					command.saddr = handle->timeout_ipv4_next_key.saddr;
					command.dport = handle->timeout_ipv4_next_key.dport;
					command.sport = handle->timeout_ipv4_next_key.sport;
					command.daddr_reply = ipv4_entry.nat_saddr;
					command.saddr_reply = ipv4_entry.nat_daddr;
					command.dport_reply = ipv4_entry.nat_sport;
					command.sport_reply = ipv4_entry.nat_dport;
					command.protocol = handle->timeout_ipv4_next_key.protocol;
					command.route_id = route_info[ipv4_entry.route_ifindex].id;
					command.flags = ipv4_entry.flags;
					command.action = FPP_ACTION_REMOVED;

					if (handle->event_cb)
						rc = handle->event_cb(FPP_CMD_IPV4_CONNTRACK_CHANGE, sizeof(fpp_ct_cmd_t), (unsigned short *)&command);
					else
						rc = CPAL_CB_STOP;

					bpf_map_delete_elem(handle->ipv4_fd, &handle->timeout_ipv4_next_key);
					bpf_map_delete_elem(handle->ipv4_fd, &ipv4_key_reply);

					if (rc <= CPAL_CB_STOP)
						goto exit;
				}
			}
		}
	}

exit:
	return rc;
}

static int check_ipv6_timeout(cpal_handle_t *handle)
{
	int count = 0, rc = 0, bidir_flag;
	struct ipv6_info ipv6_entry = {}, ipv6_entry_reply = {};
	struct ipv6_flow ipv6_key_reply = {};
	fpp_ct6_cmd_t command;

	memset(&handle->timeout_ipv6_next_key, 0, sizeof(struct ipv6_flow));

	while (count < (MAX_IPV6_ENTRIES >> CONNECTION_CHECK_RATIO_LOG)) {
		rc = bpf_map_get_next_key(handle->ipv6_fd, &handle->timeout_ipv6_next_key, &handle->timeout_ipv6_next_key);
		if (rc < 0) {
			// Not an error, we reached the end of the table, so we can exit
			if (errno == ENOENT) {
				rc = 0;
				memset(&handle->timeout_ipv6_next_key, 0, sizeof(struct ipv6_flow));
				goto exit;
			}
			cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
				__func__, __LINE__, errno, strerror(errno));
			continue;
		}

		rc = bpf_map_lookup_elem(handle->ipv6_fd, &handle->timeout_ipv6_next_key, &ipv6_entry);
		if (rc < 0) {
			cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
					__func__, __LINE__, errno, strerror(errno));
			continue;
		}

		if (ipv6_entry.flags & FP_INFO_FLAG_CONNTRACK_ORIG) {
			ipv6_addr_copy(ipv6_key_reply.saddr, ipv6_entry.nat_daddr);
			ipv6_addr_copy(ipv6_key_reply.daddr, ipv6_entry.nat_saddr);
			ipv6_key_reply.sport = ipv6_entry.nat_dport;
			ipv6_key_reply.dport = ipv6_entry.nat_sport;
			ipv6_key_reply.protocol = handle->timeout_ipv6_next_key.protocol;

			rc = bpf_map_lookup_elem(handle->ipv6_fd, &ipv6_key_reply, &ipv6_entry_reply);
			if (rc < 0) {
				cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
					__func__, __LINE__, errno, strerror(errno));
				continue;
			}

			count++;
			bidir_flag = ipv6_entry_reply.last_timer != UDP_REPLY_TIMER_INF;

			if (ipv6_entry.active || ipv6_entry_reply.active) {
				// At least one of the directions saw packets, update the required entries and continue
				if (ipv6_entry.active) {
					ipv6_entry.active = 0;
					ipv6_entry.last_timer = current_timer;

					rc = bpf_map_update_elem(handle->ipv6_fd, &handle->timeout_ipv6_next_key, &ipv6_entry, BPF_EXIST);
					if (rc < 0) {
						cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
								__func__, __LINE__, errno, strerror(errno));
						continue;
					}

				}
				if (ipv6_entry_reply.active && bidir_flag) {
					ipv6_entry_reply.active = 0;
					ipv6_entry_reply.last_timer = current_timer;

					rc = bpf_map_update_elem(handle->ipv6_fd, &ipv6_key_reply, &ipv6_entry_reply, BPF_EXIST);
					if (rc < 0) {
						cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during timeout check: %d(%s)\n",
								__func__, __LINE__, errno, strerror(errno));
						continue;
					}

				}
			} else {
				unsigned int timeout;
				// No packets in the last interval, check the timer value
				if (bidir_flag)
					timeout =  current_timer - min(ipv6_entry.last_timer, ipv6_entry_reply.last_timer);
				else
					timeout = current_timer - ipv6_entry.last_timer;

				if (timeout > get_timeout_value(ipv6_key_reply.protocol, bidir_flag)) {
					ipv6_addr_copy(command.daddr, handle->timeout_ipv6_next_key.daddr);
					ipv6_addr_copy(command.saddr, handle->timeout_ipv6_next_key.saddr);
					command.dport = handle->timeout_ipv6_next_key.dport;
					command.sport = handle->timeout_ipv6_next_key.sport;
					ipv6_addr_copy(command.daddr_reply, ipv6_entry.nat_saddr);
					ipv6_addr_copy(command.saddr_reply, ipv6_entry.nat_daddr);
					command.dport_reply = ipv6_entry.nat_sport;
					command.sport_reply = ipv6_entry.nat_dport;
					command.protocol = handle->timeout_ipv6_next_key.protocol;
					command.route_id = route_info[ipv6_entry.route_ifindex].id;
					command.flags = ipv6_entry.flags;
					command.action = FPP_ACTION_REMOVED;

					if (handle->event_cb)
						rc = handle->event_cb(FPP_CMD_IPV6_CONNTRACK_CHANGE, sizeof(fpp_ct6_cmd_t), (unsigned short *)&command);
					else
						rc = CPAL_CB_STOP;

					bpf_map_delete_elem(handle->ipv6_fd, &handle->timeout_ipv6_next_key);
					bpf_map_delete_elem(handle->ipv6_fd, &ipv6_key_reply);

					if (rc <= CPAL_CB_STOP)
						goto exit;
				}
			}
		}
	}

exit:
	return rc;
}

static int reset_ipv4_timeouts(cpal_handle_t *handle)
{
	int count, rc = 0;
	struct ipv4_info ipv4_entry = {}, ipv4_entry_reply = {};
	struct ipv4_flow ipv4_key_reply = {};

	memset(&handle->timeout_ipv4_next_key, 0, sizeof(struct ipv4_flow));
	for (count = 0; count < MAX_IPV4_ENTRIES; count++) {
		rc = bpf_map_get_next_key(handle->ipv4_fd, &handle->timeout_ipv4_next_key,
				&handle->timeout_ipv4_next_key);
		if (rc < 0) {
			if (errno == ENOENT) {
				rc = 0;
				memset(&handle->timeout_ipv4_next_key, 0, sizeof(struct ipv4_flow));
				goto exit;
			}
			count++;
			continue;
		}

		rc = bpf_map_lookup_elem(handle->ipv4_fd, &handle->timeout_ipv4_next_key,
				&ipv4_entry);
		if (rc < 0) {
			count++;
			continue;
		}

		if (ipv4_entry.flags & FP_INFO_FLAG_CONNTRACK_ORIG) {
			ipv4_key_reply.saddr = ipv4_entry.nat_daddr;
			ipv4_key_reply.daddr = ipv4_entry.nat_saddr;
			ipv4_key_reply.sport = ipv4_entry.nat_dport;
			ipv4_key_reply.dport = ipv4_entry.nat_sport;
			ipv4_key_reply.protocol = handle->timeout_ipv4_next_key.protocol;

			rc = bpf_map_lookup_elem(handle->ipv4_fd, &ipv4_key_reply, &ipv4_entry_reply);
			if (rc < 0) {
				count++;
				continue;
			}

			ipv4_entry.active = 0;
			ipv4_entry.last_timer = current_timer;
			rc = bpf_map_update_elem(handle->ipv4_fd, &handle->timeout_ipv4_next_key,
					&ipv4_entry, BPF_EXIST);
			if (rc < 0) {
				count++;
				continue;
			}

			if (ipv4_entry_reply.last_timer != UDP_REPLY_TIMER_INF) {
				ipv4_entry_reply.active = 0;
				ipv4_entry_reply.last_timer = current_timer;
				rc = bpf_map_update_elem(handle->ipv4_fd, &ipv4_key_reply,
						&ipv4_entry_reply, BPF_EXIST);
				if (rc < 0) {
					count++;
					continue;
				}
			}
		}
	}

exit:
	return rc;
}

static int reset_ipv6_timeouts(cpal_handle_t *handle)
{
	int count, rc = 0;
	struct ipv6_info ipv6_entry = {}, ipv6_entry_reply = {};
	struct ipv6_flow ipv6_key_reply = {};

	memset(&handle->timeout_ipv6_next_key, 0, sizeof(struct ipv6_flow));
	for (count = 0; count < MAX_IPV6_ENTRIES; count++) {
		rc = bpf_map_get_next_key(handle->ipv6_fd, &handle->timeout_ipv6_next_key,
				&handle->timeout_ipv6_next_key);
		if (rc < 0) {
			if (errno == ENOENT) {
				rc = 0;
				memset(&handle->timeout_ipv6_next_key, 0, sizeof(struct ipv6_flow));
				goto exit;
			}
			count++;
			continue;
		}

		rc = bpf_map_lookup_elem(handle->ipv6_fd, &handle->timeout_ipv6_next_key,
				&ipv6_entry);
		if (rc < 0) {
			count++;
			continue;
		}

		if (ipv6_entry.flags & FP_INFO_FLAG_CONNTRACK_ORIG) {
			ipv6_addr_copy(ipv6_key_reply.saddr, ipv6_entry.nat_daddr);
			ipv6_addr_copy(ipv6_key_reply.daddr, ipv6_entry.nat_saddr);
			ipv6_key_reply.sport = ipv6_entry.nat_dport;
			ipv6_key_reply.dport = ipv6_entry.nat_sport;
			ipv6_key_reply.protocol = handle->timeout_ipv6_next_key.protocol;

			rc = bpf_map_lookup_elem(handle->ipv6_fd, &ipv6_key_reply, &ipv6_entry_reply);
			if (rc < 0) {
				count++;
				continue;
			}

			ipv6_entry.active = 0;
			ipv6_entry.last_timer = current_timer;
			rc = bpf_map_update_elem(handle->ipv6_fd, &handle->timeout_ipv6_next_key,
					&ipv6_entry, BPF_EXIST);
			if (rc < 0) {
				count++;
				continue;
			}

			if (ipv6_entry_reply.last_timer != UDP_REPLY_TIMER_INF) {
				ipv6_entry_reply.active = 0;
				ipv6_entry_reply.last_timer = current_timer;
				rc = bpf_map_update_elem(handle->ipv6_fd, &ipv6_key_reply,
						&ipv6_entry_reply, BPF_EXIST);
				if (rc < 0) {
					count++;
					continue;
				}
			}
		}
	}

exit:
	return rc;
}

int cpal_catch(cpal_handle_t *handle)
{
	int rc = 0;
	struct itimerspec timer_val = {
		.it_value = {
				.tv_sec = 0,
				.tv_nsec = CONNECTION_CHECK_INTERVAL
		}
	};

	current_timer++;

	if (ff_enable) {
		rc = check_ipv4_timeout(handle);
		if (rc < 0)
			goto exit;
		rc = check_ipv6_timeout(handle);
		if (rc < 0)
			goto exit;
	}

	rc = timerfd_settime(handle->ff_catch_fd, 0, &timer_val, NULL);
exit:
	return rc;
}


int cpal_close(cpal_handle_t *handle)
{
	if (!handle)
		return -1;

	if (handle->ipv4_fd > 0)
		close(handle->ipv4_fd);

	if (handle->ipv6_fd > 0)
		close(handle->ipv6_fd);

	if (handle->route_fd > 0)
		close(handle->route_fd);

	if (handle->ff_catch_fd > 0)
		close(handle->ff_catch_fd);

	free(handle);

	return 0;
}


static int bpf_error(int is_route, int errn)
{
	int rc;

	switch (errn) {
	case EINVAL:
		rc = FPP_ERR_WRONG_PARAM_VALUE;
		break;
	case ENOMEM:
	case E2BIG:
		rc = FPP_ERR_NOT_ENOUGH_MEMORY;
		break;
	case EEXIST:
		rc = is_route?FPP_ERR_RT_ENTRY_ALREADY_REGISTERED:FPP_ERR_CT_ENTRY_ALREADY_REGISTERED;
		break;
	case ENOENT:
		rc = is_route?FPP_ERR_RT_ENTRY_NOT_FOUND:FPP_ERR_CT_ENTRY_NOT_FOUND;
		break;
	default:
		rc = FPP_ERR_CREATION_FAILED;
		break;
	}

	return rc;
}

static int cpal_IPV4_CONNTRACK(cpal_handle_t *handle, fpp_ct_cmd_t *cmd_buf, unsigned short *rep_buf, unsigned short *rep_len)
{
	int rc_orig = 0, rc_reply = 0;
	struct ipv4_flow key_orig = {}, key_reply = {};
	struct ipv4_info entry_orig = {}, entry_reply = {};
	fpp_ct_ex_cmd_t *response;

	key_orig.saddr = cmd_buf->saddr;
	key_orig.daddr = cmd_buf->daddr;
	key_orig.sport = cmd_buf->sport;
	key_orig.dport = cmd_buf->dport;
	key_orig.protocol = cmd_buf->protocol;

	key_reply.saddr = cmd_buf->saddr_reply;
	key_reply.daddr = cmd_buf->daddr_reply;
	key_reply.sport = cmd_buf->sport_reply;
	key_reply.dport = cmd_buf->dport_reply;
	key_reply.protocol = cmd_buf->protocol;


	cmm_print(DEBUG_INFO, "BPF: %s: action=%u\n", __func__, cmd_buf->action);
	switch(cmd_buf->action)
	{
	case FPP_ACTION_DEREGISTER:
		//FIXME Do we need a refcount for routes?
		cmm_print(DEBUG_INFO, "BPF: In connection de-register\n");
		rc_orig = bpf_map_lookup_elem(handle->ipv4_fd, &key_orig, &entry_orig);
		rc_reply = bpf_map_lookup_elem(handle->ipv4_fd, &key_reply, &entry_reply);

		if ((rc_orig < 0) || (rc_reply < 0) || ((entry_orig.flags & FP_INFO_FLAG_CONNTRACK_ORIG) != FP_INFO_FLAG_CONNTRACK_ORIG)) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_DEREGISTER): ct entry not found (%d %d)\n", __func__, rc_orig, rc_reply);
			return FPP_ERR_CT_ENTRY_NOT_FOUND;
		}

		if (rc_orig == 0)
			bpf_map_delete_elem(handle->ipv4_fd, &key_orig);
		if (rc_reply == 0)
			bpf_map_delete_elem(handle->ipv4_fd, &key_reply);

		break;

	case FPP_ACTION_REGISTER:
		cmm_print(DEBUG_INFO, "BPF: In connection register\n");
		rc_orig = bpf_map_lookup_elem(handle->ipv4_fd, &key_orig, &entry_orig);
		rc_reply = bpf_map_lookup_elem(handle->ipv4_fd, &key_reply, &entry_reply);

		if ((rc_orig == 0) && (rc_reply == 0) && ((entry_orig.flags & FP_INFO_FLAG_CONNTRACK_ORIG) != FP_INFO_FLAG_CONNTRACK_ORIG)) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): ct entry already exist (%d %d)\n", __func__, rc_orig, rc_reply);
			return FPP_ERR_CREATION_FAILED; // Reverse entry already exists
		}

		if ((rc_orig == 0) || (rc_reply == 0)) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): trying to add exactly the same ct entry (%d %d)\n", __func__, rc_orig, rc_reply);
			return FPP_ERR_CT_ENTRY_ALREADY_REGISTERED; // Trying to add exactly the same conntrack
		}

		entry_orig.nat_saddr = cmd_buf->daddr_reply;
		entry_orig.nat_daddr = cmd_buf->saddr_reply;
		entry_orig.nat_sport = cmd_buf->dport_reply;
		entry_orig.nat_dport = cmd_buf->sport_reply;
		entry_orig.flags = FP_INFO_FLAG_CONNTRACK_ORIG;
		entry_orig.last_timer = current_timer;
		entry_orig.last_time_ns = entry_orig.rate_limit = entry_orig.bytes_count = 0;

		entry_orig.route_ifindex = find_route_id(cmd_buf->route_id);
		if (entry_orig.route_ifindex < 0)
			cmm_print(DEBUG_WARNING, "%s(FPP_ACTION_REGISTER): invalid route for origin entry\n", __func__);
		else
			entry_orig.mtu = route_info[entry_orig.route_ifindex].mtu;

		entry_reply.nat_saddr = cmd_buf->daddr;
		entry_reply.nat_daddr = cmd_buf->saddr;
		entry_reply.nat_sport = cmd_buf->dport;
		entry_reply.nat_dport = cmd_buf->sport;
		entry_reply.flags = 0;
		entry_reply.last_time_ns = entry_reply.rate_limit = entry_reply.bytes_count = 0;
		if (key_reply.protocol == IPPROTO_UDP)
			entry_reply.last_timer = UDP_REPLY_TIMER_INF;
		else
			entry_reply.last_timer = current_timer;

		if (cmd_buf->flags & CTCMD_FLAGS_REP_DISABLED)
			entry_reply.route_ifindex = -1;
		else
			entry_reply.route_ifindex = find_route_id(cmd_buf->route_id_reply);
		if (entry_reply.route_ifindex < 0)
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): invalid route %d for reply entry\n", __func__,
					entry_reply.route_ifindex);
		else {
			entry_reply.mtu = route_info[entry_reply.route_ifindex].mtu;
			cmm_print(DEBUG_INFO, "BPF: conn route idx= %d, route ridx = %d and mtu = %d\n",
					entry_orig.route_ifindex, entry_reply.route_ifindex,
					route_info[entry_reply.route_ifindex].mtu);
		}
		set_ipv4_checksum_correction(&key_orig, &entry_orig);
		set_ipv4_checksum_correction(&key_reply, &entry_reply);

		rc_orig = bpf_map_update_elem(handle->ipv4_fd, &key_orig, &entry_orig, BPF_NOEXIST);
		if (rc_orig == 0) {
			rc_reply = bpf_map_update_elem(handle->ipv4_fd, &key_reply, &entry_reply, BPF_NOEXIST);
			if (rc_reply < 0) {
				cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): error updating ct reply entry\n", __func__);
				bpf_map_delete_elem(handle->ipv4_fd, &key_orig);
			}
		}

		if ((rc_orig < 0) || (rc_reply < 0)) {
			int errn = errno;
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): Error encountered: %d(%s) (%d %d)\n",
					__func__, errno, strerror(errno), rc_orig, rc_reply);
			return bpf_error(0, errn);
		}
		cmm_print(DEBUG_INFO, "BPF: Conn. add success %d\n", rc_orig);

		break;

	case FPP_ACTION_UPDATE:
		rc_orig = bpf_map_lookup_elem(handle->ipv4_fd, &key_orig, &entry_orig);
		rc_reply = bpf_map_lookup_elem(handle->ipv4_fd, &key_reply, &entry_reply);

		if (rc_orig < 0 || rc_reply < 0 || !(entry_orig.flags & FP_INFO_FLAG_CONNTRACK_ORIG)) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_UPDATE): ct entry not found (%d %d)\n", __func__, rc_orig, rc_reply);
			return FPP_ERR_CT_ENTRY_NOT_FOUND;
		}

		entry_orig.route_ifindex = find_route_id(cmd_buf->route_id);
		if (entry_orig.route_ifindex < 0)
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_UPDATE): invalid route for origin entry\n", __func__);
		else
			entry_orig.mtu = route_info[entry_orig.route_ifindex].mtu;

		if (cmd_buf->flags & CTCMD_FLAGS_REP_DISABLED)
			entry_reply.route_ifindex = -1;
		else
			entry_reply.route_ifindex = find_route_id(cmd_buf->route_id_reply);
		if (entry_reply.route_ifindex < 0)
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_UPDATE): invalid route for reply entry\n", __func__);
		else
			entry_reply.mtu = route_info[entry_reply.route_ifindex].mtu;

		rc_orig = bpf_map_update_elem(handle->ipv4_fd, &key_orig, &entry_orig, BPF_ANY);
		if (rc_orig == 0)
			rc_reply = bpf_map_update_elem(handle->ipv4_fd, &key_reply, &entry_reply, BPF_ANY);

		if (rc_orig < 0 || rc_reply < 0) {
			int errn = errno;
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_UPDATE): Error encountered: %d(%s) (%d %d)\n",
					__func__, errno, strerror(errno), rc_orig, rc_reply);
			return bpf_error(0, errn);
		}

		break;

	case FPP_ACTION_QUERY:
		memset(&handle->query_ipv4_next_key, 0, sizeof(struct ipv4_flow));
		/* fall-through */
	case FPP_ACTION_QUERY_CONT:
		if (rep_len == NULL)
			return FPP_ERR_WRONG_COMMAND_SIZE;

		if ((*rep_len < sizeof(fpp_ct_ex_cmd_t)) || (rep_buf == NULL))
			return FPP_ERR_WRONG_COMMAND_SIZE;

		response = (fpp_ct_ex_cmd_t *)rep_buf;
		*rep_len = sizeof(unsigned short);

		rc_orig = bpf_map_get_next_key(handle->ipv4_fd, &handle->query_ipv4_next_key, &handle->query_ipv4_next_key);
		if (rc_orig < 0) {
			*rep_buf = bpf_error(0, errno);
			goto exit;
		}

		rc_orig = bpf_map_lookup_elem(handle->ipv4_fd, &handle->query_ipv4_next_key, &entry_orig);
		if (rc_orig < 0) {
			int errn = errno;
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_QUERY) Error encountered: %d(%s)\n",
					__func__, errno, strerror(errno));
			*rep_buf = bpf_error(0, errn);
			goto exit;
		}

		response->daddr = handle->query_ipv4_next_key.daddr;
		response->saddr = handle->query_ipv4_next_key.saddr;
		response->sport = handle->query_ipv4_next_key.sport;
		response->dport = handle->query_ipv4_next_key.dport;
		response->daddr_reply = entry_orig.nat_saddr;
		response->saddr_reply = entry_orig.nat_daddr;
		response->dport_reply = entry_orig.nat_sport;
		response->sport_reply = entry_orig.nat_dport;
		response->protocol = handle->query_ipv4_next_key.protocol;
		response->route_id = route_info[entry_orig.route_ifindex].id;
		response->flags = entry_orig.flags;
		response->action = cmd_buf->action;
		*rep_len += sizeof(fpp_ct_ex_cmd_t);

exit:
		return *rep_buf;
		break;
	default:
		return FPP_ERR_UNKNOWN_ACTION;
		break;
	}
	return 0;
}

static int cpal_IPV6_CONNTRACK(cpal_handle_t *handle, fpp_ct6_cmd_t *cmd_buf, unsigned short *rep_buf, unsigned short *rep_len)
{
	int rc_orig = 0, rc_reply = 0;
	struct ipv6_flow key_orig = {}, key_reply = {};
	struct ipv6_info entry_orig = {}, entry_reply = {};
	fpp_ct6_ex_cmd_t *response;

	ipv6_addr_copy(key_orig.saddr, cmd_buf->saddr);
	ipv6_addr_copy(key_orig.daddr, cmd_buf->daddr);
	key_orig.sport = cmd_buf->sport;
	key_orig.dport = cmd_buf->dport;
	key_orig.protocol = cmd_buf->protocol;

	ipv6_addr_copy(key_reply.saddr, cmd_buf->saddr_reply);
	ipv6_addr_copy(key_reply.daddr, cmd_buf->daddr_reply);
	key_reply.sport = cmd_buf->sport_reply;
	key_reply.dport = cmd_buf->dport_reply;
	key_reply.protocol = cmd_buf->protocol;

	cmm_print(DEBUG_INFO, "%s: action=%u\n", __func__, cmd_buf->action);
	switch(cmd_buf->action)
	{
	case FPP_ACTION_DEREGISTER:
		rc_orig = bpf_map_lookup_elem(handle->ipv6_fd, &key_orig, &entry_orig);
		rc_reply = bpf_map_lookup_elem(handle->ipv6_fd, &key_reply, &entry_reply);

		if (rc_orig < 0 || rc_reply < 0 || !(entry_orig.flags & FP_INFO_FLAG_CONNTRACK_ORIG)) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_DEREGISTER): ct entry not found (%d %d)\n", __func__, rc_orig, rc_reply);
			return FPP_ERR_CT_ENTRY_NOT_FOUND;
		}

		if (rc_orig == 0)
			bpf_map_delete_elem(handle->ipv6_fd, &key_orig);
		if (rc_reply == 0)
			bpf_map_delete_elem(handle->ipv6_fd, &key_reply);

		break;

	case FPP_ACTION_REGISTER:
		rc_orig = bpf_map_lookup_elem(handle->ipv6_fd, &key_orig, &entry_orig);
		rc_reply = bpf_map_lookup_elem(handle->ipv6_fd, &key_reply, &entry_reply);

		if (rc_orig == 0 && rc_reply == 0 && !(entry_orig.flags & FP_INFO_FLAG_CONNTRACK_ORIG)) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): ct entry already exist (%d %d)\n", __func__, rc_orig, rc_reply);
			return FPP_ERR_CREATION_FAILED;
		}

		if (rc_orig == 0 || rc_reply == 0) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): trying to add exactly the same ct entry (%d %d)\n", __func__, rc_orig, rc_reply);
			return FPP_ERR_CT_ENTRY_ALREADY_REGISTERED;
		}

		ipv6_addr_copy(entry_orig.nat_saddr, cmd_buf->daddr_reply);
		ipv6_addr_copy(entry_orig.nat_daddr, cmd_buf->saddr_reply);
		entry_orig.nat_sport = cmd_buf->dport_reply;
		entry_orig.nat_dport = cmd_buf->sport_reply;
		entry_orig.flags = FP_INFO_FLAG_CONNTRACK_ORIG;
		entry_orig.last_timer = current_timer;
		entry_orig.last_time_ns = entry_orig.rate_limit = entry_orig.bytes_count = 0;

		entry_orig.route_ifindex = find_route_id(cmd_buf->route_id);
		if (entry_orig.route_ifindex < 0)
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): invalid route for origin entry\n", __func__);
		else
			entry_orig.mtu = route_info[entry_orig.route_ifindex].mtu;

		ipv6_addr_copy(entry_reply.nat_saddr, cmd_buf->daddr);
		ipv6_addr_copy(entry_reply.nat_daddr, cmd_buf->saddr);
		entry_reply.nat_sport = cmd_buf->dport;
		entry_reply.nat_dport = cmd_buf->sport;
		entry_reply.flags = 0;
		entry_reply.last_time_ns = entry_reply.rate_limit = entry_reply.bytes_count = 0;
		if (key_reply.protocol == IPPROTO_UDP)
			entry_reply.last_timer = UDP_REPLY_TIMER_INF;
		else
			entry_reply.last_timer = current_timer;

		if (cmd_buf->flags & CTCMD_FLAGS_REP_DISABLED)
			entry_reply.route_ifindex = -1;
		else
			entry_reply.route_ifindex = find_route_id(cmd_buf->route_id_reply);
		if (entry_reply.route_ifindex < 0)
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): invalid route for reply entry\n", __func__);
		else
			entry_reply.mtu = route_info[entry_reply.route_ifindex].mtu;

		set_ipv6_checksum_correction(&key_orig, &entry_orig);
		set_ipv6_checksum_correction(&key_reply, &entry_reply);

		rc_orig = bpf_map_update_elem(handle->ipv6_fd, &key_orig, &entry_orig, BPF_NOEXIST);
		if (rc_orig == 0) {
			rc_reply = bpf_map_update_elem(handle->ipv6_fd, &key_reply, &entry_reply, BPF_NOEXIST);
			if (rc_reply < 0) {
				cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): error updating ct reply entry\n", __func__);
				bpf_map_delete_elem(handle->ipv6_fd, &key_orig);
			}
		}

		if (rc_orig < 0 || rc_reply < 0) {
			int errn = errno;
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_REGISTER): Error encountered: %d(%s) (%d %d)\n",
					__func__, errno, strerror(errno), rc_orig, rc_reply);
			return bpf_error(0, errn);
		}

		break;

	case FPP_ACTION_UPDATE:
		rc_orig = bpf_map_lookup_elem(handle->ipv6_fd, &key_orig, &entry_orig);
		rc_reply = bpf_map_lookup_elem(handle->ipv6_fd, &key_reply, &entry_reply);

		if (rc_orig < 0 || rc_reply < 0 || !(entry_orig.flags & FP_INFO_FLAG_CONNTRACK_ORIG)) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_UPDATE): ct entry not found (%d %d)\n", __func__, rc_orig, rc_reply);
			return FPP_ERR_CT_ENTRY_NOT_FOUND;
		}

		entry_orig.route_ifindex = find_route_id(cmd_buf->route_id);
		if (entry_orig.route_ifindex < 0)
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_UPDATE): invalid route for origin entry\n", __func__);
		else
			entry_orig.mtu = route_info[entry_orig.route_ifindex].mtu;

		if (cmd_buf->flags & CTCMD_FLAGS_REP_DISABLED)
			entry_reply.route_ifindex = -1;
		else
			entry_reply.route_ifindex = find_route_id(cmd_buf->route_id_reply);
		if (entry_reply.route_ifindex < 0)
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_UPDATE): invalid route for reply entry\n", __func__);
		else
			entry_reply.mtu = route_info[entry_reply.route_ifindex].mtu;

		rc_orig = bpf_map_update_elem(handle->ipv6_fd, &key_orig, &entry_orig, BPF_ANY);
		if (rc_orig == 0)
			rc_reply = bpf_map_update_elem(handle->ipv6_fd, &key_reply, &entry_reply, BPF_ANY);

		if (rc_orig < 0 || rc_reply < 0) {
			int errn = errno;
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_UPDATE): Error encountered: %d(%s) (%d %d)\n",
					__func__, errno, strerror(errno), rc_orig, rc_reply);
			return bpf_error(0, errn);
		}

		break;

	case FPP_ACTION_QUERY:
		memset(&handle->query_ipv6_next_key, 0, sizeof(struct ipv6_flow));
		/* fall-through */
	case FPP_ACTION_QUERY_CONT:
		if (rep_len == NULL)
			return FPP_ERR_WRONG_COMMAND_SIZE;

		if ((*rep_len < sizeof(fpp_ct6_ex_cmd_t)) || (rep_buf == NULL))
			return FPP_ERR_WRONG_COMMAND_SIZE;

		response = (fpp_ct6_ex_cmd_t *)rep_buf;
		*rep_len = sizeof(unsigned short);

		rc_orig = bpf_map_get_next_key(handle->ipv6_fd, &handle->query_ipv6_next_key, &handle->query_ipv6_next_key);
		if (rc_orig < 0) {
			cmm_print(DEBUG_ERROR, "%s(FPP_ACTION_QUERY) Error encountered: %d(%s)\n",
				__func__, errno, strerror(errno));
			*rep_buf = bpf_error(0, errno);
			goto exit;
		}

		ipv6_addr_copy(response->daddr, handle->query_ipv6_next_key.daddr);
		ipv6_addr_copy(response->saddr, handle->query_ipv6_next_key.saddr);
		response->sport = handle->query_ipv6_next_key.sport;
		response->dport = handle->query_ipv6_next_key.dport;
		ipv6_addr_copy(response->daddr_reply, entry_orig.nat_saddr);
		ipv6_addr_copy(response->saddr_reply, entry_orig.nat_daddr);
		response->dport_reply = entry_orig.nat_sport;
		response->sport_reply = entry_orig.nat_dport;
		response->protocol = handle->query_ipv6_next_key.protocol;
		response->route_id = route_info[entry_orig.route_ifindex].id;
		response->flags = entry_orig.flags;
		response->action = cmd_buf->action;
		*rep_len += sizeof(fpp_ct6_ex_cmd_t);

exit:
		return *rep_buf;
		break;

	default:
		return FPP_ERR_UNKNOWN_ACTION;
		break;
	}

	return 0;
}

static int cpal_IP_ROUTE(cpal_handle_t *handle, fpp_rt_cmd_t *cmd_buf, unsigned short *rep_buf, unsigned short *rep_len)
{
	int rc = 0;
	int route_key = 0;
	struct route route_entry, route_zero = {0};
	struct interface *itf;
	fpp_rt_cmd_t *response;

	switch(cmd_buf->action)
	{
	case FPP_ACTION_DEREGISTER:

		route_key = find_route_id(cmd_buf->id);
		if (route_key < 0)
			return FPP_ERR_RT_ENTRY_NOT_FOUND;

		memset(&route_entry, 0, sizeof(struct route));
		rc = bpf_map_update_elem(handle->route_fd, &route_key, &route_entry, BPF_ANY);
		if (rc < 0)
			return bpf_error(1, errno);
		route_info[route_key].id = 0;
		route_info[route_key].mtu = 0;
		break;

	case FPP_ACTION_REGISTER:
	case FPP_ACTION_UPDATE:

		cmm_print(DEBUG_INFO, "BPF: In Route register/update\n");
		route_key = hash_route_id(cmd_buf->id);
		if (route_key < 0)
			return FPP_ERR_NOT_ENOUGH_MEMORY;

		rc = bpf_map_lookup_elem(handle->route_fd, &route_key, &route_entry);

		if (rc < 0)
			return bpf_error(1, errno);

		if ((cmd_buf->action == FPP_ACTION_REGISTER) && (route_entry.redir_ifindex != 0))
			return FPP_ERR_RT_ENTRY_ALREADY_REGISTERED;

		if ((cmd_buf->action == FPP_ACTION_UPDATE) && (route_entry.redir_ifindex == 0))
			return FPP_ERR_RT_ENTRY_NOT_FOUND;

		itf = __itf_find(if_nametoindex(cmd_buf->output_device));
		if (!itf)
			return FPP_ERR_UNKNOWN_INTERFACE;

		if (itf->type != ARPHRD_RAWIP) {
			rc = __itf_get_macaddr(itf, route_entry.l2_hdr + ETH_ALEN);
			if (rc < 0)
				return FPP_ERR_UNKNOWN_INTERFACE;

			memcpy(route_entry.l2_hdr, cmd_buf->dst_mac, ETH_ALEN);
			route_entry.l2_hdr_size = 2 * ETH_ALEN;

			if (__itf_is_vlan(itf)) {
				uint16_t *vlan_tpid = (uint16_t *)&route_entry.l2_hdr[2 * ETH_ALEN];
				uint16_t *vlan_tci = vlan_tpid + 1;

				*vlan_tpid = htons(ETH_P_8021Q);
				*vlan_tci = htons(itf->vlan_id);
				route_entry.l2_hdr_size += 2 * sizeof(uint16_t);
				cmm_print(DEBUG_INFO, "%s: IP_ROUTE(%u) VLAN itf:%u tpid:%0x tci:%u\n",
						__func__, cmd_buf->id, itf->ifindex, ETH_P_8021Q, itf->vlan_id);
			}

			route_entry.redir_ifindex = itf->phys_ifindex;
		}
		else {
			route_entry.redir_ifindex = itf->ifindex;
		}

		route_entry.redir_if_type = itf->type;
		route_entry.flags = 0;
		route_entry.mtu = cmd_buf->mtu;

		cmm_print(DEBUG_INFO, "%s: IP_ROUTE(%u) map element update redir_ifindex:%u itf_type:%u mtu:%u\n",
				__func__, cmd_buf->id, route_entry.redir_ifindex,
				route_entry.redir_if_type, route_entry.mtu);

		rc = bpf_map_update_elem(handle->route_fd, &route_key, &route_entry, BPF_ANY);
		if (rc < 0) {
			int errn = errno;
			cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during ROUTE REGISTER/UPDATE: %d(%s)\n",
					__func__, __LINE__,errno, strerror(errno));
			return bpf_error(1, errn);
		}

		route_info[route_key].id = cmd_buf->id;
		route_info[route_key].mtu = cmd_buf->mtu;
		break;

	case FPP_ACTION_QUERY:
		handle->route_key = 0;
		/* fall-through */
	case FPP_ACTION_QUERY_CONT:
		if (rep_len == NULL)
			return FPP_ERR_WRONG_COMMAND_SIZE;

		if ((*rep_len < sizeof(fpp_rt_cmd_t)) || (rep_buf == NULL))
			return FPP_ERR_WRONG_COMMAND_SIZE;

		response = (fpp_rt_cmd_t *)rep_buf;
		*rep_len = sizeof(unsigned short);

again:
		if (handle->route_key >= MAX_FP_ROUTES) {
			*rep_buf = FPP_ERR_RT_ENTRY_NOT_FOUND;
			goto exit;
		}

		rc = bpf_map_lookup_elem(handle->route_fd, &handle->route_key, &route_entry);
		if (rc < 0) {
			int errn = errno;
			cmm_print(DEBUG_ERROR, "%s(%d) Error encountered during ROUTE QUERY: %d(%s)\n",
					__func__, __LINE__,errno, strerror(errno));
			*rep_buf = bpf_error(1, errn);
			goto exit;
		}

		response->id = route_info[handle->route_key].id;
		handle->route_key++;
		if (!memcmp(&route_entry, &route_zero, sizeof(struct route)))
			goto again;

		rc = __itf_get_name(route_entry.redir_ifindex, response->output_device, IFNAMSIZ);
		if (rc < 0)
			return FPP_ERR_UNKNOWN_INTERFACE;

		response->mtu = route_entry.mtu;
		memcpy(response->dst_mac, route_entry.l2_hdr, 6);
		response->flags = route_entry.flags;
		response->action = cmd_buf->action;
		*rep_len += sizeof(fpp_rt_cmd_t);
exit:
		return *rep_buf;
		break;

	default:
		return FPP_ERR_UNKNOWN_ACTION;
		break;
	}


	return 0;
}

static int cpal_ROUTE_RESET(cpal_handle_t *handle)
{
	int key, next_key = 0, rc;
	struct route route_entry = {0};

	do {
		rc = bpf_map_update_elem(handle->route_fd, &next_key, &route_entry, BPF_ANY);
		if (rc < 0)
			return bpf_error(1, errno);
		key = next_key;
	} while (bpf_map_get_next_key(handle->route_fd, &key, &next_key) == 0);

	return FPP_ERR_OK;
}

static int cpal_IPV6_RESET(cpal_handle_t *handle)
{
	struct ipv6_flow key = {}, next_key;

	while (bpf_map_get_next_key(handle->ipv6_fd, &key, &next_key) == 0) {
		bpf_map_delete_elem(handle->ipv6_fd, &next_key);
		key = next_key;
	}
	return FPP_ERR_OK;
}

static int cpal_IPV4_RESET(cpal_handle_t *handle)
{
	struct ipv4_flow key = {}, next_key;
	int ret = FPP_ERR_OK;

	while (bpf_map_get_next_key(handle->ipv4_fd, &key, &next_key) == 0) {
		bpf_map_delete_elem(handle->ipv4_fd, &next_key);
		key = next_key;
	}

	ret = cpal_IPV6_RESET(handle);
	if (ret != FPP_ERR_OK)
		goto exit;
	ret = cpal_ROUTE_RESET(handle);

exit:
	return ret;
}

static int cpal_IPV4_SET_TIMEOUT(cpal_handle_t *handle, fpp_timeout_cmd_t *cmd_buf)
{
	int rc = FPP_ERR_OK;

	if (cmd_buf->timeout_value1 >= UINT_MAX/CPAL_TICKS_PER_SECOND ||
			cmd_buf->timeout_value2 >= UINT_MAX/CPAL_TICKS_PER_SECOND)
		return FPP_ERR_PARAM_VALUE_OUT_OF_RANGE;

	switch (cmd_buf->protocol) {
	case IPPROTO_TCP:
		timeouts.tcp_timeout = cmd_buf->timeout_value1 * CPAL_TICKS_PER_SECOND;
		break;
	case IPPROTO_UDP:
		timeouts.udp_bidir_timeout =
			cmd_buf->timeout_value1 * CPAL_TICKS_PER_SECOND;
		timeouts.udp_unidir_timeout =
			((cmd_buf->timeout_value2) ? cmd_buf->timeout_value2 : cmd_buf->timeout_value1) * CPAL_TICKS_PER_SECOND;
		break;
	default:
		timeouts.other_proto_timeout = cmd_buf->timeout_value1 * CPAL_TICKS_PER_SECOND;
		break;
	}

	return rc;
}

static int cpal_IPV4_GET_TIMEOUT(cpal_handle_t *handle, fpp_ct_cmd_t *cmd_buf,
		unsigned short *rep_buf, unsigned short *rep_len)
{
	int rc, bidir_flag, timeout;
	unsigned int orig_last_timer, reply_last_timer;
	struct ipv4_flow key;
	struct ipv4_info entry;
	fpp_timeout_cmd_t *response;

	memset(&key, 0, sizeof(struct ipv4_flow));
	memset(&entry, 0, sizeof(struct ipv4_info));
	key.saddr = cmd_buf->saddr;
	key.daddr = cmd_buf->daddr;
	key.sport = cmd_buf->sport;
	key.dport = cmd_buf->dport;
	key.protocol = cmd_buf->protocol;

	rc = bpf_map_lookup_elem(handle->ipv4_fd, &key, &entry);
	if (rc < 0 || !(entry.flags & FP_INFO_FLAG_CONNTRACK_ORIG))
		return FPP_ERR_CT_ENTRY_NOT_FOUND;

	orig_last_timer = entry.last_timer;

	memset(&key, 0, sizeof(struct ipv4_flow));
	memset(&entry, 0, sizeof(struct ipv4_info));
	key.saddr = cmd_buf->saddr_reply;
	key.daddr = cmd_buf->daddr_reply;
	key.sport = cmd_buf->sport_reply;
	key.dport = cmd_buf->dport_reply;
	key.protocol = cmd_buf->protocol;

	rc = bpf_map_lookup_elem(handle->ipv4_fd, &key, &entry);
	if (rc < 0)
		return FPP_ERR_CT_ENTRY_NOT_FOUND;

	reply_last_timer = entry.last_timer;

	response = (fpp_timeout_cmd_t *)rep_buf;
	response->protocol = key.protocol;

	bidir_flag = reply_last_timer != UDP_REPLY_TIMER_INF;
	if (bidir_flag)
		timeout = current_timer - max(orig_last_timer, reply_last_timer);
	else
		timeout = current_timer - orig_last_timer;

	timeout = get_timeout_value(key.protocol, bidir_flag) - timeout;
	if (timeout < 0)
		timeout = 0;
	response->timeout_value1 = (unsigned int)timeout/CPAL_TICKS_PER_SECOND;
	*rep_len = sizeof(fpp_timeout_cmd_t);

	return FPP_ERR_OK;
}

static int cpal_IPV6_GET_TIMEOUT(cpal_handle_t *handle, fpp_ct6_cmd_t *cmd_buf,
		unsigned short *rep_buf, unsigned short *rep_len)
{
	int rc, bidir_flag, timeout;
	unsigned int orig_last_timer, reply_last_timer;
	struct ipv6_flow key;
	struct ipv6_info entry;
	fpp_timeout_cmd_t *response;

	memset(&key, 0, sizeof(struct ipv6_flow));
	memset(&entry, 0, sizeof(struct ipv6_info));
	ipv6_addr_copy(key.saddr, cmd_buf->saddr);
	ipv6_addr_copy(key.daddr, cmd_buf->daddr);
	key.sport = cmd_buf->sport;
	key.dport = cmd_buf->dport;
	key.protocol = cmd_buf->protocol;

	rc = bpf_map_lookup_elem(handle->ipv6_fd, &key, &entry);
	if (rc < 0 || !(entry.flags & FP_INFO_FLAG_CONNTRACK_ORIG))
		return FPP_ERR_CT_ENTRY_NOT_FOUND;

	orig_last_timer = entry.last_timer;

	memset(&key, 0, sizeof(struct ipv6_flow));
	memset(&entry, 0, sizeof(struct ipv6_info));
	ipv6_addr_copy(key.saddr, cmd_buf->saddr_reply);
	ipv6_addr_copy(key.daddr, cmd_buf->daddr_reply);
	key.sport = cmd_buf->sport_reply;
	key.dport = cmd_buf->dport_reply;
	key.protocol = cmd_buf->protocol;

	rc = bpf_map_lookup_elem(handle->ipv6_fd, &key, &entry);
	if (rc < 0)
		return FPP_ERR_CT_ENTRY_NOT_FOUND;

	reply_last_timer = entry.last_timer;

	response = (fpp_timeout_cmd_t *)rep_buf;
	response->protocol = key.protocol;

	bidir_flag = reply_last_timer != UDP_REPLY_TIMER_INF;
	if (bidir_flag)
		timeout = current_timer - max(orig_last_timer, reply_last_timer);
	else
		timeout = current_timer - orig_last_timer;

	timeout = get_timeout_value(key.protocol, bidir_flag) - timeout;
	if (timeout < 0)
		timeout = 0;
	response->timeout_value1 = (unsigned int)timeout/CPAL_TICKS_PER_SECOND;
	*rep_len = sizeof(fpp_timeout_cmd_t);

	return FPP_ERR_OK;
}

static int cpal_IPV4_FF_CONTROL(cpal_handle_t *handle, fpp_ff_ctrl_cmd_t *cmd_buf)
{
	int rc = 0, key_global = GLOB_FF_DISABLE, entry;

	if (cmd_buf->enable == 1) {
		ff_enable = 1;
		entry = 0;
		rc = bpf_map_update_elem(handle->globals_fd, &key_global, &entry, BPF_ANY);
		if (rc  < 0) {
			rc = bpf_error(0, errno);
			goto exit;
		}
		rc = reset_ipv4_timeouts(handle);
		if (rc < 0)
			goto exit;
		rc = reset_ipv6_timeouts(handle);
	} else if (cmd_buf->enable == 0) {
		ff_enable = 0;
		entry = 1;
		rc = bpf_map_update_elem(handle->globals_fd, &key_global, &entry, BPF_ANY);
		if (rc  < 0)
			rc = bpf_error(0, errno);
	} else
		return FPP_ERR_WRONG_COMMAND_PARAM;

exit:
	return rc;
}

int cpal_cmd(cpal_handle_t *handle, unsigned short fcode, unsigned short *cmd_buf, unsigned short cmd_len, unsigned short *rep_buf, unsigned short *rep_len)
{
	cmm_print(DEBUG_INFO, "BPF: cpal cmd code =%x\n", fcode);
	switch (fcode) {
	case FPP_CMD_IPV4_CONNTRACK:
		if ((cmd_len != sizeof(fpp_ct_cmd_t)) && (cmd_len != sizeof(fpp_ct_ex_cmd_t)))
			return FPP_ERR_WRONG_COMMAND_SIZE;
		return cpal_IPV4_CONNTRACK(handle, (fpp_ct_cmd_t *)cmd_buf, rep_buf, rep_len);
		break;

	case FPP_CMD_IPV6_CONNTRACK:
		if ((cmd_len != sizeof(fpp_ct6_cmd_t)) && (cmd_len != sizeof(fpp_ct6_ex_cmd_t)))
			return FPP_ERR_WRONG_COMMAND_SIZE;
		return cpal_IPV6_CONNTRACK(handle, (fpp_ct6_cmd_t *)cmd_buf, rep_buf, rep_len);
		break;

	case FPP_CMD_IP_ROUTE:
		if (cmd_len != sizeof(fpp_rt_cmd_t))
			return FPP_ERR_WRONG_COMMAND_SIZE;
		return cpal_IP_ROUTE(handle, (fpp_rt_cmd_t *)cmd_buf, rep_buf, rep_len);
		break;

	case FPP_CMD_IPV4_RESET:
		if (cmd_len)
			return FPP_ERR_WRONG_COMMAND_SIZE;
		if (cmd_buf)
			return FPP_ERR_WRONG_COMMAND_PARAM;
		return cpal_IPV4_RESET(handle);
		break;

	case FPP_CMD_IPV6_RESET:
		if (cmd_len)
			return FPP_ERR_WRONG_COMMAND_SIZE;
		if (cmd_buf)
			return FPP_ERR_WRONG_COMMAND_PARAM;
		return FPP_ERR_OK;
		break;

	case FPP_CMD_IPV4_SET_TIMEOUT:
		if (cmd_len != sizeof(fpp_timeout_cmd_t))
			return FPP_ERR_WRONG_COMMAND_SIZE;
		return cpal_IPV4_SET_TIMEOUT(handle, (fpp_timeout_cmd_t *)cmd_buf);
		break;

	case FPP_CMD_IPV4_FF_CONTROL:
		if (cmd_len != sizeof(fpp_ff_ctrl_cmd_t))
			return FPP_ERR_WRONG_COMMAND_SIZE;
		return cpal_IPV4_FF_CONTROL(handle, (fpp_ff_ctrl_cmd_t *)cmd_buf);
		break;

	default:
		break;
	}
	return 0;
}


int cpal_write(cpal_handle_t *handle, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf)
{
	unsigned short rep_buf[CPAL_MAX_PAYLOAD / sizeof(u_int16_t)] __attribute__ ((aligned (4)));
	unsigned short rep_len = sizeof(rep_buf);
	int rc;

	rep_buf[0] = 0;
	rc = cpal_cmd(handle, fcode, cmd_buf, cmd_len, rep_buf, &rep_len);
	if (rc)
		return rc;

	return rep_buf[0];
}


int cpal_query(cpal_handle_t *handle, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf, unsigned short *rep_len, unsigned short *rep_buf)
{
	switch (fcode) {
	case FPP_CMD_IPV4_GET_TIMEOUT:
		if (cmd_len != sizeof(fpp_ct_cmd_t))
			return FPP_ERR_WRONG_COMMAND_SIZE;
		return cpal_IPV4_GET_TIMEOUT(handle, (fpp_ct_cmd_t *)cmd_buf, rep_buf, rep_len);
		break;
	case FPP_CMD_IPV6_GET_TIMEOUT:
		if (cmd_len != sizeof(fpp_ct6_cmd_t))
			return FPP_ERR_WRONG_COMMAND_SIZE;
		return cpal_IPV6_GET_TIMEOUT(handle, (fpp_ct6_cmd_t *)cmd_buf, rep_buf, rep_len);
		break;
	default:
		break;
	}
	return 0;
}

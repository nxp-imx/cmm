/* Copyright 2019,2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 */

#ifndef __CPAL_BPF_H__
#define __CPAL_BPF_H__

#include "xdp_fp.h"

#define CONNECTION_CHECK_INTERVAL	500000000UL // 500ms, in ns
#define CONNECTION_CHECK_RATIO_LOG	2	// Check 1/2^2 of all connections at each iteration
#define CPAL_TICKS_PER_SECOND		(1000000000UL/CONNECTION_CHECK_INTERVAL)

#define UDP_REPLY_TIMER_INF	0xffffffffUL 	// Special "Infinite" value, to disable timeout checks on the reply direction of unidir connections

#define TCP_TIMEOUT         432000      /*5 days*/
#define UDP_UNIDIR_TIMEOUT      30      /*30s*/
#define UDP_BIDIR_TIMEOUT       180     /*180s*/
#define OTHER_PROTO_TIMEOUT     600     /*10 minutes*/

#define min(a, b) 		((a)<b?(a):(b))
#define max(a, b) 		((a)>b?(a):(b))

struct _cpal_timeouts_t {
	uint32_t tcp_timeout;
	uint32_t udp_bidir_timeout;
	uint32_t udp_unidir_timeout;
	uint32_t other_proto_timeout;
};
typedef struct _cpal_timeouts_t cpal_timeouts_t;

struct _cpal_handle_t {
	int ipv4_fd;
	int ipv6_fd;
	int route_fd;
	int globals_fd;
	int ff_catch_fd;
	int route_key;
	struct ipv6_flow query_ipv6_next_key;
	struct ipv4_flow query_ipv4_next_key;
	struct ipv4_flow timeout_ipv4_next_key;
	struct ipv6_flow timeout_ipv6_next_key;
	int (*event_cb)(unsigned short fcode, unsigned short len, unsigned short *payload);
};
typedef struct _cpal_handle_t cpal_handle_t;

struct _cpal_route_info_t {
	uint32_t id;
	uint16_t mtu;
};
typedef struct _cpal_route_info_t cpal_route_info_t;

/* CPAL callbacks return codes */
enum CPAL_CB_ACTION {
	CPAL_CB_STOP = 0,		/* stop catching event from CPAL */
	CPAL_CB_CONTINUE,	/* continue event catching */
};


cpal_handle_t *cpal_ff_open(void);
cpal_handle_t *cpal_ff_catch_open(void);
int cpal_close(cpal_handle_t *handle);
int cpal_catch(cpal_handle_t *handle);
int cpal_write(cpal_handle_t *handle, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf);
int cpal_cmd(cpal_handle_t *handle, unsigned short fcode, unsigned short *cmd_buf, unsigned short cmd_len, unsigned short *rep_buf, unsigned short *rep_len);
int cpal_query(cpal_handle_t *handle, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf, unsigned short *rep_len, unsigned short *rep_buf);

#ifndef IPSEC_SUPPORT_DISABLED
static inline cpal_handle_t *cpal_key_open(void)
{
	return 0;
}

static inline cpal_handle_t *cpal_key_catch_open(void)
{
	return 0;
}
#endif

static inline int cpal_fd(cpal_handle_t *handle)
{
	return handle->ff_catch_fd;
}

static inline int cpal_register_cb(cpal_handle_t *handle, int (*cb)(unsigned short fcode, unsigned short len, unsigned short *payload))
{
	int rc = -1;

	if (handle) {
		handle->event_cb = cb;
		rc = 0;
	}

	return rc;
}

#endif

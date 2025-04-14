/*
 *
 *  Copyright (C) 2007 Mindspeed Technologies, Inc.
 *  Copyright 2014-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017,2021,2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 *
 *
 */

#ifndef __MODULE_ROUTE_H__
#define __MODULE_ROUTE_H__

	void cmmRouteShowPrintHelp();
	int cmmRouteShowProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
	void cmmRouteSetPrintHelp();
	int cmmRouteSetProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
	int cmmRouteProcessClientCmd(cpal_handle_t* cpalMsgHandler, int function_code, u_int8_t *cmd_buf, u_int16_t *res_buf, u_int16_t *res_len);
	struct RtEntry *cmmPolicyRouting(unsigned int srcip, unsigned int dstip, unsigned short proto, unsigned short sport, unsigned short dport);

#endif


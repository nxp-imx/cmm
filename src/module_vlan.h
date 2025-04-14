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

#ifndef __MODULE_VLAN_H__
#define __MODULE_VLAN_H__

	#include "itf.h"

	void __cmmGetVlan(int fd, struct interface *itf);
	int cmmFeVLANUpdate(cpal_handle_t *cpal_handle, int request, struct interface *itf);
	void cmmVlanReset(cpal_handle_t *cpal_handle);
	int cmmVlanLocalShow(struct cli_def *cli, char *command, char *argv[], int argc);
	int cmmVlanCheckPolicy(struct interface *itf);

/* remote command processing */
	int vlanAddProcess(daemon_handle_t daemon_handle, int argc, char *argv[]);
	int vlanDeleteProcess(daemon_handle_t daemon_handle, int argc, char *argv[]);
	int cmmVlanClient(int argc, char **argv, int firstarg, daemon_handle_t daemon_handle);
	int cmmVlanProcessClientCmd(cpal_handle_t *cpal_handle, int function_code, u_int8_t *cmd_buf, u_int16_t cmd_len, u_int16_t *res_buf, u_int16_t *res_len);
	int cmmVlanQuery(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
#endif


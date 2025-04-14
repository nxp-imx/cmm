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

#ifndef __PPPOE_H__
#define __PPPOE_H__

#include "itf.h"

#define PPPOE_PATH "/proc/net/pppoe"

	int __cmmGetPPPoESession(FILE *fp, struct interface* ifp);

	int cmmFePPPoEUpdate(cpal_handle_t *cpal_handler, int action, struct interface *itf);
	int cmmPPPoELocalShow(struct cli_def * cli, char *command, char *argv[], int argc);
	int cmmPPPoEQueryProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);

#endif


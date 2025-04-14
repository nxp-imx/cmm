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

#ifndef __ALT_CONF_H__
#define __ALT_CONF_H__

int cmmAltConfClient(int argc, char **argv, int firstarg, daemon_handle_t daemon_handle);
int altconfResetProcess(daemon_handle_t daemon_handle);
int altconfSetProcess(daemon_handle_t daemon_handle, unsigned int option_id, unsigned int num_params, unsigned int *params);

#endif


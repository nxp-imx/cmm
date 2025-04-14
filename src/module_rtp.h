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

#ifndef __MODULE_RTP_H__
#define __MODULE_RTP_H__

int cmmRTPSetProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmRTPQueryProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmRTCPQueryProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);

/******************************** RTP Stats QoS Measurement **********************/

int cmmRTPStatsSetProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmRTPStatsQueryProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);

#endif

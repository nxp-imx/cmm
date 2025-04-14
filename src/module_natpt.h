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

#ifndef __MODULE_NATPT_H__
#define __MODULE_NATPT_H__

int cmmNATPTSetProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmNATPTQueryProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmNATPTQueryProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmNATPTOpenProcessClientCmd(cpal_handle_t* cpal_handle, u_int8_t *cmd_buf, u_int16_t cmd_len, u_int16_t *res_buf, u_int16_t *res_len);
#endif

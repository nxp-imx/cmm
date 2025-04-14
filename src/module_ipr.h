/*
 *
 *  Copyright 2018,2021,2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 *
 */

#ifndef __MODULE_IPR_H__
#define __MODULE_IPR_H__
#if defined(LS1043)
int cmmIpr4StatsQuery(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmIpr6StatsQuery(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmIprStatsProcessClientCmd(cpal_handle_t *cpal_handle, int function_code, u_int8_t *cmd_buf,
        u_int16_t cmd_len, u_int16_t *res_buf, u_int16_t *res_len);
#endif
#endif


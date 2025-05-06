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

#ifndef __MODULE_RELAY_H__
#define __MODULE_RELAY_H__

#include "cmm.h"

void cmmRelayShowPrintHelp();

int cmmRelayProcessClientCmd(cpal_handle_t * cpalMsgHandler, int function_code,
                             u_int8_t *cmd_buf, u_int16_t cmd_len, u_int16_t *res_buf, u_int16_t *res_len);

int cmmRelayLocalShow(struct cli_def *cli, const char *command, char *argv[],
                      int argc);
int cmmRelayParseCmd(int argc, char ** keywords, int tabStart, daemon_handle_t daemon_handle);
#endif

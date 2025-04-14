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

#ifndef __MODULE_MC6_H__
#define __MODULE_MC6_H__

	void cmmMc6ShowPrintHelp();
	int cmmMc6ShowProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
	int cmmMc6QueryProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
	void cmmMc6SetPrintHelp();
	int cmmMc6SetProcess(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
	int cmmMc6ProcessClientCmd(cpal_handle_t* cpalMsgHandler, int function_code, u_int8_t *cmd_buf, u_int16_t cmd_len, u_int16_t *res_buf, u_int16_t *res_len);
	int cmmMc6Show(struct cli_def * cli, char *command, char *argv[], int argc);

	extern  int parse_macaddr(char *pstring, unsigned char *pmacaddr);


#endif

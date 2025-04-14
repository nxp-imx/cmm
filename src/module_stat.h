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

#ifndef __MODULE_STAT_H__
#define __MODULE_STAT_H__

int cmmStatSetProcess(char ** keywords, int tabSize, daemon_handle_t daemon_handle);
int cmmStatShowProcess(char ** keywords, int tabSize, daemon_handle_t daemon_handle);

#endif


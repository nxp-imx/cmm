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

#ifndef __MODULE_MACVLAN_H__
#define __MODULE_MACVLAN_H__

#include "itf.h"

void __cmmGetMacVlan(int fd, struct interface *itf);
int cmmFeMacVlanUpdate(cpal_handle_t *cpal_handle,int fd, int request, struct interface *itf);
int cmmMacVlanQueryProcess(char **keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmMacVlanLocalShow(struct cli_def *cli, char *command, char *argv[], int argc);

#endif


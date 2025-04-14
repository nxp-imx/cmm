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

#ifndef __MODULE_PRF_H__
#define __MODULE_PRF_H__

/*Function codes*/
#define FPP_CMD_TRC_MASK                            0xff00
#define FPP_CMD_TRC_VAL                             0x0f00

int cmmPrfMem(int argc,char **argv,int firstarg ,daemon_handle_t daemon_handle);
int cmmPrfNM(int argc,char **argv,int firstarg ,daemon_handle_t daemon_handle);
int prfMspMS(daemon_handle_t daemon_handle, int argc, char **argv);
int prfMspMSW(daemon_handle_t daemon_handle, int argc, char **argv);
int prfMspCT(daemon_handle_t daemon_handle, int argc, char **argv);
int prfStatus(daemon_handle_t daemon_handle, int argc, char **argv);
int prfPTBusyCPU(daemon_handle_t daemon_handle, int argc, char **argv);
int prfPTsetmask(daemon_handle_t daemon_handle, int argc, char **argv);
int prfPTstart(daemon_handle_t daemon_handle, int argc, char **argv);
int prfPTswitch(daemon_handle_t daemon_handle, int argc, char **argv);
int prfPTshow(daemon_handle_t daemon_handle, int argc, char **argv);

#endif


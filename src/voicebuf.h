/*
 *
 *  Copyright (C) 2010 Mindspeed Technologies, Inc.
 *  Copyright 2014-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017,2021,2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 *
 *
 */
#ifndef __VOICEBUF_H__
#define __VOICEBUF_H__

#include <sys/ioctl.h>

#define MEMBUF_CHAR_DEVNAME "/dev/membuf"

#define VOICE_FILE_MAX		8

/* These must match the kernel definitions */
#define MEMBUF_GET_SCATTER _IOR('m', 1, struct usr_scatter_list)

#define MAX_BUFFERS	48

struct usr_scatter_list
{
	u_int8_t entries;
	u_int8_t pg_order[MAX_BUFFERS];
	u_int32_t addr[MAX_BUFFERS];
};

int voice_file_load(cpal_handle_t *cpal_handle, cmmd_voice_file_load_cmd_t *cmd, u_int16_t *res_buf, u_int16_t *res_len);
int voice_file_unload(cpal_handle_t *cpal_handle, cmmd_voice_file_unload_cmd_t *cmd, u_int16_t *res_buf, u_int16_t *res_len);
int voice_buffer_reset(cpal_handle_t *cpal_handle);
int cmmVoiceBufSetProcess(int argc, char *argv[], daemon_handle_t daemon_handle);

#endif

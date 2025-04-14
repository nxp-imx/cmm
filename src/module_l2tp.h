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

#ifndef __MODULE_L2TP_H__
#define __MODULE_L2TP_H__


int l2tp_itf_add(cpal_handle_t *cpal_handle, int request, struct interface *itf);
int __l2tp_itf_del(cpal_handle_t *cpal_handle, struct interface *itf);
int l2tp_itf_del(cpal_handle_t *cpal_handle, struct interface *itf);
int l2tp_daemon(cpal_handle_t *cpal_handle,int command, cmmd_l2tp_session_t *cmd,  u_int16_t cmd_len, u_int16_t *res_buf, u_int16_t *res_len);

#endif

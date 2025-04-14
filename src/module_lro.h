/*
 *
 *  Copyright (C) 2014 Mindspeed Technologies, Inc.
 *  Copyright 2014-2016 Freescale Semiconductor, Inc.
 *  Copyright 2017,2021,2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 *
 *
 */

#ifndef __MODULE_LRO_H__
#define __MODULE_LRO_H__

int lro_interface_add(char *ifname);
void lro_interface_update(struct interface *itf);
void lro_socket_open(cpal_handle_t *cpal_handle, struct ctTable *ctEntry);
void lro_socket_close(cpal_handle_t *cpal_handle, cpal_handle_t *cpal_key_handle, struct ctTable *ctEntry);

#endif /* __MODULE_LRO_H__ */

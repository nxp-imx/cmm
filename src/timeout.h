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

/* Function prototypes */ 
int timeoutSet(daemon_handle_t daemon_handle, char *argv[], int argc);
int cmmtimeoutSet(daemon_handle_t daemon_handle, char *argv[], int argc, int tab);
int cmmFeGetTimeout(cpal_handle_t *cpal_handle, struct ctTable *ctEntry, unsigned int *timeout);
int cmmFragTimeoutSet(char ** keywords, int tabStart, daemon_handle_t daemon_handle);
unsigned long long cmm_convert_to_numeric(char *str );

#define MAX_TIMEOUT_STR_LEN 10

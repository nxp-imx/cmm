/*
 *  Copyright 2021,2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 *
 */
#ifdef LS1043
#define SUCCESS               0
#define ERROR                -1
#define INVALID_KEYWORD      -2

int cmmTxSetProcess(char **keywords, int tabStart, daemon_handle_t daemon_handle);
int cmmDSCPVlanPcpMapQueryProcess(char ** keywords, int cpt, daemon_handle_t daemon_handle);
#endif /* endif for LS1043 */

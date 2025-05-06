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
#ifndef __CMMLIB__
#define __CMMLIB__

#include <sys/types.h>

#define CMM_BUF_SIZE 512 

typedef struct cmm_handle cmm_handle_t;

typedef struct cmm_command
{
	long int	msg_type;		/* internal, user shouldn't use this*/
	u_int16_t	func;			/* function code, filled by user */ 
	u_int16_t	length;			/* buf length, filled by user */
	u_int8_t	buf[CMM_BUF_SIZE];	/* command payload, filled by user */
} __attribute__((__packed__)) cmm_command_t;	/* to be consistent with cmm_response_t, actually 
						 * no demand for "packed" here
						 */

typedef struct cmm_response
{
	long int 	msg_type;		/* internal, user shouldn't use this */
	int 		daemon_errno;		/* internal, user shouldn't use this */
	u_int16_t	func;			/* function code, set by remote side */
	u_int16_t	length;			/* length of a buf, set by remote side */
	union {
		u_int16_t	rc;			/* return code, set by remote side */
		u_int8_t	buf[CMM_BUF_SIZE];	/* response payload, set by remote side */
	};
} cmm_response_t;	/* "packed" is due to operations
						 * on the structure using memcpy() internally in CMM daemon
						 */

cmm_handle_t 	*cmm_open(void);
void		cmm_close(cmm_handle_t*);
int		cmm_send(cmm_handle_t*, cmm_command_t*, int nonblocking);
int		cmm_recv(cmm_handle_t*, cmm_response_t*, int nonblocking);

#endif


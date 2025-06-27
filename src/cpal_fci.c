/* Copyright 2019,2025 NXP
 *
 * SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0+)
 */

#include "cmm.h"
#include "cpal_fci.h"


cpal_handle_t *cpal_ff_catch_open(void)
{
	cpal_handle_t *handle = fci_open(FCILIB_FF_TYPE, NL_FF_GROUP);
	int fd;
	int size = NFNL_SOCK_SIZE;
	socklen_t socklen = sizeof(size);

	if (handle)
	{
		fd = cpal_fd(handle);

		if (fd)
		{
			if(setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &size, socklen) < 0)
			{
				cmm_print(DEBUG_ERROR, "%s:%d setsockopt(socket %d) failed %s\n", __func__, __LINE__, fd, strerror(errno));
				cpal_close(handle);
			}
		}
		else
			cpal_close(handle);
	}

	return handle;
}

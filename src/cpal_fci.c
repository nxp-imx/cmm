/* Copyright 2019,2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * The GPL-2.0 license for this file can be found in the COPYING file
 * included with this distribution or at http://www.gnu.org/licenses/gpl-2.0.html
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

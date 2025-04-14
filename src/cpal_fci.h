/* Copyright 2019,2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * The GPL-2.0 license for this file can be found in the COPYING file
 * included with this distribution or at http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef __CPAL_FCI_H__
#define __CPAL_FCI_H__

#include <libfci.h>

typedef FCI_CLIENT cpal_handle_t;


/* CPAL callbacks return codes */
enum CPAL_CB_ACTION {
	CPAL_CB_STOP = FCI_CB_STOP,		/* stop catching event from CPAL */
	CPAL_CB_CONTINUE = FCI_CB_CONTINUE,	/* continue event catching */
};


static inline cpal_handle_t *cpal_ff_open(void)
{
	return fci_open(FCILIB_FF_TYPE, 0);
}

static inline cpal_handle_t *cpal_key_open(void)
{
	return fci_open(FCILIB_KEY_TYPE, 0);
}

cpal_handle_t *cpal_ff_catch_open(void);

static inline cpal_handle_t *cpal_key_catch_open(void)
{
	return fci_open(FCILIB_KEY_TYPE, NL_KEY_ALL_GROUP);
}

static inline int cpal_close(cpal_handle_t *handle)
{
	return fci_close(handle);
}

static inline int cpal_fd(cpal_handle_t *handle)
{
	return fci_fd(handle);
}

static inline int cpal_catch(cpal_handle_t *handle)
{
	return fci_catch(handle);
}

static inline int cpal_register_cb(cpal_handle_t *handle, int (*cb)(unsigned short fcode, unsigned short len, unsigned short *payload))
{
	return fci_register_cb(handle, cb);
}

static inline int cpal_write(cpal_handle_t *handle, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf)
{
	return fci_write(handle, fcode, cmd_len, cmd_buf);
}

static inline int cpal_cmd(cpal_handle_t *handle, unsigned short fcode, unsigned short *cmd_buf, unsigned short cmd_len, unsigned short *rep_buf, unsigned short *rep_len)
{
	return fci_cmd(handle, fcode, cmd_buf, cmd_len, rep_buf, rep_len);
}

static inline int cpal_query(cpal_handle_t *handle, unsigned short fcode, unsigned short cmd_len, unsigned short *cmd_buf, unsigned short *rep_len, unsigned short *rep_buf)
{
	return fci_query(handle, fcode, cmd_len, cmd_buf, rep_len, rep_buf);
}

#endif

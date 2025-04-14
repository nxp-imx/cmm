/* Copyright 2019,2025 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * The GPL-2.0 license for this file can be found in the COPYING file
 * included with this distribution or at http://www.gnu.org/licenses/gpl-2.0.html
 */

#ifndef __CPAL_H__
#define __CPAL_H__

#ifdef CPAL_BPF
#include "cpal_bpf.h"
#else
#include "cpal_fci.h"
#endif

#endif

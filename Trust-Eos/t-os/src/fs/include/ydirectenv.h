/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system. 
 * ydirectenv.h: Environment wrappers for direct.
 *
 * Copyright (C) 2002 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 *
 * Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 *
 * $Id: ydirectenv.h,v 1.2 2004/10/10 17:54:59 charles Exp $
 *
 */
 
#ifndef __YDIRECTENV_H__
#define __YDIRECTENV_H__

// Direct interface

#include "devextras.h"
#include "tee_debug.h"
#include "tee_string.h"
#include "tee_mmf.h"
//#include "yaffscfg.h"

#define YMALLOC(x) TEE_Malloc(x,0)
#define YFREE(x)   TEE_Free(x)

//#define YINFO(s) YPRINTF(( __FILE__ " %d %s\n",__LINE__,s))
//#define YALERT(s) YINFO(s)
//#define YPRINTF(x) printf x

#define TENDSTR "\n"
#define TSTR(x) x
#define TOUT(p) TEE_Printf p


#define YAFFS_LOSTNFOUND_NAME		"lost+found"
#define YAFFS_LOSTNFOUND_PREFIX		"obj"


#define Y_CURRENT_TIME yaffsfs_CurrentTime()
#define Y_TIME_CONVERT(x) (x)

#define YAFFS_ROOT_MODE				0666
#define YAFFS_LOSTNFOUND_MODE		0666

#define yaffs_SumCompare(x,y) ((x) == (y))
#define yaffs_strcmp(a,b) TEE_StrCmp(a,b)


#define YBUG();

extern unsigned yaffs_traceMask;

#define YAFFS_TRACE_ERROR		0x0001
#define YAFFS_TRACE_OS			0x0002
#define YAFFS_TRACE_ALLOCATE	0x0004
#define YAFFS_TRACE_SCAN		0x0008
#define YAFFS_TRACE_BAD_BLOCKS	0x0010
#define YAFFS_TRACE_ERASE		0x0020
#define YAFFS_TRACE_GC			0x0040
#define YAFFS_TRACE_TRACING		0x0100
#define YAFFS_TRACE_ALWAYS		0x0200
#define YAFFS_TRACE_BUG			0x8000

#define T(mask,p) do{ if((mask) & (yaffs_traceMask | YAFFS_TRACE_ERROR)) TOUT(p);} while(0) 

#endif



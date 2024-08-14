/*
   Copyright (C) 2024 Free Software Foundation, Inc.
   Written by Simon Sobisch, Nicolas Berthier, Vedant Tewari

   This file is part of GnuCOBOL.

   The GnuCOBOL runtime library is free software: you can redistribute it
   and/or modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   GnuCOBOL is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GnuCOBOL.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef COB_JAVA_H
#define COB_JAVA_H

#include "config.h"
#include "common.h"

/* Indirect Java API-free interface (implemented in `call.c`) */

typedef struct __cob_java_static_method cob_java_handle;

COB_EXPIMP cob_java_handle* 	cob_resolve_java (const char *class_name,
						  const char* method_name,
						  const char *type_signature);
COB_EXPIMP void			cob_call_java (const cob_java_handle *method_handle);

/* Direct Java API */

/* Java API struct */
typedef struct _cob_java_api {
	cob_java_handle*	(*cob_resolve) (const char *class_name,
						const char* method_name,
						const char *method_signature);
	void			(*cob_call) (const cob_java_handle *method_handle);
} cob_java_api;

#ifdef	WITH_JNI
COB_EXPIMP int			cob_jni_init (cob_java_api *);
#endif

#endif	/* COB_JAVA_H */

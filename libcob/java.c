/*
   Copyright (C) 2024 Free Software Foundation, Inc.
   Written by Vedant Tewari
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

#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <common.h>

static JavaVM *jvm = NULL;
static JNIEnv *env = NULL;

typedef struct __cob_java_static_method {
	jclass cls;
	jmethodID mid;
} cob_java_handle;

static int			/* non-zero means there's an error */
cob_java_initialize() {
    /* JDK/JRE 6 VM initialization arguments */
    JavaVMInitArgs args;
    JavaVMOption* options = (JavaVMOption*)cob_malloc(sizeof(JavaVMOption) * 1);
    args.version = JNI_VERSION_1_6;
    /* inline */
    args.nOptions = 1;
    options[0].optionString = cob_malloc(strlen("-Djava.class.path=/usr/lib/java") + 1);
    strcpy(options[0].optionString, "-Djava.class.path=/usr/lib/java");
    args.options = options;
    args.ignoreUnrecognized = 1;
    /* loading and initializing a Java VM, returning as JNI interface */
    return JNI_CreateJavaVM(&jvm, (void**)&env, &args);
}

cob_java_handle*
cob_resolve_java (const char *class_name,
		  const char* method_name,
		  const char *type_signature) {
  jclass cls;
  jmethodID mid;
  cob_java_handle *handle;

    /* Resolving Reference to Java Class and Static Method */
    if (jvm == NULL) {
      if (cob_java_initialize ()) {
	return NULL;
      }
    }

    cls = (*env)->FindClass(env, class_name);
    if (!cls) {
        return NULL;
    }

    mid = (*env)->GetStaticMethodID(env, cls, method_name, type_signature);
    if (!mid) {
        (*env)->DeleteLocalRef(env, cls);
        return NULL;
    }

    handle = (cob_java_handle*)cob_malloc(sizeof(cob_java_handle));
    if (!handle) {
        (*env)->DeleteLocalRef(env, cls);
        return NULL;
    }

    handle->cls = (*env)->NewGlobalRef(env, cls);
    handle->mid = mid;
    (*env)->DeleteLocalRef(env, cls);

    return handle;
}

void
cob_call_java (const cob_java_handle *method_handle) {
    if (method_handle == NULL) {
        return;
    }
    (*env)->CallStaticVoidMethod(env, method_handle->cls,
					     method_handle->mid, NULL);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }
}
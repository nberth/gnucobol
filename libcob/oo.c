/*
   Copyright (C) 2002-2012, 2014-2020, 2022-2025 Free Software Foundation, Inc.
   Written by Keisuke Nishida, Roger While, Simon Sobisch, Ron Norman,
   Edwart Hard

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


#include "common.h"
// #include "config.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#ifdef	HAVE_LOCALE_H
#include <locale.h>
#endif

/* include internal and external libcob definitions, forcing exports */
#define	COB_LIB_EXPIMP
// #include "coblocal.h"

/* Append `str` to `base_str` and return a new string */
char*
append_str (const char* base_str, const char* str) 
{
    const size_t base_str_len = strlen(base_str);
    const size_t str_len = strlen(str);
    const size_t new_str_len = base_str_len + str_len;

    char* new_str = cob_malloc (new_str_len + 1);

    /* Copy `base_str` to `new_str` */
    memcpy (new_str, base_str, base_str_len);
    
    /* Append `str` */
    strcat (new_str, str);

    new_str[new_str_len + 1] = '\0';

    return new_str;
}

void*
__class_Base_ (void)
{
    /* TODO: Do Base class initialization */
    printf ("initializing Base class\n");
    return NULL;
}

/* Search method name for a particular class */
int
cob_get_factory_method (const struct cob_factory_obj* class_ptr)
{
    return -1; /* TODO */
}

struct cob_factory_obj*
cob_load_class (const char* class_name) 
{
    struct cob_factory_obj* class_obj;                /* Class factory object */
    void (*class_init) (struct cob_factory_obj*);     /* Class initializer function pointer */
    char* class_name_ = NULL;
    struct cob_factory_obj* parent_classes[] = {};

    /* const size_t len = strlen(class_name); */

    /* TODO: check whether class_name_ is already loaded. If yes, we already
       have the corresponding `struct cob_factory_obj` and we can return it
       directly. */
    class_obj = lookup_class_by_name (class_name);
    if (class_obj != NULL) {
	    return class_obj;
    }

    /* /\* Special case: Built-in `Base` class *\/ */
    /* if (strcasecmp (class_name, "base") == 0) { */
    /*     /\* */
    /*       For `Base` class, append a special qualifier string. */
    /*       This is done to separate from a user-defined function named `Base`.  */
    /*     *\/ */
    /*     class_name_ = "__class_Base_"; */
    /* } else { */
    class_name_ = append_str (class_name, "_"); /* TODO: reuse g++/clang++-style mangling? */
    /* } */

    printf ("\nClass initializer function: %s\n", class_name_);
    /* Resolve the class initializer function symbol */
    class_init = cob_resolve_oo_class (class_name_);

    printf ("Calling class initializer for: %s\n", class_name);

    class_obj = (struct cob_factory_obj*) cob_malloc (sizeof(struct cob_factory_obj));
    class_obj->class_name = class_name;

    /* push module stack, save call parameter count */
    if (cob_module_global_enter (&class_obj->module, &class_obj->cob_glob_ptr, 0, 0, 0)) {
	    return NULL;
    }

    class_init (class_obj);

    class_obj->module_init (class_obj->module);

    class_obj->module->collating_sequence = NULL;
    class_obj->module->crt_status = NULL;
    class_obj->module->cursor_pos = NULL;
    class_obj->module->xml_code = NULL;
    class_obj->module->xml_event = NULL;
    class_obj->module->xml_information = NULL;
    class_obj->module->xml_namespace = NULL;
    class_obj->module->xml_namespace_prefix = NULL;
    class_obj->module->xml_nnamespace = NULL;
    class_obj->module->xml_nnamespace_prefix = NULL;
    class_obj->module->xml_ntext = NULL;
    class_obj->module->xml_text = NULL;
    class_obj->module->json_code = NULL;
    class_obj->module->json_status = NULL;

    class_obj->parent_classes = (struct cob_factory_obj *)cob_malloc(
        sizeof(struct cob_factory_obj *) * class_obj->parent_class_count);

    for (int i = 0; i < class_obj->parent_class_count; i++) {
      parent_classes[i] = cob_load_class(&class_obj->parent_class_names[i]);
    }
    class_obj->parent_classes = parent_classes[0];

    /* for each parent class, we have parent_class[i]->class_fields */

    /* Pop module stack */
    cob_module_leave (class_obj->module);

    return class_obj;
}

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
#include <strings.h>
#include <ctype.h>

#ifdef	HAVE_LOCALE_H
#include <locale.h>
#endif

/* include internal and external libcob definitions, forcing exports */
#define	COB_LIB_EXPIMP

static int
hash_word (const char *word, const cob_u32_t mod)
{
	cob_u32_t	result = 0x811c9dc5;

	/* Perform 32-bit FNV1a hash */
	for (; *word; ++word) {
		/* CHECKME: all input should be upper-case already, but isn't */
		result ^= (cob_u32_t) toupper ((unsigned char) *word);
		result *= (cob_u32_t) 0x1677619;
	}

	return result % mod;
}

/*
	The Hashmap implementation below is same as the one inside
	`reserved.c` but with a small change in the way key-value
	pairs are stored. The Hashmap implementation in `reserved.c`
	allocates new memory and stores a copy of the passed value
	in the key-value pair. This does not work for our usecase
	where we want to use `factory_obj_map` as a cache for class
	factory objects, hence, we store the pointer to the original
	value directly.
*/

#define HASHMAP(type, type_struct, word_member)                                 \
	static struct type_struct **type##_map;                                     \
	static size_t type##_map_arr_size;                                          \
	static unsigned int num_##type##s;                                          \
                                                                                \
	static void init_##type##_map(void)                                         \
	{                                                                           \
		type##_map_arr_size = 512;                                              \
		num_##type##s = 0;                                                      \
		type##_map = cob_malloc(type##_map_arr_size * sizeof(void *));          \
		memset(type##_map, 0, type##_map_arr_size * sizeof(void *));            \
	}                                                                           \
                                                                                \
	static COB_INLINE COB_A_INLINE int type##_hash(const char *word)            \
	{                                                                           \
		return hash_word((const cob_c8_t *)word, type##_map_arr_size);          \
	}                                                                           \
                                                                                \
	static COB_INLINE COB_A_INLINE int next_##type##_key(                       \
		const unsigned int key)                                                 \
	{                                                                           \
		if (key < type##_map_arr_size - 1)                                      \
		{                                                                       \
			return key + 1;                                                     \
		}                                                                       \
		else                                                                    \
		{                                                                       \
			return 0;                                                           \
		}                                                                       \
	}                                                                           \
                                                                                \
	static unsigned int find_key_for_##type(const char *const word)             \
	{                                                                           \
		unsigned int key;                                                       \
                                                                                \
		for (key = type##_hash(word); /* FIXME: we currently cannot use strcmp  \
						here instead of strcasecmp. */                          \
			 type##_map[key] && strcasecmp(type##_map[key]->word_member, word); \
			 key = next_##type##_key(key))                                      \
			;                                                                   \
                                                                                \
		return key;                                                             \
	}                                                                           \
                                                                                \
	static void realloc_##type##_map(const size_t new_size)                     \
	{                                                                           \
		struct type_struct **new_map = cob_malloc(new_size * sizeof(void *));   \
		struct type_struct **old_map = type##_map;                              \
		size_t old_size = type##_map_arr_size;                                  \
		unsigned int i;                                                         \
		unsigned int key;                                                       \
                                                                                \
		type##_map_arr_size = new_size;                                         \
		type##_map = new_map;                                                   \
		memset(type##_map, 0, new_size * sizeof(void *));                       \
                                                                                \
		for (i = 0; i < old_size; ++i)                                          \
		{                                                                       \
			if (old_map[i])                                                     \
			{                                                                   \
				key = find_key_for_##type(old_map[i]->word_member);             \
				type##_map[key] = old_map[i];                                   \
			}                                                                   \
		}                                                                       \
                                                                                \
		cob_free(old_map);                                                      \
	}                                                                           \
                                                                                \
	static void free_##type##_with_key(const int key)                           \
	{                                                                           \
		cob_free(type##_map[key]);                                              \
		type##_map[key] = NULL;                                                 \
	}                                                                           \
                                                                                \
	static int add_##type##_to_map(struct type_struct* val,                    \
								   const int overwrite)                         \
	{                                                                           \
		unsigned int key;                                                       \
		int entry_already_there;                                                \
                                                                                \
		if (!type##_map)                                                        \
		{                                                                       \
			init_##type##_map();                                                \
		}                                                                       \
		/*                                                                      \
		The "- 1" is there so there is always one NULL entry in the             \
		array. If there is not one and the array is full,                       \
		find_##type will not terminate when given a word which                  \
		shares a hash with a different word.                                    \
		*/                                                                      \
		if (num_##type##s == type##_map_arr_size - 1)                           \
		{                                                                       \
			realloc_##type##_map(type##_map_arr_size * 2);                      \
		}                                                                       \
                                                                                \
		key = find_key_for_##type(val->word_member);                            \
		entry_already_there = !!type##_map[key];                                \
		if (entry_already_there)                                                \
		{                                                                       \
			if (overwrite)                                                      \
			{                                                                   \
				free_##type##_with_key(key);                                    \
			}                                                                   \
			else                                                                \
			{                                                                   \
				return 1;                                                       \
			}                                                                   \
		}                                                                       \
		else                                                                    \
		{                                                                       \
			++num_##type##s;                                                    \
		}                                                                       \
                                                                                \
		type##_map[key] = val;                                                  \
		return entry_already_there;                                             \
	}

HASHMAP (factory_obj, cob_factory_obj, class_name)

static cob_factory_obj *
find_factory_obj (const char * const class_name)
{
	return factory_obj_map[find_key_for_factory_obj (class_name)];
}

static void print_factory_obj_map(void) {
  for (size_t i = 0; i < factory_obj_map_arr_size; ++i) {
    if (factory_obj_map[i]) {
      printf("map[%zu]: class_name=%s, address=%p\n", i,
             factory_obj_map[i]->class_name, (void *)factory_obj_map[i]);
    }
  }

  printf("entries: %u\n", num_factory_objs);
}

/* Search method name for a particular class */
int
cob_get_factory_method (const cob_factory_obj* class_ptr)
{
    return -1; /* TODO */
}

cob_factory_obj*
cob_init_factory_obj(const char *class_name,
					const char *parent_class_names[],
					const int parent_classes_count,
					cob_resolved_method methods[],
					int methods_count
					) 
{
  cob_factory_obj *class_obj = find_factory_obj(class_name);
  cob_factory_obj **parent_class_factory_objs = NULL;
  cob_resolved_method *method_descriptors = NULL;
  int inherited_methods_count = 0;
  int method_idx = 0;

  /* Early exit if factory object already initialized */
  if (!class_obj->initialized) {
    class_obj->parent_class_count = parent_classes_count;
    class_obj->parent_class_names =
        parent_classes_count > 0 ? parent_class_names[0] : NULL;
    if (parent_classes_count > 0) {
      parent_class_factory_objs = cob_malloc(
          (size_t)parent_classes_count * sizeof(*parent_class_factory_objs));
    }

    for (int i = 0; i < parent_classes_count; i++) {
      parent_class_factory_objs[i] = cob_load_class(parent_class_names[i]);
      inherited_methods_count += parent_class_factory_objs[i]->methods_count;
    }
	class_obj->parent_class_factory_objs =
        parent_classes_count > 0 ? parent_class_factory_objs[0] : NULL;

    class_obj->methods_count = methods_count + inherited_methods_count;
    if (class_obj->methods_count > 0) {
      method_descriptors = cob_malloc((size_t)class_obj->methods_count *
                                      sizeof(*method_descriptors));
    }

	/* Add own methods first */
    for (int i = 0; i < methods_count; i++) {
      method_descriptors[method_idx++] = methods[i];
    }

	/* Add parent class methods */
    for (int i = 0; i < parent_classes_count; i++) {
      cob_factory_obj *parent = parent_class_factory_objs[i];
      for (int j = 0; j < parent->methods_count; j++) {
        method_descriptors[method_idx++] = parent->method_descriptors[j];
      }
    }
    class_obj->method_descriptors = method_descriptors;

    class_obj->initialized = 1;

    add_factory_obj_to_map(class_obj, 0);
  }

  return find_factory_obj(class_name);
}

cob_factory_obj*
cob_load_class (const char* class_name) 
{
	char 				class_name_[COB_SMALL_BUFF];
    static int			(*class_init) (const int);
    cob_factory_obj* 	class_obj = NULL;
    cob_factory_obj* 	parent_classes[] = {};

    const size_t 			class_name_len = strlen(class_name);

	if (!factory_obj_map) init_factory_obj_map();

	class_obj = find_factory_obj (class_name);
    if (!class_obj) {
        /* 
			Class name mangled according to the Itanium C++ ABI:
				https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangle.name

			For unscoped (global) classes, the naming rule is:
				_Z + N + [length of class name] + [class name] + E
		*/

		class_obj = (cob_factory_obj *)cob_malloc(sizeof(cob_factory_obj));
		class_obj->initialized = 0;
		class_obj->class_name = class_name;

		/* Add a dummy empty entry for access inside `cob_init_factory_obj ()`*/
		add_factory_obj_to_map(class_obj, 0);

		snprintf(class_name_, (size_t)COB_SMALL_MAX, "_ZN%ld%sE", class_name_len, class_name);
    
        printf ("\nClass initializer function: %s\n", class_name_);
        /* Resolve the class initializer function symbol */
        class_init = cob_resolve_oo_class (class_name_);

        printf ("Calling class initializer for: %s\n", class_name);
		class_init (0);

		class_obj = find_factory_obj (class_name);

		print_factory_obj_map ();
    }

    return class_obj;
}
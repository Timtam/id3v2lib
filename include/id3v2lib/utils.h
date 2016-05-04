/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_utils_h
#define id3v2lib_utils_h

#include "types.h"

const char * _get_mime_type_from_buffer(char *data, int size);
unsigned int btoi(char* bytes, int size, int offset);
char* itob(int integer);
int syncint_encode(int value);
int syncint_decode(int value);
void id3v2_free_tag(id3v2_tag* tag);

// String functions
int has_bom(char *string);

#endif

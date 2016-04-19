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

// this piece of code makes this header usable under MSVC
// without downloading msinttypes
#ifndef _MSC_VER
  #include <inttypes.h>
#else
  typedef unsigned short uint16_t;
#endif

#include "types.h"

unsigned int btoi(char* bytes, int size, int offset);
char* itob(int integer);
int syncint_encode(int value);
int syncint_decode(int value);
void id3v2_free_tag(id3v2_tag* tag);
char* get_mime_type_from_filename(const char* filename);

// String functions
int has_bom(char *string);
uint16_t* char_to_utf16(char* string, int size);
void println_utf16(uint16_t* string, int size);
char* get_path_to_file(const char* file);

#endif

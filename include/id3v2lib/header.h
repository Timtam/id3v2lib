/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_header_h
#define id3v2lib_header_h

#include <stdio.h>

#include "types.h"
#include "constants.h"
#include "utils.h"

void find_headers_in_file(FILE *file, int **location, int *size);
int has_id3v2tag(id3v2_header* tag_header);
int _has_id3v2tag(char* raw_header);
id3v2_header* get_header_from_file(FILE *file, int offset);
id3v2_header* get_header_from_buffer(char* buffer, int length);
int get_tag_version(id3v2_header* tag_header);
void edit_tag_size(id3v2_tag* tag);

#endif

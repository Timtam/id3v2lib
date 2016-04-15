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

void _find_header_offsets_in_file(FILE *file, int **location, int *size);
id3v2_header* _get_header_from_buffer(char* buffer, int length);
id3v2_header* _get_header_from_file(FILE *file, int offset);
int _has_buffer_id3v2tag(char* raw_header);
int _has_header_id3v2tag(id3v2_header* tag_header);
int id3v2_get_tag_version(id3v2_tag *tag);
//void edit_tag_size(id3v2_tag* tag);

#endif

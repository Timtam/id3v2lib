/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_frame_h
#define id3v2lib_frame_h

#include "types.h"
#include "constants.h"

void add_frame_to_tag(id3v2_tag *tag, id3v2_frame *frame);
id3v2_frame *get_frame_from_tag(id3v2_tag *tag, char *frame_id);
int get_mime_type_size_from_buffer(char *data);
id3v2_frame* parse_frame_from_tag(id3v2_tag *tag, char *bytes);
int get_frame_type(char* frame_id);
char *synchronize_data_from_buffer(char *data, int size);
id3v2_frame_text_content* parse_text_content_from_frame(id3v2_frame* frame);
id3v2_frame_comment_content* parse_comment_content_from_frame(id3v2_frame* frame);
id3v2_frame_apic_content* parse_apic_content_from_frame(id3v2_frame* frame);

#endif

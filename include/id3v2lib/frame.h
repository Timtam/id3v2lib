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

int _get_mime_type_size_from_buffer(char *data);
id3v2_frame* _parse_frame_from_tag(id3v2_tag *tag, char *bytes);
char *_synchronize_data_from_buffer(char *data, int size);
void id3v2_add_frame_to_tag(id3v2_tag *tag, id3v2_frame *frame);
id3v2_frame *id3v2_get_frame_from_tag(id3v2_tag *tag, char *frame_id);
int id3v2_get_frame_type(id3v2_frame *frame);
id3v2_frame_apic_content* id3v2_parse_apic_content_from_frame(id3v2_frame* frame);
id3v2_frame_comment_content* id3v2_parse_comment_content_from_frame(id3v2_frame* frame);
id3v2_frame_text_content* id3v2_parse_text_content_from_frame(id3v2_frame* frame);
void id3v2_set_frame_from_text_content(id3v2_frame *frame, id3v2_frame_text_content *content);

#endif

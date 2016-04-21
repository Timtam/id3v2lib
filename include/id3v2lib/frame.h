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
void _synchronize_frame(id3v2_frame *frame);
void id3v2_add_frame_to_tag(id3v2_tag *tag, id3v2_frame *frame);
id3v2_frame *id3v2_get_frame_from_tag(id3v2_tag *tag, char *frame_id);
int id3v2_get_frame_type(id3v2_frame *frame);
void id3v2_get_descriptor_from_frame(id3v2_frame *frame, char **descriptor, int *size);
void id3v2_get_id_from_frame(id3v2_frame *frame, char **id, int *size);
void id3v2_get_language_from_frame(id3v2_frame *frame, char **language);
void id3v2_get_mime_type_from_frame(id3v2_frame *frame, char **mime_type, int *size);
void id3v2_get_picture_from_frame(id3v2_frame *frame, char **picture, int *size);
void id3v2_get_text_from_frame(id3v2_frame *frame, char **text, int *size, char *encoding);
void id3v2_initialize_frame(id3v2_frame *frame, int type);

#endif

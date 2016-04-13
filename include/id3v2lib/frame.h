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

void add_frame(ID3v2_tag *tag, ID3v2_frame *frame);
ID3v2_frame *get_frame(ID3v2_tag *tag, char *frame_id);
int get_mime_type_size(char *data);
ID3v2_frame* parse_frame(ID3v2_tag *tag, char *bytes);
int get_frame_type(char* frame_id);
char *synchronize_data(char *data, int size);
ID3v2_frame_text_content* parse_text_frame_content(ID3v2_frame* frame);
ID3v2_frame_comment_content* parse_comment_frame_content(ID3v2_frame* frame);
ID3v2_frame_apic_content* parse_apic_frame_content(ID3v2_frame* frame);

#endif

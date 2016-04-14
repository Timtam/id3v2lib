/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_types_h
#define id3v2lib_types_h

#include "constants.h"

typedef struct id3v2_frame id3v2_frame;

typedef struct
{
    char tag[ID3V2_HEADER_TAG];
    char major_version;
    char minor_version;
    char flags;
    int tag_size;
    int extended_header_size;
} id3v2_header;

typedef struct
{
    int size;
    char encoding;
    char* data;
} id3v2_frame_text_content;

typedef struct
{
    char language[ID3V2_FRAME_LANGUAGE];
    char short_description[4];
    id3v2_frame_text_content* text;
} id3v2_frame_comment_content;

typedef struct
{
    char encoding;
    char* mime_type;
    int mime_type_size;
    char picture_type;
    char* description;
    int description_size;
    int picture_size;
    char* data;
} id3v2_frame_apic_content;

struct id3v2_frame
{
    char frame_id[ID3V2_FRAME_ID];
    int size;
    char flags[ID3V2_FRAME_FLAGS];
    int version; // needed to identify the tag version this frame is related too
    char* data;
    id3v2_frame *next;
    char parsed; // indicates if the frame could be successfully parsed or not
};

typedef struct
{
    id3v2_header* tag_header;
    id3v2_frame *frame;
} id3v2_tag;

// Constructor functions
id3v2_header* new_header();
id3v2_tag* new_tag();
id3v2_frame* new_frame();
id3v2_frame_text_content* new_text_content();
id3v2_frame_comment_content* new_comment_content();
id3v2_frame_apic_content* new_apic_content();

#endif

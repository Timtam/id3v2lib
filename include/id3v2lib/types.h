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

struct id3v2_frame
{
    char id[ID3V2_FRAME_ID];
    int size;
    char flags[ID3V2_FRAME_FLAGS];
    int version; // needed to identify the tag version this frame is related too
    char* data;
    id3v2_frame *next;
    char parsed; // indicates if the frame could be successfully parsed or not
};

typedef struct
{
    id3v2_header* header;
    id3v2_frame *frame;
} id3v2_tag;

// Constructor functions
id3v2_header* _new_header();
id3v2_frame* id3v2_new_frame(id3v2_tag *tag, int type);
id3v2_tag* id3v2_new_tag();

#endif

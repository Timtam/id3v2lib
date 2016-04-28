/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "id3v2lib.h"

id3v2_tag* id3v2_new_tag()
{
    id3v2_tag* tag = (id3v2_tag*) malloc(sizeof(id3v2_tag));

    if(tag == NULL)
    {
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      return NULL;
    }

    tag->header = _new_header();

    if(tag->header == NULL)
    {
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      free(tag);
      return NULL;
    }

    tag->frame = NULL;

    E_SUCCESS;

    return tag;
}

id3v2_header* _new_header()
{
    id3v2_header* tag_header = (id3v2_header*) malloc(sizeof(id3v2_header));
    if(tag_header != NULL)
    {
        memset(tag_header->tag, '\0', ID3V2_HEADER_TAG);
        tag_header->minor_version = 0x00;
        tag_header->major_version = 0x00;
        tag_header->flags = 0x00;
        memset(tag_header->tag, 0, ID3V2_HEADER_SIZE);
    }
    
    return tag_header;
}

id3v2_frame* id3v2_new_frame(id3v2_tag *tag, int type)
{
    id3v2_frame* frame = (id3v2_frame*) malloc(sizeof(id3v2_frame));

    if(frame == NULL)
    {
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      return NULL;
    }

    frame->next = NULL;

    frame->version = id3v2_get_tag_version(tag);

    frame->parsed = 1;

    frame->tag = tag;

    id3v2_initialize_frame(frame, type);

    if(E_GET != ID3V2_OK)
    {
      free(frame);
      return NULL;
    }

    id3v2_add_frame_to_tag(tag, frame);

    return frame;
}

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

id3v2_tag* new_tag()
{
    id3v2_tag* tag = (id3v2_tag*) malloc(sizeof(id3v2_tag));
    tag->tag_header = new_header();
    tag->frame = NULL;
    return tag;
}

id3v2_header* new_header()
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

id3v2_frame* new_frame()
{
    id3v2_frame* frame = (id3v2_frame*) malloc(sizeof(id3v2_frame));
    frame->next = NULL;
    return frame;
}

id3v2_frame_text_content* new_text_content()
{
    id3v2_frame_text_content* content = (id3v2_frame_text_content*) malloc(sizeof(id3v2_frame_text_content));
    return content;
}

id3v2_frame_comment_content* new_comment_content()
{
    id3v2_frame_comment_content* content = (id3v2_frame_comment_content*) malloc(sizeof(id3v2_frame_comment_content));
    content->text = new_text_content();
    return content;
}

id3v2_frame_apic_content* new_apic_content()
{
    id3v2_frame_apic_content* content = (id3v2_frame_apic_content*) malloc(sizeof(id3v2_frame_apic_content));
    return content;
}
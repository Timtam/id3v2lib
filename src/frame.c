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

ID3v2_frame* parse_frame(char* bytes, int offset, int version)
{
    ID3v2_frame* frame = new_frame();
    
    // Parse frame header
    memcpy(frame->frame_id, bytes + offset, (version==ID3v22 ? ID3_FRAME_SIZE2 : ID3_FRAME_ID));

    if(version==ID3v22)
      // fill the remaining space with emptyness
      frame->frame_id[3]='\0';

    // Check if we are into padding
    if(memcmp(frame->frame_id, "\0\0\0", 3) == 0)
    {
        free(frame);
        return NULL;
    }

    frame->size = btoi(bytes, (version==ID3v22 ? ID3_FRAME_SIZE2 : ID3_FRAME_SIZE), offset += (version==ID3v22 ? ID3_FRAME_ID2 : ID3_FRAME_ID));
    if(version == ID3v24)
    {
        frame->size = syncint_decode(frame->size);
    }

    if(version != ID3v22) // flags are only available in v23 and 24 tags
      memcpy(frame->flags, bytes + (offset += ID3_FRAME_SIZE), 2);
    else
      // just pushing the offset forward
      offset += ID3_FRAME_SIZE2;
    
    // Load frame data
    frame->data = (char*) malloc(frame->size * sizeof(char));
    memcpy(frame->data, bytes + (offset += (version==ID3v22 ? 0 : ID3_FRAME_FLAGS)), frame->size);
    
    frame->major_version = version;

    return frame;
}

int get_frame_type(char* frame_id)
{
    // some exceptions, for example...
    if(strcmp(frame_id, "PIC\0")==0) // not called APIC in v22, but PIC instead
      return APIC_FRAME;

    switch(frame_id[0])
    {
        case 'T':
            return TEXT_FRAME;
        case 'C':
            return COMMENT_FRAME;
        case 'A':
            return APIC_FRAME;
        default:
            return INVALID_FRAME;
    }
}

ID3v2_frame_text_content* parse_text_frame_content(ID3v2_frame* frame)
{
    ID3v2_frame_text_content* content;
    if(frame == NULL)
    {
        return NULL;
    }
    
    content = new_text_content();
    content->encoding = frame->data[0];

    if(frame->major_version==ID3v22 && content->encoding==UTF_16_ENCODING_WITH_BOM)
      // this isn't correct, the UTF_16_ENCODING_WITH_BOM flag in v22 actually means UCS-2 encoding
      content->encoding = UCS_2_ENCODING;

    content->size = frame->size - ID3_FRAME_ENCODING;
    content->data=frame->data + ID3_FRAME_ENCODING;
    return content;
}

ID3v2_frame_comment_content* parse_comment_frame_content(ID3v2_frame* frame)
{
    ID3v2_frame_comment_content *content;
    if(frame == NULL)
    {
        return NULL;
    }
    
    content = new_comment_content();
    
    content->text->encoding = frame->data[0];

    if(frame->major_version==ID3v22 && content->text->encoding==UTF_16_ENCODING_WITH_BOM)
      content->text->encoding = UCS_2_ENCODING;

    content->text->size = frame->size - ID3_FRAME_ENCODING - ID3_FRAME_LANGUAGE - ID3_FRAME_SHORT_DESCRIPTION;
    memcpy(content->language, frame->data + ID3_FRAME_ENCODING, ID3_FRAME_LANGUAGE);
    content->short_description[0] = '\0'; // Ignore short description
    content->text->data=frame->data + ID3_FRAME_ENCODING + ID3_FRAME_LANGUAGE + 1;
    
    return content;
}

char* parse_mime_type(char* data, int* i)
{
    char* mime_type = (char*) malloc(30 * sizeof(char));
    
    while(data[*i] != '\0')
    {
        mime_type[*i - 1] = data[*i];
        (*i)++;
    }
    
    return mime_type;
}

ID3v2_frame_apic_content* parse_apic_frame_content(ID3v2_frame* frame)
{
    ID3v2_frame_apic_content *content;
    int i = 1; // Skip ID3_FRAME_ENCODING

    if(frame == NULL)
    {
        return NULL;
    }
    
    content = new_apic_content();
    
    content->encoding = frame->data[0];

    if(frame->major_version==ID3v22 && content->encoding==UTF_16_ENCODING_WITH_BOM)
      content->encoding = UCS_2_ENCODING;
    
    content->mime_type = parse_mime_type(frame->data, &i);
    content->picture_type = frame->data[++i];
    content->description = &frame->data[++i];

    if (content->encoding == UTF_16_ENCODING_WITH_BOM || content->encoding == UTF_16_ENCODING_WITHOUT_BOM || content->encoding == UCS_2_ENCODING ) {
            /* skip UTF-16 / UCS-2 description */
            for ( ; * (uint16_t *) (frame->data + i); i += 2);
            i += 2;
    }
    else {
            /* skip UTF-8 or Latin-1 description */
            for ( ; frame->data[i] != '\0'; i++);
            i += 1;
    }
  
    content->picture_size = frame->size - i;
    content->data= frame->data + i;
    
    return content;
}

void add_frame(ID3v2_tag *tag, ID3v2_frame *frame)
{
  ID3v2_frame *last_frame;

  if(tag->frame == NULL)
  {
    tag->frame = frame;
    return;
  }

  last_frame = tag->frame;
  while(last_frame->next != NULL)
    last_frame=last_frame->next;

  last_frame->next = frame;

}

ID3v2_frame *get_frame(ID3v2_tag *tag, char *frame_id)
{
  char tmp_id[ID3_FRAME_ID];
  ID3v2_frame *matching_frame;

  if(tag->frame == NULL)
    // no frames in tag, return nothing
    return NULL;

  matching_frame = tag->frame;

  while(matching_frame != NULL)
  {
    // we will copy the id here so we can adjust it if necessary
    memcpy(tmp_id, frame_id, ID3_FRAME_ID);

    if(matching_frame->major_version==ID3v22)
      // since id3 v22 only has 3-byte identifiers, we will replace the fourth sign (if available) with the empty sign
      tmp_id[3]='\0';

    if(memcmp(matching_frame->frame_id, tmp_id, 4)==0)
      break; 

    matching_frame = matching_frame->next;
  }

  return matching_frame;
}
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
#include <ctype.h>

#include "id3v2lib.h"

ID3v2_frame* parse_frame(ID3v2_tag *tag, char *bytes)
{
    ID3v2_frame* frame = new_frame();
    int offset = 0;
    char unsynchronisation = 0;
    int version = get_tag_version(tag->tag_header);
    
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

    // check if all relevant chars are alphabetical
    if(version == ID3v22 && (
       !isalpha(frame->frame_id[0]) ||
       !isalpha(frame->frame_id[1]) ||
       !isalpha(frame->frame_id[2])))
    {
      free(frame);
      return NULL;
    }
    else if(version != ID3v22 && (
            !isalpha(frame->frame_id[0]) ||
            !isalpha(frame->frame_id[1]) ||
            !isalpha(frame->frame_id[2]) ||
            !isalpha(frame->frame_id[3])))
    {
      free(frame);
      return NULL;
    }

    frame->size = btoi(bytes, DECIDE_FRAME(version, ID3_FRAME_SIZE2, ID3_FRAME_SIZE), offset += DECIDE_FRAME(version, ID3_FRAME_ID2, ID3_FRAME_ID));
    if(version == ID3v24)
    {
        frame->size = syncint_decode(frame->size);
    }

    // detecting if all frames are unsynchronized
    if(tag->tag_header->flags&(1<<7)==1<<7)
      unsynchronisation = 1;

    if(version != ID3v22) // flags are only available in v23 and 24 tags
    {
      memcpy(frame->flags, bytes + (offset += ID3_FRAME_SIZE), 2);
      offset += ID3_FRAME_FLAGS;

      // if some unknown flags are set, we ignore this frame since that actually means that the frame might not be parseable
      if(frame->flags[1]&(1<<7)==(1<<7) ||
         frame->flags[1]&(1<<5)==(1<<5) ||
         frame->flags[1]&(1<<4)==(1<<4))
      {
        frame->parsed = 0;
        return frame;
      }

      // id3 v2.3 and v2.4 frames can define single-frame unsynchronisation in there flags too
      if(frame->flags[1]&(1<<1)==(1<<1))
        unsynchronisation = 1;
    }
    else
      // just pushing the offset forward
      offset += ID3_FRAME_SIZE2;
    
    // Load frame data
    if(unsynchronisation)
      frame->data = synchronize_data(bytes + offset, frame->size);
    else
      frame->data= (char *)malloc(frame->size * sizeof(char));

    memcpy(frame->data, bytes + offset, frame->size);
    
    frame->version = version;
    // the frame was successfully parsed
    frame->parsed = 1;

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

    if(content->encoding==UTF_16_ENCODING_WITH_BOM && !has_bom(frame->data+ID3_FRAME_ENCODING))
      content->encoding = UTF_16_ENCODING_WITHOUT_BOM;

    content->size = frame->size - ID3_FRAME_ENCODING;
    content->data=frame->data + ID3_FRAME_ENCODING;
    return content;
}

ID3v2_frame_comment_content* parse_comment_frame_content(ID3v2_frame* frame)
{
    ID3v2_frame_comment_content *content;
    int offset = 0;

    if(frame == NULL)
    {
        return NULL;
    }
    
    content = new_comment_content();
    
    content->text->encoding = frame->data[0];
    offset += ID3_FRAME_ENCODING;

    memcpy(content->language, frame->data + offset, ID3_FRAME_LANGUAGE);
    offset += ID3_FRAME_LANGUAGE;

    if(content->text->encoding==UTF_16_ENCODING_WITH_BOM && !has_bom(frame->data + offset))
      content->text->encoding = UTF_16_ENCODING_WITHOUT_BOM;

    if(content->text->encoding==ISO_ENCODING || content->text->encoding == UTF_8_ENCODING)
    {
      content->short_description[0]=(frame->data)[offset];
      memset(content->short_description + 1, '\0', 3);
      offset += 1;
    }
    else
    {
      memcpy(content->short_description, frame->data+offset, 4);
      offset += 4;
    }

    content->text->size = frame->size - offset;

    content->text->data=frame->data + offset;
    
    return content;
}

int get_mime_type_size(char* data)
{
    int i =0;

    while(data[i] != '\0')
    {
        i++;
    }
    
    return i;
}

ID3v2_frame_apic_content* parse_apic_frame_content(ID3v2_frame* frame)
{
    ID3v2_frame_apic_content *content;
    int offset = 1; // Skip ID3_FRAME_ENCODING

    if(frame == NULL)
    {
        return NULL;
    }
    
    content = new_apic_content();
    
    content->encoding = frame->data[0];

    if(content->encoding==UTF_16_ENCODING_WITH_BOM && !has_bom(content->description))
      content->encoding = UTF_16_ENCODING_WITHOUT_BOM;

    content->mime_type = frame->data+offset;

    if(frame->version==ID3v22)
    {
      // id3 v2.2 mime types are always (!) exactly 3 bytes long, without the terminator
      content->mime_type_size = 3;
    }
    else
    {
      content->mime_type_size=get_mime_type_size(content->mime_type);
    }

    offset += content->mime_type_size;

    content->picture_type = frame->data[++offset];

    content->description = frame->data+ ++offset;

    if (content->encoding == UTF_16_ENCODING_WITH_BOM || content->encoding==UTF_16_ENCODING_WITHOUT_BOM)
    {
      /* skip UTF-16 description */
      // a bit more understandable:
      // as long as we don't get the \0\0 end, we skip forward
      for ( ; memcmp(frame->data+offset, "\0\0", 2)!=0; offset += 2);
      offset += 2;

    }
    else
    {
      /* skip UTF-8 or Latin-1 description */
      for ( ; frame->data[offset] != '\0'; offset++);
      offset += 1;
    }

    // calculating the description length
    // a mixture of pointer and integer division/addition
    content->description_size=((frame->data + offset) - content->mime_type) - content->mime_type_size -1;
  
    content->picture_size = frame->size - offset;
    content->data= frame->data + offset;
    
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

    if(matching_frame->version==ID3v22)
      // since id3 v22 only has 3-byte identifiers, we will replace the fourth sign (if available) with the empty sign
      tmp_id[3]='\0';

    if(memcmp(matching_frame->frame_id, tmp_id, 4)==0)
      break; 

    matching_frame = matching_frame->next;
  }

  return matching_frame;
}

char *synchronize_data(char *data, int size)
{
  char check = 0; // indicated we'll have to inspect the next 2 bytes carefully
  int i;
  int sync_size = 0; // size of the synchronized data stream
  // at first allocating as much space as given into this function, if less is used we'll re-allocate later
  char *sync_data=(char *)malloc(size * sizeof(char));
 
  for(i = 0; i < size; i++)
  {
    switch(check)
    {
      case 0:
        // check if we find the relevant syncbyte here
        if(data[i]==0xFF)
          check = 1;
        sync_data[sync_size++] = data[i];
        break;
      case 1:
        if(data[i]!=0x00) // false alarm
        {
          check = 0;
          sync_data[sync_size++] = data[i];
        }
        else // inspect even more
          check++;
        break;
      case 2:
        if(data[i]!=0x00 && data[i]&(111<<7)!=(111<<7))
        // false alarm, so we have to copy the previous byte at next and reset everything
          sync_data[sync_size++] = data[i-1];
        sync_data[sync_size++] = data[i];
        check = 0;
        break;
    }
  }  
  // if we successfully synchronized something, we can re-allocate some stuff here
  if(sync_size<size)
    sync_data = (char *)realloc(sync_data, sync_size);

  return sync_data;
}
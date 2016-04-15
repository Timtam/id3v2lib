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

id3v2_frame* _parse_frame_from_tag(id3v2_tag *tag, char *bytes)
{
    id3v2_frame* frame = id3v2_new_frame(tag);
    int offset = 0;
    char unsynchronisation = 0;
    int version = id3v2_get_tag_version(tag);
    
    if(frame == NULL)
      return NULL;

    // Parse frame header
    memcpy(frame->id, bytes + offset, (version==ID3V2_2 ? ID3V2_FRAME_SIZE2 : ID3V2_FRAME_ID));

    if(version==ID3V2_2)
      // fill the remaining space with emptyness
      frame->id[3]='\0';

    // Check if we are into padding
    if(memcmp(frame->id, "\0\0\0", 3) == 0)
    {
        free(frame);
        return NULL;
    }

    // check if all relevant chars are alphabetical
    if(version == ID3V2_2 && (
       !isalpha(frame->id[0]) ||
       !isalpha(frame->id[1]) ||
       !isalpha(frame->id[2])))
    {
      free(frame);
      return NULL;
    }
    else if(version != ID3V2_2 && (
            !isalpha(frame->id[0]) ||
            !isalpha(frame->id[1]) ||
            !isalpha(frame->id[2]) ||
            !isalpha(frame->id[3])))
    {
      free(frame);
      return NULL;
    }

    frame->size = btoi(bytes, ID3V2_DECIDE_FRAME(version, ID3V2_FRAME_SIZE2, ID3V2_FRAME_SIZE), offset += ID3V2_DECIDE_FRAME(version, ID3V2_FRAME_ID2, ID3V2_FRAME_ID));
    if(version == ID3V2_4)
    {
        frame->size = syncint_decode(frame->size);
    }

    // detecting if all frames are unsynchronized
    if(tag->header->flags&(1<<7)==1<<7)
      unsynchronisation = 1;

    if(version != ID3V2_2) // flags are only available in v23 and 24 tags
    {
      memcpy(frame->flags, bytes + (offset += ID3V2_FRAME_SIZE), 2);
      offset += ID3V2_FRAME_FLAGS;

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
      offset += ID3V2_FRAME_SIZE2;
    
    // Load frame data
    if(unsynchronisation)
      frame->data = _synchronize_data_from_buffer(bytes + offset, frame->size);
    else
      frame->data= (char *)malloc(frame->size * sizeof(char));

    if(frame->data == NULL)
    {
      free(frame);
      return NULL;
    }


    memcpy(frame->data, bytes + offset, frame->size);
    
    // the frame was successfully parsed
    frame->parsed = 1;

    return frame;
}

int id3v2_get_frame_type(id3v2_frame *frame)
{
    if(frame == NULL)
    {
      E_FAIL(ID3V2_ERROR_NOT_FOUND);
      return ID3V2_INVALID_FRAME;
    }

    E_SUCCESS;

    // some exceptions, for example...
    if(strcmp(frame->id, "PIC\0")==0) // not called APIC in v22, but PIC instead
      return ID3V2_APIC_FRAME;

    switch(frame->id[0])
    {
        case 'T':
            return ID3V2_TEXT_FRAME;
        case 'C':
            return ID3V2_COMMENT_FRAME;
        case 'A':
            return ID3V2_APIC_FRAME;
        default:
            return ID3V2_INVALID_FRAME;
    }
}

id3v2_frame_text_content* id3v2_parse_text_content_from_frame(id3v2_frame* frame)
{
    id3v2_frame_text_content* content;
    if(frame == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }
    
    content = id3v2_new_text_content();

    if(content==NULL)
      return NULL;

    content->encoding = frame->data[0];

    if(content->encoding==ID3V2_UTF_16_ENCODING_WITH_BOM && !has_bom(frame->data+ID3V2_FRAME_ENCODING))
      content->encoding = ID3V2_UTF_16_ENCODING_WITHOUT_BOM;

    content->size = frame->size - ID3V2_FRAME_ENCODING;
    content->data=frame->data + ID3V2_FRAME_ENCODING;

    E_SUCCESS;

    return content;
}

id3v2_frame_comment_content* id3v2_parse_comment_content_from_frame(id3v2_frame* frame)
{
    id3v2_frame_comment_content *content;
    int offset = 0;

    if(frame == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }
    
    content = id3v2_new_comment_content();

    if(content==NULL)
      return NULL;
    
    content->text->encoding = frame->data[0];
    offset += ID3V2_FRAME_ENCODING;

    memcpy(content->language, frame->data + offset, ID3V2_FRAME_LANGUAGE);
    offset += ID3V2_FRAME_LANGUAGE;

    if(content->text->encoding==ID3V2_UTF_16_ENCODING_WITH_BOM && !has_bom(frame->data + offset))
      content->text->encoding = ID3V2_UTF_16_ENCODING_WITHOUT_BOM;

    if(content->text->encoding==ID3V2_ISO_ENCODING || content->text->encoding == ID3V2_UTF_8_ENCODING)
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

    E_SUCCESS;
    
    return content;
}

int _get_mime_type_size_from_buffer(char* data)
{
    int i =0;

    while(data[i] != '\0')
    {
        i++;
    }
    
    return i;
}

id3v2_frame_apic_content* id3v2_parse_apic_content_from_frame(id3v2_frame* frame)
{
    id3v2_frame_apic_content *content;
    int offset = 1; // Skip ID3_FRAME_ENCODING

    if(frame == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }
    
    content = id3v2_new_apic_content();

    if(content==NULL)
      return NULL;
    
    content->encoding = frame->data[0];

    if(content->encoding==ID3V2_UTF_16_ENCODING_WITH_BOM && !has_bom(content->description))
      content->encoding = ID3V2_UTF_16_ENCODING_WITHOUT_BOM;

    content->mime_type = frame->data+offset;

    if(frame->version==ID3V2_2)
    {
      // id3 v2.2 mime types are always (!) exactly 3 bytes long, without the terminator
      content->mime_type_size = 3;
    }
    else
    {
      content->mime_type_size=_get_mime_type_size_from_buffer(content->mime_type);
    }

    offset += content->mime_type_size;

    content->picture_type = frame->data[++offset];

    content->description = frame->data+ ++offset;

    if (content->encoding == ID3V2_UTF_16_ENCODING_WITH_BOM || content->encoding==ID3V2_UTF_16_ENCODING_WITHOUT_BOM)
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

    E_SUCCESS;    

    return content;
}

void id3v2_add_frame_to_tag(id3v2_tag *tag, id3v2_frame *frame)
{
  id3v2_frame *last_frame;

  if((frame->version != ID3V2_2 && id3v2_get_tag_version(tag)==ID3V2_2) ||
     (frame->version == ID3V2_2 && id3v2_get_tag_version(tag)!=ID3V2_2))
  {
    E_FAIL(ID3V2_ERROR_INCOMPATIBLE_TAG);
    return;
  }

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

id3v2_frame *id3v2_get_frame_from_tag(id3v2_tag *tag, char *frame_id)
{
  char tmp_id[ID3V2_FRAME_ID];
  id3v2_frame *matching_frame;

  if(tag->frame == NULL)
  {
    // no frames in tag, return nothing
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return NULL;
  }

  matching_frame = tag->frame;

  while(matching_frame != NULL)
  {
    // we will copy the id here so we can adjust it if necessary
    memcpy(tmp_id, frame_id, ID3V2_FRAME_ID);

    if(matching_frame->version==ID3V2_2)
      // since id3 v22 only has 3-byte identifiers, we will replace the fourth sign (if available) with the empty sign
      tmp_id[3]='\0';

    if(memcmp(matching_frame->id, tmp_id, ID3V2_FRAME_ID)==0)
      break; 

    matching_frame = matching_frame->next;
  }

  if(matching_frame == NULL)
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
  else
    E_SUCCESS;

  return matching_frame;
}

char *_synchronize_data_from_buffer(char *data, int size)
{
  char check = 0; // indicated we'll have to inspect the next 2 bytes carefully
  int i;
  int sync_size = 0; // size of the synchronized data stream
  // at first allocating as much space as given into this function, if less is used we'll re-allocate later
  char *sync_data=(char *)malloc(size * sizeof(char));
 
  if(sync_data==NULL)
    return NULL;

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
  {
    sync_data = (char *)realloc(sync_data, sync_size);
    if(sync_data == NULL)
      return NULL;
  }

  return sync_data;
}
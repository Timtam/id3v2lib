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
    id3v2_frame* frame = id3v2_new_frame(tag, ID3V2_UNDEFINED_FRAME);
    int offset = 0;
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
       !isalnum(frame->id[2])))
    {
      free(frame);
      return NULL;
    }
    else if(version != ID3V2_2 && (
            !isalpha(frame->id[0]) ||
            !isalpha(frame->id[1]) ||
            !isalpha(frame->id[2]) ||
            !isalnum(frame->id[3])))
    {
      free(frame);
      return NULL;
    }

    frame->size = btoi(bytes, ID3V2_DECIDE_FRAME(version, ID3V2_FRAME_SIZE2, ID3V2_FRAME_SIZE), offset += ID3V2_DECIDE_FRAME(version, ID3V2_FRAME_ID2, ID3V2_FRAME_ID));
    if(version == ID3V2_4)
    {
        frame->size = syncint_decode(frame->size);
    }

    if(version != ID3V2_2) // flags are only available in v23 and 24 tags
    {
      memcpy(frame->flags, bytes + (offset += ID3V2_FRAME_SIZE), ID3V2_FRAME_FLAGS);
      offset += ID3V2_FRAME_FLAGS;

      // if some unknown flags are set, we ignore this frame since that actually means that the frame might not be parseable
      if(frame->flags[1]&(1<<7)==(1<<7) ||
         frame->flags[1]&(1<<5)==(1<<5) ||
         frame->flags[1]&(1<<4)==(1<<4))
      {
        frame->parsed = 0;
        return frame;
      }
    }
    else
      // just pushing the offset forward
      offset += ID3V2_FRAME_SIZE2;
    
    free(frame->data);

    // Load frame data
    frame->data= (char *)malloc(frame->size * sizeof(char));

    if(frame->data == NULL)
    {
      frame->parsed = 0;
      return frame;
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
        case 'X':
        case 'Y':
        case 'Z':
            return ID3V2_EXPERIMENTAL_FRAME;
        default:
            return ID3V2_INVALID_FRAME;
    }
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

void _synchronize_frame(id3v2_frame *frame)
{
  char check = 0; // indicated we'll have to inspect the next 2 bytes carefully
  int i;
  int sync_size = 0; // size of the synchronized data stream
  // at first allocating as much space as given into this function, if less is used we'll re-allocate later
  char *sync_data=(char *)malloc(frame->size * sizeof(char));
 
  if(sync_data==NULL)
    return;

  for(i = 0; i < frame->size; i++)
  {
    switch(check)
    {
      case 0:
        // check if we find the relevant syncbyte here
        if(frame->data[i]==0xFF)
          check = 1;
        sync_data[sync_size++] = frame->data[i];
        break;
      case 1:
        if(frame->data[i]!=0x00) // false alarm
        {
          check = 0;
          sync_data[sync_size++] = frame->data[i];
        }
        else // inspect even more
          check++;
        break;
      case 2:
        if(frame->data[i]!=0x00 && frame->data[i]&(111<<7)!=(111<<7))
        // false alarm, so we have to copy the previous byte at next and reset everything
          sync_data[sync_size++] = frame->data[i-1];
        sync_data[sync_size++] = frame->data[i];
        check = 0;
        break;
    }
  }  
  // if we successfully synchronized something, we can re-allocate some stuff here
  if(sync_size<frame->size)
  {
    sync_data = (char *)realloc(sync_data, sync_size);
    if(sync_data == NULL)
      return;
    free(frame->data);
    frame->data = sync_data;
    frame->size = sync_size;
  }
}

void id3v2_get_text_from_frame(id3v2_frame *frame, char **text, int *size, char *encoding)
{
  int offset = ID3V2_FRAME_ENCODING;

  if(frame == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  E_SUCCESS;

  *encoding = frame->data[0];

  switch(id3v2_get_frame_type(frame))
  {
    case ID3V2_TEXT_FRAME:
      *text = frame->data + offset;
      *size = frame->size - offset;
      break;
    case ID3V2_COMMENT_FRAME:
      offset += ID3V2_FRAME_LANGUAGE;
      if(*encoding == ID3V2_ISO_ENCODING ||
         *encoding == ID3V2_UTF_8_ENCODING)
        offset++;
      else
        offset += 4;
      *text = frame->data + offset;
      *size = frame->size - offset;
      break;
    case ID3V2_APIC_FRAME:
      offset += (frame->version == ID3V2_2 ? 3 : _get_mime_type_size_from_buffer(frame->data + offset));
      offset += 2; // 1 remaining mime type byte and 1 byte picture type
      *text = frame->data + offset;
      if (*encoding == ID3V2_UTF_16_ENCODING_WITH_BOM || *encoding==ID3V2_UTF_16_ENCODING_WITHOUT_BOM)
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
        offset++;
      }
      
      *size = (frame->data + offset) - (*text);
      break;
    default:
      E_FAIL(ID3V2_ERROR_UNSUPPORTED);
  }
}

void id3v2_get_language_from_frame(id3v2_frame *frame, char **language)
{
  if(frame==NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  if(id3v2_get_frame_type(frame)!=ID3V2_COMMENT_FRAME)
  {
    E_FAIL(ID3V2_ERROR_UNSUPPORTED);
    return;
  }

  *language = frame->data + ID3V2_FRAME_ENCODING;

  E_SUCCESS;

}

void id3v2_get_descriptor_from_frame(id3v2_frame *frame, char **descriptor, int *size)
{
  int offset = ID3V2_FRAME_ENCODING;

  if(frame == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  E_SUCCESS;

  switch(id3v2_get_frame_type(frame))
  {
    case ID3V2_COMMENT_FRAME:
      offset += ID3V2_FRAME_LANGUAGE;
      *descriptor = frame->data + offset;
      if(frame->data[0]==ID3V2_UTF_16_ENCODING_WITH_BOM)
        *size = 4;
      else if(frame->data[0] == ID3V2_UTF_16_ENCODING_WITHOUT_BOM)
        *size = 2;
      else
        *size = 1;
      break;
    case ID3V2_APIC_FRAME:
      offset += (frame->version == ID3V2_2 ? 3 : _get_mime_type_size_from_buffer(frame->data + offset)) + 1;
      *descriptor = frame->data + offset;
      *size = 1;
      break;
    default:
      E_FAIL(ID3V2_ERROR_UNSUPPORTED);    
  }
}

void id3v2_get_mime_type_from_frame(id3v2_frame *frame, char **mime_type, int *size)
{
  int offset = ID3V2_FRAME_ENCODING;

  if(frame == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  if(id3v2_get_frame_type(frame)!=ID3V2_APIC_FRAME)
  {
    E_FAIL(ID3V2_ERROR_UNSUPPORTED);
    return;
  }

  *mime_type = frame->data + offset;

  *size = (frame->version == ID3V2_2 ? 3 : _get_mime_type_size_from_buffer(frame->data + offset));

  E_SUCCESS;

}

void id3v2_get_picture_from_frame(id3v2_frame *frame, char **picture, int *size)
{
  char *description;
  int description_size;
  char encoding;

  if(frame == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  if(id3v2_get_frame_type(frame)!=ID3V2_APIC_FRAME)
  {
    E_FAIL(ID3V2_ERROR_UNSUPPORTED);
    return;
  }

  id3v2_get_text_from_frame(frame, &description, &description_size, &encoding);

  *picture = description + description_size;

  *size = (frame->data + frame->size) - (*picture);

  E_SUCCESS;

}

void id3v2_get_id_from_frame(id3v2_frame *frame, char **id, int *size)
{

  if(frame == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  *id = frame->id;

  if(frame->version == ID3V2_2)
    *size = ID3V2_FRAME_ID2;
  else
    *size = ID3V2_FRAME_ID;

  E_SUCCESS;

}

void id3v2_initialize_frame(id3v2_frame *frame, int type)
{
  char *data;
  int size;

  switch(type)
  {
    case ID3V2_UNDEFINED_FRAME:
      size = ID3V2_FRAME_ENCODING + 1;
      memset(frame->id, '\0', ID3V2_FRAME_ID);
      data=(char*)malloc(size*sizeof(char));
      if(data == NULL)
      {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return;
      }
      memset(data, 0, 2);
      break;
    case ID3V2_TEXT_FRAME:
      frame->id[0] = 'T';
      size = ID3V2_FRAME_ENCODING + 1;
      data=(char*)malloc(size*sizeof(char));
      if(data == NULL)
      {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return;
      }
      memset(data, 0, 2);
      break;
    case ID3V2_COMMENT_FRAME:
      frame->id[0] = 'C';
      size = ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE +2;
      data=(char*)malloc(size*sizeof(char));
      if(data == NULL)
      {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return;
      }
      data[0] = ID3V2_ISO_ENCODING;
      memcpy(data+ID3V2_FRAME_ENCODING, "eng", ID3V2_FRAME_LANGUAGE);
      memset(data+ID3V2_FRAME_ENCODING+ID3V2_FRAME_LANGUAGE, 0, 2);
      break;
    case ID3V2_APIC_FRAME:
      memcpy(frame->id, ID3V2_ALBUM_COVER_FRAME_ID(frame->version), ID3V2_DECIDE_FRAME(frame->version, ID3V2_FRAME_ID2, ID3V2_FRAME_ID));
      size = ID3V2_FRAME_ENCODING + ID3V2_DECIDE_FRAME(frame->version, 3, 10) +3;
      data=(char*)malloc(size*sizeof(char));
      if(data == NULL)
      {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return;
      }
      data[0] = ID3V2_ISO_ENCODING;
      memcpy(data+ID3V2_FRAME_ENCODING, ID3V2_DECIDE_FRAME(frame->version, "jpg", "image/jpg\0"), ID3V2_DECIDE_FRAME(frame->version, 3, 10));
      memset(data+ID3V2_DECIDE_FRAME(frame->version, 3, 10), 0, 3);
      break;
    default:
      E_FAIL(ID3V2_ERROR_UNSUPPORTED);
      return;
  }

  if(frame->data != NULL)
    free(frame->data);

  frame->size = size;
  frame->data = data;
  memset(frame->flags, 0, ID3V2_FRAME_FLAGS);

  E_SUCCESS;

}
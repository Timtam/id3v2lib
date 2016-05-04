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
    memcpy(frame->id, bytes + offset, ID3V2_DECIDE_FRAME(version, ID3V2_FRAME_SIZE2, ID3V2_FRAME_ID));

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
      offset += ID3V2_DECIDE_FRAME(frame->version, 3, strlen(frame->data + offset));
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

char *id3v2_get_language_from_frame(id3v2_frame *frame)
{
  if(frame==NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return NULL;
  }

  if(id3v2_get_frame_type(frame)!=ID3V2_COMMENT_FRAME)
  {
    E_FAIL(ID3V2_ERROR_UNSUPPORTED);
    return NULL;
  }

  E_SUCCESS;

  return frame->data + ID3V2_FRAME_ENCODING;

}

char id3v2_get_descriptor_from_frame(id3v2_frame *frame)
{
  int offset = ID3V2_FRAME_ENCODING;

  if(frame == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return 0;
  }

  E_SUCCESS;

  switch(id3v2_get_frame_type(frame))
  {
    case ID3V2_COMMENT_FRAME:
      offset += ID3V2_FRAME_LANGUAGE;
      if(has_bom(frame->data + offset))
        offset += 2;
      return frame->data[offset];
    case ID3V2_APIC_FRAME:
      offset += ID3V2_DECIDE_FRAME(frame->version, 3, strlen(frame->data + offset) + 1);
      return frame->data[offset];
    default:
      E_FAIL(ID3V2_ERROR_UNSUPPORTED);    
      return 0;
  }
}

void id3v2_get_picture_from_frame(id3v2_frame *frame, char **picture, int *size, char **mime_type)
{
  char *description;
  int description_size;
  char encoding;
  char *mime_type_buffer;

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

  if(frame->version == ID3V2_2)
  {
    if(memcmp(frame->data + ID3V2_FRAME_ENCODING, ID3V2_JPG_MIME_TYPE2, 3)==0)
      mime_type_buffer=(char*)malloc((strlen(ID3V2_JPG_MIME_TYPE)+1)*sizeof(char));
    else
      mime_type_buffer=(char*)malloc((strlen(ID3V2_PNG_MIME_TYPE)+1)*sizeof(char));
    if(mime_type_buffer == NULL)
    {
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      *size = 0;
      return;
    }
    if(_add_allocation_to_tag(frame->tag, mime_type_buffer)==0)
    {
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      return;
    }
    if(memcmp(frame->data + ID3V2_FRAME_ENCODING, ID3V2_JPG_MIME_TYPE2, 3)==0)
      memcpy(mime_type_buffer, ID3V2_JPG_MIME_TYPE, strlen(ID3V2_JPG_MIME_TYPE)+1);
    else
      memcpy(mime_type_buffer, ID3V2_PNG_MIME_TYPE, strlen(ID3V2_PNG_MIME_TYPE)+1);
    *mime_type = mime_type_buffer;
  }
  else
    *mime_type = frame->data + ID3V2_FRAME_ENCODING;

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
      memcpy(frame->id, ID3V2_GET_ALBUM_COVER_FRAME_ID_FROM_TAG(frame->tag), ID3V2_DECIDE_FRAME(frame->version, ID3V2_FRAME_ID2, ID3V2_FRAME_ID));
      size = ID3V2_FRAME_ENCODING + ID3V2_DECIDE_FRAME(frame->version, 3, 10) +3;
      data=(char*)malloc(size*sizeof(char));
      if(data == NULL)
      {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return;
      }
      data[0] = ID3V2_ISO_ENCODING;
      memcpy(data+ID3V2_FRAME_ENCODING, ID3V2_GET_PNG_MIME_TYPE_FROM_FRAME(frame), ID3V2_DECIDE_FRAME(frame->version, 3, 10));
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

void id3v2_set_text_to_frame(id3v2_frame *frame, char *text, int size, char encoding)
{
  char *data;
  int f_size;
  char original_encoding;
  int original_size;
  char *original_text;

  if(encoding != ID3V2_ISO_ENCODING &&
     encoding != ID3V2_UTF_16_ENCODING_WITH_BOM &&
     encoding != ID3V2_UTF_16_ENCODING_WITHOUT_BOM &&
     encoding != ID3V2_UTF_8_ENCODING)
    {
      E_FAIL(ID3V2_ERROR_WRONG_ENCODING);
      return;
    }

  if(frame->version == ID3V2_3 && encoding == ID3V2_UTF_8_ENCODING)
  {
    E_FAIL(ID3V2_ERROR_WRONG_ENCODING);
    return;
  }

  if(frame->version == ID3V2_2 && (
     encoding != ID3V2_ISO_ENCODING &&
     encoding != ID3V2_UTF_16_ENCODING_WITH_BOM))
  {
    E_FAIL(ID3V2_ERROR_WRONG_ENCODING);
    return;
  }

  switch(id3v2_get_frame_type(frame))
  {
    case ID3V2_TEXT_FRAME:
      f_size = size + ID3V2_FRAME_ENCODING;
      data=(char*)malloc(f_size*sizeof(char));
      if(data == NULL)
      {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return;
      }
      memcpy(data+ID3V2_FRAME_ENCODING, text, size);
      break;
    case ID3V2_COMMENT_FRAME:
      f_size = ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + size;
      if(encoding == ID3V2_UTF_16_ENCODING_WITH_BOM)
        f_size += 4;
      else if(encoding == ID3V2_UTF_16_ENCODING_WITHOUT_BOM)
        f_size += 2;
      else
        f_size++;
      data=(char*)malloc(f_size * sizeof(char));
      if(data == NULL)
      {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return;
      }
      memcpy(data, frame->data, ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE);
      data[0] = encoding;
      switch(frame->data[0])
      {
        case ID3V2_ISO_ENCODING:
        case ID3V2_UTF_8_ENCODING:
          if(encoding == ID3V2_ISO_ENCODING ||
             encoding == ID3V2_UTF_8_ENCODING)
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE] = frame->data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE];
          else if(encoding == ID3V2_UTF_16_ENCODING_WITH_BOM)
          {
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE] = 0xFF;
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + 1] = 0xFE;
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + 2] = frame->data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE];
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + 3] = 0;
          }
          else if(encoding == ID3V2_UTF_16_ENCODING_WITHOUT_BOM)
          {
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE] = frame->data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE];
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + 1] = 0;
          }
          break;
        case ID3V2_UTF_16_ENCODING_WITH_BOM:
          if(encoding == ID3V2_ISO_ENCODING ||
             encoding == ID3V2_UTF_8_ENCODING)
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE] = frame->data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + 2];
          else if(encoding == ID3V2_UTF_16_ENCODING_WITH_BOM)
            memcpy(data+ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE, frame->data + ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE, 4);
          else if(encoding == ID3V2_UTF_16_ENCODING_WITHOUT_BOM)
            memcpy(data+ID3V2_FRAME_ENCODING+ID3V2_FRAME_LANGUAGE, frame->data+ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + 2, 2);
          break;
        case ID3V2_UTF_16_ENCODING_WITHOUT_BOM:
          if(encoding == ID3V2_ISO_ENCODING ||
             encoding == ID3V2_UTF_8_ENCODING)
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE] = frame->data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE];
          else if(encoding == ID3V2_UTF_16_ENCODING_WITH_BOM)
          {
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE] = 0xFF;
            data[ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + 1] = 0xFE;
            memcpy(data+ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE + 2, frame->data+ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE, 2);
          }
          else if(encoding == ID3V2_UTF_16_ENCODING_WITHOUT_BOM)
            memcpy(data+ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE, frame->data + ID3V2_FRAME_ENCODING + ID3V2_FRAME_LANGUAGE, 2);
          break;
      }
      memcpy(data+(f_size-size), text, size);
      break;
    case ID3V2_APIC_FRAME:
      id3v2_get_text_from_frame(frame, &original_text, &original_size, &original_encoding);
      if(E_GET != ID3V2_OK)
        return;
      f_size = ID3V2_FRAME_ENCODING + (original_text - frame->data) + ((frame->data + frame->size) - (original_text + original_size));
      data=(char*)malloc(f_size * sizeof(char));
      if(data == NULL)
      {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return;
      }
      memcpy(data, frame->data, original_text - frame->data);
      memcpy(data+(original_text - frame->data), text, size);
      memcpy(data+(original_text - frame->data)+size, original_text + original_size, (frame->data + frame->size) - (original_text + original_size));
      data[0] = encoding;
      break;
    default:
      E_FAIL(ID3V2_ERROR_UNSUPPORTED);
      return;
  }

  frame->size = f_size;
  if(frame->data != NULL)
    free(frame->data);

  frame->data = data;

  E_SUCCESS;

}

void id3v2_set_language_to_frame(id3v2_frame *frame, char *language)
{
  if(frame == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  if(id3v2_get_frame_type(frame)!=ID3V2_COMMENT_FRAME)
  {
    E_FAIL(ID3V2_ERROR_UNSUPPORTED);
    return;
  }

  memcpy(frame->data+ID3V2_FRAME_ENCODING, language, 3);

  E_SUCCESS;
}

void id3v2_set_id_to_frame(id3v2_frame *frame, char *id)
{
  if(frame == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  memcpy(frame->id, id, ID3V2_DECIDE_FRAME(frame->version, ID3V2_FRAME_ID2, ID3V2_FRAME_ID));

  E_SUCCESS;
}

void id3v2_set_descriptor_to_frame(id3v2_frame *frame, char descriptor)
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
    case ID3V2_APIC_FRAME:
      offset += ID3V2_DECIDE_FRAME(frame->version, 3, strlen(frame->data + offset) + 1);
      frame->data[offset] = descriptor;
      break;
    case ID3V2_COMMENT_FRAME:
      offset += ID3V2_FRAME_LANGUAGE;
      if(has_bom(frame->data + offset))
        offset += 2;
      frame->data[offset] = descriptor;
      if(frame->data[0] != ID3V2_UTF_8_ENCODING &&
         frame->data[0] != ID3V2_ISO_ENCODING)      
        frame->data[offset + 1] = 0;
      break;
    default:
      E_FAIL(ID3V2_ERROR_UNSUPPORTED);
  }
}

void _free_frame(id3v2_frame *frame)
{

  if(frame == NULL)
    return;

  free(frame->data);

  free(frame);
}

void id3v2_set_picture_to_frame(id3v2_frame *frame, char *picture, int size)
{
  char *data;
  int n_size=ID3V2_FRAME_ENCODING;
  char *o_description;
  char o_encoding;
  char *o_mime_type;
  char *o_picture;
  int o_description_size;
  int o_picture_size;

  if(frame==NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  if(id3v2_get_frame_type(frame)!=ID3V2_APIC_FRAME)
  {
    E_FAIL(ID3V2_ERROR_UNSUPPORTED);
    return;
  }

  if(memcmp(_get_mime_type_from_buffer(picture, size), 0, 1)==0)
  {
    E_FAIL(ID3V2_ERROR_UNKNOWN_MIME_TYPE);
    return;
  }

  n_size += strlen(_get_mime_type_from_buffer(picture, size))+2;
  
  id3v2_get_text_from_frame(frame, &o_description, &o_description_size, &o_encoding);

  if(E_GET != ID3V2_OK)
    return;

  n_size += o_description_size;
  n_size += size;

  id3v2_get_picture_from_frame(frame, &o_picture, &o_picture_size, &o_mime_type);

  if(E_GET != ID3V2_OK)
    return;

  data=(char*)malloc(n_size*sizeof(char));

  if(data==NULL)
  {
    E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
    return;
  }

  data[0] = o_encoding;

  memcpy(data+ID3V2_FRAME_ENCODING, _get_mime_type_from_buffer(picture, size), n_size - size - 1 - ID3V2_FRAME_ENCODING - o_description_size);

  data[n_size - size - o_description_size - 1] = id3v2_get_descriptor_from_frame(frame);

  memcpy(data + (n_size - size - o_description_size), o_description, o_description_size);

  memcpy(data+(n_size - size), picture, size);

  if(frame->data != NULL)
    free(frame->data);

  frame->data = data;

  frame->size = n_size;

  E_SUCCESS;

}
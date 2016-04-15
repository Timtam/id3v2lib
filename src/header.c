/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id3v2lib.h"

int _has_header_id3v2tag(id3v2_header* tag_header)
{

  if(tag_header == NULL)
    return 0;

  return _has_buffer_id3v2tag(tag_header->tag);
}

int _has_buffer_id3v2tag(char* raw_header)
{
    if(memcmp(raw_header, "ID3", 3) == 0)
    {
        return 1;
    }

    return 0;
}

id3v2_header* _get_header_from_file(FILE *file, int offset)
{
    char buffer[ID3V2_HEADER];

    if(file == NULL)
    {
        E_FAIL(ID3V2_ERROR_UNABLE_TO_OPEN);
        return NULL;
    }

    fseek(file, offset, SEEK_SET);

    fread(buffer, ID3V2_HEADER, 1, file);
    return _get_header_from_buffer(buffer, ID3V2_HEADER);
}

id3v2_header* _get_header_from_buffer(char *buffer, int length)
{
    int position = 0;
    id3v2_header *tag_header;

    if(length < ID3V2_HEADER) {
        return NULL;
    }
    if( ! _has_buffer_id3v2tag(buffer))
    {
        return NULL;
    }
    tag_header = _new_header();

    memcpy(tag_header->tag, buffer, ID3V2_HEADER_TAG);
    tag_header->major_version = buffer[position += ID3V2_HEADER_TAG];
    tag_header->minor_version = buffer[position += ID3V2_HEADER_VERSION];
    tag_header->flags = buffer[position += ID3V2_HEADER_REVISION];

    // checking if the as unused declared flags are set in any way
    // this would mean we stop parsing here, since we might encounter things we don't know how to handle
    if(tag_header->flags&(1<<3)==(1<<3) ||
       tag_header->flags&(1<<2)==(1<<2) ||
       tag_header->flags&(1<<1)==(1<<1) ||
       tag_header->flags&1==1)
    {
      free(tag_header);
      return NULL;
    }

    tag_header->tag_size = syncint_decode(btoi(buffer, ID3V2_HEADER_SIZE, position += ID3V2_HEADER_FLAGS));

    if(tag_header->flags&(1<<6)==(1<<6))
      // an extended header exists, so we retrieve the actual size of it and save it into the struct
      tag_header->extended_header_size = syncint_decode(btoi(buffer, ID3V2_EXTENDED_HEADER_SIZE, position += ID3V2_HEADER_SIZE));
    else
      // no extended header existing
      tag_header->extended_header_size = 0;

    if(tag_header->flags&(1<<4)==(1<<4))
      // footer detected, adding the size
      tag_header->tag_size += 10;

    return tag_header;
}

int id3v2_get_tag_version(id3v2_tag *tag)
{
  if(tag==NULL || tag->header == NULL)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return ID3V2_NO_COMPATIBLE_TAG;
  }

  E_SUCCESS;

  if(tag->header->major_version == 3)
  {
    return ID3V2_3;
  }
  else if(tag->header->major_version == 4)
  {
    return ID3V2_4;
  }
  else if(tag->header->major_version == 2)
  {
    return ID3V2_2;
  }
  else
  {
    return ID3V2_NO_COMPATIBLE_TAG;
  }
}

void _find_header_offsets_in_file(FILE *file, int **location, int *size)
{
  char byte; // currently processing byte
  id3v2_header *header;
  char *header_bytes; // used to store the header bytes found
  int *offsets;
  char pattern_position = 0; // amount of pattern bytes found
  char scanning = 1; // still scanning?

  *size = 0;

  header_bytes=(char *)malloc(10*sizeof(char));

  if(header_bytes==NULL)
  {
    return;
  }

  offsets=(int*)malloc(sizeof(int));

  if(offsets==NULL)
  {
    free(header_bytes);
    return;
  }

  fseek(file, 0, SEEK_SET);

  while( (byte = fgetc(file)) != EOF && scanning)
  {
    switch(pattern_position)
    {
      case 0:
        if(byte=='I')
          pattern_position++;
        break;
      case 1:
        if(byte=='D')
          pattern_position++;
        else
          pattern_position = 0;
        break;
      case 2:
        if(byte=='3')
          pattern_position++;
        else
          pattern_position = 0;
        break;
      case 3:
      case 4:
        if(byte<0xFF)
          pattern_position++;
        else
          pattern_position = 0;
        break;
      case 5:
        pattern_position++;
        break;
      case 6:
      case 7:
      case 8:
      case 9:
        if(byte<0x80)
          pattern_position++;
        else
          pattern_position = 0;
        break;
      case 10: // here the actually interesting part begins
        // we successfully found an id3 tag
        pattern_position = 0;
        fseek(file, -11, SEEK_CUR);
        // so we get the header bytes and parse them to find out if we actually found a parseable header
        fgets(header_bytes, 10, file);
        header=_get_header_from_buffer(header_bytes, 10);
        if(header==NULL)
          continue;
        // we successfully found something useful
        offsets[*size] = ftell(file)-9;
        (*size)++;
        offsets=realloc(offsets, ((*size)+1)*sizeof(int));
        if(offsets==NULL)
        {
          free(header_bytes);
          *size = 0;
          return;
        }
        if(fseek(file, header->tag_size+1, SEEK_CUR)!=0)
          // seems like the tag isn't fully contained in this file, otherwise this shouldn't happen
          scanning = 0;
        break;
    }
  }

  if( *size > 0)
  {
    offsets=(int *)realloc(offsets, (*size)*sizeof(int));
    if(offsets==NULL)
    {
      free(header_bytes);
      *size = 0;
      return;
    }
    *location = offsets;
  }

  free(header_bytes);
  return;
}
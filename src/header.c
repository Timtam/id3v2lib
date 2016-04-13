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

int has_id3v2tag(ID3v2_header* tag_header)
{
    if(memcmp(tag_header->tag, "ID3", 3) == 0)
    {
        return 1;
    }

    return 0;
}

int _has_id3v2tag(char* raw_header)
{
    if(memcmp(raw_header, "ID3", 3) == 0)
    {
        return 1;
    }

    return 0;
}

ID3v2_header* get_tag_header_from_file(FILE *file, int offset)
{
    char buffer[ID3_HEADER];
    if(file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, offset, SEEK_SET);

    fread(buffer, ID3_HEADER, 1, file);
    return get_tag_header_with_buffer(buffer, ID3_HEADER);
}
ID3v2_header* get_tag_header_with_buffer(char *buffer, int length)
{
    int position = 0;
    ID3v2_header *tag_header;

    if(length < ID3_HEADER) {
        return NULL;
    }
    if( ! _has_id3v2tag(buffer))
    {
        return NULL;
    }
    tag_header = new_header();

    memcpy(tag_header->tag, buffer, ID3_HEADER_TAG);
    tag_header->major_version = buffer[position += ID3_HEADER_TAG];
    tag_header->minor_version = buffer[position += ID3_HEADER_VERSION];
    tag_header->flags = buffer[position += ID3_HEADER_REVISION];

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

    tag_header->tag_size = syncint_decode(btoi(buffer, ID3_HEADER_SIZE, position += ID3_HEADER_FLAGS));

    if(tag_header->flags&(1<<6)==(1<<6))
    {
      // an extended header exists, so we retrieve the actual size of it and save it into the struct
      tag_header->extended_header_size = syncint_decode(btoi(buffer, ID3_EXTENDED_HEADER_SIZE, position += ID3_HEADER_SIZE));
    }
    else
    {
      // no extended header existing
      tag_header->extended_header_size = 0;
    }

    return tag_header;
}

int get_tag_version(ID3v2_header* tag_header)
{
    if(tag_header->major_version == 3)
    {
        return ID3v23;
    }
    else if(tag_header->major_version == 4)
    {
        return ID3v24;
    }
    else if(tag_header->major_version == 2)
    {
      return ID3v22;
    }
    else
    {
        return NO_COMPATIBLE_TAG;
    }
}

void find_headers_in_file(FILE *file, int **location, int *size)
{
  char byte; // currently processing byte
  int count = 0; // amount of offsets found
  ID3v2_header *header;
  char *header_bytes; // used to store the header bytes found
  int *offsets;
  char pattern_position = 0; // amount of pattern bytes found
  char scanning = 1; // still scanning?

  header_bytes=(char *)malloc(10*sizeof(char));

  if(header_bytes==NULL)
  {
    *size = 0;
    return;
  }

  offsets=(int*)malloc(sizeof(int));

  if(offsets==NULL)
  {
    free(header_bytes);
    *size = 0;
    return;
  }

  fseek(file, 0, SEEK_SET);

  while( (byte = fgetc(file)) != EOF && scanning)
  {
    switch(pattern_position)
    {
      case 0:
        if(byte=='I' || byte=='i')
          pattern_position++;
        break;
      case 1:
        if(byte=='D' || byte=='d')
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
        header=get_tag_header_with_buffer(header_bytes, 10);
        if(header==NULL)
          continue;
        // we successfully found something useful
        offsets[count] = ftell(file)-9;
        count++;
        offsets=realloc(offsets, (count+1)*sizeof(int));
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

  if( count > 0)
  {
    offsets=(int *)realloc(offsets, count*sizeof(int));
    if(offsets==NULL)
    {
      free(header_bytes);
      *size = 0;
      return;
    }
    *location = offsets;
    *size = count;
  }
  else
    *size = 0;

  free(header_bytes);
  return;
}
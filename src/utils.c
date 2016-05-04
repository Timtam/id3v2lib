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

unsigned int btoi(char* bytes, int size, int offset)
{
    unsigned int result = 0x00;
    int i = 0;
    for(i = 0; i < size; i++)
    {
        result = result << 8;
        result = result | (unsigned char) bytes[offset + i];
    }
    
    return result;
}

char* itob(int integer)
{
    int i;
    int size = 4;
    char* result = (char*) malloc(sizeof(char) * size);
    
    // We need to reverse the bytes because Intel uses little endian.
    char* aux = (char*) &integer;
    for(i = size - 1; i >= 0; i--)
    {
        result[size - 1 - i] = aux[i];
    }
    
    return result;
}

int syncint_encode(int value)
{
    int out, mask = 0x7F;
    
    while (mask ^ 0x7FFFFFFF) {
        out = value & ~mask;
        out <<= 1;
        out |= value & mask;
        mask = ((mask + 1) << 8) - 1;
        value = out;
    }
    
    return out;
}

int syncint_decode(int value)
{
    unsigned int a, b, c, d, result = 0x0;
    a = value & 0xFF;
    b = (value >> 8) & 0xFF;
    c = (value >> 16) & 0xFF;
    d = (value >> 24) & 0xFF;
    
    result = result | a;
    result = result | (b << 7);
    result = result | (c << 14);
    result = result | (d << 21);
    
    return result;
}

void id3v2_free_tag(id3v2_tag* tag)
{
    id3v2_frame *frame;
    int i;
    id3v2_frame *next_frame;

    free(tag->header);
    frame = tag->frame;
    while(frame != NULL)
    {
        next_frame = frame->next;
        _free_frame(frame);
        frame=next_frame;
    }

    for(i=0;i<tag->allocation_count;i++)
      free(tag->allocations[i]);

    free(tag);

    E_SUCCESS;
    
}

// String functions
int has_bom(char *string)
{
    if(memcmp("\xFF\xFE", string, 2) == 0 || memcmp("\xFE\xFF", string, 2) == 0)
    {   
        return 1;
    }
    
    return 0;
}


const char *_get_mime_type_from_buffer(char *data, int size)
{
  if(data[0]==0xFF && data[1]==0xD8 && data[size-2]==0xFF && data[size-1]==0xD9)
    return ID3V2_JPG_MIME_TYPE;
  else if(data[0]==0x89 && data[1]==0x50 && data[2]==0x4E && data[3]==0x47 && data[4]==0x0D && data[5]==0x0A && data[6]==0x1A && data[7]==0x0A)
    return ID3V2_PNG_MIME_TYPE;
  return "";
}
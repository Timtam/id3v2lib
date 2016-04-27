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

id3v2_tag* id3v2_load_tag_from_file(FILE *file)
{
    char *buffer;
    int count;
    id3v2_header *header;
    int *offsets;
    id3v2_tag *tag;
    int tag_size;

    if(file==NULL)
    {
      E_FAIL(ID3V2_ERROR_UNABLE_TO_OPEN);
      return NULL;
    }

    // parse file for id3 tags
    _find_header_offsets_in_file(file, &offsets, &count);

    // no headers found?
    if(count==0)
    {
      E_FAIL(ID3V2_ERROR_NOT_FOUND);
      return NULL;
    }

    // todo: process multiple tags in files consequently, meaning
      // tag appending
      // tag updating
      // tag replacing
    // for now, we will just take the first tag found in the file

    header = _get_header_from_file(file, offsets[0]);

    if(header==NULL)
    {
      // whatever went wrong here, since we already found a header there
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      return NULL;
    }

    // get header size
    tag_size = header->tag_size;
    free(header);

    // allocate buffer and fetch header
    buffer = (char*) malloc((10+tag_size) * sizeof(char));
    if(buffer == NULL)
    {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }

    fseek(file, offsets[0], SEEK_SET);

    fread(buffer, tag_size+10, 1, file);
    free(offsets);

    //parse free and return
    tag = id3v2_load_tag_from_buffer(buffer, tag_size+10);
    free(buffer);

    return tag;
}

void id3v2_load_tags_from_file(FILE *file, id3v2_tag ***tags, int *count)
{
  char *buffer;
  id3v2_header *header;
  int i;
  int *offsets;
  int tag_size;

  if(file == NULL)
  {
    E_FAIL(ID3V2_ERROR_UNABLE_TO_OPEN);
    *count = 0;
    return;
  }

  _find_header_offsets_in_file(file, &offsets, count);

  if(*count == 0)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  *tags= (id3v2_tag **)malloc((*count)*sizeof(id3v2_tag *));

  if(*tags == NULL)
  {
    E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
    *count = 0;
    return;
  }

  for(i = 0; i < *count; i++)
  {
    fseek(file, offsets[i], SEEK_SET);

    header = _get_header_from_file(file, offsets[i]);

    if(header == NULL)
    {
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      *count = 0;
      return;
    }

    tag_size = header->tag_size;

    free(header);

    fseek(file, -10, SEEK_CUR);

    buffer = (char*)malloc((tag_size+10) * sizeof(char));

    if(buffer == NULL)
    {
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      *count = 0;
      return;
    }

    fread(buffer, tag_size+10, 1, file);

    (*tags)[i]=id3v2_load_tag_from_buffer(buffer, tag_size+10);

    if((*tags)[i] == NULL)
    {
      *count = 0;
      return;
    }

    free(buffer);

  }

  free(offsets);

  E_SUCCESS;

  return;

}

void id3v2_load_tags_from_buffer(char *buffer, int length, id3v2_tag ***tags, int *count)
{
  int i;
  int *offsets;

  _find_header_offsets_in_buffer(buffer, length, &offsets, count);

  if(*count == 0)
  {
    E_FAIL(ID3V2_ERROR_NOT_FOUND);
    return;
  }

  *tags= (id3v2_tag **)malloc((*count)*sizeof(id3v2_tag *));

  if(*tags == NULL)
  {
    E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
    *count = 0;
    return;
  }

  for(i = 0; i < *count; i++)
  {

    (*tags)[i]=id3v2_load_tag_from_buffer(buffer+offsets[i], length-offsets[i]);

    if((*tags)[i] == NULL)
    {
      *count = 0;
      return;
    }

  }

  free(offsets);

  E_SUCCESS;

  return;

}

id3v2_tag* id3v2_load_tag_from_buffer(char *bytes, int length)
{
    // Declaration
    char *c_bytes;
    id3v2_frame *frame;
    id3v2_tag* tag;
    id3v2_header* tag_header;

    // Initialization
    tag_header = _get_header_from_buffer(bytes, length);

    if(tag_header == NULL) // no valid header found
    {
      E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
      return NULL;
    }

    if(length < tag_header->tag_size+10)
    {
        // Not enough bytes provided to parse completely.
        E_FAIL(ID3V2_ERROR_INSUFFICIENT_DATA);
        free(tag_header);
        return NULL;
    }

    tag = id3v2_new_tag();

    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_MEMORY_ALLOCATION);
        return NULL;
    }

    free(tag->header);

    // Associations
    tag->header = tag_header;

    if(id3v2_get_tag_version(tag) == ID3V2_NO_COMPATIBLE_TAG)
    {
        // no supported id3 tag found
        E_FAIL(ID3V2_ERROR_INCOMPATIBLE_TAG);
        id3v2_free_tag(tag);
        return NULL;
    }

    // move the bytes pointer to the correct position
    bytes+=10; // skip header
    if(tag_header->extended_header_size)
      // an extended header exists, so we skip it too
      bytes+=tag_header->extended_header_size+4; // don't forget to skip the extended header size bytes too

    c_bytes = bytes;
    
    while((bytes - c_bytes) < tag_header->tag_size)
    {
      frame=_parse_frame_from_tag(tag, bytes);
      if(frame != NULL) // a frame was found
      {
        bytes += frame->size + 10;
        if(frame->parsed) // and it got parsed
          // detect unsynchronization and reverse it if needed
          if(tag_header->flags&(1<<7)==(1<<7) ||
             frame->flags[1]&(1<<1)==(1<<1))
            _synchronize_frame(frame);
        else
        {
          if(frame->data != NULL)
            free(frame->data);
          free(frame);
        }
      }
      else
        break;
    }

    E_SUCCESS;

    return tag;
}

/* for now commented out, will be edited later
void remove_tag(const char* file_name)
{
    int c;
    FILE* file;
    FILE* temp_file;
    ID3v2_header* tag_header;

    tag_header = get_tag_header(file_name);
    if(tag_header == NULL)
    {
        return;
    }

    file=fopen(file_name, "rb");
    temp_file = tmpfile();

    fseek(file, tag_header->tag_size + 10, SEEK_SET);
    while((c = getc(file)) != EOF)
    {
        putc(c, temp_file);
    }

    // Write temp file data back to original file
    fseek(temp_file, 0, SEEK_SET);
    // we need to open file new since it is readonly for now
    fclose(file);
    file=fopen(file_name, "wb");

    while((c = getc(temp_file)) != EOF)
    {
        putc(c, file);
    }

    fclose(file);
    fclose(temp_file);

}

void write_header(id3v2_header* tag_header, FILE* file)
{
    fwrite("ID3", 3, 1, file);
    fwrite(&tag_header->major_version, 1, 1, file);
    fwrite(&tag_header->minor_version, 1, 1, file);
    fwrite(&tag_header->flags, 1, 1, file);
    fwrite(itob(syncint_encode(tag_header->tag_size)), 4, 1, file);
}

void write_frame(ID3v2_frame* frame, FILE* file)
{
    fwrite(frame->frame_id, 1, (frame->version==ID3v22 ? ID3_FRAME_SIZE2 : ID3_FRAME_SIZE), file);
    fwrite(itob(frame->size), 1, (frame->version==ID3v22 ? ID3_FRAME_SIZE2 : ID3_FRAME_SIZE), file);
    if(frame->version!=ID3v22)
      fwrite(frame->flags, 1, 2, file);
    fwrite(frame->data, 1, frame->size, file);
}

int get_tag_size(ID3v2_tag* tag)
{
    int size = 0;
    ID3v2_frame *frame;

    if(tag->frame == NULL)
    {
        return size;
    }

    frame = tag->frame;
    while(frame != NULL)
    {
        size += frame->size + (frame->version==ID3v22 ? 6 : 10);
        frame = frame->next;
    }

    return size;
}

void set_tag(const char* file_name, ID3v2_tag* tag)
{
    int c;
    FILE *file;
    ID3v2_frame *frame;
    int i;
    int padding = 2048;
    int old_size;
    FILE* temp_file;

    if(tag == NULL)
    {
        return;
    }

    old_size = tag->tag_header->tag_size;

    // Set the new tag header
    tag->tag_header = _new_header();
    memcpy(tag->tag_header->tag, "ID3", 3);
    tag->tag_header->major_version = '\x03';
    tag->tag_header->minor_version = '\x00';
    tag->tag_header->flags = '\x00';
    tag->tag_header->tag_size = get_tag_size(tag) + padding;

    // Create temp file and prepare to write
    file = fopen(file_name, "r+b");
    temp_file = tmpfile();

    // Write to file
    write_header(tag->tag_header, temp_file);
    frame = tag->frame;
    while(frame != NULL)
    {
        write_frame(frame, temp_file);
        frame = frame->next;
    }

    // Write padding
    for(i = 0; i < padding; i++)
    {
        putc('\x00', temp_file);
    }

    fseek(file, old_size + 10, SEEK_SET);
    while((c = getc(file)) != EOF)
    {
        putc(c, temp_file);
    }

    // Write temp file data back to original file
    fseek(temp_file, 0, SEEK_SET);
    fseek(file, 0, SEEK_SET);
    while((c = getc(temp_file)) != EOF)
    {
        putc(c, file);
    }

    fclose(file);
    fclose(temp_file);
}
*/

/**
 * Getter functions
 */
id3v2_frame* id3v2_get_title_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_TITLE_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_artist_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_ARTIST_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_album_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_ALBUM_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_album_artist_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_ALBUM_ARTIST_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_genre_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_GENRE_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_track_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_TRACK_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_year_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_YEAR_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_comment_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_COMMENT_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_disc_number_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_DISC_NUMBER_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_composer_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_COMPOSER_FRAME_ID(id3v2_get_tag_version(tag)));
}

id3v2_frame* id3v2_get_album_cover_frame_from_tag(id3v2_tag* tag)
{
    if(tag == NULL)
    {
        E_FAIL(ID3V2_ERROR_NOT_FOUND);
        return NULL;
    }

    return id3v2_get_frame_from_tag(tag, ID3V2_ALBUM_COVER_FRAME_ID(id3v2_get_tag_version(tag)));
}

/**
 * Setter functions

 * since rebuilding commented out
void set_text_frame(char* data, char encoding, char* frame_id, ID3v2_frame* frame)
{
    // Set frame id and size
    memcpy(frame->frame_id, frame_id, 4);
    frame->size = 1 + (int) strlen(data);

    // Set frame data
    // TODO: Make the encoding param relevant.
    frame->data = (char*) malloc(frame->size * sizeof(char));

    sprintf(frame->data, "%c%s", encoding, data);
}

void set_comment_frame(char* data, char encoding, ID3v2_frame* frame)
{
    memcpy(frame->frame_id, COMMENT_FRAME_ID(frame->version), 4);
    frame->size = 1 + 3 + 1 + (int) strlen(data); // encoding + language + description + comment

    frame->data = (char*) malloc(frame->size * sizeof(char));

    sprintf(frame->data, "%c%s%c%s", encoding, "eng", '\x00', data);
}

void set_album_cover_frame(char* album_cover_bytes, char* mimetype, int picture_size, ID3v2_frame* frame)
{
    int offset;

    memcpy(frame->frame_id, ALBUM_COVER_FRAME_ID(frame->version), 4);
    frame->size = 1 + (int) strlen(mimetype) + 1 + 1 + 1 + picture_size; // encoding + mimetype + 00 + type + description + picture

    frame->data = (char*) malloc(frame->size * sizeof(char));

    offset = 1 + (int) strlen(mimetype) + 1 + 1 + 1;
    sprintf(frame->data, "%c%s%c%c%c", '\x00', mimetype, '\x00', FRONT_COVER, '\x00');
    memcpy(frame->data + offset, album_cover_bytes, picture_size);
}

void tag_set_title(char* title, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* title_frame = NULL;
    if( ! (title_frame = tag_get_title(tag)))
    {
        title_frame = new_frame();
        add_frame(tag, title_frame);
    }

    set_text_frame(title, encoding, TITLE_FRAME_ID(get_tag_version(tag->tag_header)), title_frame);
}

void tag_set_artist(char* artist, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* artist_frame = NULL;
    if( ! (artist_frame = tag_get_artist(tag)))
    {
        artist_frame = new_frame();
        add_frame(tag, artist_frame);
    }

    set_text_frame(artist, encoding, ARTIST_FRAME_ID(get_tag_version(tag->tag_header)), artist_frame);
}

void tag_set_album(char* album, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* album_frame = NULL;
    if( ! (album_frame = tag_get_album(tag)))
    {
        album_frame = new_frame();
        add_frame(tag, album_frame);
    }

    set_text_frame(album, encoding, ALBUM_FRAME_ID(get_tag_version(tag->tag_header)), album_frame);
}

void tag_set_album_artist(char* album_artist, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* album_artist_frame = NULL;
    if( ! (album_artist_frame = tag_get_album_artist(tag)))
    {
        album_artist_frame = new_frame();
        add_frame(tag, album_artist_frame);
    }

    set_text_frame(album_artist, encoding, ALBUM_ARTIST_FRAME_ID(get_tag_version(tag->tag_header)), album_artist_frame);
}

void tag_set_genre(char* genre, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* genre_frame = NULL;
    if( ! (genre_frame = tag_get_genre(tag)))
    {
        genre_frame = new_frame();
        add_frame(tag, genre_frame);
    }

    set_text_frame(genre, encoding, GENRE_FRAME_ID(get_tag_version(tag->tag_header)), genre_frame);
}

void tag_set_track(char* track, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* track_frame = NULL;
    if( ! (track_frame = tag_get_track(tag)))
    {
        track_frame = new_frame();
        add_frame(tag, track_frame);
    }

    set_text_frame(track, encoding, TRACK_FRAME_ID(get_tag_version(tag->tag_header)), track_frame);
}

void tag_set_year(char* year, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* year_frame = NULL;
    if( ! (year_frame = tag_get_year(tag)))
    {
        year_frame = new_frame();
        add_frame(tag, year_frame);
    }

    set_text_frame(year, encoding, YEAR_FRAME_ID(get_tag_version(tag->tag_header)), year_frame);
}

void tag_set_comment(char* comment, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* comment_frame = NULL;
    if( ! (comment_frame = tag_get_comment(tag)))
    {
        comment_frame = new_frame();
        add_frame(tag, comment_frame);
    }

    set_comment_frame(comment, encoding, comment_frame);
}

void tag_set_disc_number(char* disc_number, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* disc_number_frame = NULL;
    if( ! (disc_number_frame = tag_get_disc_number(tag)))
    {
        disc_number_frame = new_frame();
        add_frame(tag, disc_number_frame);
    }

    set_text_frame(disc_number, encoding, DISC_NUMBER_FRAME_ID(get_tag_version(tag->tag_header)), disc_number_frame);
}

void tag_set_composer(char* composer, char encoding, ID3v2_tag* tag)
{
    ID3v2_frame* composer_frame = NULL;
    if( ! (composer_frame = tag_get_composer(tag)))
    {
        composer_frame = new_frame();
        add_frame(tag, composer_frame);
    }

    set_text_frame(composer, encoding, COMPOSER_FRAME_ID(get_tag_version(tag->tag_header)), composer_frame);
}

void tag_set_album_cover(const char* filename, ID3v2_tag* tag)
{
    FILE* album_cover = fopen(filename, "rb");
    char *album_cover_bytes;
    int image_size;
    char *mimetype;

    fseek(album_cover, 0, SEEK_END);
    image_size = (int) ftell(album_cover);
    fseek(album_cover, 0, SEEK_SET);

    album_cover_bytes = (char*) malloc(image_size * sizeof(char));
    fread(album_cover_bytes, 1, image_size, album_cover);

    fclose(album_cover);

    mimetype = get_mime_type_from_filename(filename);
    tag_set_album_cover_from_bytes(album_cover_bytes, mimetype, image_size, tag);

    free(album_cover_bytes);
}

void tag_set_album_cover_from_bytes(char* album_cover_bytes, char* mimetype, int picture_size, ID3v2_tag* tag)
{
    ID3v2_frame* album_cover_frame = NULL;
    if( ! (album_cover_frame = tag_get_album_cover(tag)))
    {
        album_cover_frame = new_frame();
        add_frame_to_tag(tag, album_cover_frame);
    }

    set_album_cover_frame(album_cover_bytes, mimetype, picture_size, album_cover_frame);
}
*/

int _add_allocation_to_tag(id3v2_tag *tag, void *allocation)
{
  if(tag == NULL)
    return 0;

  if(tag->allocation_count == 0)
  {
    tag->allocations = (void**)malloc(sizeof(void*));
    if(tag->allocations == NULL)
      return 0;
  }
  else
  {
    tag->allocations = (void**)realloc(tag->allocations, (tag->allocation_count+1)*sizeof(void*));
    if(tag->allocations == NULL)
    {
      tag->allocation_count = 0;
      return 0;
    }
  }

  tag->allocations[tag->allocation_count] = allocation;

  tag->allocation_count++;

  return 1;
}
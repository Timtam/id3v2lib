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

ID3v2_tag* load_tag(const char* file_name)
{
    char *buffer;
    FILE *file;
    int header_size;
    ID3v2_tag *tag;

    // get header size
    ID3v2_header *tag_header = get_tag_header(file_name);
    if(tag_header == NULL) {
        return NULL;
    }
    header_size = tag_header->tag_size;
    free(tag_header);

    // allocate buffer and fetch header
    file = fopen(file_name, "rb");
    if(file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }
    buffer = (char*) malloc((10+header_size) * sizeof(char));
    if(buffer == NULL) {
        perror("Could not allocate buffer");
        fclose(file);
        return NULL;
    }
    //fseek(file, 10, SEEK_SET);
    fread(buffer, header_size+10, 1, file);
    fclose(file);


    //parse free and return
    tag = load_tag_with_buffer(buffer, header_size);
    free(buffer);

    return tag;
}

ID3v2_tag* load_tag_with_buffer(char *bytes, int length)
{
    // Declaration
    ID3v2_frame *frame;
    int offset = 0;
    ID3v2_tag* tag;
    ID3v2_header* tag_header;

    // Initialization
    tag_header = get_tag_header_with_buffer(bytes, length);

    if(tag_header == NULL) // no valid header found
      return NULL;

    if(get_tag_version(tag_header) == NO_COMPATIBLE_TAG)
    {
        // no supported id3 tag found
        free(tag_header);
        return NULL;
    }

    if(length < tag_header->tag_size+10)
    {
        // Not enough bytes provided to parse completely. TODO: how to communicate to the user the lack of bytes?
        free(tag_header);
        return NULL;
    }

    tag = new_tag();

    // Associations
    tag->tag_header = tag_header;

    // move the bytes pointer to the correct position
    bytes+=10; // skip header
    if(tag_header->extended_header_size)
      // an extended header exists, so we skip it too
      bytes+=tag_header->extended_header_size+4; // don't forget to skip the extended header size bytes too
    
    tag->raw = (char*) malloc(tag->tag_header->tag_size * sizeof(char));
    memcpy(tag->raw, bytes, tag_header->tag_size);
    // we use tag_size here to prevent copying too much if the user provides more bytes than needed to this function

    while(offset < tag_header->tag_size)
    {
        frame = parse_frame(tag, offset);

        if(frame != NULL)
        {
            offset += frame->size + (get_tag_version(tag_header) == ID3v22 ? 6 : 10);
            add_frame(tag, frame);
        }
        else
        {
            break;
        }
    }

    return tag;
}

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

void write_header(ID3v2_header* tag_header, FILE* file)
{
    fwrite("ID3", 3, 1, file);
    fwrite(&tag_header->major_version, 1, 1, file);
    fwrite(&tag_header->minor_version, 1, 1, file);
    fwrite(&tag_header->flags, 1, 1, file);
    fwrite(itob(syncint_encode(tag_header->tag_size)), 4, 1, file);
}

void write_frame(ID3v2_frame* frame, FILE* file)
{
    fwrite(frame->frame_id, 1, (frame->major_version==ID3v22 ? ID3_FRAME_SIZE2 : ID3_FRAME_SIZE), file);
    fwrite(itob(frame->size), 1, (frame->major_version==ID3v22 ? ID3_FRAME_SIZE2 : ID3_FRAME_SIZE), file);
    if(frame->major_version!=ID3v22)
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
        size += frame->size + (frame->major_version==ID3v22 ? 6 : 10);
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
    tag->tag_header = new_header();
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

/**
 * Getter functions
 */
ID3v2_frame* tag_get_title(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, TITLE_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_artist(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, ARTIST_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_album(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, ALBUM_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_album_artist(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, ALBUM_ARTIST_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_genre(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, GENRE_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_track(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, TRACK_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_year(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, YEAR_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_comment(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, COMMENT_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_disc_number(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, DISC_NUMBER_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_composer(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, COMPOSER_FRAME_ID(get_tag_version(tag->tag_header)));
}

ID3v2_frame* tag_get_album_cover(ID3v2_tag* tag)
{
    if(tag == NULL)
    {
        return NULL;
    }

    return get_frame(tag, ALBUM_COVER_FRAME_ID(get_tag_version(tag->tag_header)));
}

/**
 * Setter functions
 */
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
    memcpy(frame->frame_id, COMMENT_FRAME_ID(frame->major_version), 4);
    frame->size = 1 + 3 + 1 + (int) strlen(data); // encoding + language + description + comment

    frame->data = (char*) malloc(frame->size * sizeof(char));

    sprintf(frame->data, "%c%s%c%s", encoding, "eng", '\x00', data);
}

void set_album_cover_frame(char* album_cover_bytes, char* mimetype, int picture_size, ID3v2_frame* frame)
{
    int offset;

    memcpy(frame->frame_id, ALBUM_COVER_FRAME_ID(frame->major_version), 4);
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
        add_frame(tag, album_cover_frame);
    }

    set_album_cover_frame(album_cover_bytes, mimetype, picture_size, album_cover_frame);
}

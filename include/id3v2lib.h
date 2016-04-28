/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_id3v2lib_h
#define id3v2lib_id3v2lib_h

#include "id3v2lib/types.h"
#include "id3v2lib/constants.h"
#include "id3v2lib/errors.h"
#include "id3v2lib/header.h"
#include "id3v2lib/frame.h"
#include "id3v2lib/utils.h"

int _add_allocation_to_tag(id3v2_tag *tag, void *allocation);
id3v2_tag* id3v2_load_tag_from_buffer(char* buffer, int length);
id3v2_tag* id3v2_load_tag_from_file(FILE *file);
void id3v2_load_tags_from_buffer(char *buffer, int length, id3v2_tag ***tags, int *count);
void id3v2_load_tags_from_file(FILE *file, id3v2_tag ***tags, int *count);
//void remove_tag(const char* file_name);
//void set_tag(const char* file_name, id3v2_tag* tag);

// Getter functions
id3v2_frame* id3v2_get_title_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_artist_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_album_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_album_artist_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_genre_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_track_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_year_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_comment_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_disc_number_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_composer_frame_from_tag(id3v2_tag* tag);
id3v2_frame* id3v2_get_album_cover_from_tag(id3v2_tag* tag);

// Setter functions
//void tag_set_title(char* title, char encoding, id3v2_tag* tag);
//void tag_set_artist(char* artist, char encoding, id3v2_tag* tag);
//void tag_set_album(char* album, char encoding, id3v2_tag* tag);
//void tag_set_album_artist(char* album_artist, char encoding, id3v2_tag* tag);
//void tag_set_genre(char* genre, char encoding, id3v2_tag* tag);
//void tag_set_track(char* track, char encoding, id3v2_tag* tag);
//void tag_set_year(char* year, char encoding, id3v2_tag* tag);
//void tag_set_comment(char* comment, char encoding, id3v2_tag* tag);
//void tag_set_disc_number(char* disc_number, char encoding, id3v2_tag* tag);
//void tag_set_composer(char* composer, char encoding, id3v2_tag* tag);
//void tag_set_album_cover(const char* filename, id3v2_tag* tag);
//void tag_set_album_cover_from_bytes(char* album_cover_bytes, char* mimetype, int picture_size, id3v2_tag* tag);

#endif

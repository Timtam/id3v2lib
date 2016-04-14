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

id3v2_tag* load_tag_from_file(const char* file_name);
id3v2_tag* load_tag_from_buffer(char* buffer, int length);
//void remove_tag(const char* file_name);
void set_tag(const char* file_name, id3v2_tag* tag);

// Getter functions
id3v2_frame* tag_get_title(id3v2_tag* tag);
id3v2_frame* tag_get_artist(id3v2_tag* tag);
id3v2_frame* tag_get_album(id3v2_tag* tag);
id3v2_frame* tag_get_album_artist(id3v2_tag* tag);
id3v2_frame* tag_get_genre(id3v2_tag* tag);
id3v2_frame* tag_get_track(id3v2_tag* tag);
id3v2_frame* tag_get_year(id3v2_tag* tag);
id3v2_frame* tag_get_comment(id3v2_tag* tag);
id3v2_frame* tag_get_disc_number(id3v2_tag* tag);
id3v2_frame* tag_get_composer(id3v2_tag* tag);
id3v2_frame* tag_get_album_cover(id3v2_tag* tag);

// Setter functions
void tag_set_title(char* title, char encoding, id3v2_tag* tag);
void tag_set_artist(char* artist, char encoding, id3v2_tag* tag);
void tag_set_album(char* album, char encoding, id3v2_tag* tag);
void tag_set_album_artist(char* album_artist, char encoding, id3v2_tag* tag);
void tag_set_genre(char* genre, char encoding, id3v2_tag* tag);
void tag_set_track(char* track, char encoding, id3v2_tag* tag);
void tag_set_year(char* year, char encoding, id3v2_tag* tag);
void tag_set_comment(char* comment, char encoding, id3v2_tag* tag);
void tag_set_disc_number(char* disc_number, char encoding, id3v2_tag* tag);
void tag_set_composer(char* composer, char encoding, id3v2_tag* tag);
void tag_set_album_cover(const char* filename, id3v2_tag* tag);
void tag_set_album_cover_from_bytes(char* album_cover_bytes, char* mimetype, int picture_size, id3v2_tag* tag);

#endif

/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_constants_h
#define id3v2lib_constants_h

/**
 * TAG_HEADER CONSTANTS
 */
#define ID3V2_HEADER 10
#define ID3V2_HEADER_TAG 3
#define ID3V2_HEADER_VERSION 1
#define ID3V2_HEADER_REVISION 1
#define ID3V2_HEADER_FLAGS 1
#define ID3V2_HEADER_SIZE 4
#define ID3V2_EXTENDED_HEADER_SIZE 4

#define ID3V2_NO_COMPATIBLE_TAG 0
#define ID3V2_2  2
#define ID3V2_3  3
#define ID3V2_4  4
// END TAG_HEADER CONSTANTS

/**
 * TAG_FRAME CONSTANTS
 */
#define ID3V2_FRAME 10
#define ID3V2_FRAME_ID 4 // for id3v23 and 24
#define ID3V2_FRAME_ID2 3 // for id3v22
#define ID3V2_FRAME_SIZE 4 // for id3v23 and 24
#define ID3V2_FRAME_SIZE2 3 // for id3v22
#define ID3V2_FRAME_FLAGS 2
#define ID3V2_FRAME_ENCODING 1
#define ID3V2_FRAME_LANGUAGE 3

#define ID3V2_UNDEFINED_FRAME -1
#define ID3V2_INVALID_FRAME 0
#define ID3V2_TEXT_FRAME 1
#define ID3V2_COMMENT_FRAME 2
#define ID3V2_APIC_FRAME 3
#define ID3V2_EXPERIMENTAL_FRAME 4

#define ID3V2_ISO_ENCODING 0 // supported in all tags
#define ID3V2_UTF_16_ENCODING_WITH_BOM 1 // in all tags possible
#define ID3V2_UTF_16_ENCODING_WITHOUT_BOM 2 // in all tags possible
#define ID3V2_UTF_8_ENCODING 3 // v24 only
 
// END TAG_FRAME CONSTANTS

/**
 * FRAME IDs
 */
#define ID3V2_DECIDE_FRAME(x,y,z) (x==ID3V2_2 ? y : z)
#define ID3V2_GET_TITLE_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TT2", "TIT2")
#define ID3V2_GET_ARTIST_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TP1", "TPE1")
#define ID3V2_GET_ALBUM_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TAL", "TALB")
#define ID3V2_GET_ALBUM_ARTIST_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TP2", "TPE2")
#define ID3V2_GET_GENRE_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TCO", "TCON")
#define ID3V2_GET_TRACK_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TRK", "TRCK")
#define ID3V2_GET_YEAR_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TYE", "TYER")
#define ID3V2_GET_COMMENT_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "COM", "COMM")
#define ID3V2_GET_DISC_NUMBER_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TPA", "TPOS")
#define ID3V2_GET_COMPOSER_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "TCM", "TCOM")
#define ID3V2_GET_ALBUM_COVER_FRAME_ID_FROM_TAG(x) ID3V2_DECIDE_FRAME(id3v2_get_tag_version(x), "PIC", "APIC")
// END FRAME IDs


/**
 * APIC FRAME CONSTANTS
 */
#define ID3V2_FRAME_PICTURE_TYPE 1
#define ID3V2_GET_JPG_MIME_TYPE_FROM_FRAME(x) ID3V2_DECIDE_FRAME(x->version, "jpg", "image/jpeg\0")
#define ID3V2_GET_PNG_MIME_TYPE_FROM_FRAME(x) ID3V2_DECIDE_FRAME(x->version, "png", "image/png\0")

// Picture types:
#define ID3V2_OTHER 0x00
#define ID3V2_FILE_ICON 0x01
#define ID3V2_OTHER_FILE_ICON 0x02
#define ID3V2_FRONT_COVER 0x03
#define ID3V2_BACK_COVER 0x04
#define ID3V2_LEAFLET_PAGE 0x05
#define ID3V2_MEDIA 0x06
#define ID3V2_LEAD_ARTIST 0x07
#define ID3V2_ARTIST 0x08
#define ID3V2_CONDUCTOR 0x09
#define ID3V2_BAND 0x0A
#define ID3V2_COMPOSER 0x0B
#define ID3V2_LYRICIST 0x0C
#define ID3V2_RECORDING_LOCATION 0x0D
#define ID3V2_DURING_RECORDING 0x0E
#define ID3V2_DURING_PERFORMANCE 0x0F
#define ID3V2_VIDEO_SCREEN_CAPTURE 0x10
#define ID3V2_A_BRIGHT_COLOURED_FISH 0x11
#define ID3V2_ILLUSTRATION 0x12
#define ID3V2_ARTIST_LOGOTYPE 0x13
#define ID3V2_PUBLISHER_LOGOTYPE 0x14
// END APIC FRAME CONSTANTS

#endif

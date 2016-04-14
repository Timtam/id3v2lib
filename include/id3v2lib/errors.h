/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#ifndef id3v2lib_errors_h
#define id3v2lib_errors_h

unsigned short id3v2_get_error();
void id3v2_set_error(unsigned short err);

// error constants

enum
{
  ID3V2_OK,
  ID3V2_ERROR_NOT_FOUND,
  ID3V2_ERROR_MEMORY_ALLOCATION,
  ID3V2_ERROR_UNABLE_TO_OPEN,
  ID3V2_ERROR_INCOMPATIBLE_TAG,
  ID3V2_ERROR_INSUFFICIENT_DATA
};

// some helper macros
#define E_FAIL(x) id3v2_set_error(x)
#define E_GET id3v2_get_error()
#define E_SUCCESS id3v2_set_error(ID3V2_OK)

#endif
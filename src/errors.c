/*
 * This file is part of the id3v2lib library
 *
 * Copyright (c) 2013, Lorenzo Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include "id3v2lib.h"

unsigned short id3v2_error = ID3V2_OK;

unsigned short id3v2_get_error()
{
  return id3v2_error;
}

void id3v2_set_error(unsigned short err)
{
  id3v2_error = err;
}
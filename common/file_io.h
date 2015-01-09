/*
 *  file_io.h
 *  Copyright (c) cnhbdu
 *  file read/right
 */

#ifndef __FILE_IO_H__
#define __FILE_IO_H__

#include "platform.h"
#include "typedefs.h"

#include <stdio.h>

FILE* vp_fopen_64(const char* filename, const char* mod);

int vp_fseek_64(FILE *stream, vp_int64_t offset, int whence);

vp_int64_t vp_ftell_64(FILE *stream);

#endif // __FILE_IO_H__
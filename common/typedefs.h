/*
 *  typedefs.h
 *  int type defines
 */

#ifndef __VP_TYPEDEFS_H__
#define __VP_TYPEDEFS_H__

#include "platform.h"

#if defined VP_PLATFORM_LINUX

#include <stdint.h>
#include <stdbool.h>

typedef int8_t vp_int8_t;
typedef int16_t vp_int16_t;
typedef int32_t vp_int24_t;
typedef int32_t vp_int32_t;
typedef int64_t vp_int64_t;
typedef uint8_t vp_uint8_t;
typedef uint16_t vp_uint16_t;
typedef uint32_t vp_uint24_t;
typedef uint32_t vp_uint32_t;
typedef uint64_t vp_uint64_t;

#elif defined VP_PLATFORM_WIN32

#include <Windows.h>

typedef INT8 vp_int8_t;
typedef INT16 vp_int16_t;
typedef INT32 vp_int24_t;
typedef INT32 vp_int32_t;
typedef INT64 vp_int64_t;
typedef UINT8 vp_uint8_t;
typedef UINT16 vp_uint16_t;
typedef UINT32 vp_uint24_t;
typedef UINT32 vp_uint32_t;
typedef UINT64 vp_uint64_t;

#ifndef __cplusplus
typedef unsigned short bool;
#define true 1
#define false 0
#endif

#if !defined(UINT64_MAX)
#define UINT64_MAX ((vp_uint64_t)(-1))
#endif


#else
#error "non type defines"
#endif

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#endif // __VP_TYPEDEFS_H__

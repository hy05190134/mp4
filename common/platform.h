/*
 *  locate which platform we are on
 */

#ifndef __VP_PLATFORM_H__
#define __VP_PLATFORM_H__

/* ---------- os ---------- */
// linux
#if defined linux || defined __linux || defined __linux__ || defined(__APPLE__) || defined(__FreeBSD__) || defined(BSD)
#define VP_PLATFORM_LINUX
// win32
#elif defined _WIN32 || defined __WIN32__ || defined WIN32
#define VP_PLATFORM_WIN32
// not support
#else
#error "Not Support platform"
#endif

#endif // __VP_PLATFORM_H__

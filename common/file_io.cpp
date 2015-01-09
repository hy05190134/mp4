#include "file_io.h"

FILE* vp_fopen_64(const char* filename, const char* mod)
{
#ifdef VP_PLATFORM_WIN32
    return fopen(filename, mod);
#else
    return fopen(filename, mod);
#endif
}

int vp_fseek_64(FILE *stream, vp_int64_t offset, int whence)
{
#ifdef VP_PLATFORM_WIN32
    return _fseeki64(stream, offset, whence);
#else
    return fseeko(stream, offset, whence);
#endif
}

vp_int64_t vp_ftell_64(FILE *stream)
{
#ifdef VP_PLATFORM_WIN32
    return _ftelli64(stream);
#else
    return ftello(stream);
#endif
}
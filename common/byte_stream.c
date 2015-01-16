#ifdef __cplusplus
extern "C"
{
#endif

#include "byte_stream.h"

#include <stdlib.h>
#include <string.h>

vp_uint8_t BytesToUI08(const char* buf)
{
    return buf[0];
}

vp_uint8_t BytesToUI08LE(const char* buf)
{
    return buf[0];
}

vp_uint16_t BytesToUI16(const char* buf)
{
    return ( (((vp_uint16_t)buf[0]) << 8 )	& 0xff00 )
        | ( (((vp_uint16_t)buf[1]))		& 0xff );
}

vp_uint16_t BytesToUI16LE(const char* buf)
{
    return ( (((vp_uint16_t)buf[1]) << 8 )	& 0xff00 )
        | ( (((vp_uint16_t)buf[0]))		& 0xff );
}

vp_uint24_t BytesToUI24(const char* buf)
{
    return ( (((vp_uint24_t)buf[0]) << 16)	& 0xff0000 )
        | ( (((vp_uint24_t)buf[1]) << 8)	& 0xff00 )
        | ( (((vp_uint24_t)buf[2]))		& 0xff );
}

vp_uint32_t BytesToUI32(const char* buf)
{
    return ( (((vp_uint32_t)buf[0]) << 24)	& 0xff000000 )
        | ( (((vp_uint32_t)buf[1]) << 16)	& 0xff0000 )
        | ( (((vp_uint32_t)buf[2]) << 8)	& 0xff00 )
        | ( (((vp_uint32_t)buf[3]))		& 0xff );
}

vp_uint32_t BytesToUI32LE(const char* buf)
{
    return ( (((vp_uint32_t)buf[3]) << 24)	& 0xff000000 )
        | ( (((vp_uint32_t)buf[2]) << 16)	& 0xff0000 )
        | ( (((vp_uint32_t)buf[1]) << 8)	& 0xff00 )
        | ( (((vp_uint32_t)buf[0]))		& 0xff );
}

vp_uint64_t BytesToUI64(const char* buf)
{
    return ( (((vp_uint64_t)buf[0]) << 56)	& 0xff00000000000000 )
        | ( (((vp_uint64_t)buf[1]) << 48)	& 0xff000000000000 )
        | ( (((vp_uint64_t)buf[2]) << 40)	& 0xff0000000000 )
        | ( (((vp_uint64_t)buf[3]) << 32)	& 0xff00000000 )
        | ( (((vp_uint64_t)buf[4]) << 24)	& 0xff000000 )
        | ( (((vp_uint64_t)buf[5]) << 16)	& 0xff0000 )
        | ( (((vp_uint64_t)buf[6]) << 8)	& 0xff00 )
        | ( (((vp_uint64_t)buf[7]))			& 0xff );
}

vp_uint64_t BytesToUI64LE(const char* buf)
{
    return ( (((vp_uint64_t)buf[7]) << 56)	& 0xff00000000000000 )
        | ( (((vp_uint64_t)buf[6]) << 48)	& 0xff000000000000 )
        | ( (((vp_uint64_t)buf[5]) << 40)	& 0xff0000000000 )
        | ( (((vp_uint64_t)buf[4]) << 32)	& 0xff00000000 )
        | ( (((vp_uint64_t)buf[3]) << 24)	& 0xff000000 )
        | ( (((vp_uint64_t)buf[2]) << 16)	& 0xff0000 )
        | ( (((vp_uint64_t)buf[1]) << 8)	& 0xff00 )
        | ( (((vp_uint64_t)buf[0]))			& 0xff );
}

double BytesToDouble(const char* buf) 
{
    unsigned char b[8];
    memcpy(b, buf, 8);

    union {
        unsigned char dc[8];
        double dd;
    } d;

    d.dc[7] = b[0];
    d.dc[6] = b[1];
    d.dc[5] = b[2];
    d.dc[4] = b[3];
    d.dc[3] = b[4];
    d.dc[2] = b[5];
    d.dc[1] = b[6];
    d.dc[0] = b[7];

    return d.dd;
}

void UI08ToBytes(char* buf, vp_uint8_t val)
{
    buf[0] = (char)(val) & 0xff;
}

void UI16ToBytes(char* buf, vp_uint16_t val)
{
    buf[0] = (char)(val >> 8) & 0xff;
    buf[1] = (char)(val) & 0xff;
}

void UI24ToBytes(char* buf, vp_uint24_t val)
{
    buf[0] = (char)(val >> 16) & 0xff;
    buf[1] = (char)(val >> 8) & 0xff;
    buf[2] = (char)(val) & 0xff;
}

void UI32ToBytes(char* buf, vp_uint32_t val)
{
    buf[0] = (char)(val >> 24) & 0xff;
    buf[1] = (char)(val >> 16) & 0xff;
    buf[2] = (char)(val >> 8) & 0xff;
    buf[3] = (char)(val) & 0xff;
}

void UI64ToBytes(char* buf, vp_uint64_t val)
{
    buf[0] = (char)(val >> 56) & 0xff;
    buf[1] = (char)(val >> 48) & 0xff;
    buf[2] = (char)(val >> 40) & 0xff;
    buf[3] = (char)(val >> 32) & 0xff;
    buf[4] = (char)(val >> 24) & 0xff;
    buf[5] = (char)(val >> 16) & 0xff;
    buf[6] = (char)(val >> 8) & 0xff;
    buf[7] = (char)(val) & 0xff;
}

void DoubleToBytes(char* buf, double val) 
{
    union {
        unsigned char dc[8];
        double dd;
    } d;
    unsigned char b[8];

    d.dd = val;

    b[0] = d.dc[7];
    b[1] = d.dc[6];
    b[2] = d.dc[5];
    b[3] = d.dc[4];
    b[4] = d.dc[3];
    b[5] = d.dc[2];
    b[6] = d.dc[1];
    b[7] = d.dc[0];

    memcpy(buf, b, 8);
}

#ifdef __cplusplus
}
#endif

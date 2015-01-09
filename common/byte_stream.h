#ifndef _BYTE_STREAM_H_
#define _BYTE_STREAM_H_

#include "typedefs.h"

#include <stdlib.h>

vp_uint8_t BytesToUI08(const char* buf);

vp_uint8_t BytesToUI08LE(const char* buf);

vp_uint16_t BytesToUI16(const char* buf);

vp_uint16_t BytesToUI16LE(const char* buf);

vp_uint24_t BytesToUI24(const char* buf);

vp_uint32_t BytesToUI32(const char* buf);

vp_uint32_t BytesToUI32LE(const char* buf);

vp_uint64_t BytesToUI64(const char* buf);

vp_uint64_t BytesToUI64LE(const char* buf);

double BytesToDouble(const char* buf);

void UI08ToBytes(char* buf, vp_uint8_t val);

void UI16ToBytes(char* buf, vp_uint16_t val);

void UI24ToBytes(char* buf, vp_uint24_t val);

void UI32ToBytes(char* buf, vp_uint32_t val);

void UI64ToBytes(char* buf, vp_uint64_t val);

void DoubleToBytes(char* buf, double val);

#endif // _BYTE_STREAM_H_

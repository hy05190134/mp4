/*******************************************************************************
 * mpegts_muxer.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 *
 *
 * -----------------------------------------------------------------------------
 * 2014-9-12 8:05 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_MPEGTS_MUXER_H_
#define _HDEV_MPEGTS_MUXER_H_

#include "common/typedefs.h"

#include <stdio.h>

typedef struct ngx_rtmp_mpegts_frame_t
{
    vp_uint64_t    pts;
    vp_uint64_t    dts;
    unsigned int  pid;
    unsigned int  sid;
    unsigned int  cc;
    unsigned    key:1;
} ngx_rtmp_mpegts_frame_t;


int ngx_rtmp_mpegts_write_header(FILE *file);

int ngx_rtmp_mpegts_write_frame(FILE *file,
    ngx_rtmp_mpegts_frame_t *f, const char* dataBuf, int dataSize);

typedef struct MpegtsMuxer
{
    char ts_filename_[50];
    FILE* fts_;

    int sample_rate_;
    int channel_count_;
    vp_uint64_t aac_frame_count_;

    char* spsppsframe_;
    int spsppsframe_len_;
    bool is_write_spspps_;
    bool is_send_aud_;

    char* tmp_264buf_;
    int tmp_264buf_capacity_;
    char* tmp_aacbuf_;
    int tmp_aacbuf_capacity_;

    int audio_cc_;
    int video_cc_;
} MpegtsMuxer;

void InitMpegtsMuxer(const char *tsFilename, MpegtsMuxer *mpegts_muxer);

void DestroyMpegtsMuxer(MpegtsMuxer *mpegts_muxer);

bool MpegOpen(int samplerate, int channel, const char* spsBuf, int spsSize,
        const char* ppsBuf, int ppsSize, MpegtsMuxer *mpegts_muxer);

void MpegClose(MpegtsMuxer *mpegts_muxer);

void AddAVCData(const char* dataBuf, int dataSize, unsigned int timestamp,
        unsigned int compositionTimestamp, bool isKeyframe, MpegtsMuxer *mpegts_muxer);

void FlushAVCData(const char* dataBuf, int dataSize, bool isKeyframe,
        vp_uint64_t pts, vp_uint64_t dts, MpegtsMuxer *mpegts_muxer);

void AddAACData(const char* dataBuf, int dataSize, unsigned int timestamp, MpegtsMuxer *mpegts_muxer);

void FlushAACData(const char* dataBuf, int dataSize,
        vp_uint64_t pts, vp_uint64_t dts, MpegtsMuxer *mpegts_muxer);

unsigned int GetSamplingFrequencyIndex(unsigned int sampling_frequency);

#endif // _HBASE_MPEGTS_MUXER_H_

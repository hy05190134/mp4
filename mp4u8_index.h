/*******************************************************************************
 * mp4u8_index.h
 * Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------
 * 2014-9-4 12:56 - Created (Haibin Du)
 ******************************************************************************/

#ifndef _HDEV_MP4U8_INDEX_H_
#define _HDEV_MP4U8_INDEX_H_

#include <cstl/cvector.h>

#include "common/typedefs.h"

// ---------------------------------------------------------------------------
// Mp4u8Sample

typedef struct Mp4u8Sample 
{
    double timestamp;
    double composition_timestamp;
    vp_uint64_t offset;
    vp_uint32_t size;
    vp_uint8_t is_key_;
} Mp4u8Sample;

const char* SampleFromBuf(const char* buf, bool isVideo, Mp4u8Sample *sample);

// ---------------------------------------------------------------------------
// Mp4u8Segment

typedef struct Mp4u8Segment 
{
    vp_uint32_t audio_beg_samno_;   // (°üº¬)
    vp_uint32_t audio_end_samno_;   // (²»°üº¬)
    vp_uint32_t video_beg_samno_;
    vp_uint32_t video_end_samno_;
    vp_uint64_t seg_data_off_;
    vp_uint64_t seg_data_size_;
} Mp4u8Segment;

const char* SegmentFromBuf(const char* buf, Mp4u8Segment *segment);

// ---------------------------------------------------------------------------

// Mp4u8Index
typedef struct Mp4u8Index
{
    //base info
    vp_uint8_t version_;
    vp_uint64_t filesize_;
    double duration_;

    //other info
    vp_uint16_t samplerate_;
    vp_uint16_t channel_;
    vp_uint16_t aac_codec_size_;
    char* aac_codec_buf_;
    vp_uint16_t width_;
    vp_uint16_t height_;
    vp_uint16_t sps_size_;
    char* sps_buf_;
    vp_uint16_t pps_size_;
    char* pps_buf_;

    //samples
    //std::vector<Mp4u8Sample> audio_samples_;
    vector_t *audio_samples_;
    //std::vector<Mp4u8Sample> video_samples_;
    vector_t *video_samples_;
    //std::vector<Mp4u8Segment> seg_map_;
    vector_t *seg_map_;
} Mp4u8Index;


bool ReadFromFile(const char *readFile, Mp4u8Index *index);

bool ReadFromBuf(const char* fromBuf, int fromBufSize, Mp4u8Index *index);

void GetAudioCodec(int* sampleRate, int* channelCount,
        char* aacCodecBuf, int* aacCodecSize, const Mp4u8Index *index);

void GetVideoCodec(int* width, int* height,
        char* spsBuf, int* spsSize,
        char* ppsBuf, int* ppsSize, const Mp4u8Index *index);

void InitMp4u8Index(Mp4u8Index *index);

void DestroyMp4u8Index(Mp4u8Index *index);

#endif // _HBASE_MP4U8_INDEX_H_

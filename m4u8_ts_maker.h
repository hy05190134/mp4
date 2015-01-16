#ifndef _HDEV_M4U8_TS_MAKER_H_
#define _HDEV_M4U8_TS_MAKER_H_

//#include <map>

#include <cstl/cmap.h>
#include <cstl/cvector.h>
#include "common/typedefs.h"
#include "mpegts_muxer.h"
#include "mp4u8_index.h"

typedef struct M4u8TSSegSample
{
    double timestamp;
    double composition_timestamp;
    vp_uint64_t offset;
    vp_uint32_t size;
    vp_uint8_t is_key_;
    bool is_video_;
} M4u8TSSegSample;

typedef struct M4u8TSMaker
{
    Mp4u8Index* mp4u8_index_;
    char mp4_filename_[100];
    char index_name_[100];
    int seg_index_;
    char ts_name_[100];

    MpegtsMuxer* mpegts_muxer_;

    double audio_duration_;
    int samplerate_;
    int channel_;
    char* aac_codec_frame_;
    int aac_codec_frame_len_;

    double video_duration_;
    int width_;
    int height_;
    char* spsbuf_;
    int spssize_;
    char* ppsbuf_;
    int ppssize_;
    char* spsppsframe_;
    int spsppsframe_len_;
    char *buf_body_;
    bool is_write_aud_;

    //std::multimap<vp_uint64_t, M4u8TSSegSample> samples_;
    multimap_t *samples_;
} M4u8TSMaker;

void InitM4u8TSMaker(M4u8TSMaker *maker, const char *mp4Filename, const char *indexName, const char *tsName);

void DestroyM4u8TSMaker(M4u8TSMaker *maker);

void OnStart(int segIndex, M4u8TSMaker *maker);

void CalcSamples(const Mp4u8Segment *seg, Mp4u8Index *mp4u8_index, M4u8TSMaker *maker);

void getRange(const char *server_host, int server_port, const char *uri, int be, int en, int maxsize, char *body_buf);

static void MakeAACBuf(const char* frameBuf, int frameLen, char* aacBuf, int *aacLen,
    int samplerate, int channel);

void WriteIntoFile(M4u8TSMaker *maker);

#endif

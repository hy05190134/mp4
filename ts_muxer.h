#ifndef _TS_MUXER_H_
#define _TS_MUXER_H_

//#include "base/base.h"

#include "mpegtsenc.h"

#define _USE_TS_MUXER_LOG_


void WriterHeader(const TSMuxer &tsmuxer);

void WriteEnd(int* audioCC, int* videoCC, int* patCC, int* pmtCC, 
        const TSMuxer &tsmuxer);

void WriteVideoData(char* buf, int bufLen, bool isKeyframe, long long pts, long long dts, 
        const TSMuxer &tsmuxer);

void WriteAudioData(char* buf, int bufLen, long long pts, long long dts, 
        const TSMuxer &tsmuxer);

void InitTsMuxer(const char *filename, TSMuxer &tsmuxer);

void DestroyTsMuxer(TSMuxer &tsmuxer);

struct TSMuxer
{
    const char *filename_;
    long long time_begin_;
    FILE* fp_;
    bool is_video_begin_;
    MpegTSWrite* mpegts_write_;
    std::vector<MpegTSWriteStream *> mpegts_write_stream_;;

#ifdef _USE_TS_MUXER_LOG_
    FILE* fp_log_;
#endif
    long long begin_pts_;
};

#endif // _TS_MUXER_H_

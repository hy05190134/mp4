#ifdef __cplusplus
extern "C"
{
#endif

#include "ts_muxer.h"

void InitTSMuxer(const char* filename, TSMuxer &tsmuxer)
{
    tsmuxer.filename_ = filename;
    tsmuxer.fp_ = fopen(filename, "wb");
    tsmuxer.mpegts_write_ = (MpegTSWrite *)malloc(sizeof(MpegTSWrite));
    InitMpegTSWrite(*tsmuxer.mpegts_write_);
    tsmuxer.time_begin_ = -1;
    tsmuxer.is_video_begin_ = false;

#ifdef _USE_TS_MUXER_LOG_
    char tmp[100] = {0};
    strcpy(tmp, filename);
    const char *log = strcat(tmp, ".log");
    tsmuxer.fp_log_ = fopen(log, "w");
#endif
    tsmuxer.begin_pts_ = -1;
}

void DestroyTSMuxer(TSMuxer &tsmuxer)
{
    //DestroyMpegTSWrite(*tsmuxer.mpegts_write_);
    free(tsmuxer.mpegts_write_);
    fclose(tsmuxer.fp_);

#ifdef _USE_TS_MUXER_LOG_
    fclose(tsmuxer.fp_log_);
#endif
}

void WriterHeader(const TSMuxer &tsmuxer)
{
    mpegts_write_header(tsmuxer.mpegts_write_, tsmuxer.mpegts_write_stream_, -1, -1, -1, -1);

#ifdef _USE_TS_MUXER_LOG_
    fprintf(tsmuxer.fp_log_, "write ts file header\n");
#endif
}

void WriteEnd(int* audioCC, int* videoCC, int* patCC, int* pmtCC, const TSMuxer &tsmuxer)
{
    mpegts_write_end(tsmuxer.mpegts_write_, tsmuxer.fp_, tsmuxer.mpegts_write_stream_,
        audioCC, videoCC, patCC, pmtCC);

#ifdef _USE_TS_MUXER_LOG_
    fprintf(tsmuxer.fp_log_, "write ts file end\n");
#endif
}

void WriteVideoData(char* buf, int bufLen, bool isKeyframe, long long pts, long long dts, const TSMuxer &tsmuxer)
{
    if (begin_pts_ < 0)
    {
        begin_pts_ = pts;
    }
    pts = dts = (pts - begin_pts_);

    mpegts_write_packet(tsmuxer.mpegts_write_, tsmuxer.mpegts_write_stream_, tsmuxer.mpegts_write_stream_[1], tsmuxer.fp_, (uint8_t*)buf, bufLen,
        pts, dts, isKeyframe ? 1 : 0, 1);

    tsmuxer.is_video_begin_ = true;

#ifdef _USE_TS_MUXER_LOG_
    fprintf(tsmuxer.fp_log_, "write ts video data, pts: %lld, dts: %lld, iskey: %d\n", 
        pts, dts, isKeyframe);
#endif
}

void WriteAudioData(char* buf, int bufLen, long long pts, long long dts, const TSMuxer &tsmuxer)
{
    //if (false == is_video_begin_) return;

    if (begin_pts_ < 0)
    {
        begin_pts_ = pts;
    }
    pts = dts = (pts - begin_pts_);

    mpegts_write_packet(tsmuxer.mpegts_write_, tsmuxer.mpegts_write_stream_, tsmuxer.mpegts_write_stream_[0], tsmuxer.fp_, (uint8_t*)buf, bufLen,
        pts, dts, 1, 0);

#ifdef _USE_TS_MUXER_LOG_
    fprintf(tsmuxer.fp_log_, "write ts audio data, pts: %lld\n", pts);
#endif
}

#ifdef __cplusplus
}
#endif

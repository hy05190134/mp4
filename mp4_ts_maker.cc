#ifdef __cplusplus
extern "C"
{
#endif

#include "netutil.h"
#include "mp4_ts_maker.h"
#include "mp4u8_index.h"

const char kStartCode[] = {0x00, 0x00, 0x00, 0x01};

void InitM4u8TSMaker(M4u8TSMaker &maker, const char *mp4Filename, const char *indexName, const char *tsName)
{
    maker.mp4_filename_ = mp4Filename;
    maker.index_name_ = indexName;
    maker.ts_name_ = tsName;

    maker.mp4u8_index_ = NULL;
    maker.mpegts_muxer_ = NULL;

    maker.audio_duration_ = 0.0;
    maker.aac_codec_frame_ = (char *)malloc(1024);

    maker.video_duration_ = 0.0;
    maker.spsbuf_ = (char *)malloc(1024);
    maker.spssize_ = 0;

    maker.ppsbuf_ = (char *)malloc(1024);
    maker.ppssize_ = 0;
    maker.spsppsframe_ = NULL;
    maker.is_write_aud_ = false;

    maker.seg_index_ = -1;
}

void DestroyM4u8TSMaker(M4u8TSMaker &maker)
{
    if (maker.mpegts_muxer_){
        DestroyMpegTSMuxer(*maker.mpegts_muxer_);
        maker.mpegts_muxer_ = NULL;
    }

    if (maker.mp4u8_index_){
        DestroyMp4u8Index(*maker.mp4u8_index_);
        maker.mp4u8_index_ = NULL;
    }

    if (maker.aac_codec_frame_){
        free(maker.aac_codec_frame_);
        maker.aac_codec_frame_ = NULL;
    }
    
    if (maker.spsbuf_){
        free(maker.spsbuf_);
        maker.spsbuf_ = NULL;
    }
    
    if (maker.ppsbuf_){
        free(maker.ppsbuf_);
        maker.ppsbuf_ = NULL;
    }
    
    if (maker.spsppsframe_){
        free(maker.spsppsframe_);
        maker.spsppsframe_ = NULL;
    }
}

void OnStart(int segIndex, M4u8TSMaker &maker)
{
    Mp4u8Index mp4u8_index;
    InitMp4u8Index(mp4u8_index);
    ReadFromFile(maker.index_name_, mp4u8_index);

    char tmpaaccodec[100];
    int tmpaaccodeclen = 0;
    GetAudioCodec(&maker.samplerate_, &maker.channel_, tmpaaccodec, &tmpaaccodeclen, mp4u8_index);

    maker.aac_codec_frame_len_ = tmpaaccodeclen + 7;
    maker.aac_codec_frame_ = (char *)malloc(maker.aac_codec_frame_len_);
    MakeAACBuf(tmpaaccodec, tmpaaccodeclen, maker.aac_codec_frame_, &maker.aac_codec_frame_len_, maker.samplerate_, maker.channel_);

    GetVideoCodec(&maker.width_, &maker.height_, maker.spsbuf_, maker.spssize_, maker.ppsbuf_, &maker.ppssize_);

    maker.spsppsframe_len_ = spssize_ + ppssize_ + 4 + 4;
    maker.spsppsframe_ = (char *)malloc(spsppsframe_len_);
    char *pframe = maker.spsppsframe_;
    memcpy(pframe, kStartCode, 4); pframe += 4;
    memcpy(pframe, maker.spsbuf_, maker.spssize_); pframe += maker.spssize_;
    memcpy(pframe, kStartCode, 4); pframe += 4;
    memcpy(pframe, maker.ppsbuf_, maker.ppssize_); pframe += maker.ppssize_;

    Mp4u8Segment seg = mp4u8_index.seg_map_[segIndex];

    CalcSamples(seg, mp4u8_index, maker);

    maker.buf_body = (char *)malloc(seg.seg_data_size_ + 1);
    int be = seg.seg_data_off_;
    int en = seg.seg_data_off_ + seg.seg_data_size_;
    int maxsize = seg.seg_data_size_ + 1;
    getRange(server_host, server_port, mp4_uri, be, en, maxsize, maker.buf_body);

    WriteIntoFile(maker);
    DestroyMp4u8Index(mp4u8_index);
}

void CalcSamples(const Mp4u8Segment &seg, Mp4u8Index &mp4u8_index, M4u8TSMaker &maker)
{
    maker.samples_.clear();
    maker.audio_duration_ = maker.video_duration_ = 0.0;

    for (unsigned int i = seg.audio_beg_samno_; i < seg.audio_end_samno_; ++i)
    {
        Mp4u8Sample& audio_sample = mp4u8_index.audio_samples_[i];

        M4u8TSSegSample seg_sample;
        seg_sample.timestamp = audio_sample.timestamp;
        seg_sample.composition_timestamp = audio_sample.composition_timestamp;
        seg_sample.offset = audio_sample.offset - seg.seg_data_off_;
        seg_sample.size = audio_sample.size;
        seg_sample.is_key_ = audio_sample.is_key_;
        seg_sample.is_video_ = false;

        maker.samples_.insert(std::make_pair(seg_sample.offset, seg_sample));
        //samples_.insert(std::make_pair(seg_sample.timestamp*1000, seg_sample));
    }

    if (seg.audio_end_samno_ == mp4u8_index.audio_samples_.size())
    {
        maker.audio_duration_ = mp4u8_index.duration_ -
            mp4u8_index.audio_samples_[seg.audio_beg_samno_].timestamp;
    }
    else
    {
        maker.audio_duration_ = mp4u8_index.audio_samples_[seg.audio_end_samno_].timestamp - 
            mp4u8_index.audio_samples_[seg.audio_beg_samno_].timestamp;
    }

    for (unsigned int i = seg.video_beg_samno_; i < seg.video_end_samno_; ++i)
    {
        Mp4u8Sample& video_sample = mp4u8_index.video_samples_[i];

        M4u8TSSegSample seg_sample;
        seg_sample.timestamp = video_sample.timestamp;
        seg_sample.composition_timestamp = video_sample.composition_timestamp;
        seg_sample.offset = video_sample.offset - seg.seg_data_off_;
        seg_sample.size = video_sample.size;
        seg_sample.is_key_ = video_sample.is_key_;
        seg_sample.is_video_ = true;

        samples_.insert(std::make_pair(seg_sample.offset, seg_sample));
        //samples_.insert(std::make_pair(seg_sample.timestamp*1000, seg_sample));
    }

    if (seg.video_end_samno_ == mp4u8_index.video_samples_.size())
    {
        maker.video_duration_ = mp4u8_index.duration_ - 
            mp4u8_index.video_samples_[seg.video_beg_samno_].timestamp;
    }
    else
    {
        maker.video_duration_ = mp4u8_index.video_samples_[seg.video_end_samno_].timestamp - 
            mp4u8_index.video_samples_[seg.video_beg_samno_].timestamp;
    }
}

void getRange(const char *server_host, int server_port, const char *uri, int be, int en, int maxsize, char *body_buf)
{
    char request[2048] = {0};
    char range_str[100] = {0};
    sprintf(range_str, "Range: bytes=%d-%d", be, en);
    sprintf(request, "GET %s HTTP/1.0\r\n%s\r\n\r\n", uri, range_str);
    int err = 0;
    int body_len = 0;
    char *buf = NULL;
    char *ds = netutil_get_remote_server_data(server_host, server_port, request, &buf, &body_len, &err);
    if (ds) {
        if (maxsize == body_len){
            memcpy(body_buf, buf, body_len);
        }
        free(ds);
    }
}

static unsigned int GetSamplingFrequencyIndex(unsigned int sampling_frequency)
{
    switch (sampling_frequency) {
    case 96000: return 0;
    case 88200: return 1;
    case 64000: return 2;
    case 48000: return 3;
    case 44100: return 4;
    case 32000: return 5;
    case 24000: return 6;
    case 22050: return 7;
    case 16000: return 8;
    case 12000: return 9;
    case 11025: return 10;
    case 8000:  return 11;
    case 7350:  return 12;
    default:    return 0;
    }
}

void MakeAACBuf(const char* frameBuf, int frameLen, char* aacBuf, int *aacLen,
    int samplerate, int channel)
{
    unsigned int sampling_frequency_index = GetSamplingFrequencyIndex(samplerate);

    aacBuf[0] = 0xFF;
    aacBuf[1] = 0xF1; // 0xF9 (MPEG2)
    aacBuf[2] = 0x40 | (sampling_frequency_index << 2) | (channel >> 2);
    aacBuf[3] = ((channel & 0x3)<<6) | ((frameLen+7) >> 11);
    aacBuf[4] = ((frameLen+7) >> 3)&0xFF;
    aacBuf[5] = (((frameLen+7) << 5)&0xFF) | 0x1F;
    aacBuf[6] = 0xFC;

    memcpy(aacBuf+7, frameBuf, frameLen);
    *aacLen = frameLen + 7;
}

void WriteInfoFile(const M4u8TSMaker &maker)
{
    char tsname[4096];
    char tsname2[4096];

    if (maker.ts_name_ == ""){
        int l = strlen(maker.mp4_filename_);
        if (l > 4096){
            l = 4096;
        }
        memcpy(tsname2, maker.mp4_filename_, l);
        const char *p = tsname2, *pe = tsname2;
        if (maker.ts_name_ == ""){
            while (*p) {
                if (*(++p) == '/'){
                    pe = p+1;
                }
            }
        }
        sprintf(tsname, "%s.up.%d.ts", pe, maker.seg_index_);
    }
    else {
        int l = strlen(maker.ts_name_);
        if (l > 4096){
            l = 4096;
        }
        memcpy(tsname2, maker.ts_name_, l);
        const char *pe = tsname2;
        sprintf(tsname, "%s.up.%d.ts", pe, maker.seg_index_);
    }


    //#define US_TS_MUXER 1

    TSMuxer tsmuxer;
    InitTSMuxer(tsname, tsmuxer);
    WriterHeader(tsmuxer);

    long long begin_timestamp = 0.0;
    bool is_audio_begin_ = false;

    for (std::multimap<vp_uint64_t, M4u8TSSegSample>::iterator it =
            maker.samples_.begin(); it != maker.samples_.end(); ++it)
    {
        char *data_buf = maker.buf_body_ + it->second.offset;

        if (begin_timestamp == 0) begin_timestamp = it->second.timestamp;
        long long sam_timestamp = it->second.timestamp*1000;
        long long sam_composition_timestamp =
            it->second.composition_timestamp*1000;
 
        if (it->second.is_video_)   // ÊÓÆµ
        {
            // Ìæ»»ÆðÊ¼Âë
            char *pbuf = data_buf;
            char *pend = pbuf + it->second.size;
            while (pbuf < pend)
            {
                vp_uint32_t unitsize = BytesToUI32(pbuf);

                memcpy(pbuf, kStartCode, 4); 

                pbuf += 4;
                pbuf += unitsize;
            }

            if (it->second.is_key_)
            {
                char *xxbuf = (char *)malloc(it->second.size + maker.spsppsframe_len_);
                char *pxx = xxbuf;
                memcpy(pxx, maker.spsppsframe_, maker.spsppsframe_len_);
                pxx += maker.spsppsframe_len_;
                memcpy(pxx, data_buf, it->second.size);
                pxx += it->second.size;

                WriteVideoData(xxbuf, (pxx-xxbuf), it->second.is_key_,
                        sam_composition_timestamp*90, sam_timestamp*90, tsmuxer);

                free(xxbuf);
            }
            else
            {
                WriteVideoData(data_buf, it->second.size, it->second.is_key_, 
                        sam_composition_timestamp*90, sam_timestamp*90, tsmuxer);
            }

            assert(pbuf == pend);
        }
        else    // ÒôÆµ
        {
            char* aacbuf = (char *)malloc(it->second.size + 7);
            int aaclen = 0;
            MakeAACBuf(data_buf, it->second.size, aacbuf, &aaclen, maker.samplerate_, maker.channel_);
            WriteAudioData(aacbuf, aaclen, sam_composition_timestamp*90, sam_timestamp*90, tsmuxer);
            free(aacbuf);

            is_audio_begin_ = true;
        }
    }

    WriteEnd(NULL, NULL, NULL, NULL, tsmuxer);

    exit(0);
    printf("end\n");
}


#ifdef __cplusplus
}
#endif

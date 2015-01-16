#ifdef __cplusplus
extern "C"
{
#endif

#include "netutil.h"
#include "mpegts_muxer.h"
#include "m4u8_ts_maker.h"
#include "common/byte_stream.h"
#include "mp4u8_index.h"

const char kStartCode[] = {0x00, 0x00, 0x00, 0x01};

//use to register Mp4u8TSSample userdata type
static void _sam_init(const void *input, void *output)
{
    ((M4u8TSSegSample *)input)->timestamp = 0.0;
    ((M4u8TSSegSample *)input)->composition_timestamp = 0.0;
    ((M4u8TSSegSample *)input)->offset = 0;
    ((M4u8TSSegSample *)input)->size = 0;
    ((M4u8TSSegSample *)input)->is_key_ = 0;
    ((M4u8TSSegSample *)input)->is_video_ = false;
    *(bool_t *)output = true;
}

static void _sam_destroy(const void *input, void *output)
{
    ((M4u8TSSegSample *)input)->timestamp = 0.0;
    ((M4u8TSSegSample *)input)->composition_timestamp = 0.0;
    ((M4u8TSSegSample *)input)->offset = 0;
    ((M4u8TSSegSample *)input)->size = 0;
    ((M4u8TSSegSample *)input)->is_key_ = 0;
    ((M4u8TSSegSample *)input)->is_video_ = false;
    *(bool_t *)output = true;
}

static void _sam_copy(const void *first, const void *second, void *output)
{
    ((M4u8TSSegSample *)first)->timestamp = ((M4u8TSSegSample *)second)->timestamp;
    ((M4u8TSSegSample *)first)->composition_timestamp = ((M4u8TSSegSample *)second)->composition_timestamp;
    ((M4u8TSSegSample *)first)->offset = ((M4u8TSSegSample *)second)->offset;
    ((M4u8TSSegSample *)first)->size = ((M4u8TSSegSample *)second)->size;
    ((M4u8TSSegSample *)first)->is_key_ = ((M4u8TSSegSample *)second)->is_key_;
    ((M4u8TSSegSample *)first)->is_video_ = ((M4u8TSSegSample *)second)->is_video_;
    *(bool_t *)output = true;
}

static void _sam_less(const void *first, const void *second, void *output)
{
    *(bool_t *)output = ((M4u8TSSegSample *)first)->timestamp < ((M4u8TSSegSample *)second)->timestamp ? true : false;
}

void InitM4u8TSMaker(M4u8TSMaker *maker, const char *mp4Filename, const char *indexName, const char *tsName)
{
    if (mp4Filename != NULL) {strncpy((char *)maker->mp4_filename_, mp4Filename, 100); maker->mp4_filename_[99] = 0;}
    if (indexName != NULL) {strncpy((char *)maker->index_name_, indexName, 100); maker->index_name_[99] = 0;}
    if (tsName != NULL) {strncpy((char *)maker->ts_name_, tsName, 100); maker->ts_name_[99]= 0;}

    maker->mp4u8_index_ = (Mp4u8Index *)malloc(sizeof(Mp4u8Index));
    maker->mpegts_muxer_ = (MpegtsMuxer *)malloc(sizeof(MpegtsMuxer));

    maker->audio_duration_ = 0.0;
    maker->aac_codec_frame_ = (char *)malloc(1024);

    maker->video_duration_ = 0.0;
    maker->spsbuf_ = (char *)malloc(1024);
    maker->spssize_ = 0;

    maker->ppsbuf_ = (char *)malloc(1024);
    maker->ppssize_ = 0;
    maker->spsppsframe_ = NULL;
    maker->is_write_aud_ = false;

    maker->seg_index_ = -1;
    
    type_register(M4u8TSSegSample, _sam_init, _sam_copy, _sam_less, _sam_destroy);
    maker->samples_ = create_multimap(unsigned long long, M4u8TSSegSample);
    assert(NULL != maker->samples_);
    multimap_init(maker->samples_);
}

void DestroyM4u8TSMaker(M4u8TSMaker *maker)
{
    if (maker->mpegts_muxer_){
        DestroyMpegtsMuxer(maker->mpegts_muxer_);
        maker->mpegts_muxer_ = NULL;
    }

    if (maker->mp4u8_index_){
        DestroyMp4u8Index(maker->mp4u8_index_);
        maker->mp4u8_index_ = NULL;
    }

    if (maker->aac_codec_frame_){
        free(maker->aac_codec_frame_);
        maker->aac_codec_frame_ = NULL;
    }
    
    if (maker->spsbuf_){
        free(maker->spsbuf_);
        maker->spsbuf_ = NULL;
    }
    
    if (maker->ppsbuf_){
        free(maker->ppsbuf_);
        maker->ppsbuf_ = NULL;
    }
    
    if (maker->spsppsframe_){
        free(maker->spsppsframe_);
        maker->spsppsframe_ = NULL;
    }

    if (maker->samples_){
        multimap_destroy(maker->samples_);
        maker->samples_ = NULL;
    }
}

void OnStart(int segIndex, M4u8TSMaker *maker)
{
    maker->seg_index_ = segIndex;
    InitMp4u8Index(maker->mp4u8_index_);
    ReadFromFile(maker->index_name_, maker->mp4u8_index_);

    char tmpaaccodec[100];
    int tmpaaccodeclen = 0;
    GetAudioCodec(&maker->samplerate_, &maker->channel_, tmpaaccodec, &tmpaaccodeclen, maker->mp4u8_index_);

    maker->aac_codec_frame_len_ = tmpaaccodeclen + 7;
    maker->aac_codec_frame_ = (char *)malloc(maker->aac_codec_frame_len_);
    MakeAACBuf(tmpaaccodec, tmpaaccodeclen, maker->aac_codec_frame_, &maker->aac_codec_frame_len_, maker->samplerate_, maker->channel_);

    GetVideoCodec(&maker->width_, &maker->height_, maker->spsbuf_, &maker->spssize_, maker->ppsbuf_, &maker->ppssize_, maker->mp4u8_index_);

    maker->spsppsframe_len_ = maker->spssize_ + maker->ppssize_ + 4 + 4;
    maker->spsppsframe_ = (char *)malloc(maker->spsppsframe_len_);
    char *pframe = maker->spsppsframe_;
    memcpy(pframe, kStartCode, 4); pframe += 4;
    memcpy(pframe, maker->spsbuf_, maker->spssize_); pframe += maker->spssize_;
    memcpy(pframe, kStartCode, 4); pframe += 4;
    memcpy(pframe, maker->ppsbuf_, maker->ppssize_); pframe += maker->ppssize_;

    Mp4u8Segment seg = *(Mp4u8Segment *)vector_at(maker->mp4u8_index_->seg_map_, segIndex);

    CalcSamples(&seg, maker->mp4u8_index_, maker);

    maker->buf_body_ = (char *)malloc(seg.seg_data_size_ + 1);
    int be = seg.seg_data_off_;
    int en = seg.seg_data_off_ + seg.seg_data_size_;
    int maxsize = seg.seg_data_size_ + 1;
    
    const char *server_host = "10.0.5.112";
    int server_port = 1991;
    const char *mp4_uri = "/laki/HZW.mp4";
    getRange(server_host, server_port, mp4_uri, be, en, maxsize, maker->buf_body_);

    WriteIntoFile(maker);
}

void CalcSamples(const Mp4u8Segment *seg, Mp4u8Index *mp4u8_index, M4u8TSMaker *maker)
{
    maker->audio_duration_ = maker->video_duration_ = 0.0;

    pair_t *ppair_p = create_pair(unsigned long long, M4u8TSSegSample);
    pair_init(ppair_p);
    unsigned int i = 0;
    for (i = seg->audio_beg_samno_; i < seg->audio_end_samno_; ++i)
    {
        Mp4u8Sample audio_sample = *(Mp4u8Sample *)vector_at(mp4u8_index->audio_samples_, i);

        M4u8TSSegSample seg_sample;
        seg_sample.timestamp = audio_sample.timestamp;
        seg_sample.composition_timestamp = audio_sample.composition_timestamp;
        seg_sample.offset = audio_sample.offset - seg->seg_data_off_;
        seg_sample.size = audio_sample.size;
        seg_sample.is_key_ = audio_sample.is_key_;
        seg_sample.is_video_ = false;

        pair_make(ppair_p, seg_sample.offset, seg_sample);
        multimap_insert(maker->samples_, ppair_p);
        //samples_.insert(std::make_pair(seg_sample.timestamp*1000, seg_sample));
    }

    if (seg->audio_end_samno_ == vector_size(mp4u8_index->audio_samples_))
    {
        maker->audio_duration_ = mp4u8_index->duration_ -
            (*(Mp4u8Sample *)vector_at(mp4u8_index->audio_samples_, seg->audio_beg_samno_)).timestamp;
    }
    else
    {
        maker->audio_duration_ = (*(Mp4u8Sample *)vector_at(mp4u8_index->audio_samples_, seg->audio_end_samno_)).timestamp - (*(Mp4u8Sample *)vector_at(mp4u8_index->audio_samples_, seg->audio_beg_samno_)).timestamp;
;
    }

    unsigned int j = 0;
    for (j = seg->video_beg_samno_; j < seg->video_end_samno_; ++j)
    {
        Mp4u8Sample video_sample = *(Mp4u8Sample *)vector_at(mp4u8_index->video_samples_, j);

        M4u8TSSegSample seg_sample;
        seg_sample.timestamp = video_sample.timestamp;
        seg_sample.composition_timestamp = video_sample.composition_timestamp;
        seg_sample.offset = video_sample.offset - seg->seg_data_off_;
        seg_sample.size = video_sample.size;
        seg_sample.is_key_ = video_sample.is_key_;
        seg_sample.is_video_ = true;

        pair_make(ppair_p, seg_sample.offset, seg_sample);
        multimap_insert(maker->samples_, ppair_p);
        //samples_.insert(std::make_pair(seg_sample.timestamp*1000, seg_sample));
    }

    if (seg->video_end_samno_ == vector_size(mp4u8_index->video_samples_))
    {
        maker->video_duration_ = mp4u8_index->duration_ - 
            (*(Mp4u8Sample *)vector_at(mp4u8_index->video_samples_, seg->video_beg_samno_)).timestamp;
    }
    else
    {
        maker->video_duration_ = (*(Mp4u8Sample *)vector_at(mp4u8_index->video_samples_, seg->video_end_samno_)).timestamp - (*(Mp4u8Sample *)vector_at(mp4u8_index->video_samples_, seg->video_beg_samno_)).timestamp;
    }

    pair_destroy(ppair_p);
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
    char *ds = netutil_get_remote_server_data((char *)server_host, server_port, request, &buf, &body_len, &err);
    if (ds) {
        if (maxsize == body_len){
            memcpy(body_buf, buf, body_len);
        }
        free(ds);
    }
}

static void MakeAACBuf(const char* frameBuf, int frameLen, char* aacBuf, int *aacLen,
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

void WriteIntoFile(M4u8TSMaker *maker)
{
    char tsname[4096];
    char tsname2[4096];

    if (!strcmp(maker->ts_name_, "x")){
        int l = strlen(maker->mp4_filename_);
        if (l > 4095){
            l = 4095;
        }
        strncpy(tsname2, maker->mp4_filename_, l+1);
        const char *p = tsname2, *pe = tsname2;
        if (!strcmp(maker->ts_name_, "x")){
            while (*p) {
                if (*(++p) == '/'){
                    pe = p+1;
                }
            }
        }
        sprintf(tsname, "%s.up.%d.ts", pe, maker->seg_index_);
    }
    else {
        int l = strlen(maker->ts_name_);
        if (l > 4095){
            l = 4095;
        }
        strncpy(tsname2, maker->ts_name_, l+1);
        const char *pe = tsname2;
        sprintf(tsname, "%s.up.%d.ts", pe, maker->seg_index_);
    }

    InitMpegtsMuxer(tsname, maker->mpegts_muxer_);
    MpegOpen(maker->samplerate_, maker->channel_, maker->spsbuf_, maker->spssize_, maker->ppsbuf_, maker->ppssize_, maker->mpegts_muxer_);

    long long begin_timestamp = 0.0;
    bool is_audio_begin_ = false;

    multimap_iterator_t it = multimap_begin(maker->samples_);
    for (; !iterator_equal(it, multimap_end(maker->samples_)); it = iterator_next(it))
    {
        M4u8TSSegSample *sam = (M4u8TSSegSample *)pair_second((const pair_t *)iterator_get_pointer(it));
        char* data_buf = maker->buf_body_ + sam->offset;

        if (begin_timestamp == 0) begin_timestamp = sam->timestamp;
        long long sam_timestamp = sam->timestamp*1000;
        long long sam_composition_timestamp =
            sam->composition_timestamp*1000;

        if (sam->is_video_)   // ÊÓÆµ
        {
            // Ìæ»»ÆðÊ¼Âë
            char* pbuf = data_buf;
            char* pend = pbuf + sam->size;
            while (pbuf < pend)
            {
                vp_uint32_t unitsize = BytesToUI32(pbuf);

                memcpy(pbuf, kStartCode, 4);

                //                 mpegts_muxer_->AddAVCData(pbuf, unitsize+4, 
                //                     h264_pts, it->second.is_key_);


                pbuf += 4;
                pbuf += unitsize;
            }

            AddAVCData(data_buf, sam->size, 
                    sam_timestamp, sam_composition_timestamp, sam->is_key_, maker->mpegts_muxer_);

            //assert(pbuf == pend);
        }
        else    // ÒôÆµ
        {
            AddAACData(data_buf, sam->size, sam_timestamp, maker->mpegts_muxer_);
            is_audio_begin_ = true;
        }
    }
    
    MpegClose(maker->mpegts_muxer_);

    exit(0);
    printf("end\n");
}

#ifdef __cplusplus
}
#endif

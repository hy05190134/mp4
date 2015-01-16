#ifdef __cplusplus
extern "C"
{
#endif

#include "mp4u8_index.h"

#include <stdio.h>
#include "common/byte_stream.h" //use for serialize & deserialize

void GetAudioCodec(int* sampleRate, int* channelCount,
        char* aacCodecBuf, int* aacCodecSize, const Mp4u8Index *index)
{
    *sampleRate = index->samplerate_;
    *channelCount = index->channel_;

    *aacCodecSize = index->aac_codec_size_;
    memcpy(aacCodecBuf, index->aac_codec_buf_, index->aac_codec_size_);
}

void GetVideoCodec(int* width, int* height,
        char* spsBuf, int* spsSize,
        char* ppsBuf, int* ppsSize, const Mp4u8Index *index)
{
    *width = index->width_;
    *height = index->height_;

    *spsSize = index->sps_size_;
    memcpy(spsBuf, index->sps_buf_, index->sps_size_);

    *ppsSize = index->pps_size_;
    memcpy(ppsBuf, index->pps_buf_, index->pps_size_);
}

//use to register Mp4u8Sample userdata type
static void _sample_init(const void *input, void *output)
{
    ((Mp4u8Sample *)input)->timestamp = 0.0;
    ((Mp4u8Sample *)input)->composition_timestamp = 0.0;
    ((Mp4u8Sample *)input)->offset = 0;
    ((Mp4u8Sample *)input)->size = 0;
    ((Mp4u8Sample *)input)->is_key_ = 0;
    *(bool_t *)output = true;
}

static void _sample_destroy(const void *input, void *output)
{
    ((Mp4u8Sample *)input)->timestamp = 0.0;
    ((Mp4u8Sample *)input)->composition_timestamp = 0.0;
    ((Mp4u8Sample *)input)->offset = 0;
    ((Mp4u8Sample *)input)->size = 0;
    ((Mp4u8Sample *)input)->is_key_ = 0;
    *(bool_t *)output = true;
}

static void _sample_copy(const void *first, const void *second, void *output)
{
    ((Mp4u8Sample *)first)->timestamp = ((Mp4u8Sample *)second)->timestamp;
    ((Mp4u8Sample *)first)->composition_timestamp = ((Mp4u8Sample *)second)->composition_timestamp;
    ((Mp4u8Sample *)first)->offset = ((Mp4u8Sample *)second)->offset;
    ((Mp4u8Sample *)first)->size = ((Mp4u8Sample *)second)->size;
    ((Mp4u8Sample *)first)->is_key_ = ((Mp4u8Sample *)second)->is_key_;
    *(bool_t *)output = true;
}

static void _sample_less(const void *first, const void *second, void *output)
{
    *(bool_t *)output = ((Mp4u8Sample *)first)->timestamp < ((Mp4u8Sample *)second)->timestamp ? true : false;
}

//use to register Mp4u8Segment userdata type
static void _segment_init(const void *input, void *output)
{
    ((Mp4u8Segment *)input)->audio_beg_samno_ = 0;
    ((Mp4u8Segment *)input)->audio_end_samno_ = 0;
    ((Mp4u8Segment *)input)->video_beg_samno_ = 0;
    ((Mp4u8Segment *)input)->video_beg_samno_ = 0;
    ((Mp4u8Segment *)input)->seg_data_off_ = 0;
    ((Mp4u8Segment *)input)->seg_data_size_ = 0;
    *(bool_t *)output = true;
}

static void _segment_destroy(const void *input, void *output)
{
    ((Mp4u8Segment *)input)->audio_beg_samno_ = 0;
    ((Mp4u8Segment *)input)->audio_end_samno_ = 0;
    ((Mp4u8Segment *)input)->video_beg_samno_ = 0;
    ((Mp4u8Segment *)input)->video_beg_samno_ = 0;
    ((Mp4u8Segment *)input)->seg_data_off_ = 0;
    ((Mp4u8Segment *)input)->seg_data_size_ = 0;
    *(bool_t *)output = true;
}

static void _segment_copy(const void *first, const void *second, void *output)
{
    ((Mp4u8Segment *)first)->audio_beg_samno_ = ((Mp4u8Segment *)second)->audio_beg_samno_;
    ((Mp4u8Segment *)first)->audio_end_samno_ = ((Mp4u8Segment *)second)->audio_end_samno_;
    ((Mp4u8Segment *)first)->video_beg_samno_ = ((Mp4u8Segment *)second)->video_beg_samno_;
    ((Mp4u8Segment *)first)->video_end_samno_ = ((Mp4u8Segment *)second)->video_end_samno_;
    ((Mp4u8Segment *)first)->seg_data_off_ = ((Mp4u8Segment *)second)->seg_data_off_;
    ((Mp4u8Segment *)first)->seg_data_size_ = ((Mp4u8Segment *)second)->seg_data_size_;
    *(bool_t *)output = true;
}

static void _segment_less(const void *first, const void *second, void *output)
{
    *(bool_t *)output = ((Mp4u8Segment *)first)->audio_beg_samno_ < ((Mp4u8Segment *)second)->audio_beg_samno_ ? true : false;
}

void InitMp4u8Index(Mp4u8Index *index)
{
    index->version_ = -1;
    index->aac_codec_buf_ = NULL;
    index->sps_buf_ = NULL;
    index->pps_buf_ = NULL;
    type_register(Mp4u8Sample, _sample_init, _sample_copy, _sample_less, _sample_destroy);
    type_register(Mp4u8Segment, _segment_init, _segment_copy, _segment_less, _sample_destroy);
    index->audio_samples_ = create_vector(Mp4u8Sample);
    assert(NULL != index->audio_samples_);
    vector_init(index->audio_samples_);
    index->video_samples_ = create_vector(Mp4u8Sample);
    assert(NULL != index->video_samples_);
    vector_init(index->video_samples_);
    index->seg_map_ = create_vector(Mp4u8Segment);
    assert(NULL != index->seg_map_);
    vector_init(index->seg_map_);
}

void DestroyMp4u8Index(Mp4u8Index *index)
{
    if (index->aac_codec_buf_){
        free(index->aac_codec_buf_);
        index->aac_codec_buf_ = NULL;
    }

    if (index->sps_buf_){
        free(index->sps_buf_);
        index->sps_buf_ = NULL;
    }

    if (index->pps_buf_){
        free(index->pps_buf_);
        index->pps_buf_ = NULL;
    }

    if (index->audio_samples_ != NULL){
        vector_destroy(index->audio_samples_);
        index->audio_samples_ = NULL;
    }

    if (index->video_samples_ != NULL){
        vector_destroy(index->video_samples_);
        index->seg_map_ = NULL;
    }

    if (index->seg_map_ != NULL){
        vector_destroy(index->seg_map_);
        index->seg_map_ = NULL;
    }
}


const char* SampleFromBuf(const char *buf, bool isVideo, Mp4u8Sample *sample)
{
    if (isVideo)
    {
        sample->timestamp = BytesToDouble(buf); buf += 8;
        sample->composition_timestamp = BytesToDouble(buf); buf += 8;
        sample->offset = BytesToUI64(buf);      buf += 8;
        sample->size = BytesToUI24(buf);        buf += 3;   // sizeÕ¼3¸ö×Ö½Ú
        sample->is_key_ = BytesToUI08(buf);     buf += 1;
    }
    else    // ÒôÆµÃ»ÓÐcomposition_timestamp£¬ºÍtimestampÒ»ÖÂ
    {
        sample->timestamp = BytesToDouble(buf); buf += 8;
        sample->offset = BytesToUI64(buf);      buf += 8;
        sample->size = BytesToUI24(buf);        buf += 3;   // sizeÕ¼3¸ö×Ö½Ú

        sample->composition_timestamp = sample->timestamp;
        sample->is_key_ = 1;
    }

    return buf;
}

// ---------------------------------------------------------------------------
// Mp4u8Segment

const char* SegmentFromBuf(const char *buf, Mp4u8Segment *segment)
{
    segment->audio_beg_samno_ = BytesToUI32(buf);    buf += 4;
    segment->audio_end_samno_ = BytesToUI32(buf);    buf += 4;
    segment->video_beg_samno_ = BytesToUI32(buf);    buf += 4;
    segment->video_end_samno_ = BytesToUI32(buf);    buf += 4;
    segment->seg_data_off_    = BytesToUI64(buf);    buf += 8;
    segment->seg_data_size_   = BytesToUI64(buf);    buf += 8;

    return buf;
}

bool ReadFromFile(const char *readFile, Mp4u8Index *mp4u8_index)
{
    FILE *fp = fopen(readFile, "rb");
    if (!fp) {
        return false;
    }

    int read_file_size = 0;
    fseek(fp, 0, SEEK_END);
    read_file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *tmpbuf = (char *)malloc(read_file_size);
    if (tmpbuf == NULL){
        return false;
    }
    fread(tmpbuf, read_file_size, 1, fp);
    fclose(fp);

    ReadFromBuf(tmpbuf, read_file_size, mp4u8_index);
    free(tmpbuf);

    return true;
}

bool ReadFromBuf(const char *fromBuf, int fromBufSize, Mp4u8Index *mp4u8_index)
{
    const char* pbuf = fromBuf;
    const char* pend = fromBuf + fromBufSize;

    // 1. base info

    mp4u8_index->version_ = BytesToUI08(pbuf);       pbuf += 1;
    mp4u8_index->filesize_ = BytesToUI64(pbuf);      pbuf += 8;
    mp4u8_index->duration_ = BytesToDouble(pbuf);    pbuf += 8;

    // 2. other info

    mp4u8_index->samplerate_ = BytesToUI16(pbuf);    pbuf += 2;
    mp4u8_index->channel_ = BytesToUI16(pbuf);       pbuf += 2;
    mp4u8_index->aac_codec_size_ = BytesToUI16(pbuf);            pbuf += 2;
    mp4u8_index->aac_codec_buf_ = (char *)malloc(mp4u8_index->aac_codec_size_);
    memcpy(mp4u8_index->aac_codec_buf_, pbuf, mp4u8_index->aac_codec_size_);  
    pbuf += mp4u8_index->aac_codec_size_;

    mp4u8_index->width_ = BytesToUI16(pbuf);         pbuf += 2;
    mp4u8_index->height_ = BytesToUI16(pbuf);        pbuf += 2;

    mp4u8_index->sps_size_ = BytesToUI16(pbuf);      pbuf += 2;
    mp4u8_index->sps_buf_ = (char *)malloc(mp4u8_index->sps_size_);
    memcpy(mp4u8_index->sps_buf_, pbuf, mp4u8_index->sps_size_);  
    pbuf += mp4u8_index->sps_size_;

    mp4u8_index->pps_size_ = BytesToUI16(pbuf);      pbuf += 2;
    mp4u8_index->pps_buf_ = (char *)malloc(mp4u8_index->pps_size_);
    memcpy(mp4u8_index->pps_buf_, pbuf, mp4u8_index->pps_size_);  
    pbuf += mp4u8_index->pps_size_;

    // 3. audio & video samples

    // 3.1 audio_samples
    vp_uint32_t audio_count = BytesToUI32(pbuf);    pbuf += 4;
    unsigned int i = 0;
    for (; i < audio_count; ++i)
    {
        Mp4u8Sample sam;
        pbuf = SampleFromBuf(pbuf, false, &sam);

        vector_push_back(mp4u8_index->audio_samples_, &sam);
    }

    // 3.2 video samples
    vp_uint32_t video_count = BytesToUI32(pbuf);    pbuf += 4;
    unsigned int j = 0;
    for (; j < video_count; ++j)
    {
        Mp4u8Sample sam;
        pbuf = SampleFromBuf(pbuf, true, &sam);

        vector_push_back(mp4u8_index->video_samples_, &sam);
    }

    vp_uint32_t seg_count = BytesToUI32(pbuf);    pbuf += 4;
    unsigned int k = 0;
    for (;k < seg_count; ++k)
    {
        Mp4u8Segment segment;
        pbuf = SegmentFromBuf(pbuf, &segment);

        vector_push_back(mp4u8_index->seg_map_, &segment);
    }

    return true;
}

#ifdef __cplusplus
}
#endif

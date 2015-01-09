#ifdef __cplusplus
extern "C"
{
#endif

#include "mp4u8_index.h"

#include <stdio.h>
#include "common/byte_stream.h" //use for serialize & deserialize

const char* Mp4u8Sample::FromBuf(const char* buf, bool isVideo)
{
    if (isVideo)
    {
        timestamp = BytesToDouble(buf); buf += 8;
        composition_timestamp = BytesToDouble(buf); buf += 8;
        offset = BytesToUI64(buf);      buf += 8;
        size = BytesToUI24(buf);        buf += 3;   // sizeÕ¼3¸ö×Ö½Ú
        is_key_ = BytesToUI08(buf);     buf += 1;
    }
    else    // ÒôÆµÃ»ÓÐcomposition_timestamp£¬ºÍtimestampÒ»ÖÂ
    {
        timestamp = BytesToDouble(buf); buf += 8;
        offset = BytesToUI64(buf);      buf += 8;
        size = BytesToUI24(buf);        buf += 3;   // sizeÕ¼3¸ö×Ö½Ú

        composition_timestamp = timestamp;
        is_key_ = 1;
    }

    return buf;
}

char* Mp4u8Sample::ToBuf(char* buf, bool isVideo)
{
    if (isVideo)
    {
        DoubleToBytes(buf, timestamp);  buf += 8;
        DoubleToBytes(buf, composition_timestamp);  buf += 8;
        UI64ToBytes(buf, offset);       buf += 8;
        UI24ToBytes(buf, size);         buf += 3;   // sizeÕ¼3¸ö×Ö½Ú
        UI08ToBytes(buf, is_key_);      buf += 1;
    }
    else
    {
        DoubleToBytes(buf, timestamp);  buf += 8;
        UI64ToBytes(buf, offset);       buf += 8;
        UI24ToBytes(buf, size);         buf += 3;   // sizeÕ¼3¸ö×Ö½Ú
    }

    return buf;
}

// ---------------------------------------------------------------------------
// Mp4u8Segment

const char* Mp4u8Segment::FromBuf(const char* buf)
{
    audio_beg_samno_ = BytesToUI32(buf);    buf += 4;
    audio_end_samno_ = BytesToUI32(buf);    buf += 4;
    video_beg_samno_ = BytesToUI32(buf);    buf += 4;
    video_end_samno_ = BytesToUI32(buf);    buf += 4;
    seg_data_off_    = BytesToUI64(buf);    buf += 8;
    seg_data_size_   = BytesToUI64(buf);    buf += 8;

    return buf;
}

char* Mp4u8Segment::ToBuf(char* buf)
{
    UI32ToBytes(buf, audio_beg_samno_);      buf += 4;
    UI32ToBytes(buf, audio_end_samno_);      buf += 4;
    UI32ToBytes(buf, video_beg_samno_);      buf += 4;
    UI32ToBytes(buf, video_end_samno_);      buf += 4;
    UI64ToBytes(buf, seg_data_off_);         buf += 8;
    UI64ToBytes(buf, seg_data_size_);        buf += 8;

    return buf;
}

bool ReadFromFile(const char *readFile, Mp4u8Index &mp4u8_index)
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

bool ReadFromBuf(const char *fromBuf, int fromBufSize, Mp4u8Index &mp4u8_index)
{
    const char* pbuf = fromBuf;
    const char* pend = fromBuf + fromBufSize;

    // 1. base info

    mp4u8_index.version_ = BytesToUI08(pbuf);       pbuf += 1;
    mp4u8_index.filesize_ = BytesToUI64(pbuf);      pbuf += 8;
    mp4u8_index.duration_ = BytesToDouble(pbuf);    pbuf += 8;

    // 2. other info

    mp4u8_index.samplerate_ = BytesToUI16(pbuf);    pbuf += 2;
    mp4u8_index.channel_ = BytesToUI16(pbuf);       pbuf += 2;
    mp4u8_index.aac_codec_size_ = BytesToUI16(pbuf);            pbuf += 2;
    mp4u8_index.aac_codec_buf_ = (char *)malloc(mp4u8_index.aac_codec_size_);
    memcpy(mp4u8_index.aac_codec_buf_, pbuf, mp4u8_index.aac_codec_size_);  
    pbuf += mp4u8_index.aac_codec_size_;

    mp4u8_index.width_ = BytesToUI16(pbuf);         pbuf += 2;
    mp4u8_index.height_ = BytesToUI16(pbuf);        pbuf += 2;

    mp4u8_index.sps_size_ = BytesToUI16(pbuf);      pbuf += 2;
    mp4u8_index.sps_buf_ = (char *)malloc(mp4u8_index.sps_size_);
    memcpy(mp4u8_index.sps_buf_, pbuf, mp4u8_index.sps_size_);  
    pbuf += mp4u8_index.sps_size_;

    mp4u8_index.pps_size_ = BytesToUI16(pbuf);      pbuf += 2;
    mp4u8_index.pps_buf_ = (char *)malloc(mp4u8_index.pps_size_);
    memcpy(mp4u8_index.pps_buf_, pbuf, mp4u8_index.pps_size_);  
    pbuf += mp4u8_index.pps_size_;

    // 3. audio & video samples

    // 3.1 audio_samples
    mp4u8_index.audio_samples_.clear();
    vp_uint32_t audio_count = BytesToUI32(pbuf);    pbuf += 4;
    for (unsigned int i = 0; i < audio_count; ++i)
    {
        Mp4u8Sample sam;
        pbuf = sam.FromBuf(pbuf, false);

        mp4u8_index.audio_samples_.push_back(sam);
    }

    // 3.2 video samples
    mp4u8_index.video_samples_.clear();
    vp_uint32_t video_count = BytesToUI32(pbuf);    pbuf += 4;
    for (unsigned int i = 0; i < video_count; ++i)
    {
        Mp4u8Sample sam;
        pbuf = sam.FromBuf(pbuf, true);

        mp4u8_index.video_samples_.push_back(sam);
    }

    mp4u8_index.seg_map_.clear();
    vp_uint32_t seg_count = BytesToUI32(pbuf);    pbuf += 4;
    for (unsigned int i = 0; i < seg_count; ++i)
    {
        Mp4u8Segment segment;
        pbuf = segment.FromBuf(pbuf);

        mp4u8_index.seg_map_.push_back(segment);
    }

    return true;
}

#ifdef __cplusplus
}
#endif

/*******************************************************************************
* mpegts_muxer->cpp
* Copyright: (c) 2014 Haibin Du(haibindev.cnblogs.com). All rights reserved.
* -----------------------------------------------------------------------------
*
* -----------------------------------------------------------------------------
* 2014-9-12 8:07 - Created (Haibin Du)
******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

#include <memory.h>
#include <stdlib.h>
#include "mpegts_muxer.h"

static unsigned char ngx_rtmp_mpegts_header[] = {

    /* TS */
    0x47, 0x40, 0x00, 0x10, 0x00,
    /* PSI */
    0x00, 0xb0, 0x0d, 0x00, 0x01, 0xc1, 0x00, 0x00,
    /* PAT */
    0x00, 0x01, 0xf0, 0x01,
    /* CRC */
    0x2e, 0x70, 0x19, 0x05,
    /* stuffing 167 bytes */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    /* TS */
    0x47, 0x50, 0x01, 0x10, 0x00,
    /* PSI */
    0x02, 0xb0, 0x17, 0x00, 0x01, 0xc1, 0x00, 0x00,
    /* PMT */
    0xe1, 0x00,
    0xf0, 0x00,
    0x1b, 0xe1, 0x00, 0xf0, 0x00, /* h264 */
    0x0f, 0xe1, 0x01, 0xf0, 0x00, /* aac */
    /*0x03, 0xe1, 0x01, 0xf0, 0x00,*/ /* mp3 */
    /* CRC */
    0x2f, 0x44, 0xb9, 0x9b, /* crc for aac */
    /*0x4e, 0x59, 0x3d, 0x1e,*/ /* crc for mp3 */
    /* stuffing 157 bytes */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


/* 700 ms PCR delay */
//#define NGX_RTMP_HLS_DELAY  63000
#define NGX_RTMP_HLS_DELAY 0


int
    ngx_rtmp_mpegts_write_header(FILE *file)
{
    size_t rc = fwrite(ngx_rtmp_mpegts_header, 
        sizeof(ngx_rtmp_mpegts_header), 1, file);

    return rc > 0;
}


static unsigned char *
    ngx_rtmp_mpegts_write_pcr(unsigned char *p, vp_uint64_t pcr)
{
    *p++ = (unsigned char) (pcr >> 25);
    *p++ = (unsigned char) (pcr >> 17);
    *p++ = (unsigned char) (pcr >> 9);
    *p++ = (unsigned char) (pcr >> 1);
    *p++ = (unsigned char) (pcr << 7 | 0x7e);
    *p++ = 0;

    return p;
}


static unsigned char *
    ngx_rtmp_mpegts_write_pts(unsigned char *p, unsigned int fb, vp_uint64_t pts)
{
    unsigned int val;

    val = fb << 4 | (((pts >> 30) & 0x07) << 1) | 1;
    *p++ = (unsigned char) val;

    val = (((pts >> 15) & 0x7fff) << 1) | 1;
    *p++ = (unsigned char) (val >> 8);
    *p++ = (unsigned char) val;

    val = (((pts) & 0x7fff) << 1) | 1;
    *p++ = (unsigned char) (val >> 8);
    *p++ = (unsigned char) val;

    return p;
}


int
    ngx_rtmp_mpegts_write_frame(FILE *file, ngx_rtmp_mpegts_frame_t *f,
    const char* dataBuf, int dataSize)
{
    unsigned int  pes_size, header_size, body_size, in_size, stuff_size, flags;
    unsigned char      packet[188], *p, *base;
    int   first, rc;

    first = 1;

    const char* pdata = dataBuf;
    const char* pdataend = dataBuf + dataSize;
    while (pdata < pdataend)
    {
        p = packet;

        f->cc++;

        *p++ = 0x47;
        *p++ = (unsigned char) (f->pid >> 8);

        if (first)
        {
            p[-1] |= 0x40;
        }

        *p++ = (unsigned char) f->pid;
        *p++ = 0x10 | (f->cc & 0x0f); /* payload */

        if (first)
        {

            if (f->key)
            {
                packet[3] |= 0x20; /* adaptation */

                *p++ = 7;    /* size */
                *p++ = 0x50; /* random access + PCR */

                p = ngx_rtmp_mpegts_write_pcr(p, f->dts - NGX_RTMP_HLS_DELAY);
            }

            /* PES header */

            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x01;
            *p++ = (unsigned char) f->sid;

            header_size = 5;
            flags = 0x80; /* PTS */

            if (f->dts != f->pts)
            {
                header_size += 5;
                flags |= 0x40; /* DTS */
            }

            pes_size = (pdataend - pdata) + header_size + 3;
            if (pes_size > 0xffff)
            {
                pes_size = 0;
            }

            *p++ = (unsigned char) (pes_size >> 8);
            *p++ = (unsigned char) pes_size;
            *p++ = 0x80; /* H222 */
            *p++ = (unsigned char) flags;
            *p++ = (unsigned char) header_size;

            p = ngx_rtmp_mpegts_write_pts(p, flags >> 6, f->pts +
                NGX_RTMP_HLS_DELAY);

            if (f->dts != f->pts)
            {
                p = ngx_rtmp_mpegts_write_pts(p, 1, f->dts +
                    NGX_RTMP_HLS_DELAY);
            }

            first = 0;
        }

        body_size = (unsigned int) (packet + sizeof(packet) - p);
        in_size = (unsigned int) (pdataend - pdata);

        if (body_size <= in_size)
        {
            memcpy(p, pdata, body_size);
            pdata += body_size;
        }
        else
        {
            stuff_size = (body_size - in_size);

            if (packet[3] & 0x20)
            {

                /* has adaptation */

                base = &packet[5] + packet[4];
                int len = p - base;
                p = (unsigned char *)memmove(base + stuff_size, base, p - base);
                p += len;   // ######
                memset(base, 0xff, stuff_size);
                packet[4] += (unsigned char) stuff_size;

            }
            else
            {

                /* no adaptation */

                packet[3] |= 0x20;
                int len = p - &packet[4];
                p = (unsigned char *)memmove(&packet[4] + stuff_size, &packet[4],
                    p - &packet[4]);
                p += len;   // ######

                packet[4] = (unsigned char) (stuff_size - 1);
                if (stuff_size >= 2)
                {
                    packet[5] = 0;
                    memset(&packet[6], 0xff, stuff_size - 2);
                }
            }

            memcpy(p, pdata, in_size);
            pdata = pdataend;
        }

        rc = fwrite(packet, sizeof(packet), 1, file);
        if (rc < 0)
        {
            return rc;
        }
    }

    return 0;
}

static const char kStartCode[] = {0x00, 0x00, 0x00, 0x01};

void InitMpegtsMuxer(const char *tsFilename, MpegtsMuxer *mpegts_muxer)
{
    strcpy(mpegts_muxer->ts_filename_, tsFilename);
    mpegts_muxer->fts_ = NULL;

    mpegts_muxer->sample_rate_ = 0;
    mpegts_muxer->channel_count_ = 0;
    mpegts_muxer->aac_frame_count_ = 0;

    mpegts_muxer->spsppsframe_ = NULL;
    mpegts_muxer->spsppsframe_len_ = 0;
    mpegts_muxer->is_send_aud_ = false;
    mpegts_muxer->is_write_spspps_ = false;

    mpegts_muxer->tmp_264buf_ = NULL;
    mpegts_muxer->tmp_264buf_capacity_ = 0;
    mpegts_muxer->tmp_aacbuf_ = NULL;
    mpegts_muxer->tmp_aacbuf_capacity_ = 0;

    mpegts_muxer->audio_cc_ = mpegts_muxer->video_cc_ = 0;
}

void DestroyMpegtsMuxer(MpegtsMuxer *mpegts_muxer)
{
    if (mpegts_muxer->spsppsframe_){
        free(mpegts_muxer->spsppsframe_);
        mpegts_muxer->spsppsframe_ = NULL;
    }
    if (mpegts_muxer->tmp_264buf_){
        free(mpegts_muxer->tmp_264buf_);
        mpegts_muxer->tmp_264buf_ = NULL;
    }
    if (mpegts_muxer->tmp_aacbuf_){
        free(mpegts_muxer->tmp_aacbuf_);
        mpegts_muxer->tmp_aacbuf_ = NULL; 
    }
}

bool MpegOpen(int samplerate, int channel, const char* spsBuf, int spsSize,
    const char* ppsBuf, int ppsSize, MpegtsMuxer *mpegts_muxer)
{
    mpegts_muxer->fts_ = fopen(mpegts_muxer->ts_filename_, "wb");
    if (!mpegts_muxer->fts_) return false;

    mpegts_muxer->sample_rate_ = samplerate;
    mpegts_muxer->channel_count_ = channel;

    mpegts_muxer->spsppsframe_len_ = spsSize + ppsSize + 4 + 4;
    mpegts_muxer->spsppsframe_ = (char *)malloc(mpegts_muxer->spsppsframe_len_);
    char* pframe = mpegts_muxer->spsppsframe_;
    memcpy(pframe, kStartCode, 4);      pframe += 4;
    memcpy(pframe, spsBuf, spsSize);  pframe += spsSize;
    memcpy(pframe, kStartCode, 4);      pframe += 4;
    memcpy(pframe, ppsBuf, ppsSize);  pframe += ppsSize;

    ngx_rtmp_mpegts_write_header(mpegts_muxer->fts_);

    /* start fragment with audio to make iPhone happy */

    return true;
}

void MpegClose(MpegtsMuxer *mpegts_muxer)
{
    if (mpegts_muxer->fts_)
    {
        fclose(mpegts_muxer->fts_);
        mpegts_muxer->fts_ = NULL;
    }
}

void AddAVCData(const char* dataBuf, int dataSize,
    unsigned int timestamp, unsigned int compositionTimestamp,
    bool isKeyframe, MpegtsMuxer *mpegts_muxer)
{
    static unsigned char aud_nal[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xf0 };

    vp_uint8_t naltype = dataBuf[4] & 0x1f;

    int need_size = dataSize + 1024;
    if (need_size > mpegts_muxer->tmp_264buf_capacity_)
    {
        free(mpegts_muxer->tmp_264buf_);
        mpegts_muxer->tmp_264buf_ = (char *)malloc(need_size);
        mpegts_muxer->tmp_264buf_capacity_ = need_size;
    }

    char* tmpbuf = mpegts_muxer->tmp_264buf_;
    char* pbuf = tmpbuf;

//    if (false == is_send_aud_)
    {
        switch (naltype) {
        case 1:
        case 5:
        case 6:
            // aud
            memcpy(pbuf, aud_nal, sizeof(aud_nal));
            pbuf += sizeof(aud_nal);
            mpegts_muxer->is_send_aud_ = true;
        case 9:
            mpegts_muxer->is_send_aud_ = true;
            break;
        }
    }

    switch (naltype) {
    case 1:
        mpegts_muxer->is_write_spspps_ = false;
        break;
    case 5:
        if (mpegts_muxer->is_write_spspps_) {
            break;
        }
        // sps pps
        memcpy(pbuf, mpegts_muxer->spsppsframe_, mpegts_muxer->spsppsframe_len_);
        pbuf += mpegts_muxer->spsppsframe_len_;
        mpegts_muxer->is_write_spspps_ = true;
        break;
    }
    if (isKeyframe && !mpegts_muxer->is_write_spspps_)
    {
        memcpy(pbuf, mpegts_muxer->spsppsframe_, mpegts_muxer->spsppsframe_len_);
        pbuf += mpegts_muxer->spsppsframe_len_;
    }

    memcpy(pbuf, dataBuf, dataSize);
    pbuf += dataSize;

    vp_uint64_t pts = compositionTimestamp * 90;
    vp_uint64_t dts = timestamp * 90;
    
    FlushAVCData(tmpbuf, (pbuf-tmpbuf), isKeyframe, pts, dts, mpegts_muxer);
}

void FlushAVCData(const char* dataBuf, int dataSize,
    bool isKeyframe, vp_uint64_t pts, vp_uint64_t dts, MpegtsMuxer *mpegts_muxer)
{
    ngx_rtmp_mpegts_frame_t frame;

    memset(&frame, 0, sizeof(frame));

    int cts = 0;
    frame.cc = mpegts_muxer->video_cc_;
    frame.dts = dts;
    frame.pts = pts;
    frame.pid = 0x100;
    frame.sid = 0xe0;
    frame.key = isKeyframe ? 1: 0;

    ngx_rtmp_mpegts_write_frame(mpegts_muxer->fts_, &frame, dataBuf, dataSize);

    mpegts_muxer->video_cc_ = frame.cc;
}

static void MakeAACBuf(const char* frameBuf, int frameLen, 
    char* aacBuf, int *aacLen, const MpegtsMuxer *mpegts_muxer)
{
    unsigned int sampling_frequency_index = GetSamplingFrequencyIndex(mpegts_muxer->sample_rate_);

    aacBuf[0] = 0xFF;
    aacBuf[1] = 0xF1; // 0xF9 (MPEG2)
    aacBuf[2] = 0x40 | (sampling_frequency_index << 2) | (mpegts_muxer->channel_count_ >> 2);
    aacBuf[3] = ((mpegts_muxer->channel_count_ & 0x3)<<6) | ((frameLen+7) >> 11);
    aacBuf[4] = ((frameLen+7) >> 3)&0xFF;
    aacBuf[5] = (((frameLen+7) << 5)&0xFF) | 0x1F;
    aacBuf[6] = 0xFC;

    memcpy(aacBuf+7, frameBuf, frameLen);
    *aacLen = frameLen + 7;
}

void AddAACData(const char* dataBuf, int dataSize,
    unsigned int timestamp, MpegtsMuxer *mpegts_muxer)
{
    vp_uint64_t pts = timestamp * 90;

    mpegts_muxer->aac_frame_count_++;
    vp_uint64_t aacpts = mpegts_muxer->aac_frame_count_ * 90000 * 1024 / mpegts_muxer->sample_rate_;

    int need_size = dataSize+7;
    if (need_size > mpegts_muxer->tmp_aacbuf_capacity_)
    {
        free(mpegts_muxer->tmp_aacbuf_);
        mpegts_muxer->tmp_aacbuf_ = (char *)malloc(need_size);
        mpegts_muxer->tmp_aacbuf_capacity_ = need_size;
    }

    char* aacbuf = mpegts_muxer->tmp_aacbuf_;
    int aaclen;

    MakeAACBuf(dataBuf, dataSize, aacbuf, &aaclen, mpegts_muxer);

    FlushAACData(aacbuf, aaclen, pts, pts, mpegts_muxer);
}

void FlushAACData(const char* dataBuf, int dataSize,
    vp_uint64_t pts, vp_uint64_t dts, MpegtsMuxer *mpegts_muxer)
{
    ngx_rtmp_mpegts_frame_t frame;

    memset(&frame, 0, sizeof(frame));

    frame.dts = dts;
    frame.pts = pts;
    frame.cc = mpegts_muxer->audio_cc_;
    frame.pid = 0x101;
    frame.sid = 0xc0;

    ngx_rtmp_mpegts_write_frame(mpegts_muxer->fts_, &frame, dataBuf, dataSize);

    mpegts_muxer->audio_cc_ = frame.cc;
}

unsigned int GetSamplingFrequencyIndex(unsigned int sampling_frequency)
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


#ifdef __cplusplus
}
#endif

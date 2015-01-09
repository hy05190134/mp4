/*
 * MPEG2 transport stream (aka DVB) muxer
 * Copyright (c) 2003 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common/typedefs.h"
#include "bswap.h"
#include "crc.h"

#include "mpegts.h"

#include "common/byte_stream.h"

#include <stdlib.h>

#include <memory.h>
#include <string.h>
#include <assert.h>

#include "mpegtsenc.h"

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

/* write DVB SI sections */

/*********************************************/
/* mpegts section writer */


// static const AVOption options[] = {
//     { "mpegts_transport_stream_id", "Set transport_stream_id field.",
//       offsetof(MpegTSWrite, transport_stream_id), FF_OPT_TYPE_INT, {.dbl = 0x0001 }, 0x0001, 0xffff, AV_OPT_FLAG_ENCODING_PARAM},
//     { "mpegts_original_network_id", "Set original_network_id field.",
//       offsetof(MpegTSWrite, original_network_id), FF_OPT_TYPE_INT, {.dbl = 0x0001 }, 0x0001, 0xffff, AV_OPT_FLAG_ENCODING_PARAM},
//     { "mpegts_service_id", "Set service_id field.",
//       offsetof(MpegTSWrite, service_id), FF_OPT_TYPE_INT, {.dbl = 0x0001 }, 0x0001, 0xffff, AV_OPT_FLAG_ENCODING_PARAM},
//     { "mpegts_pmt_start_pid", "Set the first pid of the PMT.",
//       offsetof(MpegTSWrite, pmt_start_pid), FF_OPT_TYPE_INT, {.dbl = 0x1000 }, 0x1000, 0x1f00, AV_OPT_FLAG_ENCODING_PARAM},
//     { "mpegts_start_pid", "Set the first pid.",
//       offsetof(MpegTSWrite, start_pid), FF_OPT_TYPE_INT, {.dbl = 0x0100 }, 0x0100, 0x0f00, AV_OPT_FLAG_ENCODING_PARAM},
//     {"mpegts_m2ts_mode", "Enable m2ts mode.",
//         offsetof(MpegTSWrite, m2ts_mode), FF_OPT_TYPE_INT, {.dbl = -1 },
//         -1,1, AV_OPT_FLAG_ENCODING_PARAM},
//     { NULL },
// };

// static const AVClass mpegts_muxer_class = {
//     .class_name     = "MPEGTS muxer",
//     .item_name      = av_default_item_name,
//     .option         = options,
//     .version        = LIBAVUTIL_VERSION_INT,
// };

/* NOTE: 4 bytes must be left at the end for the crc32 */
static void mpegts_write_section(FILE* fp, MpegTSSection *s, uint8_t *buf, int len)
{
    unsigned int crc;
    unsigned char packet[TS_PACKET_SIZE];
    const unsigned char *buf_ptr;
    unsigned char *q;
    int first, b, len1, left;

    crc = av_bswap32(av_crc(av_crc_get_table(AV_CRC_32_IEEE), -1, buf, len - 4));
    buf[len - 4] = (crc >> 24) & 0xff;
    buf[len - 3] = (crc >> 16) & 0xff;
    buf[len - 2] = (crc >> 8) & 0xff;
    buf[len - 1] = (crc) & 0xff;

    /* send each packet */
    buf_ptr = buf;
    while (len > 0) {
        first = (buf == buf_ptr);
        q = packet;
        *q++ = 0x47;
        b = (s->pid >> 8);
        if (first)
            b |= 0x40;
        *q++ = b;
        *q++ = s->pid;
        s->cc = (s->cc + 1) & 0xf;
        *q++ = 0x10 | s->cc;
        if (first)
            *q++ = 0; /* 0 offset */
        len1 = TS_PACKET_SIZE - (q - packet);
        if (len1 > len)
            len1 = len;
        memcpy(q, buf_ptr, len1);
        q += len1;
        /* add known padding data */
        left = TS_PACKET_SIZE - (q - packet);
        if (left > 0)
            memset(q, 0xff, left);

        s->write_packet(fp, s, packet);

        buf_ptr += len1;
        len -= len1;
    }
}

static inline void put16(uint8_t **q_ptr, int val)
{
    uint8_t *q;
    q = *q_ptr;
    *q++ = val >> 8;
    *q++ = val;
    *q_ptr = q;
}

static int mpegts_write_section1(FILE* fp, MpegTSSection *s, int tid, int id,
                          int version, int sec_num, int last_sec_num,
                          uint8_t *buf, int len)
{
    uint8_t section[1024], *q;
    unsigned int tot_len;
    /* reserved_future_use field must be set to 1 for SDT */
    unsigned int flags = tid == SDT_TID ? 0xf000 : 0xb000;

    tot_len = 3 + 5 + len + 4;
    /* check if not too big */
    if (tot_len > 1024)
        return -1;

    q = section;
    *q++ = tid;
    put16(&q, flags | (len + 5 + 4)); /* 5 byte header + 4 byte CRC */
    put16(&q, id);
    *q++ = 0xc1 | (version << 1); /* current_next_indicator = 1 */
    *q++ = sec_num;
    *q++ = last_sec_num;
    memcpy(q, buf, len);

    mpegts_write_section(fp, s, section, tot_len);
    return 0;
}



/* we retransmit the SI info at this rate */
#define SDT_RETRANS_TIME 500
#define PAT_RETRANS_TIME 100
#define PCR_RETRANS_TIME 20


static void mpegts_write_pat(MpegTSWrite *ts, FILE* fp)
{
    MpegTSService *service;
    uint8_t data[1012], *q;
    int i;

    q = data;
    for(i = 0; i < ts->nb_services; i++) 
    {
        service = ts->services[i];
        put16(&q, service->sid);
        put16(&q, 0xe000 | service->pmt.pid);
    }
    mpegts_write_section1(fp, &ts->pat, PAT_TID, ts->tsid, 0, 0, 0,
                          data, q - data);
}

static void mpegts_write_pmt(FILE* fp, MpegTSWriteStream* aacStream, MpegTSWriteStream* h264Stream, MpegTSService *service)
{
    //    MpegTSWrite *ts = s->priv_data;
    uint8_t data[1012], *q, *desc_length_ptr, *program_info_length_ptr;
    int val, stream_type;

    q = data;
    put16(&q, 0xe000 | service->pcr_pid);

    program_info_length_ptr = q;
    q += 2; /* patched after */

    /* put program info here */

    val = 0xf000 | (q - program_info_length_ptr - 2);
    program_info_length_ptr[0] = val >> 8;
    program_info_length_ptr[1] = val;

    // audio

    stream_type = STREAM_TYPE_AUDIO_AAC;
    *q++ = stream_type;
    put16(&q, 0xe000 | aacStream->pid);
    desc_length_ptr = q;
    q += 2; /* patched after */

    val = 0xf000 | (q - desc_length_ptr - 2);
    desc_length_ptr[0] = val >> 8;
    desc_length_ptr[1] = val;

    // video

    stream_type = STREAM_TYPE_VIDEO_H264;
    *q++ = stream_type;
    put16(&q, 0xe000 | h264Stream->pid);
    desc_length_ptr = q;
    q += 2; /* patched after */

    val = 0xf000 | (q - desc_length_ptr - 2);
    desc_length_ptr[0] = val >> 8;
    desc_length_ptr[1] = val;

    mpegts_write_section1(fp, &service->pmt, PMT_TID, service->sid, 0, 0, 0,
                          data, q - data);
}

/* NOTE: str == NULL is accepted for an empty string */
static void putstr8(uint8_t **q_ptr, const char *str)
{
    uint8_t *q;
    int len;

    q = *q_ptr;
    if (!str)
        len = 0;
    else
        len = strlen(str);
    *q++ = len;
    memcpy(q, str, len);
    q += len;
    *q_ptr = q;
}

static void mpegts_write_sdt(MpegTSWrite *ts, FILE* fp)
{
    MpegTSService *service;
    uint8_t data[1012], *q, *desc_list_len_ptr, *desc_len_ptr;
    int i, running_status, free_ca_mode, val;

    q = data;
    put16(&q, ts->onid);
    *q++ = 0xff;
    for(i = 0; i < ts->nb_services; i++) {
        service = ts->services[i];
        put16(&q, service->sid);
        *q++ = 0xfc | 0x00; /* currently no EIT info */
        desc_list_len_ptr = q;
        q += 2;
        running_status = 4; /* running */
        free_ca_mode = 0;

        /* write only one descriptor for the service name and provider */
        *q++ = 0x48;
        desc_len_ptr = q;
        q++;
        *q++ = 0x01; /* digital television service */
        putstr8(&q, service->provider_name.c_str());
        putstr8(&q, service->name.c_str());
        desc_len_ptr[0] = q - desc_len_ptr - 1;

        /* fill descriptor length */
        val = (running_status << 13) | (free_ca_mode << 12) |
            (q - desc_list_len_ptr - 2);
        desc_list_len_ptr[0] = val >> 8;
        desc_list_len_ptr[1] = val;
    }
    mpegts_write_section1(fp, &ts->sdt, SDT_TID, ts->tsid, 0, 0, 0,
                          data, q - data);
}

static MpegTSService *mpegts_add_service(MpegTSWrite *ts,
                                         int sid,
                                         const char *provider_name,
                                         const char *name)
{
    MpegTSService *service = (MpegTSService *)malloc(sizeof(MpegTSService));
    if (!service)
        return NULL;

    InitMpegTSService(*service);
    service->pmt.pid = ts->pmt_start_pid + ts->nb_services;
    service->sid = sid;
    service->provider_name = provider_name;
    service->name = name;
    service->pcr_pid = 0x1fff;

    ts->services.push_back(service);
    ts->nb_services++;

    return service;
}

// static int64_t get_pcr(const MpegTSWrite *ts, AVIOContext *pb)
// {
//     return av_rescale(avio_tell(pb) + 11, 8 * PCR_TIME_BASE, ts->mux_rate) +
//            ts->first_pcr;
// }
// 
// static void mpegts_prefix_m2ts_header(MpegTSWrite *ts)
// {
//     if (ts->m2ts_mode) {
//         int64_t pcr = get_pcr(ts, s->pb);
//         uint32_t tp_extra_header = pcr % 0x3fffffff;
//         tp_extra_header = AV_RB32(&tp_extra_header);
//         avio_write(s->pb, (unsigned char *) &tp_extra_header,
//                 sizeof(tp_extra_header));
//     }
// }

static void section_write_packet(FILE* fp, MpegTSSection *s, const uint8_t *packet)
{
    //AVFormatContext *ctx = s->opaque;
    //mpegts_prefix_m2ts_header(ctx);
    //avio_write(ctx->pb, packet, TS_PACKET_SIZE);
    fwrite(packet, TS_PACKET_SIZE, 1, fp);
}

int mpegts_write_header(MpegTSWrite *ts, std::vector<MpegTSWriteStream *>& tswritestream,
    int initAudioCC, int initVideoCC, int initPatCC, int initPmtCC)
{
    MpegTSWriteStream *ts_st;
    MpegTSService *service;

    const char *service_name = DEFAULT_SERVICE_NAME;
    const char *provider_name = DEFAULT_PROVIDER_NAME;

    ts->tsid = ts->transport_stream_id;
    ts->onid = ts->original_network_id;

    service = mpegts_add_service(ts, ts->service_id, provider_name, service_name);
    service->pmt.write_packet = section_write_packet;
    //service->pmt.opaque = s;
    if (initPmtCC >= 0)
        service->pmt.cc = initPmtCC;
    else
        service->pmt.cc = 15;

    ts->pat.pid = PAT_PID;
    if (initPatCC >= 0)
        ts->pat.cc = initPatCC;
    else
        // Initialize at 15 so that it wraps and be equal to 0 for the first packet we write
        ts->pat.cc = 15;
    ts->pat.write_packet = section_write_packet;
    //ts->pat.opaque = s;

    ts->sdt.pid = SDT_PID;
    ts->sdt.cc = 15;
    ts->sdt.write_packet = section_write_packet;
    //ts->sdt.opaque = s;

    //////////////////////////////////////////////////////////////////////////
    // audio stream

    ts_st = (MpegTSWriteStream *)malloc(sizeof(MpegTSWriteStream));
    if (!ts_st)
        goto fail;
    InitMpegTSWriteStream(*ts_st);
    tswritestream.push_back(ts_st);
    ts_st->service = service;

    /* MPEG pid values < 16 are reserved. Applications which set st->id in
    * this range are assigned a calculated pid. */
    ts_st->pid = 31;

    ts_st->payload_pts = AV_NOPTS_VALUE;
    ts_st->payload_dts = AV_NOPTS_VALUE;
    ts_st->first_pts_check = 1;
    if (initAudioCC >= 0)
        ts_st->cc = initAudioCC;
    else
        ts_st->cc = 15;

    //////////////////////////////////////////////////////////////////////////
    // video stream

    ts_st = (MpegTSWriteStream *)malloc(sizeof(MpegTSWriteStream));
    if (!ts_st)
        goto fail;
    InitMpegTSWriteStream(*ts_st);
    tswritestream.push_back(ts_st);
    ts_st->service = service;

    ts_st->pid = 32;

    ts_st->payload_pts = AV_NOPTS_VALUE;
    ts_st->payload_dts = AV_NOPTS_VALUE;
    ts_st->first_pts_check = 1;
    if (initVideoCC >= 0)
        ts_st->cc = initVideoCC;
    else
        ts_st->cc = 15;

    /* update PCR pid by using the first video stream */
    if (service->pcr_pid == 0x1fff) 
    {
        service->pcr_pid = ts_st->pid;
        //pcr_st = st;
    }


    /* if no video stream, use the first stream as PCR */
    if (service->pcr_pid == 0x1fff)
    {
        service->pcr_pid = ts_st->pid;
    }

    //ts->mux_rate = s->mux_rate ? s->mux_rate : 1;
    ts->mux_rate = 1;

    if (ts->mux_rate > 1)
    {
        int max_delay = 200;

        service->pcr_packet_period = (ts->mux_rate * PCR_RETRANS_TIME) /
            (TS_PACKET_SIZE * 8 * 1000);
        ts->sdt_packet_period      = (ts->mux_rate * SDT_RETRANS_TIME) /
            (TS_PACKET_SIZE * 8 * 1000);
        ts->pat_packet_period      = (ts->mux_rate * PAT_RETRANS_TIME) /
            (TS_PACKET_SIZE * 8 * 1000);

        ts->first_pcr = max_delay * PCR_TIME_BASE / AV_TIME_BASE;
    } 
    else 
    {
        /* Arbitrary values, PAT/PMT could be written on key frames */
        ts->sdt_packet_period = 200;
        ts->pat_packet_period = 4000;
        service->pcr_packet_period = 1;
    }

    // output a PCR as soon as possible
    service->pcr_packet_count = service->pcr_packet_period;
    ts->pat_packet_count = ts->pat_packet_period-1;
    ts->sdt_packet_count = ts->sdt_packet_period-1;

//     if (ts->mux_rate == 1)
//         av_log(s, AV_LOG_INFO, "muxrate VBR, ");
//     else
//         av_log(s, AV_LOG_INFO, "muxrate %d, ", ts->mux_rate);
//     av_log(s, AV_LOG_INFO, "pcr every %d pkts, "
//            "sdt every %d, pat/pmt every %d pkts\n",
//            service->pcr_packet_period,
//            ts->sdt_packet_period, ts->pat_packet_period);

    //avio_flush(s->pb);

    return 0;

 fail:
    return -1;
}

/* send SDT, PAT and PMT tables regulary */
static void retransmit_si_info(MpegTSWrite *ts, FILE* fp, std::vector<MpegTSWriteStream *>& tswritestream)
{
    int i;

    if (++ts->sdt_packet_count == ts->sdt_packet_period) 
    {
        ts->sdt_packet_count = 0;
        //mpegts_write_sdt(ts, fp);
    }

    if (++ts->pat_packet_count == ts->pat_packet_period) 
    {
        ts->pat_packet_count = 0;
        mpegts_write_pat(ts, fp);

        for(i = 0; i < ts->nb_services; i++) 
        {
            mpegts_write_pmt(fp, tswritestream[0], tswritestream[1], ts->services[i]);
        }
    }
}

static int write_pcr_bits(uint8_t *buf, int64_t pcr)
{
    int64_t pcr_low = pcr % 300, pcr_high = pcr / 300;

    *buf++ = static_cast<uint8_t>(pcr_high >> 25);
    *buf++ = static_cast<uint8_t>(pcr_high >> 17);
    *buf++ = static_cast<uint8_t>(pcr_high >> 9);
    *buf++ = static_cast<uint8_t>(pcr_high >> 1);
    *buf++ = static_cast<uint8_t>(pcr_high << 7 | pcr_low >> 8 | 0x7e);
    *buf++ = static_cast<uint8_t>(pcr_low);

    return 6;
}

/* Write a single null transport stream packet */
static void mpegts_insert_null_packet(FILE* fp)
{
    uint8_t *q;
    uint8_t buf[TS_PACKET_SIZE];

    q = buf;
    *q++ = 0x47;
    *q++ = 0x00 | 0x1f;
    *q++ = 0xff;
    *q++ = 0x10;
    memset(q, 0x0FF, TS_PACKET_SIZE - (q - buf));
    //mpegts_prefix_m2ts_header(s);
    //avio_write(s->pb, buf, TS_PACKET_SIZE);
    fwrite(buf, TS_PACKET_SIZE, 1, fp);
}

/* Write a single transport stream packet with a PCR and no payload */
static void mpegts_insert_pcr_only(MpegTSWrite *ts, MpegTSWriteStream *ts_st, FILE* fp)
{
    uint8_t *q;
    uint8_t buf[TS_PACKET_SIZE];

    q = buf;
    *q++ = 0x47;
    *q++ = ts_st->pid >> 8;
    *q++ = ts_st->pid;
    *q++ = 0x20 | ts_st->cc;   /* Adaptation only */
    /* Continuity Count field does not increment (see 13818-1 section 2.4.3.3) */
    *q++ = TS_PACKET_SIZE - 5; /* Adaptation Field Length */
    *q++ = 0x10;               /* Adaptation flags: PCR present */

    /* PCR coded into 6 bytes */
    //q += write_pcr_bits(q, get_pcr(ts, s->pb));
    // !!!!!!!!!!!!!!###########################################################
    // #############################################################################
    // ##########################################################################

    /* stuffing bytes */
    memset(q, 0xFF, TS_PACKET_SIZE - (q - buf));

    //mpegts_prefix_m2ts_header(s);
    //avio_write(s->pb, buf, TS_PACKET_SIZE);
    fwrite(buf, TS_PACKET_SIZE, 1, fp);
}

static void write_pts(uint8_t *q, int fourbits, int64_t pts)
{
    int val;

    val = fourbits << 4 | (((pts >> 30) & 0x07) << 1) | 1;
    *q++ = val;
    val = (((pts >> 15) & 0x7fff) << 1) | 1;
    *q++ = val >> 8;
    *q++ = val;
    val = (((pts) & 0x7fff) << 1) | 1;
    *q++ = val >> 8;
    *q++ = val;
}

/* Set an adaptation field flag in an MPEG-TS packet*/
static void set_af_flag(uint8_t *pkt, int flag)
{
    // expect at least one flag to set
    //assert(flag);

    if ((pkt[3] & 0x20) == 0) 
    {
        // no AF yet, set adaptation field flag
        pkt[3] |= 0x20;
        // 1 byte length, no flags
        pkt[4] = 1;
        pkt[5] = 0;
    }
    pkt[5] |= flag;
}

/* Extend the adaptation field by size bytes */
static void extend_af(uint8_t *pkt, int size)
{
    // expect already existing adaptation field
    assert(pkt[3] & 0x20);
    pkt[4] += size;
}

/* Get a pointer to MPEG-TS payload (right after TS packet header) */
static uint8_t *get_ts_payload_start(uint8_t *pkt)
{
    if (pkt[3] & 0x20)
        return pkt + 5 + pkt[4];
    else
        return pkt + 4;
}

/* Add a pes header to the front of payload, and segment into an integer number of
 * ts packets. The final ts packet is padded using an over-sized adaptation header
 * to exactly fill the last ts packet.
 * NOTE: 'payload' contains a complete PES payload.
 */
static void mpegts_write_pes(MpegTSWrite *ts, std::vector<MpegTSWriteStream *>& tswritestream,
    MpegTSWriteStream *ts_st, FILE* fp,
    const uint8_t *payload, int payload_size,
    int64_t pts, int64_t dts, int key, int isVideo)
{
    uint8_t buf[TS_PACKET_SIZE];
    uint8_t *q;
    int val, is_start, len, header_len, write_pcr, private_code, flags;
    int afc_len, stuffing_len;
    int64_t pcr = -1; /* avoid warning */
    int max_delay = 200;
    int64_t delay = max_delay * 90000 / AV_TIME_BASE;

    is_start = 1;
    while (payload_size > 0) 
    {
        retransmit_si_info(ts, fp, tswritestream);

        write_pcr = 0;
        if (ts_st->pid == ts_st->service->pcr_pid) 
        {
            if (ts->mux_rate > 1 || is_start) // VBR pcr period is based on frames
                ts_st->service->pcr_packet_count++;

            if (ts_st->service->pcr_packet_count >=
                ts_st->service->pcr_packet_period) 
            {
                ts_st->service->pcr_packet_count = 0;
                write_pcr = 1;
            }
        }

//         if (ts->mux_rate > 1 && dts != AV_NOPTS_VALUE &&
//             (dts - get_pcr(ts, s->pb)/300) > delay)
//         {
//             /* pcr insert gets priority over null packet insert */
//             if (write_pcr)
//                 mpegts_insert_pcr_only(ts, ts_st, fp);
//             else
//                 mpegts_insert_null_packet(fp);
//             continue; /* recalculate write_pcr and possibly retransmit si_info */
//         }

        /* prepare packet header */
        q = buf;
        *q++ = 0x47;
        val = (ts_st->pid >> 8);
        if (is_start)
            val |= 0x40;
        *q++ = val;
        *q++ = ts_st->pid;
        ts_st->cc = (ts_st->cc + 1) & 0xf;
        *q++ = 0x10 | ts_st->cc; // payload indicator + CC
        if (key && is_start && pts != AV_NOPTS_VALUE) 
        {
            // set Random Access for key frames
            if (ts_st->pid == ts_st->service->pcr_pid)
                write_pcr = 1;
            set_af_flag(buf, 0x40);
            q = get_ts_payload_start(buf);
        }
        if (write_pcr) 
        {
            set_af_flag(buf, 0x10);
            q = get_ts_payload_start(buf);
            // add 11, pcr references the last byte of program clock reference base
//             if (ts->mux_rate > 1)
//                 pcr = get_pcr(ts, s->pb);
//             else
                pcr = (dts - delay)*300;
            if (dts != AV_NOPTS_VALUE && dts < pcr / 300)
            {
                //av_log(s, AV_LOG_WARNING, "dts < pcr, TS is invalid\n");
                //assert(0);
            }
            extend_af(buf, write_pcr_bits(q, pcr));
            q = get_ts_payload_start(buf);
        }

        if (is_start) 
        {
            int pes_extension = 0;
            /* write PES header */
            *q++ = 0x00;
            *q++ = 0x00;
            *q++ = 0x01;
            private_code = 0;
            if (isVideo) 
            {
                *q++ = 0xe0;
            } 
            else if (1) 
            {
                *q++ = 0xc0;
            } 
            else 
            {
                *q++ = 0xbd;
            }

            header_len = 0;
            flags = 0;
            if (pts != AV_NOPTS_VALUE) {
                header_len += 5;
                flags |= 0x80;
            }
            if (dts != AV_NOPTS_VALUE && pts != AV_NOPTS_VALUE && dts != pts) {
                header_len += 5;
                flags |= 0x40;
            }

            len = payload_size + header_len + 3;
            if (private_code != 0)
                len++;
            if (len > 0xffff)
                len = 0;
            *q++ = len >> 8;
            *q++ = len;
            val = 0x80;

            *q++ = val;
            *q++ = flags;
            *q++ = header_len;
            if (pts != AV_NOPTS_VALUE) 
            {
                write_pts(q, flags >> 6, pts);
                q += 5;
            }
            if (dts != AV_NOPTS_VALUE && pts != AV_NOPTS_VALUE && dts != pts) 
            {
                write_pts(q, 1, dts);
                q += 5;
            }

            if (private_code != 0)
                *q++ = private_code;
            is_start = 0;
        }
        /* header size */
        header_len = q - buf;
        /* data len */
        len = TS_PACKET_SIZE - header_len;
        if (len > payload_size)
            len = payload_size;
        stuffing_len = TS_PACKET_SIZE - header_len - len;
        if (stuffing_len > 0) 
        {
            /* add stuffing with AFC */
            if (buf[3] & 0x20) 
            {
                /* stuffing already present: increase its size */
                afc_len = buf[4] + 1;
                memmove(buf + 4 + afc_len + stuffing_len,
                        buf + 4 + afc_len,
                        header_len - (4 + afc_len));
                buf[4] += stuffing_len;
                memset(buf + 4 + afc_len, 0xff, stuffing_len);
            } 
            else 
            {
                /* add stuffing */
                memmove(buf + 4 + stuffing_len, buf + 4, header_len - 4);
                buf[3] |= 0x20;
                buf[4] = stuffing_len - 1;
                if (stuffing_len >= 2) {
                    buf[5] = 0x00;
                    memset(buf + 6, 0xff, stuffing_len - 2);
                }
            }
        }
        memcpy(buf + TS_PACKET_SIZE - len, payload, len);
        payload += len;
        payload_size -= len;
        //mpegts_prefix_m2ts_header(s);
        //avio_write(s->pb, buf, TS_PACKET_SIZE);
        fwrite(buf, TS_PACKET_SIZE, 1, fp);
    }

    fflush(fp);
    //avio_flush(s->pb);
}

#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
const uint8_t *ff_find_start_code(const uint8_t* p, const uint8_t *end, uint32_t * state)
{
    int i;

    assert(p<=end);
    if(p>=end)
        return end;

    for(i=0; i<3; i++){
        uint32_t tmp= *state << 8;
        *state= tmp + *(p++);
        if(tmp == 0x100 || p==end)
            return p;
    }

    while(p<end){
        if     (p[-1] > 1      ) p+= 3;
        else if(p[-2]          ) p+= 2;
        else if(p[-3]|(p[-1]-1)) p++;
        else{
            p++;
            break;
        }
    }

    p= FFMIN(p, end)-4;
    *state= BytesToUI32((const char*)p);

    return p+4;
}

int mpegts_write_packet(MpegTSWrite* ts, std::vector<MpegTSWriteStream *>& tswritestream,
    MpegTSWriteStream *ts_st, FILE* fp,
    uint8_t *buf, int size, 
    int64_t dataPTS, int64_t dataDTS, 
    int isKey, int isVideo)
{
    uint8_t *data= NULL;
    int max_delay = 200;
    const uint64_t delay = (max_delay * 90000/  AV_TIME_BASE) * 2;

    int64_t dts = AV_NOPTS_VALUE, pts = AV_NOPTS_VALUE;

    if (dataPTS != AV_NOPTS_VALUE)
        pts = dataPTS + delay;
    if (dataDTS != AV_NOPTS_VALUE)
        dts = dataDTS + delay;

    if (ts_st->first_pts_check && pts == AV_NOPTS_VALUE)
    {
        //av_log(s, AV_LOG_ERROR, "first pts value must set\n");
        return -1;
    }
    ts_st->first_pts_check = 0;

    if (isVideo)
    {
        const uint8_t *p = buf, *buf_end = p+size;
        uint32_t state = -1;

        do {
            p = ff_find_start_code(p, buf_end, &state);
            //av_log(s, AV_LOG_INFO, "nal %d\n", state & 0x1f);
        } while (p < buf_end && (state & 0x1f) != 9 &&
                 (state & 0x1f) != 5 && (state & 0x1f) != 1);

        if ((state & 0x1f) != 9)    // AUD NAL
        {
            data = (uint8_t *)malloc(sizeof(uint8_t)*(size+6));
            if (!data)
                return -1;
            memcpy(data+6, buf, size);
            UI32ToBytes((char*)data, 0x00000001);
            data[4] = 0x09;
            data[5] = 0xf0; // any slice type (0xe) + rbsp stop one bit
            buf  = data;
            size = size+6;
        }
    } 
    else
    {
        if (size < 2)
            return -1;
        if ((BytesToUI16((char*)buf) & 0xfff0) != 0xfff0) 
        {
            assert(0);
        }
    }

    if (isVideo) 
    {
        // for video and subtitle, write a single pes packet
        mpegts_write_pes(ts, tswritestream, ts_st, fp, buf, size, pts, dts, isKey, isVideo);
        //av_free(data);
        free(data);
        return 0;
    }

    if (ts_st->payload_index + size > DEFAULT_PES_PAYLOAD_SIZE) 
    {
        mpegts_write_pes(ts, tswritestream, ts_st, fp, ts_st->payload, ts_st->payload_index,
                         ts_st->payload_pts, ts_st->payload_dts,
                         isKey, isVideo);
        ts_st->payload_index = 0;
    }

    if (!ts_st->payload_index) 
    {
        ts_st->payload_pts = pts;
        ts_st->payload_dts = dts;
        ts_st->payload_flags = isKey;
    }

    memcpy(ts_st->payload + ts_st->payload_index, buf, size);
    ts_st->payload_index += size;

    //av_free(data);
    free(data);

    return 0;
}

int mpegts_write_end(MpegTSWrite *ts, FILE* fp, std::vector<MpegTSWriteStream *>& tswritestream,
    int* audioCC, int* videoCC, int* patCC, int* pmtCC)
{
    MpegTSService *service;
    int i;

    /* flush current packets */

    if (tswritestream[0]->payload_index > 0)
    {
        MpegTSWriteStream* ts_st = tswritestream[0];
        mpegts_write_pes(ts, tswritestream, ts_st, fp, 
            ts_st->payload, ts_st->payload_index,
            ts_st->payload_pts, ts_st->payload_dts,
            1, 0);
    }

    if (tswritestream[1]->payload_index > 0)
    {
        MpegTSWriteStream* ts_st = tswritestream[1];
        mpegts_write_pes(ts, tswritestream, ts_st, fp, 
            ts_st->payload, ts_st->payload_index,
            ts_st->payload_pts, ts_st->payload_dts,
            1, 1);
    }

    // 保存上次cc数值
    if (audioCC)
    {
        *audioCC = tswritestream[0]->cc;
    }
    if (videoCC)
    {
        *videoCC = tswritestream[1]->cc;
    }
    if (patCC)
    {
        *patCC = ts->pat.cc;
    }
    if (pmtCC)
    {
        *pmtCC = tswritestream[0]->service->pmt.cc;
    }

    for(i = 0; i < ts->nb_services; i++)
    {
        service = ts->services[i];
        free(service);
    }
    ts->services.clear();

    free(tswritestream[0]);
    free(tswritestream[1]);
    tswritestream.clear();

    return 0;
}

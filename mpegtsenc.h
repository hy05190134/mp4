#ifdef __cplusplus 
extern "C"
{
#endif

#ifndef _MPEG_TS_ENC_H_
#define _MPEG_TS_ENC_H_

#include <stdio.h>
#include <stdint.h>

#include <vector>
#include <string>

#define PCR_TIME_BASE 27000000

#define AV_NOPTS_VALUE          INT64_C(0x8000000000000000)
#define AV_TIME_BASE            1000000

typedef struct MpegTSSection {
    int pid;
    int cc;
    void (*write_packet)(FILE*, struct MpegTSSection *s, const uint8_t *packet);
    void *opaque;    
} MpegTSSection;

void InitMpegTSSection(MpegTSSection &section)
{
    section.pid = 0;
    section.cc = 0;
    section.write_packet = 0;
    section.opaque = 0;
}

typedef struct MpegTSService {
    MpegTSSection pmt; /* MPEG2 pmt table context */
    int sid;           /* service ID */
    std::string name;
    std::string provider_name;
    int pcr_pid;
    int pcr_packet_count;
    int pcr_packet_period;
} MpegTSService;

void InitMpegTSService(MpegTSService &service)
{
    service.sid = 0;
    service.pcr_pid = 0;
    service.pcr_packet_count = 0;
    service.pcr_packet_period = 0;
}

typedef struct MpegTSWrite {
    //const AVClass *av_class;
    MpegTSSection pat; /* MPEG2 pat table */
    MpegTSSection sdt; /* MPEG2 sdt table context */
    std::vector<MpegTSService*> services;
    int sdt_packet_count;
    int sdt_packet_period;
    int pat_packet_count;
    int pat_packet_period;
    int nb_services;
    int onid;
    int tsid;
    int64_t first_pcr;
    int mux_rate; ///< set to 1 when VBR

    int transport_stream_id;    // "mpegts_transport_stream_id"
    int original_network_id;    // "mpegts_original_network_id"
    int service_id;

    int pmt_start_pid;
    int start_pid;
    int m2ts_mode; 
} MpegTSWrite;

void InitMpegTSWrite(MpegTSWrite &mpeg_ts_write){
    mpeg_ts_write.sdt_packet_count = 0;
    mpeg_ts_write.sdt_packet_period = 0;
    mpeg_ts_write.pat_packet_count = 0;
    mpeg_ts_write.pat_packet_period = 0;
    mpeg_ts_write.nb_services = 0;
    mpeg_ts_write.onid = 0;
    mpeg_ts_write.tsid = 0;
    mpeg_ts_write.first_pcr = 0;
    mpeg_ts_write.mux_rate = 0;

    mpeg_ts_write.transport_stream_id = 0x0001;
    mpeg_ts_write.original_network_id = 0x0001;
    mpeg_ts_write.service_id = 0x0001;

    mpeg_ts_write.pmt_start_pid = 0x1000;
    mpeg_ts_write.start_pid = 0x0100;
    mpeg_ts_write.m2ts_mode = 0;
}

/*********************************************/
/* mpegts writer */

#define DEFAULT_PROVIDER_NAME   "hdev_provider"
#define DEFAULT_SERVICE_NAME    "hdev_service"

/* a PES packet header is generated every DEFAULT_PES_HEADER_FREQ packets */
#define DEFAULT_PES_HEADER_FREQ 16
#define DEFAULT_PES_PAYLOAD_SIZE ((DEFAULT_PES_HEADER_FREQ - 1) * 184 + 170)

typedef struct MpegTSWriteStream {
    struct MpegTSService *service;
    int pid; /* stream associated pid */
    int cc;
    int payload_index;
    int first_pts_check; ///< first pts check needed
    int64_t payload_pts;
    int64_t payload_dts;
    int payload_flags;
    uint8_t payload[DEFAULT_PES_PAYLOAD_SIZE];
    //ADTSContext *adts;
} MpegTSWriteStream;

void InitMpegTSWriteStream(MpegTSWriteStream &stream)
{
    stream.service = NULL;
    stream.pid = 0;
    stream.cc = 0;
    stream.payload_index = 0;
    stream.first_pts_check = 0;
    stream.payload_pts = 0;
    stream.payload_dts = 0;
    stream.payload_flags = 0;
}

int mpegts_write_header(MpegTSWrite *ts, std::vector<MpegTSWriteStream *>& tswritestream,
        int initAudioCC, int initVideoCC, int initPatCC, int initPmtCC);

int mpegts_write_packet(MpegTSWrite* ts, std::vector<MpegTSWriteStream *>& tswritestream,
        MpegTSWriteStream *ts_st, FILE* fp,
        uint8_t *buf, int size, 
        int64_t dataPTS, int64_t dataDTS, 
        int isKey, int isVideo);

int mpegts_write_end(MpegTSWrite *ts, FILE* fp, std::vector<MpegTSWriteStream *>& tswritestream,
    int* audioCC, int* videoCC, int* patCC, int* pmtCC);

#endif // _MPEG_TS_ENC_H_

#ifdef __cplusplus
}
#endif

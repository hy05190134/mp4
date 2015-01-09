#ifndef _HDEV_M4U8_TS_MAKER_H_
#define _HDEV_M4U8_TS_MAKER_H_

struct M4u8TSSegSample
{
    double timestamp;
    double composition_timestamp;
    vp_uint64_t offset;
    vp_uint32_t size;
    vp_uint8_t is_key_;
    bool is_video_;
};

struct MpegtsMuxer;
struct Mp4u8Index;
struct M4u8TSMaker
{
public:
    M4u8TSMaker(boost::asio::io_service& ios, 
        const std::string& mp4Filename,
        const std::string& indexName,
        const std::string& ts_name);

    ~M4u8TSMaker();

    void Start(int segIndex);

public:
    void OnStart(int segIndex);

    void OnHttpDownload(fw::IOBuffer buf, const boost::system::error_code& ec,
        const Mp4u8Segment& seg);

private:
    void CalcSamples(const Mp4u8Segment& seg);

private:
    boost::asio::io_service& ios_;
    fw::GetHttpFile::Ptr http_get_;

    Mp4u8Index* mp4u8_index_;
    std::string mp4_filename_;
    std::string index_name_;
    int seg_index_;
    std::string ts_name;

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
    bool is_write_aud_;

    std::multimap<vp_uint64_t, M4u8TSSegSample> samples_;
};



#endif

#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include <string>


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"
}


class decodethread
{
public:
    decodethread();
    void setRtsp(const std::string &rtsp) noexcept { mRtsp = rtsp; }
    void SetSize(uint32_t w, uint32_t h) noexcept { width_ = w , height_ = h; }
    void run();

private:
    std::string mRtsp;
    uint32_t width_;
    uint32_t height_;
    AVFrame* pFrame = nullptr;
    AVFrame* pFrameRGB = nullptr;
    AVFrame* pFrameScaled = nullptr;;
    struct SwsContext* img_ctx = nullptr;
    struct SwsContext* img_scale_ctx = nullptr;
    unsigned char* out_buffer = nullptr;
    unsigned char* scale_buffer = nullptr;

    int DecodeVideoAndPushData(AVCodecContext * pCodecCtx, AVPacket & dec_pkt);
};

#endif // DECODETHREAD_H

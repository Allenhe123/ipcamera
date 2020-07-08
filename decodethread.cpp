#include "decodethread.h"
#include <sys/time.h>
#include <iostream>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types_c.h>


uint64_t Now() {
    auto now  = std::chrono::high_resolution_clock::now();
    auto nano_time_pt = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
    auto epoch = nano_time_pt.time_since_epoch();
    uint64_t now_nano = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch).count();
    return now_nano;
}

decodethread::decodethread()
{
}

void decodethread::run()
{
    unsigned int    i;
    int             ret;
    int             video_st_index = -1;
    int             audio_st_index = -1;
    AVCodecContext  *pCodecCtx= NULL;
    AVStream        *st = NULL;
    AVCodec         *pCodec;
    int numBytes;

    char            errbuf[64];
    long mv0 = 0;

    av_register_all();   
    // avcodec_register_all();                                             // Register all codecs and formats so that they can be used.
    avformat_network_init();                                                    // Initialization of network components

    // AVDictionary* options = NULL;
    // std::string transport("tcp");
    // if(true||transport == "tcp")
    // {
    //     av_dict_set(&options, "rtsp_transport", "tcp", 0);
    // }

    AVFormatContext* ifmt_ctx = avformat_alloc_context();

    // Open the input file for reading.
    if ((ret = avformat_open_input(&ifmt_ctx, mRtsp.c_str(), 0, 0)) < 0) {
        printf("Could not open input file '%s' (error '%s')\n", mRtsp.c_str(), 
                av_make_error_string(errbuf, sizeof(errbuf), ret));
        return;   // release resource!?
    }

    // Get information on the input file (number of streams etc.).
    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {                
        printf("Could not open find stream info (error '%s')\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
        return;
    }

    // dump information
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                
        av_dump_format(ifmt_ctx, i, mRtsp.c_str(), 0);
    }

    // find video stream index
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                
        st = ifmt_ctx->streams[i];
        switch(st->codec->codec_type) {
            case AVMEDIA_TYPE_AUDIO: audio_st_index = i; break;
            case AVMEDIA_TYPE_VIDEO: video_st_index = i; break;
            default: break;
        }
    }
    if (-1 == video_st_index) {
        printf("No H.264 video stream in the input file\n");
        return;
    }

    // for(int i = 0; i < ifmt_ctx->nb_streams; i++ ){
    //     if(ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
    //         video_st_index = i;
    //     }
    // }

    std::cout << "video_st_index: " << video_st_index << std::endl;

    //=================================  查找解码器 ===================================//
    pCodecCtx = ifmt_ctx->streams[video_st_index]->codec;
    if (pCodecCtx == NULL) std::cout << "get pCodecCtx failed" << std::endl;
    else std::cout << "codecid: " << pCodecCtx->codec_id << std::endl;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        printf("Codec not found.\n");
        return ;
    }

    //================================  打开解码器 ===================================//
    if (avcodec_open2(pCodecCtx, pCodec, NULL)<0)
    {
        printf("Could not open codec.\n");
        return ;
    }

    //==================================== 分配空间 ==================================//
    pFrame    = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    pFrameScaled = av_frame_alloc();

    std::cout << "pCodecCtx->width, pCodecCtx->height: " << pCodecCtx->width << ", " << pCodecCtx->height << std::endl;
    //一帧图像数据大小
    numBytes = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
    out_buffer = (unsigned char *) av_malloc(numBytes * sizeof(unsigned char));
    scale_buffer = (unsigned char *) av_malloc(width_ * height_ * 3 * sizeof(unsigned char));

    //会将pFrameRGB的数据按RGB格式自动"关联"到buffer  即pFrameRGB中的数据改变了 out_buffer中的数据也会相应的改变
    avpicture_fill((AVPicture *)pFrameRGB, out_buffer, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

    //会将pFrameScaled的数据按RGB格式自动"关联"到buffer  即pFrameScaled中的数据改变了scale_buffer中的数据也会相应的改变
    avpicture_fill((AVPicture *)pFrameScaled, scale_buffer, AV_PIX_FMT_BGR24, width_, height_);

    //Output Info---输出一些文件（RTSP）信息
//    printf("---------------- File Information ---------------\n");
//    av_dump_format(pFormatCtx, 0, filepath, 0);
//    printf("-------------------------------------------------\n");

    //================================ 设置数据转换参数 ================================//

    img_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
    img_scale_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        width_, height_, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

    //=========================== 分配AVPacket结构体 ===============================//
    AVBitStreamFilterContext* bsfcH264 = av_bitstream_filter_init("h264_mp4toannexb");
    if(!bsfcH264)
    {
        printf("can not create h264_mp4toannexb filter \n");
    }

    AVPacket pkt;
    av_init_packet(&pkt);

    while(1)
    {
        //===========================  读取视频信息 ===============================//
        if (av_read_frame(ifmt_ctx, &pkt) < 0) //读取的是一帧视频  数据存入一个AVPacket的结构中
        {
            std::cout << "read error." << std::endl;
            continue ;
        }
        //此时数据存储在packet中
        //=========================== 对视频数据进行解码 ===============================//
        if (pkt.stream_index == video_st_index)
        {
            // Frame frm;
            // frm.pData = new char[frm.head.DataSize];
            // memcpy(frm.pData,pkt.data,pkt.size);
            

            struct timeval tv;
            gettimeofday(&tv,NULL);
            long mv1 = tv.tv_sec*1000000+tv.tv_usec;
            double fps = (fps*4+1000000.0f/(mv1 - mv0))/5;
            mv0 = mv1;
            printf("get frame:: fps:%2.2f \n", fps);

            //filter
            if(!(pkt.data[0] == 0x0 && pkt.data[1] == 0x0 && pkt.data[2] == 0x0 && pkt.data[3] == 0x01))
            {
                AVPacket tempPack;
                av_init_packet(&tempPack);
                int nRet = av_bitstream_filter_filter(bsfcH264, pCodecCtx, NULL, &tempPack.data, &tempPack.size, pkt.data, pkt.size, 0);
                if(nRet >= 0)
                {
                    DecodeVideoAndPushData(pCodecCtx, tempPack);

                    if(tempPack.data != NULL)
                    {
                        av_free(tempPack.data);
                        tempPack.data = NULL;
                    }
                }
            }
            else
            {
                DecodeVideoAndPushData(pCodecCtx, pkt);
            }
        }

        av_free_packet(&pkt);
    }

//    if (NULL != ifmt_ctx) {
//        avformat_close_input(&ifmt_ctx);
//        ifmt_ctx = NULL;
//    }
    if(bsfcH264)
    {
        av_bitstream_filter_close(bsfcH264);
        bsfcH264 = nullptr;
    }

    av_free(out_buffer);
    av_free(scale_buffer);
    sws_freeContext(img_ctx);
    sws_freeContext(img_scale_ctx);

    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);
    av_frame_free(&pFrameScaled);
    avformat_close_input(&ifmt_ctx);
}

 int decodethread::DecodeVideoAndPushData(AVCodecContext * pCodecCtx, AVPacket & dec_pkt)
 {
     auto start = Now();
     //视频解码函数  解码之后的数据存储在 pFrame中
     int got_picture = 0;
     int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &dec_pkt);
     if (ret < 0)
     {
        std::cout << "decode error." << std::endl;
         return 0;
     }
     auto delta = Now() - start;
     double delta_mill = delta / 1000000.0f;
     std::cout << "decode a yuv frame, time cost: " << delta_mill << std::endl;

     //=========================== YUV=>RGB ===============================//
     if (got_picture)
     {
         //转换一帧图像
         sws_scale(img_ctx,pFrame->data, pFrame->linesize, 0, pCodecCtx->height,  //源
                   pFrameRGB->data, pFrameRGB->linesize);                                 //目的
         sws_scale(img_scale_ctx,pFrame->data, pFrame->linesize, 0, pCodecCtx->height,  //源
                   pFrameScaled->data, pFrameScaled->linesize);                                 //目的

        cv::Mat img(pCodecCtx->height, pCodecCtx->width, CV_8UC3, out_buffer);
        cv::imshow("bst", img);
        cv::waitKey(10);
     }
 }

#include "videoencoder.h"
#include <iostream>
extern "C"{
#include <libavutil/opt.h>
}

VideoEncoder::VideoEncoder(AVCodecParameters* codecParamters,AVCodecID videoCodecID)
{
    videoCodec=avcodec_find_encoder(videoCodecID); // 查找H264编码器，H264是MPG4的一种
    if(videoCodec==NULL){
        cerr<<"Could not find video codec"<<endl;
        exit(1);
    }
    videoCodecContext=avcodec_alloc_context3(videoCodec); // 分配编码器上下文
    if(videoCodecContext==NULL){
        cerr<<"Could not allocate video codec context"<<endl;
        exit(1);
    }
    videoCodecContext->time_base = { 1, 25 };           // 设置时间基
    videoCodecContext->bit_rate= codecParamters->bit_rate; // 设置比特率
    videoCodecContext->framerate= {25,1};               // 设置帧率
    videoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;    // 设置像素格式
    videoCodecContext->codec_id = videoCodec->id;       // 设置编码器ID
    videoCodecContext->width = codecParamters->width;   // 设置宽度
    videoCodecContext->height = codecParamters->height; // 设置高度
    videoCodecContext->max_b_frames = 3;
    AVDictionary* codecOptions = NULL;                  // 编码器选项
    if (videoCodec->id == AV_CODEC_ID_H264){            // 如果是H264编码器
        av_dict_set(&codecOptions, "preset", "medium", 0);
    }
    if(avcodec_open2(videoCodecContext,videoCodec,&codecOptions)<0){ // 打开编码器
        cerr<<"Could not open video codec"<<endl;
        exit(1);
    }
    // if(avcodec_parameters_from_context(codecParamters,videoCodecContext)<0){
    //     cerr<<"Could not copy codec context to codec parameters"<<endl;
    //     exit(1);
    // }
    av_dict_free(&codecOptions);
}

VideoEncoder::~VideoEncoder()
{
    avcodec_close(videoCodecContext);
    avcodec_free_context(&videoCodecContext);
}

void VideoEncoder::readPackets(MutexQueue<shared_ptr<Frame>>& videoFrameQueue,function<void(shared_ptr<Packet>)> packet_handler)
{
    while(!videoFrameQueue.empty()||!videoFrameQueue.isFinished()){
        auto frame=videoFrameQueue.pop();
        int ret = avcodec_send_frame(videoCodecContext, frame->getFrame());
        if (ret < 0) {
            cerr << "Error sending a frame for encoding\n";
            exit(1);
        }

        while (ret >= 0) {
            auto pkt= make_shared<Packet>(av_packet_alloc());
            ret = avcodec_receive_packet(videoCodecContext, pkt->getPacket());
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            }
            else if (ret < 0) {
                cerr << "Error during encoding\n";
                exit(1);
            }
            packet_handler(pkt);
        }
    }
    int ret = avcodec_send_frame(videoCodecContext, NULL);
    if (ret < 0) {
        cerr << "Error sending a frame for encoding\n";
        exit(1);
    }

    while (ret >= 0) {
        auto pkt= make_shared<Packet>(av_packet_alloc());
        ret = avcodec_receive_packet(videoCodecContext, pkt->getPacket());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            break;
        }
        else if (ret < 0) {
            cerr << "Error during encoding\n";
            exit(1);
        }
        packet_handler(pkt);
    }
}
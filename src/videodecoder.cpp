#include"videodecoder.h"
#include<iostream>
#include<fstream>
#include <packet.h>
using namespace std;

VideoDecoder::VideoDecoder(AVCodecParameters* codecParamters)
{
    videoCodecContext=avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(videoCodecContext,codecParamters);
    videoCodec=avcodec_find_decoder(videoCodecContext->codec_id);
    if(videoCodec==NULL){
        cerr<<"Could not find video codec"<<endl;
        exit(1);
    }
    if(avcodec_open2(videoCodecContext,videoCodec,NULL)<0){
        cerr<<"Could not open video codec"<<endl;
        exit(1);
    }
}

VideoDecoder::~VideoDecoder()
{
    avcodec_close(videoCodecContext);
    avcodec_free_context(&videoCodecContext);
}

bool VideoDecoder::saveYuv(MutexQueue<shared_ptr<Packet>>& videoPacketQueue)
{
    fstream file;
    file.open("output.yuv",ios::out|ios::binary);
    if(!file.is_open()){
        cerr<<"Could not open output file"<<endl;
        exit(1);
    }
    AVFrame* frame=av_frame_alloc();
    if (!frame) {
        cerr << "Could not allocate audio frame\n";
        return false;
    }
    while(!videoPacketQueue.empty()||!videoPacketQueue.isFinished()){
        auto pkt=videoPacketQueue.pop();
        int ret = avcodec_send_packet(videoCodecContext, pkt->getPacket()); // 发送解码数据包
        if (ret < 0)
        {
            cerr << "Error sending a packet for decoding\n"; // 发送解码数据包出错
            exit(1);
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_frame(videoCodecContext, frame); // 接收解码后的帧
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            }
            else if (ret < 0)
            {
                fprintf(stderr, "Error during decoding\n"); // 解码出错
                exit(1);
            }
            // 写入Y平面
            for (int i = 0; i < frame->height; i++)
            {
                file.write((char*)(frame->data[0] + i * frame->linesize[0]), frame->width);
            }

            // 写入U平面
            for (int i = 0; i < frame->height / 2; i++)
            {
                file.write((char*)(frame->data[1] + i * frame->linesize[1]), frame->width / 2);
            }

            // 写入V平面
            for (int i = 0; i < frame->height / 2; i++)
            {
                file.write((char*)(frame->data[2] + i * frame->linesize[2]), frame->width / 2);
            }
        }
    }
    av_frame_free(&frame);
    file.close();
    return true;
}


void VideoDecoder::readFrames(MutexQueue<shared_ptr<Packet>>& videoPacketQueue,function<void(shared_ptr<Frame>)> frame_handler)
{
    while(!videoPacketQueue.empty()||!videoPacketQueue.isFinished()){
        auto pkt=videoPacketQueue.pop();
        int ret = avcodec_send_packet(videoCodecContext, pkt->getPacket()); // 发送解码数据包
        if (ret < 0)
        {
            cerr << "Error sending a packet for decoding\n"; // 发送解码数据包出错
            exit(1);
        }

        while (ret >= 0)
        {
            auto frame=make_shared<Frame>(av_frame_alloc());
            ret = avcodec_receive_frame(videoCodecContext, frame->getFrame()); // 接收解码后的帧
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            }
            else if (ret < 0)
            {
                cerr << "Error during decoding\n"; // 解码出错
                exit(1);
            }
            frame_handler(frame);
        }
    }
}
#include"audiodecoder.h"
#include<iostream>
#include<fstream>
#include<libswresample/swresample.h>
#include<libavutil/opt.h>
using namespace std;

AudioDecoder::AudioDecoder(AVCodecParameters* codecParamters)
{
    audioCodecContext=avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(audioCodecContext,codecParamters);
    audioCodec=avcodec_find_decoder(audioCodecContext->codec_id);
    if(audioCodec==NULL){
        cerr<<"Could not find audio codec"<<endl;
        exit(1);
    }
    if(avcodec_open2(audioCodecContext,audioCodec,NULL)<0){
        cerr<<"Could not open audio codec"<<endl;
        exit(1);
    }
}

AudioDecoder::~AudioDecoder()
{
    avcodec_close(audioCodecContext);
    avcodec_free_context(&audioCodecContext);
}

bool AudioDecoder::savePcm(MutexQueue<shared_ptr<Packet>>& audioPacketQueue)
{
    fstream file;
    file.open("output.pcm",ios::out|ios::binary);
    if(!file.is_open()){
        cerr<<"Could not open output file"<<endl;
        exit(1);
    }
    AVFrame* frame=av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate audio frame\n";
        return false;
    }

    while(!audioPacketQueue.empty()||!audioPacketQueue.isFinished()){
        auto pkt=audioPacketQueue.pop();
        int ret = avcodec_send_packet(audioCodecContext, pkt->getPacket()); // 发送解码数据包
        if (ret < 0)
        {
            cerr << "Error sending a packet for decoding\n"; // 发送解码数据包出错
            exit(1);
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_frame(audioCodecContext, frame); // 接收解码后的帧
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;
            }
            else if (ret < 0)
            {
                cerr << "Error during decoding\n"; // 解码出错
                exit(1);
            }
            int data_size = av_get_bytes_per_sample(audioCodecContext->sample_fmt);// 获取每个样本的字节数
            if (data_size < 0)
            {
                cerr << "Failed to calculate data size\n";
                exit(1);
            }
            for(int i=0;i<frame->nb_samples;i++){// 对于每个样本
                for(int j=0;j<audioCodecContext->channels;j++){ // 依次处理每个声道
                    file.write((char*)frame->data[j]+data_size*i,data_size);
                }
            }
        }
    }
    av_frame_free(&frame);
    file.close();
    return true;
}

void AudioDecoder::readFrames(MutexQueue<shared_ptr<Packet>>& audioPacketQueue,function<void(shared_ptr<Frame>)> frame_handler)
{
    while(!audioPacketQueue.empty()||!audioPacketQueue.isFinished()){
        auto pkt=audioPacketQueue.pop();
        int ret = avcodec_send_packet(audioCodecContext, pkt->getPacket()); // 发送解码数据包
        if (ret < 0)
        {
            cerr << "Error sending a packet for decoding\n"; // 发送解码数据包出错
            exit(1);
        }

        while (ret >= 0)
        {
            auto frame=make_shared<Frame>(av_frame_alloc());
            ret = avcodec_receive_frame(audioCodecContext, frame->getFrame()); // 接收解码后的帧
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
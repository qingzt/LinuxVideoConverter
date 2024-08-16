#include "muxer.h"
#include <iostream>
using namespace std;
Muxer::Muxer(string filename,AVStream* oldvideoStream,AVStream* oldaudioStream,AVCodecID videoCodecID,AVCodecID audioCodecID){
    avformat_alloc_output_context2(&formatContext, NULL, NULL, filename.c_str());
    if(formatContext==NULL){
        cerr<<"Could not create output context"<<endl;
        exit(1);
    }
    videoStream=avformat_new_stream(formatContext,NULL);
    if(videoStream==NULL){
        cerr<<"Could not create video stream"<<endl;
        exit(1);
    }
    videoStream->id=formatContext->nb_streams-1;
    videoStream->time_base = oldvideoStream->time_base;
    avcodec_parameters_copy(videoStream->codecpar,oldvideoStream->codecpar);
    videoStream->codecpar->codec_id = videoCodecID;
    videoStream->codecpar->codec_tag = 0;
    
    audioStream=avformat_new_stream(formatContext,NULL);
    if(audioStream==NULL){
        cerr<<"Could not create audio stream"<<endl;
        exit(1);
    }
    audioStream->id=formatContext->nb_streams-1;
    audioStream->time_base = oldaudioStream->time_base;
    avcodec_parameters_copy(audioStream->codecpar,oldaudioStream->codecpar);
    audioStream->codecpar->codec_id = audioCodecID;
    audioStream->codecpar->codec_tag = 0;
}
Muxer::~Muxer(){
    avformat_free_context(formatContext);
}

void Muxer::run(MutexQueue<shared_ptr<Packet>>& videoPacketQueue,MutexQueue<shared_ptr<Packet>>& audioPacketQueue){
    AVPacket* packet;
    avio_open(&formatContext->pb, formatContext->url, AVIO_FLAG_WRITE);
    int re = avformat_write_header(formatContext, NULL);
    if(re < 0){
        cerr<<"Could not write header"<<endl;
        exit(1);
    }
    while((!videoPacketQueue.empty()||!videoPacketQueue.isFinished())||(!audioPacketQueue.empty()||!audioPacketQueue.isFinished())){
        if(!videoPacketQueue.empty()||!videoPacketQueue.isFinished()){
            auto packetPtr=videoPacketQueue.pop();
            auto packet=packetPtr->getPacket();
            packet->stream_index=videoStream->id;
            av_interleaved_write_frame(formatContext,packet);
        }
        if(!audioPacketQueue.empty()||!audioPacketQueue.isFinished()){
            auto packetPtr=audioPacketQueue.pop();
            auto packet=packetPtr->getPacket();
            packet->stream_index=audioStream->id;
            av_interleaved_write_frame(formatContext,packet);
        }
    }
    av_write_trailer(formatContext);
    avio_close(formatContext->pb);
}
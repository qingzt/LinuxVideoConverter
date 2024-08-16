#include"demuxer.h"
#include<iostream>
#include<memory>
#include <packet.h>
using namespace std;

Demuxer::Demuxer(const std::string& file_path){
    format_context=nullptr;
    videoStreamIndex=-1;
    audioStreamIndex=-1;
    open(file_path);
}

Demuxer::~Demuxer(){
    if(format_context!=nullptr){
        avformat_close_input(&format_context);
        avformat_free_context(format_context);
    }
}

bool Demuxer::open(const std::string& file_path){
    format_context=avformat_alloc_context();//为AVFormatContext分配内存
    if(format_context==nullptr){
        cerr<<"Could not allocate memory for AVFormatContext"<<endl;
        return false;
    }
    if(avformat_open_input(&format_context,file_path.c_str(),nullptr,nullptr)!=0){
        cerr<<"Could not open file: "<<file_path<<endl;
        return false;
    }
    for(int i=0;i<format_context->nb_streams;i++){
        if(format_context->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
            videoStreamIndex=i;//找到视频流
        }
        if(format_context->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
            audioStreamIndex=i;//找到音频流
        }
    }

    return true;
}

bool Demuxer::readPackets(function<void(shared_ptr<Packet>)> vedio_packet_handler, function<void(shared_ptr<Packet>)> audio_packet_handler){
    while(true){
        auto packet=make_shared<Packet>(av_packet_alloc());
        if(av_read_frame(format_context,packet->getPacket())<0){
            break;
        }
        if(packet->getPacket()->stream_index==videoStreamIndex){
            vedio_packet_handler(packet);
        }
        if(packet->getPacket()->stream_index==audioStreamIndex){
            audio_packet_handler(packet);
        }
    }
    return true;
}

AVStream* Demuxer::getVideoStream(){
    return format_context->streams[videoStreamIndex];
}

AVStream* Demuxer::getAudioStream(){
    return format_context->streams[audioStreamIndex];
}
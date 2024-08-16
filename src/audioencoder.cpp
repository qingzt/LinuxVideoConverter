#include "audioencoder.h"
#include <iostream>
#include "resampler.h"


AudioEncoder::AudioEncoder(AVCodecParameters* codecParamters,AVCodecID audioCodecID)
{
    audioCodec=avcodec_find_encoder(audioCodecID);
    if(audioCodec==NULL){
        cerr<<"Could not find audio codec"<<endl;
        exit(1);
    }
    audioCodecContext=avcodec_alloc_context3(audioCodec);
    if(audioCodecContext==NULL){
        cerr<<"Could not allocate audio codec context"<<endl;
        exit(1);
    }
    audioCodecContext->bit_rate = codecParamters->bit_rate;
    audioCodecContext->sample_fmt = audioCodec->sample_fmts ? audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    if (audioCodecContext->sample_fmt == AV_SAMPLE_FMT_NONE) {
        cerr << "Specified sample format is invalid or not supported" << endl;
        exit(1);
    }    
    audioCodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
    audioCodecContext->channels       = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    audioCodecContext->sample_rate    = codecParamters->sample_rate;
    if (audioCodecContext->channel_layout == 0) {
        std::cerr << "Specified channel layout is invalid or not supported" << std::endl;
        exit(1);
    }

    if(avcodec_open2(audioCodecContext,audioCodec,NULL)<0){
        cerr<<"Could not open audio codec"<<endl;
        exit(1);
    }
    
    if(avcodec_parameters_from_context(codecParamters,audioCodecContext)<0){
        cerr<<"Could not copy codec context to codec parameters"<<endl;
        exit(1);
    }

    fifo=av_audio_fifo_alloc(audioCodecContext->sample_fmt,audioCodecContext->channels,audioCodecContext->frame_size*2);
}

AudioEncoder::~AudioEncoder()
{
    avcodec_close(audioCodecContext);
    avcodec_free_context(&audioCodecContext);
    av_audio_fifo_free(fifo);
}

void AudioEncoder::readPackets(RingBuffer<shared_ptr<Frame>>& audioFrameQueue,function<void(shared_ptr<Packet>)> packet_handler)
{
    static int64_t pts = 0;
    // ReSampler resampler(44100,AV_SAMPLE_FMT_FLTP,AV_CH_LAYOUT_STEREO,audioCodecContext->sample_rate,audioCodecContext->sample_fmt,audioCodecContext->channel_layout);
    while(!audioFrameQueue.empty()||!audioFrameQueue.isFinished()){
        auto receviedframe=audioFrameQueue.pop();
        if(receviedframe==nullptr||receviedframe->getFrame()==nullptr){
            continue;
        }
        av_audio_fifo_write(fifo,(void**)receviedframe->getFrame()->data,//收到的音频先写入fifo
                            receviedframe->getFrame()->nb_samples);
        if(av_audio_fifo_size(fifo)<audioCodecContext->frame_size){ //如果fifo中的音频不足一个frame_size，则继续等待
            continue;
        }
        while(av_audio_fifo_size(fifo)>=audioCodecContext->frame_size){ //如果fifo中的音频大于一个frame_size，则开始编码
            auto frame=make_shared<Frame>(av_frame_alloc());
            frame->getFrame()->nb_samples=audioCodecContext->frame_size;    //设置frame一些属性
            frame->getFrame()->format=audioCodecContext->sample_fmt;
            frame->getFrame()->channel_layout=audioCodecContext->channel_layout;
            frame->getFrame()->sample_rate=audioCodecContext->sample_rate;
            int ret=av_frame_get_buffer(frame->getFrame(),0);
            av_audio_fifo_read(fifo,(void**)frame->getFrame()->data, //从fifo中读取音频
                                audioCodecContext->frame_size);
            frame->getFrame()->pts = pts; //设置pts
            pts += frame->getFrame()->nb_samples; //更新pts
            ret = avcodec_send_frame(audioCodecContext, frame->getFrame()); //发送frame给编码器
            if (ret < 0) {
                cerr << "Error sending a frame for encoding\n";
                exit(1);
            }

            while (ret >= 0) {
                auto pkt= make_shared<Packet>(av_packet_alloc());
                ret = avcodec_receive_packet(audioCodecContext, pkt->getPacket());
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                    break;
                }
                if (ret < 0) {
                    cerr << "Error during encoding\n";
                    exit(1);
                }
                packet_handler(pkt);
            }
        }
    }
}
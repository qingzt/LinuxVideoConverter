#include "resampler.h"
#include <iostream>
using namespace std;

ReSampler::ReSampler(int inSampleRate,int inSampleFormat,int inChannelLayout,int outSampleRate,int outSampleFormat,int outChannelLayout)
{
    swrContext=swr_alloc_set_opts(NULL,
                                  outChannelLayout,
                                  (AVSampleFormat)outSampleFormat,
                                  outSampleRate,
                                  inChannelLayout,
                                  (AVSampleFormat)inSampleFormat,
                                  inSampleRate,
                                  0,
                                  NULL);
    if(swrContext==NULL){
        cerr<<"Could not allocate resampler context"<<endl;
        exit(1);
    }
    if(swr_init(swrContext)<0){
        cerr<<"Could not initialize resampler context"<<endl;
        exit(1);
    }
    this->outSampleRate=outSampleRate;
    this->outSampleFormat=outSampleFormat;
    this->outChannelLayout=outChannelLayout;
    this->inSampleRate=inSampleRate;
    this->inSampleFormat=inSampleFormat;
    this->inChannelLayout=inChannelLayout;
}

ReSampler::~ReSampler()
{
    swr_free(&swrContext);
}

shared_ptr<Frame> ReSampler::resample(shared_ptr<Frame> inFrame,int new_nb_samples)
{

    shared_ptr<Frame> outFrame = make_shared<Frame>(av_frame_alloc());

    // 复制输入帧的属性到输出帧
    av_frame_copy_props(outFrame->getFrame(), inFrame->getFrame());

    // 设置输出帧的参数
    outFrame->getFrame()->channel_layout = inFrame->getFrame()->channel_layout;
    outFrame->getFrame()->format = inFrame->getFrame()->format;
    outFrame->getFrame()->sample_rate = inFrame->getFrame()->sample_rate;
    outFrame->getFrame()->nb_samples = new_nb_samples;

    // 分配输出帧的缓冲区
    int ret=av_frame_get_buffer(outFrame->getFrame(), 0);
    if(ret<0){
        cerr<<"Could not allocate output frame buffer"<<endl;
        exit(1);
    }

    // 计算每个通道的样本数
    int channels = av_get_channel_layout_nb_channels(inFrame->getFrame()->channel_layout);
    int sample_size = av_get_bytes_per_sample((AVSampleFormat)inFrame->getFrame()->format);
    for (int ch = 0; ch < channels; ++ch) {
        outFrame->getFrame()->linesize[ch] = new_nb_samples * sample_size;
    }
    // 复制数据并进行简单的线性插值
    for (int ch = 0; ch < channels; ++ch) {
        uint8_t *in_data = inFrame->getFrame()->data[ch];
        uint8_t *out_data = outFrame->getFrame()->data[ch];

        for (int i = 0; i < new_nb_samples; ++i) {
            int src_index = i * (inFrame->getFrame()->nb_samples - 1) / (new_nb_samples - 1);
            memcpy(out_data + i * sample_size, in_data + src_index * sample_size, sample_size);
        }
    }

    return outFrame;

    // shared_ptr<Frame> outFrame = make_shared<Frame>(av_frame_alloc());

    // // 复制输入帧的属性到输出帧
    // av_frame_copy_props(outFrame->getFrame(), frame->getFrame());

    // // 设置输出帧的参数
    // outFrame->getFrame()->channel_layout = outChannelLayout;
    // outFrame->getFrame()->format = outSampleFormat;
    // outFrame->getFrame()->sample_rate = outSampleRate;
    // outFrame->getFrame()->nb_samples = nb_samples;

    // // 分配输出帧的缓冲区
    // av_frame_get_buffer(outFrame->getFrame(), 0);

    // int max_out_samples = av_rescale_rnd(swr_get_delay(swrContext, frame->getFrame()->sample_rate) + frame->getFrame()->nb_samples, outSampleRate, frame->getFrame()->sample_rate, AV_ROUND_UP);

    // // 执行重采样
    // int res = swr_convert(swrContext, outFrame->getFrame()->data, max_out_samples,
    //                       (const uint8_t **)frame->getFrame()->data, frame->getFrame()->nb_samples);
    // if (res < 0) {
    //     std::cerr << "Could not resample frame" << std::endl;
    //     return nullptr;
    // }

    // // 更新输出帧的nb_samples
    // outFrame->getFrame()->nb_samples = res;

    // return outFrame;
}
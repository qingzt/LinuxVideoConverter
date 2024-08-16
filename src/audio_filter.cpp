#include "audio_filter.h"
#include <iostream>
extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}
using namespace std;

AudioFilter::AudioFilter(int sample_rate, AVSampleFormat sample_fmt, uint64_t channel_layout,const std::string& filter_descr )
{
    char args[512];
    // if(!channel_layout){
        channel_layout=AV_CH_LAYOUT_STEREO;
    // }
    if(sample_fmt==AV_SAMPLE_FMT_NONE){
        sample_fmt=AV_SAMPLE_FMT_FLTP;
    }
    static const enum AVSampleFormat out_sample_fmts[] = { sample_fmt, AV_SAMPLE_FMT_NONE };
    static const int64_t out_channel_layouts[] = { channel_layout, -1 };
    static const int out_sample_rates[] = { sample_rate, -1 };
    AVFilterInOut* inputs = avfilter_inout_alloc(); // 分配输入滤镜链表
    AVFilterInOut* outputs = avfilter_inout_alloc(); // 分配输出滤镜链表
    filterGraph = avfilter_graph_alloc(); // 分配滤镜图
    const AVFilter* buffersrc = avfilter_get_by_name("abuffer"); // 获取输入源滤镜
    const AVFilter* buffersink = avfilter_get_by_name("abuffersink"); // 获取输出源滤镜
    snprintf(args, sizeof(args),
             "time_base=1/48000:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             sample_rate, av_get_sample_fmt_name(sample_fmt), channel_layout);
    // 创建并配置输入源滤镜上下文
    int ret=avfilter_graph_create_filter(&buffersrcCtx,buffersrc,"in",args,NULL,filterGraph);
    if(ret<0){
        cerr<<"Could not create buffer source"<<endl;
        exit(1);
    }
    // 创建并配置输出源滤镜上下文
    ret=avfilter_graph_create_filter(&buffersinkCtx,buffersink,"out",NULL,NULL,filterGraph);
    if(ret<0){
        cerr<<"Could not create buffer sink"<<endl;
        exit(1);
    }
    ret = av_opt_set_int_list(buffersinkCtx, "sample_fmts", out_sample_fmts , -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        cerr<<"Cannot set output sample format"<<endl;
        exit(1);
    }

    ret = av_opt_set_int_list(buffersinkCtx, "channel_layouts", out_channel_layouts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        cerr<<"Cannot set output channel layout"<<endl;
        exit(1);
    }

    ret = av_opt_set_int_list(buffersinkCtx, "sample_rates", out_sample_rates, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        cerr<<"Cannot set output sample rate"<<endl;
        exit(1);
    }

    // 连接滤镜
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrcCtx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersinkCtx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if(avfilter_graph_parse_ptr(filterGraph,filter_descr.c_str(), &inputs, &outputs, NULL)<0){
        cerr<<"Could not parse filter graph"<<endl;
        exit(1);
    }

    if(avfilter_graph_config(filterGraph,NULL)<0){
        cerr<<"Could not configure filter graph"<<endl;
        exit(1);
    }

    const AVFilterLink *outlink=buffersinkCtx->inputs[0];
    av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
    cout<<"Output: srate: "<<outlink->sample_rate<<" fmt: "<<(char *)av_x_if_null(av_get_sample_fmt_name(AVSampleFormat(outlink->format)),"?")<<" ch layout: "<<args<<endl;

    avfilter_inout_free(&inputs); // 释放输入滤镜链表
    avfilter_inout_free(&outputs); // 释放输出滤镜链表
}

AudioFilter::~AudioFilter()
{
    avfilter_free(buffersrcCtx); // 释放输入源滤镜上下文
    avfilter_free(buffersinkCtx); // 释放输出源滤镜上下文
    avfilter_graph_free(&filterGraph); // 释放滤镜图
}

shared_ptr<Frame>AudioFilter::applyFilter(shared_ptr<Frame> frame)
{
    int ret=av_buffersrc_add_frame(buffersrcCtx,frame->getFrame()); // 向输入源滤镜上下文添加帧
    if(ret<0){
        cerr<<"Error while feeding the filtergraph"<<endl;
        exit(1);
    }
    shared_ptr<Frame> filtered_frame=make_shared<Frame>(av_frame_alloc());
    ret=av_buffersink_get_frame(buffersinkCtx,filtered_frame->getFrame()); // 从输出源滤镜上下文获取帧
    if(ret<0){
        filtered_frame=make_shared<Frame>(nullptr);
    }
    return filtered_frame;
}
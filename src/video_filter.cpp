#include "video_filter.h"
#include <iostream>
using namespace std;
VideoFilter::VideoFilter(int width,int height,AVPixelFormat pix_fmt,const std::string& filter_descr)
{
    char args[512];
    AVFilterInOut* inputs = avfilter_inout_alloc(); // 分配输入滤镜链表
    AVFilterInOut* outputs = avfilter_inout_alloc(); // 分配输出滤镜链表
    filterGraph = avfilter_graph_alloc(); // 分配滤镜图
    const AVFilter* buffersrc = avfilter_get_by_name("buffer"); // 获取输入源滤镜
    const AVFilter* buffersink = avfilter_get_by_name("buffersink"); // 获取输出源滤镜
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/25:pixel_aspect=1/1",
             width, height, pix_fmt);
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

    // 连接滤镜
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrcCtx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersinkCtx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if(avfilter_graph_parse_ptr(filterGraph,filter_descr.c_str(),&inputs,&outputs,NULL)<0){ // 解析滤镜图
        cerr<<"Could not parse filter graph"<<endl;
        exit(1);
    }

    if(avfilter_graph_config(filterGraph,NULL)<0){
        cerr<<"Could not configure filter graph"<<endl;
        exit(1);
    }

    avfilter_inout_free(&inputs); // 释放输入滤镜链表
    avfilter_inout_free(&outputs); // 释放输出滤镜链表
}

VideoFilter::~VideoFilter()
{
    avfilter_free(buffersrcCtx); // 释放输入源滤镜上下文
    avfilter_free(buffersinkCtx); // 释放输出源滤镜上下文
    avfilter_graph_free(&filterGraph); // 释放滤镜图
}

std::shared_ptr<Frame> VideoFilter::applyFilter(std::shared_ptr<Frame> frame)
{
    int ret = av_buffersrc_add_frame(buffersrcCtx, frame->getFrame()); // 向输入源滤镜上下文添加帧
    if (ret < 0) {
        std::cerr << "Error while feeding the filtergraph\n";
        exit(1);
    }

    auto filtered_frame = make_shared<Frame>(av_frame_alloc()); // 创建过滤后的帧
    ret = av_buffersink_get_frame(buffersinkCtx, filtered_frame->getFrame()); // 从输出源滤镜上下文获取帧
    if (ret < 0) {
        std::cerr << "Error while getting the filtered frame\n";
        filtered_frame = nullptr;
    }
    return filtered_frame;
}
#pragma once
#include <memory>
#include <string>
#include <iostream>
extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}
#include "frame.h"

class VideoFilter {
private:
    AVFilterGraph* filterGraph;     // 滤镜图
    AVFilterContext* buffersrcCtx;  // 输入源滤镜上下文
    AVFilterContext* buffersinkCtx; // 输出源滤镜上下文

public:
    VideoFilter(int width, int height, AVPixelFormat pix_fmt, const std::string& filter_descr); // 构造滤镜，传入视频帧的宽高和像素格式，以及滤镜描述字符串
    ~VideoFilter();
    std::shared_ptr<Frame> applyFilter(std::shared_ptr<Frame> frame); // 对视频帧应用滤镜得到新的帧
};
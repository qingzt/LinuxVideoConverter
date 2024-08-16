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

class AudioFilter {
private:
    AVFilterGraph* filterGraph;     // 滤镜图
    AVFilterContext* buffersrcCtx;  // 输入源滤镜上下文
    AVFilterContext* buffersinkCtx; // 输出源滤镜上下文
public:
    AudioFilter(int sample_rate, AVSampleFormat sample_fmt, uint64_t channel_layout,const std::string& filter_descr);
    ~AudioFilter();
    std::shared_ptr<Frame> applyFilter(std::shared_ptr<Frame> frame);
};
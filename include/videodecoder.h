#pragma once
#include "av_queue.h"
#include "functional"
extern "C"{
#include <libavcodec/avcodec.h>
}
#include <memory>
#include "packet.h"
#include "frame.h"
using namespace std;
class VideoDecoder
{
    private:
    AVCodecContext *videoCodecContext;              // 视频解码器上下文
    AVCodec *videoCodec;                            // 视频解码器
    public:
    VideoDecoder(AVCodecParameters* codecParamters);// 构造函数
    ~VideoDecoder();
    bool saveYuv(MutexQueue<shared_ptr<Packet>>& videoPacketQueue);   // 解码视频包
    void readFrames(MutexQueue<shared_ptr<Packet>>& videoPacketQueue,function<void(shared_ptr<Frame>)> frame_handler);  // 获取视频帧
};
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
class VideoEncoder
{
    private:
    AVCodecContext *videoCodecContext;              // 视频编码器上下文
    AVCodec *videoCodec;                            // 视频编码器
    public:
    VideoEncoder(AVCodecParameters* codecParamters,AVCodecID videoCodecID);// 构造函数
    ~VideoEncoder();
    void readPackets(MutexQueue<shared_ptr<Frame>>& videoFrameQueue,function<void(shared_ptr<Packet>)> frame_handler);  // 获取视频帧
};
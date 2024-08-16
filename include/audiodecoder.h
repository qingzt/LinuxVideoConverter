#pragma once
#include "av_queue.h"
#include <memory>
#include <functional>
extern "C"{
#include <libavcodec/avcodec.h>
#include "packet.h"
#include "frame.h"
}
using namespace std;
class AudioDecoder
{
    private:
    AVCodecContext *audioCodecContext;              // 音频解码器上下文
    AVCodec *audioCodec;                            // 音频解码器
    public:
    AudioDecoder(AVCodecParameters* codecParamters);// 构造函数
    ~AudioDecoder();
    bool savePcm(MutexQueue<shared_ptr<Packet>>& audioPacketQueue);   // 解码音频包
    void readFrames(MutexQueue<shared_ptr<Packet>>& audioPacketQueue,function<void(shared_ptr<Frame>)> frame_handler);  // 获取音频帧
};
#pragma once
#include "av_queue.h"
#include "ring_buffer.h"
#include "functional"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
}
#include <memory>
#include "packet.h"
#include "frame.h"
using namespace std;
class AudioEncoder
{
    private:
    AVCodecContext *audioCodecContext;              // 音频编码器上下文
    AVCodec *audioCodec;                            // 音频编码器
    AVAudioFifo *fifo;                                // 音频Fifo
    public:
    AudioEncoder(AVCodecParameters* codecParamters,AVCodecID audioCodecID);// 构造函数
    ~AudioEncoder();
    void readPackets(RingBuffer<shared_ptr<Frame>>& videoFrameQueue,function<void(shared_ptr<Packet>)> frame_handler);  // 获取音频包
};
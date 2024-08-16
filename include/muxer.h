#pragma once
extern "C"{
#include <libavformat/avformat.h>
}
#include <memory>
#include <functional>
#include "packet.h"
#include "av_queue.h"

class Muxer
{
    private:
    AVFormatContext *formatContext;
    AVStream *videoStream;
    AVStream *audioStream;
    public:
    Muxer(string filename,AVStream* videoStream,AVStream* audioStream,AVCodecID videoCodecID,AVCodecID audioCodecID); // 构造函数，利用已有的视频流和音频流的参数构造一个Muxer对象
    ~Muxer();
    void run(MutexQueue<shared_ptr<Packet>>& videoPacketQueue,MutexQueue<shared_ptr<Packet>>& audioPacketQueue); // 将视频包和音频包写入到输出文件
};
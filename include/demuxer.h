#pragma once
#include <string>
extern "C" {
#include <libavformat/avformat.h>
}
#include <functional>
#include <memory>
#include "packet.h"
using namespace std;
class Demuxer {
    private:
    AVFormatContext* format_context; // 格式上下文，包含当前打开的文件的所有信息
    int videoStreamIndex; // 视频流索引
    int audioStreamIndex; // 音频流索引
    bool open(const std::string& file_path); // 打开文件

    public:
    Demuxer(const std::string& file_path); // 构造函数
    
    ~Demuxer();
    bool readPackets(function<void(shared_ptr<Packet>)> vedio_packet_handler, function<void(shared_ptr<Packet>)> audio_packet_handler);// 读取包，并根据包的类型调用不同的回调函数

    AVStream* getVideoStream(); // 获取视频流的编解码参数
    AVStream* getAudioStream(); // 获取音频流的编解码参数
};
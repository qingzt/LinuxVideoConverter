#pragma once
extern "C"{
#include <libavcodec/avcodec.h>
}
class Packet
{
private:
    AVPacket *packet;
public:
    Packet():packet(nullptr){}
    Packet(AVPacket *packet);
    ~Packet();
    AVPacket *getPacket();
    void setPacket(AVPacket *packet);
};
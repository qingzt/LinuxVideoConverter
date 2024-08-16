#include "packet.h"

Packet::Packet(AVPacket *packet)
{
    this->packet = packet;
}
Packet::~Packet()
{
    av_packet_free(&packet);
}
AVPacket *Packet::getPacket()
{
    return packet;
}
void Packet::setPacket(AVPacket *packet)
{
    av_packet_free(&this->packet);
    this->packet = packet;
}
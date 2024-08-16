#include "frame.h"

Frame::Frame(AVFrame *frame)
{
    this->frame = frame;
}
Frame::~Frame()
{
    av_frame_free(&frame);
}
AVFrame *Frame::getFrame()
{
    return frame;
}
void Frame::setFrame(AVFrame *frame)
{
    av_frame_free(&this->frame);
    this->frame = frame;
}
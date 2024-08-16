#pragma once
extern "C"{
#include <libavutil/frame.h>
}
class Frame
{
private:
    AVFrame *frame;
public:
    Frame():frame(nullptr){}
    Frame(AVFrame *frame);
    ~Frame();
    AVFrame *getFrame();
    void setFrame(AVFrame *frame);
};
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}
#include <memory>
#include "frame.h"
using std::shared_ptr;
class ReSampler
{
    private:
    SwrContext *swrContext;
    int outSampleRate;
    int outSampleFormat;
    int outChannelLayout;
    int inSampleRate;
    int inSampleFormat;
    int inChannelLayout;
    public:
    ReSampler(int inSampleRate,int inSampleFormat,int inChannelLayout,int outSampleRate,int outSampleFormat,int outChannelLayout);
    ~ReSampler();
    shared_ptr<Frame> resample(shared_ptr<Frame> frame,int nbSamples);
};
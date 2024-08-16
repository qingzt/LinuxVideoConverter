#include <iostream>
#include "demuxer.h"
#include "av_queue.h"
#include "videodecoder.h"
#include "audiodecoder.h"
#include <future>
#include "packet.h"
#include "videoencoder.h"
#include "muxer.h"
#include "video_filter.h"
#include "audio_filter.h"
#include "audioencoder.h"
#include "ring_buffer.h"
using namespace std;
using std::async;
int main()
{
    double speed=1;
    cout<<"Please input the speed of the video between 0.5 and 4"<<endl;
    cin>>speed;
    MutexQueue<shared_ptr<Packet>> videoPacketQueue; // 解复用器输出的视频包队列
    MutexQueue<shared_ptr<Packet>> audioPacketQueue; // 解复用器输出的音频包队列
    MutexQueue<shared_ptr<Frame>> videoFrameQueue;  // 视频解码器输出的视频帧队列
    RingBuffer<shared_ptr<Frame>> audioFrameBuffer(100); // 音频解码器输出的音频帧的环形缓冲区
    MutexQueue<shared_ptr<Packet>> videoEnPacketQueue; // 视频编码器输出的视频包队列
    MutexQueue<shared_ptr<Packet>> audioEnPacketQueue; // 音频编码器输出的音频包队列

    Demuxer demuxer("input.mp4"); // 创建解复用器
    
    VideoDecoder videoDecoder(demuxer.getVideoStream()->codecpar); // 创建视频解码器
    
    AudioDecoder audioDecoder(demuxer.getAudioStream()->codecpar); // 创建音频解码器

    VideoFilter videoTransposeFilter(demuxer.getVideoStream()->codecpar->width,
                                        demuxer.getVideoStream()->codecpar->height,
                                        AV_PIX_FMT_YUV420P,"transpose=1");// 创建视频旋转滤镜
    VideoFilter videoSpeedFilter(demuxer.getVideoStream()->codecpar->height,//此滤镜作用在旋转后的视频上
                                    demuxer.getVideoStream()->codecpar->width,//因此宽高互换
                                    AV_PIX_FMT_YUV420P,
                                    "setpts="+to_string(1/speed)+"*PTS");// 创建视频变速滤镜

    AudioFilter audioFilter(demuxer.getAudioStream()->codecpar->sample_rate,
                            AVSampleFormat(demuxer.getAudioStream()->codecpar->format),
                            demuxer.getAudioStream()->codecpar->channel_layout,
                            "atempo="+to_string(speed)); // 创建音频变速滤镜
    
    // 解码需要旋转后视频的参数
    auto newCodecParamters=avcodec_parameters_alloc();
    avcodec_parameters_copy(newCodecParamters,demuxer.getVideoStream()->codecpar);
    int width=demuxer.getVideoStream()->codecpar->width;
    int height=demuxer.getVideoStream()->codecpar->height;
    newCodecParamters->width=height;    // 旋转后的宽高
    newCodecParamters->height=width;
    VideoEncoder videoEncoder(newCodecParamters,AV_CODEC_ID_MPEG4);   // 创建视频编码器
    avcodec_parameters_free(&newCodecParamters);

    AudioEncoder audioEncoder(demuxer.getAudioStream()->codecpar,AV_CODEC_ID_AC3); // 创建音频编码器

    Muxer muxer("output.mp4",demuxer.getVideoStream(),demuxer.getAudioStream(),AV_CODEC_ID_MPEG4,AV_CODEC_ID_AC3);// 创建封装器
    
    auto demuxerFuture = async(launch::async, [&]() { // 异步执行解复用
        cout<<"Start demuxer"<<endl;
        demuxer.readPackets([&videoPacketQueue](shared_ptr<Packet> packet) {
            videoPacketQueue.push(packet);// 将视频包放入视频包队列
        }, [&audioPacketQueue](shared_ptr<Packet> packet) {
            audioPacketQueue.push(packet);// 将音频包放入音频包队列
        });
        videoPacketQueue.setFinish(true); // 设置视频包队列结束
        audioPacketQueue.setFinish(true);// 设置音频包队列结束
        cout<<"Finish demuxer"<<endl;
    });

    auto videoDecoderFuture = async(launch::async, [&]() { // 异步执行视频解码
        cout<<"Start video decoder"<<endl;
        videoDecoder.readFrames(videoPacketQueue, [&](shared_ptr<Frame> frame) {
            auto newFrame=videoTransposeFilter.applyFilter(frame); // 旋转视频
            newFrame=videoSpeedFilter.applyFilter(newFrame); // 变速视频
            videoFrameQueue.push(newFrame); // 将视频帧放入视频帧队列
        });
        videoFrameQueue.setFinish(true); // 设置视频帧队列结束
        cout<<"Finish video decoder"<<endl;
    });

    auto audioDecoderFuture = async(launch::async, [&]() { // 异步执行音频解码
        cout<<"Start audio decoder"<<endl;
        audioDecoder.readFrames(audioPacketQueue, [&](shared_ptr<Frame> frame) {
            auto newFrame=audioFilter.applyFilter(frame); // 变速音频
            audioFrameBuffer.push(newFrame); // 将音频帧放入音频帧环形缓冲区
        });
        audioFrameBuffer.setFinish(true); // 设置音频帧环形缓冲区结束
        cout<<"Finish audio decoder"<<endl;
    });

    auto videoEncoderFuture = async(launch::async, [&]() { // 异步执行视频编码
        cout<<"Start video encoder"<<endl;
        videoEncoder.readPackets(videoFrameQueue, [&](shared_ptr<Packet> packet) {
            videoEnPacketQueue.push(packet); // 将编码后的视频包放入视频包队列
        });
        videoEnPacketQueue.setFinish(true); // 设置视频包队列结束
        cout<<"Finish video encoder"<<endl; // 输出视频编码结束
    });
    
    auto audioEncoderFuture = async(launch::async, [&]() { // 异步执行音频编码
        cout<<"Start audio encoder"<<endl;
        audioEncoder.readPackets(audioFrameBuffer, [&](shared_ptr<Packet> packet) {
            audioEnPacketQueue.push(packet); // 将编码后的音频包放入音频包队列
        });
        audioEnPacketQueue.setFinish(true); // 设置音频包队列结束
        cout<<"Finish audio encoder"<<endl;
    });

    auto muxerFuture = async(launch::async, [&]() { // 异步执行封装
        cout<<"Start muxer"<<endl;
        muxer.run(videoEnPacketQueue,
                    audioEnPacketQueue); // 将编码后的视频包和音频包解复用
        cout<<"Finish muxer"<<endl;
    });

    // 等待所有任务结束
    demuxerFuture.get();
    videoDecoderFuture.get();
    audioDecoderFuture.get();
    videoEncoderFuture.get();
    audioEncoderFuture.get();
    muxerFuture.get();
    
    return 0;
}
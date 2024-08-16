# LinuxVideoConverter
## 项目结构
```
📦LinuxVideoConverter
 ┣ 📂include            # 头文件
 ┃ ┣ 📜audio_filter.h   # 音频滤镜，实现音频变速
 ┃ ┣ 📜audiodecoder.h   # 音频解码器
 ┃ ┣ 📜audioencoder.h   # 音频编码器，使用AC3编码
 ┃ ┣ 📜av_queue.h       # 线程安全队列
 ┃ ┣ 📜demuxer.h        # 解复用器
 ┃ ┣ 📜frame.h          # 封装的AVFrame，便于使用智能指针
 ┃ ┣ 📜muxer.h          # 复用器
 ┃ ┣ 📜packet.h         # 封装的AVPacket，便于使用智能指针
 ┃ ┣ 📜resampler.h      # 重采样，实际没用到
 ┃ ┣ 📜ring_buffer.h    # 线程安全的环形缓冲区
 ┃ ┣ 📜video_filter.h   # 视频滤镜，用于视频变速和视频旋转
 ┃ ┣ 📜videodecoder.h   # 视频解码器
 ┃ ┗ 📜videoencoder.h   # 视频编码器，使用libx264
 ┣ 📂libs               # 库文件
 ┃ ┣ 📂include          # 库文件的头文件
 ┃ ┣ 📜libavcodec-wht.a
 ┃ ┣ 📜libavdevice-wht.a
 ┃ ┣ 📜libavfilter-wht.a
 ┃ ┣ 📜libavformat-wht.a
 ┃ ┣ 📜libavutil-wht.a
 ┃ ┣ 📜libpostproc-wht.a
 ┃ ┣ 📜libswresample-wht.a
 ┃ ┣ 📜libswscale-wht.a
 ┃ ┗ 📜libx264-wht.a
 ┣ 📂src                # 源代码
 ┃ ┣ 📜audio_filter.cpp # 音频过滤器的源代码
 ┃ ┣ 📜audiodecoder.cpp # 音频解码器的源代码
 ┃ ┣ 📜audioencoder.cpp # 音频编码器的源代码
 ┃ ┣ 📜av_queue.cpp     # 线程安全队列的源代码
 ┃ ┣ 📜demuxer.cpp      # 解复用器的源代码
 ┃ ┣ 📜frame.cpp        # 封装的AVFrame的源代码
 ┃ ┣ 📜muxer.cpp        # 复用器的源代码
 ┃ ┣ 📜packet.cpp       # 封装的AVPacket的源代码
 ┃ ┣ 📜resampler.cpp    # 重采样器的源代码，实际没有用到
 ┃ ┣ 📜ring_buffer.cpp  # 环形缓冲区的源代码
 ┃ ┣ 📜video_filter.cpp # 视频滤镜的源代码
 ┃ ┣ 📜videodecoder.cpp # 视频解码器的源代码
 ┃ ┗ 📜videoencoder.cpp # 视频编码器的源代码
 ┣ 📜.gitignore
 ┣ 📜1.mp4              # 测试用的视频文件
 ┣ 📜CMakeLists.txt     # CMake
 ┣ 📜README.md
 ┣ 📜README01.md
 ┣ 📜README02.md
 ┣ 📜build_ffmpeg.sh    # 编译ffmpeg的脚本
 ┣ 📜play.sh            # 播放解码后的原始文件的脚本
 ┣ 📜transcode.cpp      # 主程序
 ┣ 📜视频音频复用及视频旋转演示视频.mp4
 ┗ 📜解码运行视频.mp4
```
## 解决的问题
1. 需要使用AC3进行编码，由于nb_samples的大小不同，导致无法编码，查看FFMpeg的示例程序transcode_aac.c，发现使用av_audio_fifo可以实现缓冲nb_samples
```cpp
av_audio_fifo_write(fifo,(void**)receviedframe->getFrame()->data,//收到的音频先写入fifo
                            receviedframe->getFrame()->nb_samples);
if(av_audio_fifo_size(fifo)<audioCodecContext->frame_size){ //如果fifo中的音频不足一个frame_size，则继续等待
    continue;
}
while(av_audio_fifo_size(fifo)>=audioCodecContext->frame_size){ //如果fifo中的音频大于一个frame_size，则开始编码
    auto frame=make_shared<Frame>(av_frame_alloc());
    frame->getFrame()->nb_samples=audioCodecContext->frame_size;    //设置frame一些属性
    frame->getFrame()->format=audioCodecContext->sample_fmt;
    frame->getFrame()->channel_layout=audioCodecContext->channel_layout;
    frame->getFrame()->sample_rate=audioCodecContext->sample_rate;
    int ret=av_frame_get_buffer(frame->getFrame(),0);
    av_audio_fifo_read(fifo,(void**)frame->getFrame()->data, //从fifo中读取音频
                        audioCodecContext->frame_size);
    frame->getFrame()->pts = pts; //设置pts
    pts += frame->getFrame()->nb_samples; //更新pts
    ret = avcodec_send_frame(audioCodecContext, frame->getFrame()); //发送frame给编码
    ··· //编码后的包处理
}
```
## 封装AVFrame和AVPacket
```cpp
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
```
1. 内部有AVFrame和AVPacket的指针
2. 在析构函数中使用av_frame_free和av_packet_free释放资源
## 解复用器部分
```cpp
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

    AVStream* getVideoStream(); // 获取视频流
    AVStream* getAudioStream(); // 获取音频流
};
```
1. 在解复用器部分封装了FFMpeg的AVFormat的功能
2. 对外提供了读取包的接口，根据包的类型调用不同的回调函数
3. 同时提供了获取视频流和音频流的接口供解码器使用
## 包队列部分
```cpp
template<typename T>
class MyNode{ //队列节点
public:
    T val;
    MyNode<T>* next;
    MyNode<T>(){
        next=nullptr;
    }
};

template<typename T>
class MyQueue{ //队列
private:
    MyNode<T>* head;
    MyNode<T>* tail;
    size_t lenth;
public:
    MyQueue();
    T front();
    void pop_front();
    void push_back(const T& val);
    bool empty();
    size_t size();
    void clear();
};

template<typename T>
class MutexQueue { //线程安全队列
private:
    MyQueue<T> items;
    mutex mtx;                      // 互斥锁，保证线程安全
    condition_variable cv;          // 条件变量，用于线程同步
    bool finish;                    // 标志位，表示队列是否已经结束
public:
    MutexQueue();
    ~MutexQueue();
    void push(const T& vedio);// 入队
    T pop();                 // 出队
    bool empty();                   // 判断队列是否为空
    size_t size();                  // 获取队列大小
    void setFinish(bool val);       // 设置结束标志
    bool isFinished();              // 判断队列是否结束
};
```
1. 实现了一个通用的包队列
2. 使用互斥锁保证线程安全
3. 使用条件变量实现当队列为空时阻塞线程
4. 标志位finish表示队列是否还有新的包入队
## 环形缓冲区部分
```cpp
template<typename T>
class RingBuffer 
{
private:
    T* buffer;
    int head;
    int tail;
    int count;
    int maxSize;
    mutex mtx;
    condition_variable cv;
    bool isFinish;
    
public:
    RingBuffer(int size);
    ~RingBuffer();
    void push(const T& val);
    T pop();
    bool empty();
    bool full();
    int size();
    void setFinish(bool finish);
    bool isFinished();
};
```
1. 底层使用数组实现
2. 有线程安全机制
## 视频解码器部分
```cpp
class VideoDecoder
{
    private:
    AVCodecContext *videoCodecContext;              // 视频解码器上下文
    AVCodec *videoCodec;                            // 视频解码器
    public:
    VideoDecoder(AVCodecParameters* codecParamters);// 构造函数
    ~VideoDecoder();
    bool saveYuv(MutexQueue<shared_ptr<Packet>>& videoPacketQueue);   // 保存YUV
    void readFrames(MutexQueue<shared_ptr<Packet>>& videoPacketQueue,function<void(shared_ptr<Frame>)> frame_handler) // 读取帧并调用回调函数
};
```
1. 视频解码器部分封装了FFMpeg的视频解码功能
2. 构造时需要传入视频流的编解码参数
3. 可以根据功能选择保存YUV或者读取帧并调用回调函数
4. 音频解码器部分类似
## 视频编码器部分
```cpp
class VideoEncoder
{
    private:
    AVCodecContext *videoCodecContext;              // 视频编码器上下文
    AVCodec *videoCodec;                            // 视频编码器
    public:
    VideoEncoder(AVCodecParameters* codecParamters,AVCodecID videoCodecID);// 构造函数
    ~VideoEncoder();
    void readPackets(MutexQueue<shared_ptr<Frame>>& videoFrameQueue,function<void(shared_ptr<Packet>)> frame_handler);  // 获取视频帧
};
```
1. 视频编码器部分封装了FFMpeg的视频编码功能
2. 从帧获取包并调用回调函数
3. 支持自定义编码器
## 音频编码器部分
```cpp
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
```
1. 音频编码器部分封装了FFMpeg的音频编码功能
2. 从帧获取包并调用回调函数
3. 使用av_audio_fifo缓冲nb_samples
4. 支持自定义编码器
## 滤镜部分
```cpp
class Filter {
private:
    AVFilterGraph* filterGraph;     // 滤镜图
    AVFilterContext* buffersrcCtx;  // 输入源滤镜上下文
    AVFilterContext* buffersinkCtx; // 输出源滤镜上下文

public:
    Filter(int width, int height, AVPixelFormat pix_fmt, const std::string& filter_descr); // 构造滤镜，传入视频帧的宽高和像素格式，以及滤镜描述字符串
    ~Filter();
    std::shared_ptr<Frame> applyFilter(std::shared_ptr<Frame> frame); // 对视频帧应用滤镜得到新的帧
    void addFilter(const std::string& filter_descr); // 添加滤镜
};
```
1. 滤镜部分封装了FFMpeg的滤镜功能
2. 构造时需要传入视频帧的宽高和像素格式，以及滤镜描述字符串
3. 可以对视频帧应用滤镜得到新的帧，比如旋转视频
4. 将新的帧传递给视频编码器可以得到旋转后的视频
## 复用器部分
```cpp
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
```
1. 复用器部分封装了FFMpeg的封装功能
2. 构造时需要传入视频流和音频流，利用已有的视频流和音频流的参数
3. 可以将视频包和音频包写入到输出文件
## 主程序部分
```cpp
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

Muxer muxer("output.mp4",demuxer.getVideoStream(),demuxer.getAudioStream(),AV_CODEC_ID_MPEG4,AV_CODEC_ID_AC3);//创建封装器

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
```
1. 在主程序中创建解复用器、包队列、编码器、封装器的对象以及连接两个线程之间的队列
2. 对视频依次执行解复用、视频解码、视频滤镜、视频编码
3. 对音频依次执行解复用、音频解码、音频滤镜、音频编码
4. 使用多线程异步执行
cmake_minimum_required(VERSION 3.14)
project(LinuxVideoConverter)


add_executable(LinuxVideoConverter
    src/av_queue.cpp
    src/ring_buffer.cpp
    src/demuxer.cpp
    src/videodecoder.cpp
    src/audiodecoder.cpp
    src/videoencoder.cpp
    src/audioencoder.cpp
    src/packet.cpp
    src/frame.cpp
    src/muxer.cpp
    src/video_filter.cpp
    src/audio_filter.cpp
    transcode.cpp
)

target_include_directories(LinuxVideoConverter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_directories(LinuxVideoConverter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

target_link_libraries(LinuxVideoConverter PUBLIC
    avfilter-wht
    avformat-wht
    avcodec-wht
    avutil-wht
    swresample-wht
    swscale-wht
    avdevice-wht
    x264-wht
    postproc-wht
    dl
    z
    m
)
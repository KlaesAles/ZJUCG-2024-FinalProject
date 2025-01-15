#ifndef CAPTURE_MANAGER_H
#define CAPTURE_MANAGER_H

#include <string>
#include <vector>
#include <filesystem>

class CaptureManager {
public:
    CaptureManager(unsigned int width, unsigned int height);
    ~CaptureManager();

    // 截图到文件
    bool captureScreen(const std::string& filePath);

    // 录制视频
    bool startRecording(const std::string& videoPath, int fps = 30);
    void stopRecording();
    void recordFrame();
    void ensureCaptureFolderExists();

private:
    unsigned int SCR_WIDTH;
    unsigned int SCR_HEIGHT;
    int frameRate;                      // 录制帧率
    bool recording;                     // 录制状态
    FILE* ffmpegPipe;                   // ffmpeg 进程管道
    std::vector<unsigned char> pixelBuffer;  // 像素数据缓冲
    std::string videoPath;              // 视频保存路径

    // 初始化 ffmpeg 管道
    bool initFfmpegPipe(const std::string& videoPath, int fps);
    void closeFfmpegPipe();
};

#endif // CAPTURE_MANAGER_H

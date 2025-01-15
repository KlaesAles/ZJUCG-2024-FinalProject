#ifndef CAPTURE_MANAGER_H
#define CAPTURE_MANAGER_H

#include <string>
#include <vector>
#include <filesystem>

class CaptureManager {
public:
    CaptureManager(unsigned int width, unsigned int height);
    ~CaptureManager();

    // ��ͼ���ļ�
    bool captureScreen(const std::string& filePath);

    // ¼����Ƶ
    bool startRecording(const std::string& videoPath, int fps = 30);
    void stopRecording();
    void recordFrame();
    void ensureCaptureFolderExists();

private:
    unsigned int SCR_WIDTH;
    unsigned int SCR_HEIGHT;
    int frameRate;                      // ¼��֡��
    bool recording;                     // ¼��״̬
    FILE* ffmpegPipe;                   // ffmpeg ���̹ܵ�
    std::vector<unsigned char> pixelBuffer;  // �������ݻ���
    std::string videoPath;              // ��Ƶ����·��

    // ��ʼ�� ffmpeg �ܵ�
    bool initFfmpegPipe(const std::string& videoPath, int fps);
    void closeFfmpegPipe();
};

#endif // CAPTURE_MANAGER_H

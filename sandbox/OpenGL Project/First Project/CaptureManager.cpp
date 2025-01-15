#include "CaptureManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <stdexcept>

// 如果使用 stb_image_write
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

CaptureManager::CaptureManager(unsigned int width, unsigned int height)
    : SCR_WIDTH(width), SCR_HEIGHT(height), recording(false), ffmpegPipe(nullptr)
{
    pixelBuffer.resize(SCR_WIDTH * SCR_HEIGHT * 4); // RGBA 每像素4字节
    ensureCaptureFolderExists();
}

CaptureManager::~CaptureManager() {
    if (recording) {
        stopRecording();
    }
}

bool CaptureManager::captureScreen(const std::string& basePath) {
    // 静态计数器，记录截图次数
    static int screenshotCounter = 1;

    // 生成带有序号的文件名
    char fileName[256];
    snprintf(fileName, sizeof(fileName), "/screenshot_%04d.png", screenshotCounter);
    std::string filePath = basePath + fileName;

    // 从默认帧缓冲区读取像素
    glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());

    // 翻转像素数据
    std::vector<unsigned char> flippedBuffer(pixelBuffer.size());
    for (unsigned int y = 0; y < SCR_HEIGHT; ++y) {
        memcpy(&flippedBuffer[y * SCR_WIDTH * 4],
            &pixelBuffer[(SCR_HEIGHT - y - 1) * SCR_WIDTH * 4],
            SCR_WIDTH * 4);
    }

    // 保存成 PNG 文件
    if (stbi_write_png(filePath.c_str(), SCR_WIDTH, SCR_HEIGHT, 4, flippedBuffer.data(), SCR_WIDTH * 4)) {
        std::cout << "[CaptureManager] Screenshot saved to: " << filePath << std::endl;
        screenshotCounter++; // 自增计数器
        return true;
    }
    else {
        std::cerr << "[CaptureManager] Failed to save screenshot: " << filePath << std::endl;
        return false;
    }
}



bool CaptureManager::startRecording(const std::string& basePath, int fps) {
    if (recording) {
        std::cerr << "[CaptureManager] Already recording!" << std::endl;
        return false;
    }

    // 静态计数器，记录录屏次数
    static int recordingCounter = 1;

    // 生成带有序号的文件名
    char fileName[256];
    snprintf(fileName, sizeof(fileName), "/capture_%04d.mp4", recordingCounter);
    std::string videoPath = basePath + fileName;

    if (!initFfmpegPipe(videoPath, fps)) {
        std::cerr << "[CaptureManager] Failed to start recording." << std::endl;
        return false;
    }

    frameRate = fps;
    recording = true;
    recordingCounter++; // 自增计数器
    std::cout << "[CaptureManager] Recording started: " << videoPath << std::endl;
    return true;
}

void CaptureManager::stopRecording() {
    if (!recording) return;

    closeFfmpegPipe();
    recording = false;
    std::cout << "[CaptureManager] Recording stopped." << std::endl;
}

void CaptureManager::recordFrame() {
    if (!recording || !ffmpegPipe) return;

    // 读取当前帧缓冲区像素数据
    glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());

    // 翻转像素数据
    std::vector<unsigned char> flippedBuffer(pixelBuffer.size());
    for (unsigned int y = 0; y < SCR_HEIGHT; ++y) {
        memcpy(&flippedBuffer[y * SCR_WIDTH * 4],
            &pixelBuffer[(SCR_HEIGHT - y - 1) * SCR_WIDTH * 4],
            SCR_WIDTH * 4);
    }

    // 写入翻转后的像素数据到 ffmpeg 管道
    fwrite(flippedBuffer.data(), 1, flippedBuffer.size(), ffmpegPipe);
}

bool CaptureManager::initFfmpegPipe(const std::string& videoPath, int fps) {
    // ffmpeg 命令示例：将 RGBA 数据流保存为 mp4 格式
    std::string command = "ffmpeg -y -f rawvideo -vcodec rawvideo "
        "-pixel_format rgba -video_size " + std::to_string(SCR_WIDTH) + "x" + std::to_string(SCR_HEIGHT) +
        " -r " + std::to_string(fps) + " -i - -c:v libx264 -preset ultrafast -pix_fmt yuv420p " + videoPath;

    // 打开管道
    ffmpegPipe = _popen(command.c_str(), "w");
    if (!ffmpegPipe) {
        std::cerr << "[CaptureManager] Failed to open ffmpeg pipe." << std::endl;
        return false;
    }
    return true;
}

void CaptureManager::closeFfmpegPipe() {
    if (ffmpegPipe) {
        _pclose(ffmpegPipe);
        ffmpegPipe = nullptr;
    }
}

void CaptureManager::ensureCaptureFolderExists() {
    std::filesystem::path captureFolder("./capture");
    if (!std::filesystem::exists(captureFolder)) {
        std::filesystem::create_directories(captureFolder);
    }
}

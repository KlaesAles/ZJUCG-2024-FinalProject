#include "CaptureManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <stdexcept>

// ���ʹ�� stb_image_write
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

CaptureManager::CaptureManager(unsigned int width, unsigned int height)
    : SCR_WIDTH(width), SCR_HEIGHT(height), recording(false), ffmpegPipe(nullptr)
{
    pixelBuffer.resize(SCR_WIDTH * SCR_HEIGHT * 4); // RGBA ÿ����4�ֽ�
    ensureCaptureFolderExists();
}

CaptureManager::~CaptureManager() {
    if (recording) {
        stopRecording();
    }
}

bool CaptureManager::captureScreen(const std::string& basePath) {
    // ��̬����������¼��ͼ����
    static int screenshotCounter = 1;

    // ���ɴ�����ŵ��ļ���
    char fileName[256];
    snprintf(fileName, sizeof(fileName), "/screenshot_%04d.png", screenshotCounter);
    std::string filePath = basePath + fileName;

    // ��Ĭ��֡��������ȡ����
    glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());

    // ��ת��������
    std::vector<unsigned char> flippedBuffer(pixelBuffer.size());
    for (unsigned int y = 0; y < SCR_HEIGHT; ++y) {
        memcpy(&flippedBuffer[y * SCR_WIDTH * 4],
            &pixelBuffer[(SCR_HEIGHT - y - 1) * SCR_WIDTH * 4],
            SCR_WIDTH * 4);
    }

    // ����� PNG �ļ�
    if (stbi_write_png(filePath.c_str(), SCR_WIDTH, SCR_HEIGHT, 4, flippedBuffer.data(), SCR_WIDTH * 4)) {
        std::cout << "[CaptureManager] Screenshot saved to: " << filePath << std::endl;
        screenshotCounter++; // ����������
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

    // ��̬����������¼¼������
    static int recordingCounter = 1;

    // ���ɴ�����ŵ��ļ���
    char fileName[256];
    snprintf(fileName, sizeof(fileName), "/capture_%04d.mp4", recordingCounter);
    std::string videoPath = basePath + fileName;

    if (!initFfmpegPipe(videoPath, fps)) {
        std::cerr << "[CaptureManager] Failed to start recording." << std::endl;
        return false;
    }

    frameRate = fps;
    recording = true;
    recordingCounter++; // ����������
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

    // ��ȡ����
    glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());

    // ��ת����
    for (unsigned int y = 0; y < SCR_HEIGHT / 2; ++y) {
        std::swap_ranges(
            pixelBuffer.begin() + y * SCR_WIDTH * 4,
            pixelBuffer.begin() + (y + 1) * SCR_WIDTH * 4,
            pixelBuffer.begin() + (SCR_HEIGHT - y - 1) * SCR_WIDTH * 4
        );
    }

    // д��ܵ�
    fwrite(pixelBuffer.data(), 1, pixelBuffer.size(), ffmpegPipe);
}


bool CaptureManager::initFfmpegPipe(const std::string& videoPath, int fps) {
    std::string command = "ffmpeg -y -f rawvideo -vcodec rawvideo "
        "-pix_fmt rgba -s " + std::to_string(SCR_WIDTH) + "x" + std::to_string(SCR_HEIGHT) +
        " -r " + std::to_string(fps) + " -i - -c:v libx264 -preset ultrafast -crf 23 -pix_fmt yuv420p " + videoPath;

    ffmpegPipe = _popen(command.c_str(), "wb"); // ʹ�ö�����ģʽ
    char errorBuffer[256]; // ���建����
    if (!ffmpegPipe) {
        std::cerr << "[CaptureManager] Failed to open ffmpeg pipe: " << strerror_s(errorBuffer, sizeof(errorBuffer), errno) << std::endl;
        return false;
    }
    return true;
}

void CaptureManager::closeFfmpegPipe() {
    if (ffmpegPipe) {
        fflush(ffmpegPipe);  // ȷ���������е�����д���ļ�
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

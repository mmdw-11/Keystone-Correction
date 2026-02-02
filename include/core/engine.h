#pragma once
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace core {

class AutoKeystonePro {
public:
    // 构造与析构
    AutoKeystonePro(const std::string& configPath = "config.npy", cv::Size canvasSize = {1280, 720});
    ~AutoKeystonePro() = default;

    // 校正主流程
    bool calibrate(const cv::Mat& capturedImage);

    // 应用校正矩阵
    cv::Mat apply(const cv::Mat& frame, bool reverse = true) const;

    // 保存/加载校正矩阵
    void saveCorrectionMatrix() const;
    bool loadCorrectionMatrix();

    // 重置校正状态
    void resetCalibration();

    // 获取状态
    struct Status {
        bool isCalibrated;
        std::string configPath;
        cv::Size canvasSize;
        bool matrixLoaded;
        bool detectedTrapezoid;
    };
    Status getStatus() const;

private:
    // 梯形角点检测
    std::vector<cv::Point2f> detectTrapezoid(const cv::Mat& image) const;
    // 角点排序
    std::vector<cv::Point2f> sortPoints(const std::vector<cv::Point2f>& pts) const;

    // 成员变量
    std::string configPath_;
    cv::Size canvasSize_;
    cv::Mat forwardMatrix_;   // 梯形→矩形
    cv::Mat inverseMatrix_;   // 矩形→梯形
    bool isCalibrated_;
    std::vector<cv::Point2f> detectedTrapezoid_;

    // 目标矩形常量
    static std::vector<cv::Point2f> getTargetRect();
};

} // namespace core

#include "core/engine.h"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>

namespace core {

// 目标矩形常量
std::vector<cv::Point2f> AutoKeystonePro::getTargetRect() {
    return {
        {0.f, 0.f},
        {1280.f, 0.f},
        {1280.f, 720.f},
        {0.f, 720.f}
    };
}

AutoKeystonePro::AutoKeystonePro(const std::string& configPath, cv::Size canvasSize)
    : configPath_(configPath), canvasSize_(canvasSize), isCalibrated_(false) {
    forwardMatrix_ = cv::Mat();
    inverseMatrix_ = cv::Mat();
    detectedTrapezoid_.clear();
    loadCorrectionMatrix();
}

bool AutoKeystonePro::calibrate(const cv::Mat& capturedImage) {
    try {
        std::cout << "[*] 检测梯形四个角点..." << std::endl;
        auto trapPts = detectTrapezoid(capturedImage);
        if (trapPts.size() != 4) {
            std::cout << "[!] 未能检测到梯形边界" << std::endl;
            return false;
        }
        detectedTrapezoid_ = trapPts;
        std::cout << "[✓] 检测到梯形坐标：" << std::endl;
        for (size_t i = 0; i < trapPts.size(); ++i) {
            std::cout << "    点" << i << ": (" << trapPts[i].x << ", " << trapPts[i].y << ")" << std::endl;
        }
        // 计算正向变换：梯形 → 矩形
        std::cout << "[*] 计算正向变换矩阵（梯形→矩形）..." << std::endl;
        forwardMatrix_ = cv::getPerspectiveTransform(trapPts, getTargetRect());
        std::cout << "[✓] 正向变换矩阵已计算" << std::endl;
        // 计算反向变换：矩形 → 梯形
        std::cout << "[*] 计算反向变换矩阵（矩形→梯形）..." << std::endl;
        inverseMatrix_ = cv::getPerspectiveTransform(getTargetRect(), trapPts);
        std::cout << "[✓] 反向变换矩阵已计算" << std::endl;
        saveCorrectionMatrix();
        isCalibrated_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cout << "[✗] 校正失败: " << e.what() << std::endl;
        return false;
    }
}

std::vector<cv::Point2f> AutoKeystonePro::detectTrapezoid(const cv::Mat& image) const {
    try {
        cv::Mat gray, binary;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, binary, 200, 255, cv::THRESH_BINARY);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        if (contours.empty()) {
            std::cout << "[!] 未检测到任何轮廓" << std::endl;
            return {{300, 150}, {980, 150}, {900, 570}, {380, 570}};
        }
        auto largest = std::max_element(contours.begin(), contours.end(),
            [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                return cv::contourArea(a) < cv::contourArea(b);
            });
        double epsilon = 0.02 * cv::arcLength(*largest, true);
        std::vector<cv::Point> approx;
        cv::approxPolyDP(*largest, approx, epsilon, true);
        if (approx.size() == 4) {
            std::vector<cv::Point2f> pts;
            for (const auto& pt : approx) pts.emplace_back(pt);
            return sortPoints(pts);
        } else {
            std::cout << "[!] 检测到 " << approx.size() << " 个点，使用固定梯形坐标" << std::endl;
            return {{300, 150}, {980, 150}, {900, 570}, {380, 570}};
        }
    } catch (const std::exception& e) {
        std::cout << "[!] 检测梯形失败: " << e.what() << std::endl;
        return {{300, 150}, {980, 150}, {900, 570}, {380, 570}};
    }
}

std::vector<cv::Point2f> AutoKeystonePro::sortPoints(const std::vector<cv::Point2f>& pts) const {
    if (pts.size() != 4) return pts;
    cv::Point2f center(0, 0);
    for (const auto& pt : pts) center += pt;
    center *= 0.25f;
    std::vector<std::pair<double, cv::Point2f>> angle_pts;
    for (const auto& pt : pts) {
        double angle = std::atan2(pt.y - center.y, pt.x - center.x);
        angle_pts.emplace_back(angle, pt);
    }
    std::sort(angle_pts.begin(), angle_pts.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    std::vector<cv::Point2f> sorted_pts;
    for (const auto& ap : angle_pts) sorted_pts.push_back(ap.second);
    // 保证第一个点为左上角
    if (sorted_pts[0].x > center.x) std::rotate(sorted_pts.begin(), sorted_pts.begin() + 2, sorted_pts.end());
    return sorted_pts;
}

cv::Mat AutoKeystonePro::apply(const cv::Mat& frame, bool reverse) const {
    if (!isCalibrated_) return frame;
    const cv::Mat& matrix = reverse ? inverseMatrix_ : forwardMatrix_;
    if (matrix.empty()) return frame;
    cv::Mat output;
    cv::warpPerspective(frame, output, matrix, canvasSize_);
    return output;
}

void AutoKeystonePro::saveCorrectionMatrix() const {
    if (inverseMatrix_.empty()) {
        std::cout << "[!] 反向矩阵未计算" << std::endl;
        return;
    }
    try {
        // cv::FileStorage fs(configPath_, cv::FileStorage::WRITE | cv::FileStorage::FORMAT_BINARY);
        cv::FileStorage fs(configPath_, cv::FileStorage::WRITE);
        fs << "inverse_matrix" << inverseMatrix_;
        fs.release();
        std::cout << "[✓] 校正矩阵已保存: " << configPath_ << std::endl;
    } catch (const std::exception& e) {
        std::cout << "[✗] 保存校正矩阵失败: " << e.what() << std::endl;
    }
}

bool AutoKeystonePro::loadCorrectionMatrix() {
    std::ifstream f(configPath_);
    if (!f.good()) {
        std::cout << "[*] 配置文件不存在（首次运行）: " << configPath_ << std::endl;
        return false;
    }
    try {
        // cv::FileStorage fs(configPath_, cv::FileStorage::READ | cv::FileStorage::FORMAT_BINARY);
cv::FileStorage fs(configPath_, cv::FileStorage::READ);        fs["inverse_matrix"] >> inverseMatrix_;
        fs.release();
        isCalibrated_ = !inverseMatrix_.empty();
        std::cout << "[✓] 校正矩阵已加载: " << configPath_ << std::endl;
        return isCalibrated_;
    } catch (const std::exception& e) {
        std::cout << "[✗] 加载校正矩阵失败: " << e.what() << std::endl;
        return false;
    }
}

void AutoKeystonePro::resetCalibration() {
    forwardMatrix_.release();
    inverseMatrix_.release();
    isCalibrated_ = false;
    detectedTrapezoid_.clear();
    std::cout << "[✓] 校正状态已重置" << std::endl;
}

AutoKeystonePro::Status AutoKeystonePro::getStatus() const {
    return Status{
        isCalibrated_,
        configPath_,
        canvasSize_,
        !inverseMatrix_.empty(),
        !detectedTrapezoid_.empty()
    };
}

} // namespace core

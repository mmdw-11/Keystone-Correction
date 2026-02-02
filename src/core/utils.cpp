#include "core/utils.h"
#include <opencv2/opencv.hpp>
#include <iostream>

namespace core {

void Utils::printHello() {
    std::cout << "Hello from Utils!" << std::endl;
}

bool Utils::saveImage(const std::string& path, const cv::Mat& image) {
    try {
        return cv::imwrite(path, image);
    } catch (const std::exception& e) {
        std::cerr << "[✗] 保存图像失败: " << e.what() << std::endl;
        return false;
    }
}

cv::Mat Utils::loadImage(const std::string& path) {
    try {
        cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
        if (img.empty()) {
            std::cerr << "[✗] 加载图像失败: " << path << std::endl;
        }
        return img;
    } catch (const std::exception& e) {
        std::cerr << "[✗] 加载图像异常: " << e.what() << std::endl;
        return cv::Mat();
    }
}

void Utils::drawPolygon(cv::Mat& image, const std::vector<cv::Point2f>& pts, const cv::Scalar& color, int thickness) {
    if (pts.size() < 2) return;
    for (size_t i = 0; i < pts.size(); ++i) {
        cv::line(image, pts[i], pts[(i+1)%pts.size()], color, thickness);
    }
}

void Utils::drawPoints(cv::Mat& image, const std::vector<cv::Point2f>& pts, const cv::Scalar& color, int radius) {
    for (const auto& pt : pts) {
        cv::circle(image, pt, radius, color, cv::FILLED);
    }
}

cv::Mat Utils::createGridPattern(int width, int height) {
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(255,255,255));
    for (int x = 0; x < width; x += 100)
        cv::line(img, {x,0}, {x,height}, cv::Scalar(200,200,200), 1);
    for (int y = 0; y < height; y += 100)
        cv::line(img, {0,y}, {width,y}, cv::Scalar(200,200,200), 1);
    int corner_radius = 20;
    std::vector<cv::Point> corners = {
        {corner_radius, corner_radius},
        {width-corner_radius, corner_radius},
        {width-corner_radius, height-corner_radius},
        {corner_radius, height-corner_radius}
    };
    for (const auto& c : corners)
        cv::circle(img, c, 5, cv::Scalar(0,0,255), -1);
    return img;
}

cv::Mat Utils::createCheckerboardPattern(int width, int height, int squareSize) {
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(0,0,0));
    for (int y = 0; y < height; y += squareSize) {
        for (int x = 0; x < width; x += squareSize) {
            int row = y / squareSize, col = x / squareSize;
            if ((row + col) % 2 == 0)
                img(cv::Rect(x, y, std::min(squareSize, width-x), std::min(squareSize, height-y))) = cv::Scalar(255,255,255);
        }
    }
    return img;
}

cv::Mat Utils::createSceneryPattern(int width, int height) {
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(235,206,135));
    cv::circle(img, {250,150}, 60, cv::Scalar(0,0,255), -1);
    cv::rectangle(img, {700,200}, {950,350}, cv::Scalar(255,255,255), -1);
    for (int x = 0; x < width; x += 100)
        cv::line(img, {x,0}, {x,height}, cv::Scalar(200,200,200), 1);
    for (int y = 0; y < height; y += 100)
        cv::line(img, {0,y}, {width,y}, cv::Scalar(200,200,200), 1);
    return img;
}

cv::Mat Utils::drawTrapezoidVisualization(const cv::Mat& img, const std::vector<cv::Point2f>& trapPts) {
    cv::Mat img_copy = img.clone();
    if (trapPts.size() != 4) return img_copy;
    std::vector<cv::Point> pts;
    for (const auto& pt : trapPts) pts.emplace_back(pt);
    cv::polylines(img_copy, pts, true, cv::Scalar(0,255,0), 3);
    for (size_t i = 0; i < trapPts.size(); ++i) {
        cv::circle(img_copy, pts[i], 8, cv::Scalar(0,0,255), -1);
        cv::putText(img_copy, std::to_string(i), pts[i]+cv::Point(10,10), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0,0,255), 2);
    }
    return img_copy;
}

bool Utils::detectChessboardCorners(const cv::Mat& img, std::vector<cv::Point2f>& corners, cv::Size patternSize) {
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    return cv::findChessboardCorners(gray, patternSize, corners);
}

void Utils::saveTestPattern(const std::string& filename, const std::string& patternType, int width, int height) {
    cv::Mat img;
    if (patternType == "grid")
        img = createGridPattern(width, height);
    else if (patternType == "scenery")
        img = createSceneryPattern(width, height);
    else if (patternType == "checkerboard")
        img = createCheckerboardPattern(width, height);
    else
        throw std::invalid_argument("Unknown pattern type: " + patternType);
    cv::imwrite(filename, img);
    std::cout << "[INFO] 测试图案已保存: " << filename << std::endl;
}

cv::Mat Utils::overlayText(const cv::Mat& img, const std::string& text, cv::Point position, double fontScale, int thickness) {
    cv::Mat img_copy = img.clone();
    cv::putText(img_copy, text, position, cv::FONT_HERSHEY_SIMPLEX, fontScale, cv::Scalar(0,0,255), thickness);
    return img_copy;
}

} // namespace core

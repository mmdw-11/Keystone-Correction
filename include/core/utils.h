#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

namespace core {

class Utils {
public:
    // 打印调试信息
    static void printHello();

    // 图像保存
    static bool saveImage(const std::string& path, const cv::Mat& image);

    // 图像加载
    static cv::Mat loadImage(const std::string& path);

    // 画多边形辅助线
    static void drawPolygon(cv::Mat& image, const std::vector<cv::Point2f>& pts, const cv::Scalar& color = {0,255,0}, int thickness = 2);

    // 画点
    static void drawPoints(cv::Mat& image, const std::vector<cv::Point2f>& pts, const cv::Scalar& color = {0,0,255}, int radius = 5);

    // 生成网格测试图案
    static cv::Mat createGridPattern(int width, int height);

    // 生成棋盘测试图案
    static cv::Mat createCheckerboardPattern(int width, int height, int squareSize = 200);

    // 生成场景测试图案
    static cv::Mat createSceneryPattern(int width, int height);

    // 绘制梯形边界
    static cv::Mat drawTrapezoidVisualization(const cv::Mat& img, const std::vector<cv::Point2f>& trapPts);

    // 检测棋盘格角点
    static bool detectChessboardCorners(const cv::Mat& img, std::vector<cv::Point2f>& corners, cv::Size patternSize = {7, 5});

    // 保存测试图案
    static void saveTestPattern(const std::string& filename, const std::string& patternType = "grid", int width = 1280, int height = 720);

    // 图像上叠加文字
    static cv::Mat overlayText(const cv::Mat& img, const std::string& text, cv::Point position = {50, 50}, double fontScale = 1.0, int thickness = 2);
};

} // namespace core

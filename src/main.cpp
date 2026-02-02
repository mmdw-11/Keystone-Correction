#include "core/engine.h"
#include "core/utils.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <exception>
#include <windows.h>
using namespace core;

// 生成棋盘测试图案（等价于Python的create_checkerboard_pattern）
// 用于投影仪校正时显示标准棋盘格
cv::Mat create_checkerboard_pattern(int width, int height, int square_size) {
    cv::Mat pattern(height, width, CV_8UC3, cv::Scalar(255,255,255));
    for (int y = 0; y < height; y += square_size) {
        for (int x = 0; x < width; x += square_size) {
            // Fill with black and white alternately
            if (((x / square_size) + (y / square_size)) % 2 == 0) {
                cv::rectangle(pattern, cv::Rect(x, y, square_size, square_size), cv::Scalar(0,0,0), cv::FILLED);
            }
        }
    }
    return pattern;
}

// 投影仪梯形校正系统主类，负责摄像头管理、校正流程、主循环等
class KeystoneProjectorSystem {
public:
    KeystoneProjectorSystem(int camera_index = 0, const std::string& config_file = "config.npy")
        : camera_index_(camera_index), config_file_(config_file), canvas_width_(1280), canvas_height_(720),
          engine_(config_file, cv::Size(1280, 720)), is_running_(true), calibration_in_progress_(false) {
        std::cout << "[*] 初始化梯形校正引擎..." << std::endl;
        cap_.open(camera_index_);
        _setup_camera();
        std::cout << "[✓] 系统初始化完成" << std::endl;
        _print_system_status();
    }
    ~KeystoneProjectorSystem() { cleanup(); }

    void _setup_camera() {
        if (!cap_.isOpened()) {
            std::cout << "[!] 警告：无法打开摄像头" << std::endl;
            return;
        }
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, canvas_width_);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, canvas_height_);
        cap_.set(cv::CAP_PROP_FPS, 30);
    }

    void _print_system_status() {
        auto status = engine_.getStatus();
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "【系统状态】" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "校正状态:     " << status.isCalibrated << std::endl;
        std::cout << "配置文件:     " << status.configPath << std::endl;
        std::cout << "画布分辨率:   " << status.canvasSize.width << " x " << status.canvasSize.height << std::endl;
        std::cout << "矩阵已加载:   " << status.matrixLoaded << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "\n快捷键说明：\n  [C] - 开始校正流程（投射网格→拍照→计算矩阵）\n  [R] - 重置校正（恢复到未校正状态）\n  [Q] - 退出程序\n";
        std::cout << std::string(70, '=') << std::endl;
    }

    void trigger_calibration_workflow() {
        if (calibration_in_progress_) {
            std::cout << "[!] 校正流程正在进行中，请稍候..." << std::endl;
            return;
        }
        calibration_in_progress_ = true;
        std::cout << std::string(70, '=') << std::endl;
        std::cout << "【开始自动校正流程】" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        try {
            std::cout << "[1/5] 投射棋盘校准图案..." << std::endl;
            cv::Mat test_pattern = create_checkerboard_pattern(canvas_width_, canvas_height_, 60);
            cv::imshow("PROJECTOR OUTPUT - Checkerboard Pattern", test_pattern);
            std::cout << "[✓] 完美的棋盘图案已显示" << std::endl;
            std::cout << "    （在实际系统中，这会投射到投影仪屏幕）" << std::endl;
            std::cout << "\n[2/5] 请将摄像头对准投影的网格表格..." << std::endl;
            std::cout << "      按 SPACE 键开始拍照（或 Q 取消）" << std::endl;

            while (true) {
                int key = cv::waitKey(100) & 0xFF;
                if (key == ' ') break;
                if (key == 'q' || key == 'Q') {
                    std::cout << "[✗] 校正已取消" << std::endl;
                    cv::destroyWindow("PROJECTOR OUTPUT - Checkerboard Pattern");
                    calibration_in_progress_ = false;
                    return;
                }
            }
            cv::destroyWindow("PROJECTOR OUTPUT - Checkerboard Pattern");
            std::cout << "\n[3/5] 自动拍照..." << std::endl;
            cv::Mat frame;
            cap_ >> frame;
            if (frame.empty()) {
                std::cout << "[✗] 错误：无法从摄像头获取图像"  << std::endl;
                calibration_in_progress_ = false;
                return;
            }
            std::cout << "[✓] 图像捕获成功" << std::endl;
            std::cout << "    （摄像头看到的是梯形变形的网格）" << std::endl;
            std::cout << "\n[4/5] 检测梯形边界..." << std::endl;
            std::cout << "[5/5] 计算校正矩阵..." << std::endl;
            bool success = engine_.calibrate(frame);
            if (success) {
                std::cout << std::string(70, '=') << std::endl;
                std::cout << "✓ 校正流程完成！" << std::endl;
                std::cout << std::string(70, '=') << std::endl;
                std::cout << "校正成功！系统现在可以正常投影了。\n";
                std::cout << "工作原理：\n- 保存了梯形→矩形 的正向变换矩阵\n- 保存了矩形→梯形 的反向变换矩阵（关键！）\n- 投影任何内容时，会自动应用反向变换\n- 梯形变形的内容投到梯形屏幕上 = 显示正确的矩形\n";
                std::cout << "如果想重新校正，按 [R] 重置后再按 [C] 校正。\n";
            } else {
                std::cout << "\n[✗] 校正失败，请检查摄像头和网格图案" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "\n[✗] 错误: " << e.what() << std::endl;
        }
        calibration_in_progress_ = false;
    }

    void reset_calibration() {
        engine_.resetCalibration();
        std::cout << "\n[✓] 校正状态已重置，系统回到未校正状态" << std::endl;
    }

    void run_main_loop() {
        while (is_running_) {
            cv::Mat camera_frame;
            cap_ >> camera_frame;
            if (camera_frame.empty()) {
                std::cout << "[!] 警告：无法从摄像头获取帧" << std::endl;
                continue;
            }
            cv::Mat projection_content = create_checkerboard_pattern(canvas_width_, canvas_height_, 100);
            cv::Mat output_content;
            std::string status_text;
            cv::Scalar status_color;
            if (engine_.getStatus().isCalibrated) {
                output_content = engine_.apply(projection_content, true);
                status_text = "CALIBRATED ✓";
                status_color = cv::Scalar(0,255,0);
            } else {
                output_content = projection_content;
                status_text = "NOT CALIBRATED ✗";
                status_color = cv::Scalar(0,0,255);
            }
            cv::putText(output_content, status_text, cv::Point(100,100), cv::FONT_HERSHEY_SIMPLEX, 1.5, status_color, 3);
            cv::putText(output_content, "C=Calibrate, R=Reset, Q=Quit", cv::Point(100,100), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255,255,255), 2);
            cv::imshow("Projector Output (after correction)", output_content);
            int key = cv::waitKey(30) & 0xFF;
            if (key == 'c' || key == 'C') {
                trigger_calibration_workflow();
            } else if (key == 'r' || key == 'R') {
                reset_calibration();
            } else if (key == 'q' || key == 'Q') {
                std::cout << "\n[*] 接收到退出信号..." << std::endl;
                is_running_ = false;
            }
        }
    }

    void cleanup() {
        std::cout << "\n[*] 清理资源..." << std::endl;
        if (cap_.isOpened()) cap_.release();
        cv::destroyAllWindows();
        std::cout << "[✓] 程序安全退出" << std::endl;
    }

private:
    int camera_index_;
    std::string config_file_;
    int canvas_width_;
    int canvas_height_;
    AutoKeystonePro engine_;
    cv::VideoCapture cap_;
    bool is_running_;
    bool calibration_in_progress_;
};

// 主程序入口，负责创建系统对象并启动主循环
#include "core/autofocus.h"

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::cout << "--- 投影校正系统 ---" << std::endl;
    try {
        // 第一次自动对焦
        std::cout << "\n[自动对焦] 阶段1：开始自动对焦...\n";
        autofocus_run(argc, argv);

        // 创建投影系统实例并开始摄像头校正流程
        KeystoneProjectorSystem system(0, "config.npy");
        system.run_main_loop();

        
    } catch (const std::exception& e) {
        std::cout << "\n[✗] 错误: " << e.what() << std::endl;
    }
    return 0;
}

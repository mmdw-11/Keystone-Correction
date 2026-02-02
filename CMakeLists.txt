

cmake_minimum_required(VERSION 3.10)
project(tmf8820_21_28_app_keystone_cpp)



set(CMAKE_CXX_STANDARD 17)

# 查找OpenCV
set(OpenCV_DIR "D:/SoftWare/opencv_final/x64/mingw/lib")  # 这里的路径要指向包含 OpenCVConfig.cmake 的文件夹


find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 包含头文件目录
include_directories(include)

# 源文件
file(GLOB SOURCES src/*.cpp src/core/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

# 链接OpenCV库
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

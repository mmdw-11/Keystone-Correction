# tmf8820_21_28_app_keystone_cpp

本项目为原Python版tmf8820_21_28_app_keystone的C++重构版本。

## 目录结构
- src/         源码目录
- include/     头文件目录
- core/        主要功能模块
- assets/      静态资源
- tests/       单元测试

## 构建方法

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

如需OpenCV等第三方库，请先安装并配置好环境。

#!/usr/bin/env python3
"""
生成测试BMP图像文件的脚本
创建30张1280x720的BMP图像，每张有不同的对焦效果（模拟焦距变化）
"""

import struct
import os

def create_bmp_header(width, height, filename):
    """创建BMP文件头"""
    # BMP文件格式
    filesize = 54 + width * height * 3  # 文件大小
    
    # BMP文件头
    bfType = b'BM'  # 文件类型
    bfSize = struct.pack('<I', filesize)  # 文件大小
    bfReserved1 = struct.pack('<H', 0)
    bfReserved2 = struct.pack('<H', 0)
    bfOffBits = struct.pack('<I', 54)  # 像素数据偏移
    
    # BMP信息头
    biSize = struct.pack('<I', 40)  # 信息头大小
    biWidth = struct.pack('<i', width)  # 图像宽度
    biHeight = struct.pack('<i', height)  # 图像高度
    biPlanes = struct.pack('<H', 1)  # 平面数
    biBitCount = struct.pack('<H', 24)  # 每像素位数
    biCompression = struct.pack('<I', 0)  # 压缩类型
    biSizeImage = struct.pack('<I', 0)
    biXPelsPerMeter = struct.pack('<i', 0)
    biYPelsPerMeter = struct.pack('<i', 0)
    biClrUsed = struct.pack('<I', 0)
    biClrImportant = struct.pack('<I', 0)
    
    header = (bfType + bfSize + bfReserved1 + bfReserved2 + bfOffBits +
              biSize + biWidth + biHeight + biPlanes + biBitCount +
              biCompression + biSizeImage + biXPelsPerMeter + biYPelsPerMeter +
              biClrUsed + biClrImportant)
    
    return header

def create_test_bmp(index, width=1280, height=720):
    """创建带有逐渐清晰变化的测试BMP图像"""
    filename = f"{index}.bmp"
    
    # 构建BMP文件头
    header = create_bmp_header(width, height, filename)
    
    # 生成图像数据（BGR格式，因为BMP使用BGR）
    # 创建一个渐变的对比度效果来模拟焦距变化
    # 在焦点清晰时有高对比度的条纹，模糊时变平缓
    
    pixel_data = bytearray()
    
    # 焦距参数：index越接近15，对比度越高（对焦最清晰）
    blur_factor = abs(index - 15) * 2  # 0-30的范围
    sharpness = 255 - blur_factor  # 越清晰值越大
    
    for y in range(height):
        for x in range(width):
            # 创建条纹图案，条纹间距受焦距影响
            pattern = ((x // (10 + blur_factor)) + (y // (10 + blur_factor))) % 2
            
            if pattern == 0:
                # 条纹1
                r = int(sharpness * 0.8)
                g = int(sharpness * 0.8)
                b = int(sharpness * 0.8)
            else:
                # 条纹2 - 白色
                r = 255
                g = 255
                b = 255
            
            # BMP格式是BGR
            pixel_data.append(min(b, 255))
            pixel_data.append(min(g, 255))
            pixel_data.append(min(r, 255))
    
    # 写入文件
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(pixel_data)
    
    print(f"生成: {filename} (对焦指数: {index}, 清晰度: {sharpness})")

def main():
    """生成30张测试BMP图像"""
    print("正在生成测试BMP图像...")
    print(f"当前目录: {os.getcwd()}")
    
    for i in range(1, 31):
        create_test_bmp(i)
    
    print("\n✓ 完成！生成了30张测试BMP图像 (1.bmp 到 30.bmp)")

if __name__ == "__main__":
    main()

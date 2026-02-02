// By QL.Liu & Adaptive AI
#include "core/autofocus.h"
#include "core/readBMP.h"
#include "stdio.h"

#define max(a,b) (a>b?a:b)
#define min(a,b) (a<b?a:b)

// 模拟马达动作（用于算法流程和调试）
void Hardware_MotorAction(AF_ControlBlock *cb) {
    if (cb == NULL) return;
    if (cb->status == AF_CONVERGED || cb->status == AF_REACHED_LIMIT) {
        printf("[Motor] FINAL MOVE: Returning to Best Pos [%d] from Current Pos [%d] (Move %d steps)\n",
                cb->best_pos, cb->current_pos, cb->focus_step);
    } else {
        printf("[Motor] Step: %d, Dir: %d | Absolute Pos: %d/%d\n",
                cb->focus_step, cb->move_direction, cb->current_pos, cb->max_limit);
    }
}

//绝对值判断
unsigned int abs_(int value) {
    int t = value >> 31;
    return t ^ (value + t);
}
//开方
unsigned short sqrt_(unsigned int n) {
    unsigned short a = 0;
    while (n >= (2 * a) + 1) {
        n -= (2 * a++) + 1;
    }
    return a;
}

// 这些常量可以放在头文件或作为配置传入
static const unsigned short IMG_W = 1280;
static const unsigned short IMG_H = 720;

// 计算区域到中心距离
static unsigned int cal_distance_tocenter(struct roidefine roi) {
    unsigned int imageCenterX = IMG_W >> 1;
    unsigned int imageCenterY = IMG_H >> 1;
    unsigned int regionCenterX = roi.roi_x + roi.roi_width / 2;
    unsigned int regionCenterY = roi.roi_y + roi.roi_height / 2;

    unsigned int dx = abs_(imageCenterX - regionCenterX);
    unsigned int dy = abs_(imageCenterY - regionCenterY);
    unsigned int ndx = dx * 100 / IMG_W;
    unsigned int ndy = dy * 100 / IMG_H;
    return sqrt_(ndx * ndx + ndy * ndy);
}

// 得到像素点指针位置（优化：使用宏或内联提高效率）
unsigned char *get_position(unsigned char *image, ushort x, ushort y) {
    return (image + y * IMG_W + x);
}

// --- 图像分析核心 ---
//计算反差值
uint get_region_contrast(unsigned char *image, struct roidefine roi) {
    // 算法逻辑保持不变，但确保它只依赖传入的 roi 参数
    unsigned char step_y = (roi.roi_height >> 3) + 1;
    unsigned char step_x = (roi.roi_width >> 3) + 1;
    unsigned int sharpness = 0;
	//分区计算反差值
    // 水平方向梯度计算
    for (unsigned char y = step_y; (y + 4) < roi.roi_height; y += step_y) {
        unsigned int max_contrast = 0;
        for (unsigned char x = 0; (x + 16) < roi.roi_width; x += 4) {
            int a = 0, b = 0;
            for (unsigned char i = 0; i < 8; ++i)
                for (unsigned char j = 0; j < 4; ++j)
                    a += *get_position(image, roi.roi_x + x + i, roi.roi_y + y + j);
            a >>= 4;
            for (unsigned char i = 8; i < 16; ++i)
                for (unsigned char j = 0; j < 4; ++j)
                    b += *get_position(image, roi.roi_x + x + i, roi.roi_y + y + j);
            b >>= 4;
            max_contrast = max((uint)abs_(a - b), max_contrast);
        }
        sharpness += max_contrast;
    }

    //左右分块比较
    for (unsigned int x = step_x; (x + 4) < roi.roi_width; x += step_x) {
        unsigned int max_contrast = 0;
        for (unsigned int y = 0; (y + 16) < roi.roi_height; y += 4) {
            int a = 0, b = 0;
            for (unsigned int i = 0; i < 8; ++i)
                for (unsigned int j = 0; j < 4; ++j)
                    a += *get_position(image, roi.roi_x + x + i, roi.roi_y + y + j);
            a >>= 4;
            for (unsigned int i = 8; i < 16; ++i)
                for (unsigned int j = 0; j < 4; ++j)
                    b += *get_position(image, roi.roi_x + x + i, roi.roi_y + y + j);
            b >>= 4;
            max_contrast = max((uint)abs_(a - b), max_contrast);
        }
        sharpness += max_contrast;
    }
    return sharpness;
}

//获取高反差的块
struct roidefine get_roi_region(unsigned char *image) {
    // 返回最佳 ROI 结构体
    struct roifocus rf[200];
    struct roidefine result;
    ushort regions_x = IMG_W / 128;
    ushort regions_y = IMG_H / 128;
    uint start_x = (IMG_W - regions_x * 128) / 2;
    uint start_y = (IMG_H - regions_y * 128) / 2;

    for (ushort y = 0; y < regions_y; ++y) {
        for (ushort x = 0; x < regions_x; ++x) {
            struct roidefine r = { (uint)(start_x + x * 128), (uint)(start_y + y * 128), 128, 128 };
            rf[x + y * regions_x].roi_x = r.roi_x;
            rf[x + y * regions_x].roi_y = r.roi_y;
            rf[x + y * regions_x].roi_sharpness = get_region_contrast(image, r);
        }
    }

    uint maxsharpness = 0;
    int maxroi_idx = 0;
    for (int i = 0; i < (regions_x * regions_y); ++i) {
        struct roidefine r = { rf[i].roi_x, rf[i].roi_y, 128, 128 };
        uint dist = cal_distance_tocenter(r) + 60;
        rf[i].roi_weighted_sharpness = (rf[i].roi_sharpness * 10000) / (dist * dist);
        
        if (rf[i].roi_weighted_sharpness > maxsharpness) {
            maxsharpness = rf[i].roi_weighted_sharpness;
            maxroi_idx = i;
        }
    }

    result.roi_x = rf[maxroi_idx].roi_x;
    result.roi_y = rf[maxroi_idx].roi_y;
    result.roi_width = 128;
    result.roi_height = 128;
    return result;
}

// --- 核心重构：爬山算法逻辑迭代器 ---
void focus_step_logic(unsigned char *grey_data, AF_ControlBlock *cb, struct roidefine roi) {
    //roi表示当前聚焦区域，cb表示控制块，grey_data表示灰度图像数据
    if (cb == NULL || grey_data == NULL) return;

    // 1. 设置合理的阈值
    const uint rate_epsylon = 0; 

    // 2. 初始化状态
    if (cb->status == AF_IDLE) {
        cb->status = AF_SCANNING;
        cb->focus_step = 10;
        cb->move_direction = 1;
        cb->max_rate = 0;
        cb->epoch_tomax = 0; // 在这里，我们将它重定义为“连续下坡计数器”
        cb->have_max = 0;
        cb->current_frame_idx = 0;

		// 硬件相关初始化
        cb->current_pos = 1000;    // 假设初始位置在行程中间
        cb->best_pos = 1000;       // 初始最优位置
        cb->max_limit = 2048;      // HZF-24BYJ48-2 电机常见的安全行程
    }

    cb->current_frame_idx++;
    cb->current_rate = get_region_contrast(grey_data, roi);

    // 3. 初次或刷新最高纪录
    // 只要当前分数比历史最高高，就更新，并重置下坡计数
    if (cb->max_rate == 0 || cb->current_rate > (cb->max_rate + rate_epsylon)) {
        cb->max_rate = cb->current_rate;
        cb->best_frame_idx = cb->current_frame_idx;
		cb->best_pos = cb->current_pos; // 【关键】记录最清晰时的电机步数
        cb->have_max = 1;
        cb->epoch_tomax = 0; // 只要还在创新高，下坡计数就归零
        cb->last_rate = cb->current_rate;
        // return;
    }

    // 4. 【关键改动】确认收敛逻辑：检测是否已经翻过山顶
    if (cb->have_max) {
        // 如果当前分数明显低于最高纪录，说明我们在走下坡路
        if (cb->current_rate < (cb->max_rate - rate_epsylon)) {
            cb->epoch_tomax++; 
        } 
		

        // 连续 3 帧确认在下坡，判定为收敛
        if (cb->epoch_tomax >= 3) {
            cb->status = AF_CONVERGED;
            cb->move_direction = -1; // 准备回退
            // 回退步长：从当前帧位置退回到最高分那一帧的位置
            cb->focus_step = (cb->current_frame_idx - cb->best_frame_idx) * 10;
            return;
        }
    }

    // 5. 步长和方向的细微调整（用于搜索阶段）
    int delta_rate = (int)cb->current_rate - (int)cb->last_rate;
    if (delta_rate < -(int)rate_epsylon && !cb->have_max) {
        cb->move_direction *= -1; // 还没见过山顶就一直在掉，说明方向反了
    }
	// 6. 【关键】边界检查逻辑
    // 预测下一步的位置
    int next_pos = cb->current_pos + (cb->move_direction * cb->focus_step);

    if (next_pos > cb->max_limit || next_pos < 0) {
        // 如果撞墙了
        cb->status = AF_REACHED_LIMIT;
        // 计算强制回到历史最优点的步数
        cb->focus_step = abs_(cb->current_pos - cb->best_pos);
        cb->move_direction = (cb->best_pos > cb->current_pos) ? 1 : -1;
        return;
    }

    // 7. 更新当前物理位置记录，并保存当前分数
    cb->current_pos = next_pos;
    
    cb->last_rate = cb->current_rate;
}

// 保留原有命令行参数和配置特性，主流程接口
int autofocus_run(int argc, char** argv) {
    Image image;
    Image *imagepointer = &image;
    AF_ControlBlock af_cb = { AF_IDLE };
    struct roidefine ez;
    char filename[30][20] = {
        "../assets/1.bmp", "../assets/2.bmp", "../assets/3.bmp", "../assets/4.bmp", "../assets/5.bmp",
        "../assets/6.bmp", "../assets/7.bmp", "../assets/8.bmp", "../assets/9.bmp", "../assets/10.bmp",
        "../assets/11.bmp", "../assets/12.bmp", "../assets/13.bmp", "../assets/14.bmp", "../assets/15.bmp",
        "../assets/16.bmp", "../assets/17.bmp", "../assets/18.bmp", "../assets/19.bmp", "../assets/20.bmp",
        "../assets/21.bmp", "../assets/22.bmp", "../assets/23.bmp", "../assets/24.bmp", "../assets/25.bmp",
        "../assets/26.bmp", "../assets/27.bmp", "../assets/28.bmp", "../assets/29.bmp", "../assets/30.bmp"
    };
    printf("==============================================\n");
    printf("   Projector Autofocus System - Hardware Ready\n");
    printf("==============================================\n\n");
    printf("[Stage 1] Locking ROI (Region of Interest)...\n");
    if (ImageLoad(filename[0], imagepointer)) {
        ez = get_roi_region(imagepointer->greydata);
        printf("Target Locked at: (%d, %d)\n\n", ez.roi_x, ez.roi_y);
    } else {
        printf("Error: Could not load initial frame.\n");
        return -1;
    }
    printf("[Stage 2] Running hill climbing algorithm...\n");
    printf("----------------------------------------------------------------------\n");
    printf("Iter   Frame      Contrast  Dir   Step   MaxRate   Status\n");
    printf("----------------------------------------------------------------------\n");
    for (int frame_loc = 0; frame_loc < 30; ++frame_loc) {
        if (ImageLoad(filename[frame_loc], imagepointer)) {
            focus_step_logic(imagepointer->greydata, &af_cb, ez);
            const char* status_str = "Scanning";
            if (af_cb.status == AF_CONVERGED) status_str = "CONVERGED";
            else if (af_cb.status == AF_REACHED_LIMIT) status_str = "HIT_LIMIT";
            else if (af_cb.status == AF_ERROR) status_str = "ERROR";
            char dir_char = (af_cb.move_direction > 0) ? '+' : '-';
            printf("%2d     %-10s %-9d %c   %-6d %-9d %s\n",
                frame_loc + 1,
                filename[frame_loc] + 10,
                // filename[frame_loc],
                af_cb.current_rate,
                dir_char,
                af_cb.focus_step,
                af_cb.max_rate,
                status_str);
            if (af_cb.status == AF_SCANNING) {
                printf("    [Motor] Moving to %d...\n", af_cb.current_pos);
            }
            Hardware_MotorAction(&af_cb);
            if (af_cb.status == AF_CONVERGED || af_cb.status == AF_REACHED_LIMIT) {
                break;
            }
        }
    }
    printf("\n--------------------------------------------------------------------------------\n");
    if (af_cb.status == AF_CONVERGED) {
        printf(">>> SUCCESS: Focus achieved at Best Position: %d (Frame %d)\n",
                af_cb.best_pos, af_cb.best_frame_idx);
    } else if (af_cb.status == AF_REACHED_LIMIT) {
        printf(">>> WARNING: Motor reached rotation limit (%d). Returning to Best possible point: %d\n",
                af_cb.max_limit, af_cb.best_pos);
    } else {
        printf(">>> FAILED: Search complete but target not found.\n");
    }
    Hardware_MotorAction(&af_cb);
    printf("--------------------------------------------------------------------------------\n\n");
    return 0;
}

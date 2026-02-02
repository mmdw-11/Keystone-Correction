#ifndef AUTO_FOCUS_H_INC_
#define AUTO_FOCUS_H_INC_

typedef unsigned int uint;
typedef unsigned short ushort;

// --- 1. 定义算法运行状态 ---
typedef enum {
    AF_IDLE = 0,      // 空闲/未开始
    AF_SCANNING,      // 正在爬山搜索中
    AF_CONVERGED,     // 已找到最佳焦点（对焦成功）
    AF_REACHED_LIMIT, // 新增：触碰物理/软件边界（转到头了）
    AF_ERROR          // 出错（如画面太暗、全白）
} AF_Status;

// --- 2. 核心状态结构体（重要：把全局变量封装到这里） ---
typedef struct {
    AF_Status status;         // 当前状态
    uint current_rate;        // 当前帧的对比度分数
    uint last_rate;           // 上一帧的分数
    uint max_rate;            // 历史最高分
    
    ushort best_frame_idx;    // 记录哪一帧分数最高
    ushort current_frame_idx; // 当前是第几帧
    

    // ==========================================
    // 硬件适配新增：位置控制相关变量
    // ==========================================
    int current_pos;          // 当前绝对位置（步数，从 0 开始累计）
    int best_pos;             // 达到 max_rate 时的绝对位置（用于最后“倒车”归位）
    int max_limit;            // 电机最大行程限制（带教老师提到的旋转角度限制）

    short focus_step;         // 当前马达移动步长
    char move_direction;      // 移动方向：1 为正向，-1 为负向
    
    ushort epoch_tomax;       // 确认下坡的计数器
	ushort step_tomax;        // 记录到达最大值时的累积步长
    unsigned char have_max;   // 是否已经见过峰值
} AF_ControlBlock;

// --- 3. 基础图像处理结构体（保持不变） ---
struct roifocus {
    uint roi_x;
    uint roi_y;
    uint roi_sharpness;
    uint roi_weighted_sharpness;
};

struct roidefine {
    uint roi_x;
    uint roi_y;
    uint roi_width;
    uint roi_height;
};

// --- 4. 工具函数 ---
unsigned int abs_(int value);
unsigned short sqrt_(unsigned int n);
unsigned char *get_position(unsigned char *image, ushort x, ushort y);

// --- 5. 核心算法接口 ---

// 获取当前画面的对比度得分
uint get_region_contrast(unsigned char *image, struct roidefine roi);

// 在图像中自动寻找纹理最丰富的区域（靶心）
struct roidefine get_roi_region(unsigned char *image);

/**
 * 爬山算法核心迭代（重构重点）
 * @param grey_data: 当前帧的灰度数据
 * @param cb: 状态控制块，存储对焦过程中的所有记忆
 * @param roi: 锁定的对焦区域
 */
void focus_step_logic(unsigned char *grey_data, AF_ControlBlock *cb, struct roidefine roi);

// --- 6. 预留的硬件驱动接口（现在只定义，不实现） ---
// 以后有了投影仪，只需要在底层实现这个函数即可
void Hardware_MotorMove(short steps, char direction);
void Hardware_MotorMoveTo(int absolute_pos); // 新增：直接驱动马达去往指定位置

// 模拟马达动作（用于算法流程和调试）
void Hardware_MotorAction(AF_ControlBlock *cb);

// --- 7. 主流程接口 ---
#ifdef __cplusplus
extern "C" {
#endif

// 保留原有命令行参数和配置特性
int autofocus_run(int argc, char** argv);

#ifdef __cplusplus
}
#endif

#endif
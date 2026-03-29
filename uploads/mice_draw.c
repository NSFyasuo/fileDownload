#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>   // 新增，用于 memcpy

#define C_W 10
#define C_H 17

// 鼠标样式宏定义
#define BORD 0x000000  // 边框黑色
#define T___ 0x1000000 // 透明
#define X___ 0x000000  // 填充黑色

// 创意1: 多颜色画笔
int colors[] = {0xf80000, 0x001f00, 0x0000ff, 0x000000};  // 红色、绿色、蓝色、黑色
int color_index = 0;

// 创意3: 擦除模式
int erase_mode = 0;

// 光标像素数组（固定形状）
int cursor_pixel[C_W * C_H] = {
   BORD, T___, T___, T___, T___, T___, T___, T___, T___, T___,
   BORD, BORD, T___, T___, T___, T___, T___, T___, T___, T___,
   BORD, X___, BORD, T___, T___, T___, T___, T___, T___, T___,
   BORD, X___, X___, BORD, T___, T___, T___, T___, T___, T___,
   BORD, X___, X___, X___, BORD, T___, T___, T___, T___, T___,
   BORD, X___, X___, X___, X___, BORD, T___, T___, T___, T___,
   BORD, X___, X___, X___, X___, X___, BORD, T___, T___, T___,
   BORD, X___, X___, X___, X___, X___, X___, BORD, T___, T___,
   BORD, X___, X___, X___, X___, X___, X___, X___, BORD, T___,
   BORD, X___, X___, X___, X___, X___, X___, X___, X___, BORD,
   BORD, X___, X___, X___, X___, X___, BORD, BORD, BORD, BORD,
   BORD, X___, X___, BORD, X___, X___, BORD, T___, T___, T___,
   BORD, X___, BORD, T___, BORD, X___, X___, BORD, T___, T___,
   BORD, BORD, T___, T___, BORD, X___, X___, BORD, T___, T___,
   T___, T___, T___, T___, T___, BORD, X___, X___, BORD, T___,
   T___, T___, T___, T___, T___, BORD, X___, X___, BORD, T___,
   T___, T___, T___, T___, T___, T___, BORD, BORD, T___, T___
};
// 用于保存光标下方背景的缓冲区
int bg_buffer[C_W * C_H];

typedef struct {
    int button;
    int dx;
    int dy;
    int dz;
} mevent_t;

typedef struct {
    int fb_fd;
    struct fb_var_screeninfo fb_var;
    int w;
    int h;
    int bpp;
    int *fb_mem;
} fb_dev, *pfb_dev;

// 前向声明
void draw_cursor(pfb_dev pfb, int x, int y);
void draw_brush(pfb_dev pfb, int x, int y, int color);

// 鼠标读取函数（修正：检查返回值，使用3字节协议）
int mice(int mfd, mevent_t *mevent) {
    signed char buf[3];
    int n = read(mfd, buf, 3);
    if (n != 3) {
        return -1;   // 读取失败或数据不完整
    }
    mevent->button = buf[0] & 7;  // 修正：包含中键（低3位：左、右、中）
    mevent->dx = buf[1];
    mevent->dy = -buf[2];   // Y轴方向反转
    return 0;
}

// 帧缓冲初始化（增加错误处理）
int fb_init(pfb_dev pfb) {
    pfb->fb_fd = open("/dev/fb0", O_RDWR);
    if (pfb->fb_fd < 0) {
        perror("open /dev/fb0");
        return -1;
    }
    if (ioctl(pfb->fb_fd, FBIOGET_VSCREENINFO, &(pfb->fb_var)) < 0) {
        perror("ioctl");
        close(pfb->fb_fd);
        return -1;
    }
    pfb->w = pfb->fb_var.xres;
    pfb->h = pfb->fb_var.yres;
    pfb->bpp = pfb->fb_var.bits_per_pixel;
    printf("w = %d, h = %d, bpp = %d\n", pfb->w, pfb->h, pfb->bpp);

    pfb->fb_mem = mmap(NULL, pfb->w * pfb->h * pfb->bpp / 8,
                       PROT_READ | PROT_WRITE, MAP_SHARED, pfb->fb_fd, 0);
    if (pfb->fb_mem == MAP_FAILED) {
        perror("mmap");
        close(pfb->fb_fd);
        return -1;
    }
    return 0;
}

// 画一个像素（带边界检查）
void fb_point(pfb_dev pfb, int x, int y, int color) {
    if (x >= 0 && x < pfb->w && y >= 0 && y < pfb->h)
        pfb->fb_mem[y * pfb->w + x] = color;
}

// 用指定颜色填充整个屏幕（修正坐标顺序）
void screen_color(pfb_dev pfb, int color) {
    int j, i;
    for (j = 0; j < pfb->h; ++j)
        for (i = 0; i < pfb->w; ++i)
            fb_point(pfb, i, j, color);
}

// 设置光标颜色（填充cursor_pixel数组）- 已移除，光标固定形状

// 在指定位置绘制光标（使用cursor_pixel，支持透明）
void draw_cursor(pfb_dev pfb, int x, int y) {
    int j, i;
    for (j = 0; j < C_H; ++j)
        for (i = 0; i < C_W; ++i) {
            int color = cursor_pixel[j * C_W + i];
            if (color != 0x1000000) {  // 非透明则绘制
                fb_point(pfb, x + i, y + j, color);
            }
        }
}

// 在指定位置绘制画笔（实心，使用指定颜色）
void draw_brush(pfb_dev pfb, int x, int y, int color) {
    int j, i;
    for (j = 0; j < C_H; ++j)
        for (i = 0; i < C_W; ++i)
            fb_point(pfb, x + i, y + j, color);
}

// 保存光标下方的背景到bg_buffer
void save_bg(pfb_dev pfb, int x, int y) {
    int j, i;
    for (j = 0; j < C_H; ++j)
        for (i = 0; i < C_W; ++i)
            bg_buffer[j * C_W + i] = pfb->fb_mem[(y + j) * pfb->w + (x + i)];
}

// 用bg_buffer恢复光标下方的背景
void restore_bg(pfb_dev pfb, int x, int y) {
    int j, i;
    for (j = 0; j < C_H; ++j)
        for (i = 0; i < C_W; ++i)
            fb_point(pfb, x + i, y + j, bg_buffer[j * C_W + i]);
}

int main(int argc, char *argv[]) {
    fb_dev fb;
    int m_x, m_y;
    int mfd;
    mevent_t mevent;

    // 初始化帧缓冲
    if (fb_init(&fb) < 0) {
        fprintf(stderr, "fb_init failed\n");
        exit(1);
    }
    screen_color(&fb, 0xffffff);   // 设置背景色

    // 打开鼠标设备
    mfd = open("/dev/input/mice", O_RDWR);
    if (mfd < 0) {
        perror("open /dev/input/mice");
        munmap(fb.fb_mem, fb.w * fb.h * fb.bpp / 8);
        close(fb.fb_fd);
        exit(1);
    }

    // 初始化光标位置（居中）
    m_x = fb.w / 2 - C_W / 2;
    m_y = fb.h / 2 - C_H / 2;
    // 边界调整
    if (m_x < 0) m_x = 0;
    if (m_y < 0) m_y = 0;
    if (m_x > fb.w - C_W) m_x = fb.w - C_W;
    if (m_y > fb.h - C_H) m_y = fb.h - C_H;

    save_bg(&fb, m_x, m_y);       // 保存初始背景
    draw_cursor(&fb, m_x, m_y);          // 绘制初始光标

    while (1) {
        if (mice(mfd, &mevent) < 0)
            continue;              // 读取失败则跳过

        // 1. 擦除旧光标（恢复背景）
        restore_bg(&fb, m_x, m_y);

        // 2. 更新坐标
        m_x += mevent.dx;
        m_y += mevent.dy;

        // 3. 边界限制（考虑光标大小）
        if (m_x < 0) m_x = 0;
        if (m_y < 0) m_y = 0;
        if (m_x > fb.w - C_W) m_x = fb.w - C_W;
        if (m_y > fb.h - C_H) m_y = fb.h - C_H;

        // 创意1: 右键切换颜色
        if (mevent.button & 2) {
            color_index = (color_index + 1) % 4;
        }

        // 创意3: 中键切换擦除模式
        if (mevent.button & 4) {
            erase_mode = !erase_mode;
        }

        // 4. 画笔功能：如果左键按下，在当前光标位置绘制痕迹
        if (mevent.button & 1) {
            int brush_color = erase_mode ? 0xffffff : colors[color_index];
            draw_brush(&fb, m_x, m_y, brush_color);
        }

        // 5. 保存新位置的背景（此时背景已包含可能绘制的痕迹）
        save_bg(&fb, m_x, m_y);

        // 6. 在新位置绘制光标
        draw_cursor(&fb, m_x, m_y);
    }

    // 以下代码实际不会执行，但保留以释放资源
    close(mfd);
    munmap(fb.fb_mem, fb.w * fb.h * fb.bpp / 8);
    close(fb.fb_fd);
    return 0;
}
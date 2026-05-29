/**
 * @file test_font_render_win.c
 * @brief Windows平台字库渲染测试程序
 *
 * 编译命令（MinGW）：
 * gcc test_font_render_win.c lvgl_font_render.c -o test_font_render.exe -lgdi32 -luser32
 *
 * 编译命令（MSVC）：
 * cl test_font_render_win.c lvgl_font_render.c user32.lib gdi32.lib
 */

#include "lvgl_font_render.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
 * 全局变量
 * ========================================================================== */

static HWND g_hwnd = NULL;
static HDC g_hdc_mem = NULL;
static HBITMAP g_hbitmap = NULL;
static uint8_t *g_framebuffer = NULL;
static int g_fb_width = 800;
static int g_fb_height = 600;

// 外部字体声明（需要包含生成的字库文件）
extern const lv_font_t my_font_16;  // 请替换为你的字体名称

/* ============================================================================
 * 帧缓冲操作
 * ========================================================================== */

/**
 * @brief 初始化帧缓冲
 */
static bool init_framebuffer(int width, int height)
{
    g_fb_width = width;
    g_fb_height = height;

    // 分配帧缓冲（RGBA格式）
    g_framebuffer = (uint8_t*)calloc(width * height * 4, 1);
    if (!g_framebuffer) {
        return false;
    }

    // 创建内存DC和位图
    HDC hdc = GetDC(g_hwnd);
    g_hdc_mem = CreateCompatibleDC(hdc);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // 负值表示从上到下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits;
    g_hbitmap = CreateDIBSection(g_hdc_mem, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    SelectObject(g_hdc_mem, g_hbitmap);

    ReleaseDC(g_hwnd, hdc);

    return true;
}

/**
 * @brief 清空帧缓冲
 */
static void clear_framebuffer(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < g_fb_width * g_fb_height; i++) {
        g_framebuffer[i * 4 + 0] = b;  // B
        g_framebuffer[i * 4 + 1] = g;  // G
        g_framebuffer[i * 4 + 2] = r;  // R
        g_framebuffer[i * 4 + 3] = 255;  // A
    }
}

/**
 * @brief 像素绘制回调（用于字体渲染）
 */
static void draw_pixel_callback(int16_t x, int16_t y, uint8_t alpha, void *user_data)
{
    if (x < 0 || x >= g_fb_width || y < 0 || y >= g_fb_height) {
        return;
    }

    // 前景色（黑色）
    uint8_t fg_r = 0, fg_g = 0, fg_b = 0;

    // 背景色（白色）
    uint8_t bg_r = 255, bg_g = 255, bg_b = 255;

    // Alpha混合
    int idx = (y * g_fb_width + x) * 4;
    g_framebuffer[idx + 0] = (fg_b * alpha + bg_b * (255 - alpha)) / 255;  // B
    g_framebuffer[idx + 1] = (fg_g * alpha + bg_g * (255 - alpha)) / 255;  // G
    g_framebuffer[idx + 2] = (fg_r * alpha + bg_r * (255 - alpha)) / 255;  // R
    g_framebuffer[idx + 3] = 255;  // A
}

/**
 * @brief 刷新显示
 */
static void refresh_display(void)
{
    if (!g_hdc_mem || !g_hbitmap) return;

    // 复制帧缓冲到位图
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_fb_width;
    bmi.bmiHeader.biHeight = -g_fb_height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBits(g_hdc_mem, g_hbitmap, 0, g_fb_height, g_framebuffer, &bmi, DIB_RGB_COLORS);

    // 触发重绘
    InvalidateRect(g_hwnd, NULL, FALSE);
}

/* ============================================================================
 * 测试用例
 * ========================================================================== */

/**
 * @brief 测试1：简单文本绘制
 */
static void test_simple_text(void)
{
    clear_framebuffer(255, 255, 255);

    lv_draw_text_simple(&my_font_16, 10, 30, "Hello World!", 0, draw_pixel_callback, NULL);
    lv_draw_text_simple(&my_font_16, 10, 60, "你好世界！", 0, draw_pixel_callback, NULL);
    lv_draw_text_simple(&my_font_16, 10, 90, "0123456789", 2, draw_pixel_callback, NULL);

    refresh_display();
}

/**
 * @brief 测试2：左对齐文本
 */
static void test_align_left(void)
{
    clear_framebuffer(255, 255, 255);

    lv_text_config_t config;
    lv_text_config_init(&config);
    config.window.x = 50;
    config.window.y = 50;
    config.window.width = 700;
    config.window.height = 500;
    config.h_align = LV_TEXT_ALIGN_LEFT;
    config.word_wrap = true;
    config.line_spacing = 5;

    const char *text = "这是一段测试文本。\n"
                      "This is a test text.\n"
                      "支持自动换行和多行显示。\n"
                      "Supports word wrap and multi-line display.";

    lv_draw_text_advanced(&my_font_16, text, &config, draw_pixel_callback, NULL);

    // 绘制窗口边框
    for (int x = config.window.x; x < config.window.x + config.window.width; x++) {
        draw_pixel_callback(x, config.window.y, 128, NULL);
        draw_pixel_callback(x, config.window.y + config.window.height - 1, 128, NULL);
    }
    for (int y = config.window.y; y < config.window.y + config.window.height; y++) {
        draw_pixel_callback(config.window.x, y, 128, NULL);
        draw_pixel_callback(config.window.x + config.window.width - 1, y, 128, NULL);
    }

    refresh_display();
}

/**
 * @brief 测试3：居中对齐文本
 */
static void test_align_center(void)
{
    clear_framebuffer(255, 255, 255);

    lv_text_config_t config;
    lv_text_config_init(&config);
    config.window.x = 50;
    config.window.y = 50;
    config.window.width = 700;
    config.window.height = 500;
    config.h_align = LV_TEXT_ALIGN_CENTER;
    config.word_wrap = true;
    config.line_spacing = 5;

    const char *text = "居中对齐\n"
                      "Center Aligned\n"
                      "每行文本都会居中显示";

    lv_draw_text_advanced(&my_font_16, text, &config, draw_pixel_callback, NULL);

    // 绘制窗口边框
    for (int x = config.window.x; x < config.window.x + config.window.width; x++) {
        draw_pixel_callback(x, config.window.y, 128, NULL);
        draw_pixel_callback(x, config.window.y + config.window.height - 1, 128, NULL);
    }
    for (int y = config.window.y; y < config.window.y + config.window.height; y++) {
        draw_pixel_callback(config.window.x, y, 128, NULL);
        draw_pixel_callback(config.window.x + config.window.width - 1, y, 128, NULL);
    }

    refresh_display();
}

/**
 * @brief 测试4：右对齐文本
 */
static void test_align_right(void)
{
    clear_framebuffer(255, 255, 255);

    lv_text_config_t config;
    lv_text_config_init(&config);
    config.window.x = 50;
    config.window.y = 50;
    config.window.width = 700;
    config.window.height = 500;
    config.h_align = LV_TEXT_ALIGN_RIGHT;
    config.word_wrap = true;
    config.line_spacing = 5;

    const char *text = "右对齐\n"
                      "Right Aligned\n"
                      "每行文本都会右对齐显示";

    lv_draw_text_advanced(&my_font_16, text, &config, draw_pixel_callback, NULL);

    // 绘制窗口边框
    for (int x = config.window.x; x < config.window.x + config.window.width; x++) {
        draw_pixel_callback(x, config.window.y, 128, NULL);
        draw_pixel_callback(x, config.window.y + config.window.height - 1, 128, NULL);
    }
    for (int y = config.window.y; y < config.window.y + config.window.height; y++) {
        draw_pixel_callback(config.window.x, y, 128, NULL);
        draw_pixel_callback(config.window.x + config.window.width - 1, y, 128, NULL);
    }

    refresh_display();
}

/* ============================================================================
 * Windows窗口处理
 * ========================================================================== */

static int g_current_test = 0;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            g_hwnd = hwnd;
            init_framebuffer(g_fb_width, g_fb_height);
            test_simple_text();
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            if (g_hdc_mem) {
                BitBlt(hdc, 0, 0, g_fb_width, g_fb_height, g_hdc_mem, 0, 0, SRCCOPY);
            }

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_KEYDOWN:
            switch (wParam) {
                case '1':
                    g_current_test = 0;
                    test_simple_text();
                    break;
                case '2':
                    g_current_test = 1;
                    test_align_left();
                    break;
                case '3':
                    g_current_test = 2;
                    test_align_center();
                    break;
                case '4':
                    g_current_test = 3;
                    test_align_right();
                    break;
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
            }
            break;

        case WM_DESTROY:
            if (g_framebuffer) free(g_framebuffer);
            if (g_hbitmap) DeleteObject(g_hbitmap);
            if (g_hdc_mem) DeleteDC(g_hdc_mem);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

/* ============================================================================
 * 主函数
 * ========================================================================== */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const char CLASS_NAME[] = "FontRenderTestWindow";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "LVGL Font Render Test - Press 1/2/3/4 to switch tests, ESC to exit",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, g_fb_width + 16, g_fb_height + 39,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

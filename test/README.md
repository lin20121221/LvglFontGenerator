# LVGL字库渲染测试程序

这是一个通用的LVGL字库渲染引擎，可以在任何MCU平台上使用，无需引入完整的LVGL库。

## 文件说明

### 核心文件（可移植到MCU）

- **lvgl_font_render.h** - 渲染引擎头文件
- **lvgl_font_render.c** - 渲染引擎实现

### 测试文件（仅用于Windows测试）

- **test_font_render_win.c** - Windows平台测试程序
- **README.md** - 本文档

## 功能特性

### ✅ 核心功能

- 支持LVGL 8.x和9.x生成的字库
- 支持1/2/4/8位BPP（抗锯齿级别）
- 支持UTF-8编码
- 完全独立，无需LVGL库

### ✅ 文本渲染

- 单字符绘制
- 字符串绘制
- 自动换行
- 字符间距调整
- 行间距调整

### ✅ 对齐模式

- **水平对齐**：左对齐、居中、右对齐、两端对齐
- **垂直对齐**：顶部、居中、底部
- **窗口模式**：支持在指定区域内渲染

## 使用方法

### 1. 生成字库文件

使用LVGL字体生成工具生成字库文件，例如 `my_font_16.c`

### 2. 在MCU项目中集成

#### 步骤1：添加文件到项目

```
your_project/
├── lvgl_font_render.h
├── lvgl_font_render.c
└── my_font_16.c          # 生成的字库文件
```

#### 步骤2：包含头文件

```c
#include "lvgl_font_render.h"

// 声明字体（在my_font_16.c中定义）
extern const lv_font_t my_font_16;
```

#### 步骤3：实现像素绘制回调

```c
// 示例：绘制到帧缓冲
uint8_t framebuffer[128 * 64 / 8];  // 128x64单色屏

void draw_pixel_callback(int16_t x, int16_t y, uint8_t alpha, void *user_data)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    
    // 对于单色屏，alpha > 128 视为点亮
    if (alpha > 128) {
        int byte_index = (y / 8) * 128 + x;
        int bit_index = y % 8;
        framebuffer[byte_index] |= (1 << bit_index);
    }
}
```

#### 步骤4：绘制文本

**简单模式（无窗口限制）：**

```c
// 清空帧缓冲
memset(framebuffer, 0, sizeof(framebuffer));

// 绘制文本
lv_draw_text_simple(&my_font_16, 10, 20, "Hello 世界", 0, 
                    draw_pixel_callback, NULL);

// 刷新显示屏
display_update(framebuffer);
```

**高级模式（支持窗口、对齐、换行）：**

```c
// 配置渲染参数
lv_text_config_t config;
lv_text_config_init(&config);

config.window.x = 0;
config.window.y = 0;
config.window.width = 128;
config.window.height = 64;
config.h_align = LV_TEXT_ALIGN_CENTER;  // 居中对齐
config.v_align = LV_TEXT_ALIGN_MIDDLE;  // 垂直居中
config.word_wrap = true;                // 自动换行
config.line_spacing = 2;                // 行间距
config.letter_spacing = 0;              // 字符间距

// 绘制文本
const char *text = "这是一段\n测试文本\nTest Text";
lv_draw_text_advanced(&my_font_16, text, &config, 
                      draw_pixel_callback, NULL);

// 刷新显示屏
display_update(framebuffer);
```

### 3. Windows平台测试

#### 准备工作

1. 生成测试字库文件（包含常用字符）：
   - 字符集：`0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz你好世界测试文本这是一段支持自动换行和多行显示居中对齐右左每都会`
   - 字体大小：16px
   - 输出名称：`my_font_16`

2. 将生成的 `my_font_16.c` 复制到 `test` 目录

#### 编译测试程序

**使用MinGW：**

```bash
cd test
gcc test_font_render_win.c lvgl_font_render.c my_font_16.c -o test_font_render.exe -lgdi32 -luser32
```

**使用MSVC：**

```bash
cd test
cl test_font_render_win.c lvgl_font_render.c my_font_16.c user32.lib gdi32.lib
```

#### 运行测试

```bash
./test_font_render.exe
```

**操作说明：**
- 按 `1` - 测试简单文本绘制
- 按 `2` - 测试左对齐
- 按 `3` - 测试居中对齐
- 按 `4` - 测试右对齐
- 按 `ESC` - 退出程序

## API参考

### 初始化配置

```c
void lv_text_config_init(lv_text_config_t *config);
```

初始化默认文本配置。

### 字体查询

```c
int lv_font_get_glyph_index(const lv_font_t *font, uint32_t unicode);
const lv_font_glyph_dsc_t* lv_font_get_glyph_dsc(const lv_font_t *font, uint32_t unicode);
```

根据Unicode查找字形。

### 文本测量

```c
int16_t lv_text_get_width(const lv_font_t *font, const char *text, uint8_t letter_spacing);
```

计算文本宽度（像素）。

### 字符绘制

```c
int16_t lv_draw_char(const lv_font_t *font, int16_t x, int16_t y, uint32_t unicode,
                     lv_draw_pixel_cb_t draw_pixel, void *user_data);
```

绘制单个字符，返回字符宽度。

### 文本绘制

```c
void lv_draw_text_simple(const lv_font_t *font, int16_t x, int16_t y, const char *text,
                         uint8_t letter_spacing, lv_draw_pixel_cb_t draw_pixel, void *user_data);
```

简单模式：在指定位置绘制文本。

```c
void lv_draw_text_advanced(const lv_font_t *font, const char *text,
                           const lv_text_config_t *config,
                           lv_draw_pixel_cb_t draw_pixel, void *user_data);
```

高级模式：支持窗口、对齐、换行。

### UTF-8解码

```c
uint8_t lv_utf8_decode(const char *text, uint32_t *unicode);
```

解码UTF-8字符，返回消耗的字节数。

## 对齐模式说明

### 水平对齐

- `LV_TEXT_ALIGN_LEFT` - 左对齐（默认）
- `LV_TEXT_ALIGN_CENTER` - 水平居中
- `LV_TEXT_ALIGN_RIGHT` - 右对齐
- `LV_TEXT_ALIGN_JUSTIFY` - 两端对齐（拉伸）

### 垂直对齐

- `LV_TEXT_ALIGN_TOP` - 顶部对齐（默认）
- `LV_TEXT_ALIGN_MIDDLE` - 垂直居中
- `LV_TEXT_ALIGN_BOTTOM` - 底部对齐

## 平台适配指南

### 单色OLED（128x64）

```c
// SSD1306 OLED示例
uint8_t oled_buffer[128 * 64 / 8];

void oled_draw_pixel(int16_t x, int16_t y, uint8_t alpha, void *user_data)
{
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    
    if (alpha > 128) {
        int page = y / 8;
        int bit = y % 8;
        oled_buffer[page * 128 + x] |= (1 << bit);
    }
}

// 使用
memset(oled_buffer, 0, sizeof(oled_buffer));
lv_draw_text_simple(&my_font_16, 0, 16, "Hello", 0, oled_draw_pixel, NULL);
ssd1306_display(oled_buffer);
```

### 彩色TFT（RGB565）

```c
// ST7789 TFT示例
uint16_t tft_buffer[240 * 320];

void tft_draw_pixel(int16_t x, int16_t y, uint8_t alpha, void *user_data)
{
    if (x < 0 || x >= 240 || y < 0 || y >= 320) return;
    
    // 前景色（黑色）和背景色（白色）混合
    uint16_t fg_color = 0x0000;  // 黑色
    uint16_t bg_color = 0xFFFF;  // 白色
    
    // Alpha混合（简化）
    if (alpha > 128) {
        tft_buffer[y * 240 + x] = fg_color;
    }
}

// 使用
for (int i = 0; i < 240 * 320; i++) tft_buffer[i] = 0xFFFF;
lv_draw_text_simple(&my_font_16, 10, 30, "你好", 0, tft_draw_pixel, NULL);
st7789_display(tft_buffer);
```

## 性能优化建议

1. **使用合适的BPP**：单色屏使用1位，彩色屏使用8位
2. **限制字符集**：只包含实际需要的字符
3. **缓存字形查找**：频繁使用的字符可以缓存索引
4. **使用DMA传输**：显示刷新时使用DMA提高效率

## 内存占用

- **代码大小**：约2-3KB（编译优化后）
- **RAM占用**：几乎为0（字库数据在Flash中）
- **栈占用**：约100-200字节

## 常见问题

### Q: 字符显示不完整？

A: 检查窗口大小是否足够，或者字体的line_height设置。

### Q: 中文显示乱码？

A: 确保源代码使用UTF-8编码，并且字库包含这些中文字符。

### Q: 如何支持垂直居中？

A: 使用高级模式，设置 `config.v_align = LV_TEXT_ALIGN_MIDDLE`。

### Q: 如何实现滚动文本？

A: 调整窗口的y坐标，配合定时器实现滚动效果。

## 许可证

MIT License - 可自由用于商业和开源项目

## 相关资源

- LVGL官方文档：https://docs.lvgl.io/
- 字体生成工具：本项目的LvglFontGenerator

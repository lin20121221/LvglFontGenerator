# 在非LVGL平台使用生成的字库

本工具生成的字库文件虽然是为LVGL设计的，但其数据结构简单清晰，可以在任何MCU平台上使用，无需引入完整的LVGL库。

## 字库文件结构

生成的.c文件包含以下主要部分：

1. **位图数据** (`glyph_bitmap[]`) - 每个字符的像素数据
2. **字形描述** (`glyph_dsc[]`) - 每个字符的尺寸和偏移信息
3. **字符映射表** (`cmaps[]`) - Unicode到字形的映射关系
4. **字体描述符** (`font_dsc`) - 字体的整体信息

## 独立使用方法

### 方法一：提取核心数据结构（推荐）

创建一个简化的头文件，只包含必要的数据结构：

```c
// simple_font.h
#ifndef SIMPLE_FONT_H
#define SIMPLE_FONT_H

#include <stdint.h>

// 字形描述结构
typedef struct {
    uint32_t bitmap_index;  // 位图数据在数组中的起始位置
    uint8_t adv_w;          // 字符前进宽度（单位：1/16像素）
    uint8_t box_w;          // 字形宽度（像素）
    uint8_t box_h;          // 字形高度（像素）
    int8_t ofs_x;           // X轴偏移（像素）
    int8_t ofs_y;           // Y轴偏移（像素）
} glyph_dsc_t;

// 字符映射表结构
typedef struct {
    uint32_t range_start;   // Unicode起始值
    uint32_t range_length;  // 范围长度
    uint32_t glyph_id_start;// 字形ID起始值
    const uint16_t *unicode_list;  // Unicode列表（稀疏映射时使用）
    uint8_t list_length;    // 列表长度
    uint8_t type;           // 映射类型
} cmap_t;

// 字体描述符结构
typedef struct {
    const uint8_t *glyph_bitmap;    // 位图数据
    const glyph_dsc_t *glyph_dsc;   // 字形描述数组
    const cmap_t *cmaps;            // 字符映射表
    uint8_t cmap_num;               // 映射表数量
    uint8_t bpp;                    // 每像素位数
    uint8_t line_height;            // 行高
    uint8_t base_line;              // 基线
} font_dsc_t;

#endif // SIMPLE_FONT_H
```

### 方法二：修改生成的字库文件

将生成的.c文件中的LVGL相关代码替换为简化版本：

**原始代码：**
```c
#include "lvgl.h"

static const lv_font_fmt_txt_dsc_t font_dsc = {
    // ...
};

const lv_font_t my_font = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    // ...
};
```

**修改为：**
```c
#include "simple_font.h"

static const font_dsc_t font_dsc = {
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .cmap_num = 1,
    .bpp = 8,
    .line_height = 16,
    .base_line = 0
};

const font_dsc_t *my_font = &font_dsc;
```

## 使用示例

### 1. 查找字符的字形数据

```c
#include "simple_font.h"
#include "my_font.c"  // 生成的字库文件

// 根据Unicode查找字形索引
int find_glyph_index(const font_dsc_t *font, uint32_t unicode) {
    const cmap_t *cmap = &font->cmaps[0];
    
    // 连续映射
    if (cmap->unicode_list == NULL) {
        if (unicode >= cmap->range_start && 
            unicode < cmap->range_start + cmap->range_length) {
            return cmap->glyph_id_start + (unicode - cmap->range_start);
        }
    }
    // 稀疏映射
    else {
        for (int i = 0; i < cmap->list_length; i++) {
            if (cmap->unicode_list[i] == unicode) {
                return cmap->glyph_id_start + i;
            }
        }
    }
    
    return -1;  // 未找到
}

// 获取字形描述
const glyph_dsc_t* get_glyph(const font_dsc_t *font, uint32_t unicode) {
    int index = find_glyph_index(font, unicode);
    if (index < 0) return NULL;
    
    return &font->glyph_dsc[index];
}
```

### 2. 绘制单个字符

```c
// 绘制字符到帧缓冲区
void draw_char(uint8_t *framebuffer, int fb_width, int fb_height,
               int x, int y, uint32_t unicode, const font_dsc_t *font) {
    
    const glyph_dsc_t *glyph = get_glyph(font, unicode);
    if (!glyph) return;
    
    // 计算实际绘制位置
    int draw_x = x + glyph->ofs_x;
    int draw_y = y + glyph->ofs_y;
    
    // 获取位图数据
    const uint8_t *bitmap = &font->glyph_bitmap[glyph->bitmap_index];
    
    // 根据BPP绘制
    if (font->bpp == 1) {
        // 1位：黑白
        draw_char_1bpp(framebuffer, fb_width, fb_height, 
                      draw_x, draw_y, glyph, bitmap);
    }
    else if (font->bpp == 8) {
        // 8位：灰度
        draw_char_8bpp(framebuffer, fb_width, fb_height, 
                      draw_x, draw_y, glyph, bitmap);
    }
}

// 1位位图绘制
void draw_char_1bpp(uint8_t *fb, int fb_width, int fb_height,
                    int x, int y, const glyph_dsc_t *glyph, 
                    const uint8_t *bitmap) {
    
    int bit_index = 0;
    
    for (int row = 0; row < glyph->box_h; row++) {
        for (int col = 0; col < glyph->box_w; col++) {
            int px = x + col;
            int py = y + row;
            
            if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                // 读取位
                int byte_index = bit_index / 8;
                int bit_offset = 7 - (bit_index % 8);
                uint8_t pixel = (bitmap[byte_index] >> bit_offset) & 0x01;
                
                if (pixel) {
                    // 设置像素（根据你的显示格式调整）
                    fb[py * fb_width + px] = 0xFF;
                }
            }
            bit_index++;
        }
    }
}

// 8位位图绘制
void draw_char_8bpp(uint8_t *fb, int fb_width, int fb_height,
                    int x, int y, const glyph_dsc_t *glyph, 
                    const uint8_t *bitmap) {
    
    for (int row = 0; row < glyph->box_h; row++) {
        for (int col = 0; col < glyph->box_w; col++) {
            int px = x + col;
            int py = y + row;
            
            if (px >= 0 && px < fb_width && py >= 0 && py < fb_height) {
                uint8_t alpha = bitmap[row * glyph->box_w + col];
                
                // Alpha混合（如果需要）
                fb[py * fb_width + px] = alpha;
            }
        }
    }
}
```

### 3. 绘制字符串

```c
void draw_string(uint8_t *framebuffer, int fb_width, int fb_height,
                 int x, int y, const char *text, const font_dsc_t *font) {
    
    int cursor_x = x;
    
    // 简单的UTF-8解码和绘制
    while (*text) {
        uint32_t unicode = 0;
        int bytes = 0;
        
        // UTF-8解码
        if ((*text & 0x80) == 0) {
            unicode = *text;
            bytes = 1;
        }
        else if ((*text & 0xE0) == 0xC0) {
            unicode = ((*text & 0x1F) << 6) | (*(text+1) & 0x3F);
            bytes = 2;
        }
        else if ((*text & 0xF0) == 0xE0) {
            unicode = ((*text & 0x0F) << 12) | 
                     ((*(text+1) & 0x3F) << 6) | 
                     (*(text+2) & 0x3F);
            bytes = 3;
        }
        
        // 绘制字符
        draw_char(framebuffer, fb_width, fb_height, 
                 cursor_x, y, unicode, font);
        
        // 移动光标
        const glyph_dsc_t *glyph = get_glyph(font, unicode);
        if (glyph) {
            cursor_x += glyph->adv_w / 16;  // adv_w单位是1/16像素
        }
        
        text += bytes;
    }
}
```

## 完整示例

```c
#include "simple_font.h"
#include "my_font_16.c"  // 你生成的字库

// 假设有一个128x64的单色显示屏
uint8_t framebuffer[128 * 64 / 8];

int main(void) {
    // 初始化显示屏...
    
    // 清空帧缓冲
    memset(framebuffer, 0, sizeof(framebuffer));
    
    // 绘制文本
    draw_string(framebuffer, 128, 64, 10, 20, "Hello 世界", my_font);
    
    // 刷新显示屏...
    
    return 0;
}
```

## 注意事项

1. **内存占用**：字库数据存储在Flash中，运行时只需要很小的RAM
2. **字符编码**：确保源代码使用UTF-8编码
3. **显示格式**：根据你的显示屏格式调整像素设置代码
4. **性能优化**：可以添加字形缓存来提高查找速度
5. **抗锯齿**：如果使用8位BPP，需要实现Alpha混合

## 优势

✅ 无需引入LVGL库，节省Flash空间  
✅ 数据结构简单，易于理解和修改  
✅ 支持任意MCU平台和显示屏  
✅ 可以根据需求裁剪功能  
✅ 完全控制渲染过程

## 工具生成建议

使用本工具时的建议设置：
- **字体类型**：选择"内部字体"
- **抗锯齿**：根据显示屏选择合适的BPP
  - 单色屏：1位
  - 灰度屏：2位或4位
  - 彩色屏：8位
- **字符集**：只包含实际需要的字符，减小字库大小

## 参考资源

- LVGL字体格式文档：https://docs.lvgl.io/master/overview/font.html
- UTF-8编码说明：https://en.wikipedia.org/wiki/UTF-8

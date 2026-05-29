/**
 * @file lvgl_font_render.c
 * @brief 通用LVGL字库渲染引擎实现
 */

#include "lvgl_font_render.h"
#include <string.h>

/* ============================================================================
 * 内部辅助函数
 * ========================================================================== */

/**
 * @brief UTF-8解码
 */
uint8_t lv_utf8_decode(const char *text, uint32_t *unicode)
{
    if (!text || !unicode) return 0;

    uint8_t c = (uint8_t)text[0];

    if ((c & 0x80) == 0) {
        // 单字节 (0xxxxxxx)
        *unicode = c;
        return 1;
    }
    else if ((c & 0xE0) == 0xC0) {
        // 双字节 (110xxxxx 10xxxxxx)
        *unicode = ((c & 0x1F) << 6) | (text[1] & 0x3F);
        return 2;
    }
    else if ((c & 0xF0) == 0xE0) {
        // 三字节 (1110xxxx 10xxxxxx 10xxxxxx)
        *unicode = ((c & 0x0F) << 12) |
                   ((text[1] & 0x3F) << 6) |
                   (text[2] & 0x3F);
        return 3;
    }
    else if ((c & 0xF8) == 0xF0) {
        // 四字节 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        *unicode = ((c & 0x07) << 18) |
                   ((text[1] & 0x3F) << 12) |
                   ((text[2] & 0x3F) << 6) |
                   (text[3] & 0x3F);
        return 4;
    }

    // 无效UTF-8序列
    *unicode = 0;
    return 1;
}

/* ============================================================================
 * 字体查找函数
 * ========================================================================== */

/**
 * @brief 根据Unicode查找字形索引
 */
int lv_font_get_glyph_index(const lv_font_t *font, uint32_t unicode)
{
    if (!font || !font->dsc) return -1;

    const lv_font_fmt_txt_dsc_t *dsc = font->dsc;

    // 遍历所有字符映射表
    for (uint16_t i = 0; i < dsc->cmap_num; i++) {
        const lv_font_fmt_txt_cmap_t *cmap = &dsc->cmaps[i];

        // 连续映射
        if (cmap->type == LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY) {
            if (unicode >= cmap->range_start &&
                unicode < cmap->range_start + cmap->range_length) {
                return cmap->glyph_id_start + (unicode - cmap->range_start);
            }
        }
        // 稀疏映射
        else if (cmap->type == LV_FONT_FMT_TXT_CMAP_SPARSE_TINY) {
            for (uint16_t j = 0; j < cmap->list_length; j++) {
                if (cmap->unicode_list[j] == unicode) {
                    return cmap->glyph_id_start + j;
                }
            }
        }
    }

    return -1;  // 未找到
}

/**
 * @brief 获取字形描述
 */
const lv_font_glyph_dsc_t* lv_font_get_glyph_dsc(const lv_font_t *font, uint32_t unicode)
{
    int index = lv_font_get_glyph_index(font, unicode);
    if (index < 0) return NULL;

    return &font->dsc->glyph_dsc[index];
}

/* ============================================================================
 * 文本测量函数
 * ========================================================================== */

/**
 * @brief 计算文本宽度
 */
int16_t lv_text_get_width(const lv_font_t *font, const char *text, uint8_t letter_spacing)
{
    if (!font || !text) return 0;

    int16_t width = 0;
    const char *p = text;

    while (*p) {
        uint32_t unicode;
        uint8_t len = lv_utf8_decode(p, &unicode);
        if (len == 0) break;

        const lv_font_glyph_dsc_t *glyph = lv_font_get_glyph_dsc(font, unicode);
        if (glyph) {
            width += glyph->adv_w / 16;  // adv_w单位是1/16像素
            if (*(p + len)) {  // 不是最后一个字符
                width += letter_spacing;
            }
        }

        p += len;
    }

    return width;
}

/* ============================================================================
 * 像素绘制函数
 * ========================================================================== */

/**
 * @brief 绘制1位位图
 */
static void draw_glyph_1bpp(int16_t x, int16_t y, const lv_font_glyph_dsc_t *glyph,
                           const uint8_t *bitmap, lv_draw_pixel_cb_t draw_pixel, void *user_data)
{
    int bit_index = 0;

    for (int row = 0; row < glyph->box_h; row++) {
        for (int col = 0; col < glyph->box_w; col++) {
            int byte_index = bit_index / 8;
            int bit_offset = 7 - (bit_index % 8);
            uint8_t pixel = (bitmap[byte_index] >> bit_offset) & 0x01;

            if (pixel) {
                draw_pixel(x + col, y + row, 255, user_data);
            }

            bit_index++;
        }
    }
}

/**
 * @brief 绘制2位位图
 */
static void draw_glyph_2bpp(int16_t x, int16_t y, const lv_font_glyph_dsc_t *glyph,
                           const uint8_t *bitmap, lv_draw_pixel_cb_t draw_pixel, void *user_data)
{
    int bit_index = 0;

    for (int row = 0; row < glyph->box_h; row++) {
        for (int col = 0; col < glyph->box_w; col++) {
            int byte_index = bit_index / 4;
            int bit_offset = 6 - ((bit_index % 4) * 2);
            uint8_t pixel = (bitmap[byte_index] >> bit_offset) & 0x03;

            // 2位转8位：0->0, 1->85, 2->170, 3->255
            uint8_t alpha = pixel * 85;

            if (alpha > 0) {
                draw_pixel(x + col, y + row, alpha, user_data);
            }

            bit_index++;
        }
    }
}

/**
 * @brief 绘制4位位图
 */
static void draw_glyph_4bpp(int16_t x, int16_t y, const lv_font_glyph_dsc_t *glyph,
                           const uint8_t *bitmap, lv_draw_pixel_cb_t draw_pixel, void *user_data)
{
    int pixel_index = 0;

    for (int row = 0; row < glyph->box_h; row++) {
        for (int col = 0; col < glyph->box_w; col++) {
            int byte_index = pixel_index / 2;
            int nibble = pixel_index % 2;
            uint8_t pixel = nibble ? (bitmap[byte_index] & 0x0F) : (bitmap[byte_index] >> 4);

            // 4位转8位：乘以17
            uint8_t alpha = pixel * 17;

            if (alpha > 0) {
                draw_pixel(x + col, y + row, alpha, user_data);
            }

            pixel_index++;
        }
    }
}

/**
 * @brief 绘制8位位图
 */
static void draw_glyph_8bpp(int16_t x, int16_t y, const lv_font_glyph_dsc_t *glyph,
                           const uint8_t *bitmap, lv_draw_pixel_cb_t draw_pixel, void *user_data)
{
    for (int row = 0; row < glyph->box_h; row++) {
        for (int col = 0; col < glyph->box_w; col++) {
            uint8_t alpha = bitmap[row * glyph->box_w + col];

            if (alpha > 0) {
                draw_pixel(x + col, y + row, alpha, user_data);
            }
        }
    }
}

/* ============================================================================
 * 字符绘制函数
 * ========================================================================== */

/**
 * @brief 绘制单个字符
 */
int16_t lv_draw_char(const lv_font_t *font, int16_t x, int16_t y, uint32_t unicode,
                     lv_draw_pixel_cb_t draw_pixel, void *user_data)
{
    if (!font || !draw_pixel) return 0;

    const lv_font_glyph_dsc_t *glyph = lv_font_get_glyph_dsc(font, unicode);
    if (!glyph) return 0;

    // 计算实际绘制位置
    int16_t draw_x = x + glyph->ofs_x;
    int16_t draw_y = y + glyph->ofs_y;

    // 获取位图数据
    const uint8_t *bitmap = &font->dsc->glyph_bitmap[glyph->bitmap_index];

    // 根据BPP绘制
    switch (font->dsc->bpp) {
        case 1:
            draw_glyph_1bpp(draw_x, draw_y, glyph, bitmap, draw_pixel, user_data);
            break;
        case 2:
            draw_glyph_2bpp(draw_x, draw_y, glyph, bitmap, draw_pixel, user_data);
            break;
        case 4:
            draw_glyph_4bpp(draw_x, draw_y, glyph, bitmap, draw_pixel, user_data);
            break;
        case 8:
            draw_glyph_8bpp(draw_x, draw_y, glyph, bitmap, draw_pixel, user_data);
            break;
        default:
            break;
    }

    return glyph->adv_w / 16;
}

/* ============================================================================
 * 文本绘制函数
 * ========================================================================== */

/**
 * @brief 绘制文本（简单模式）
 */
void lv_draw_text_simple(const lv_font_t *font, int16_t x, int16_t y, const char *text,
                         uint8_t letter_spacing, lv_draw_pixel_cb_t draw_pixel, void *user_data)
{
    if (!font || !text || !draw_pixel) return;

    int16_t cursor_x = x;
    const char *p = text;

    while (*p) {
        uint32_t unicode;
        uint8_t len = lv_utf8_decode(p, &unicode);
        if (len == 0) break;

        int16_t char_width = lv_draw_char(font, cursor_x, y, unicode, draw_pixel, user_data);
        cursor_x += char_width + letter_spacing;

        p += len;
    }
}

/**
 * @brief 初始化默认文本配置
 */
void lv_text_config_init(lv_text_config_t *config)
{
    if (!config) return;

    config->window.x = 0;
    config->window.y = 0;
    config->window.width = 320;
    config->window.height = 240;
    config->h_align = LV_TEXT_ALIGN_LEFT;
    config->v_align = LV_TEXT_ALIGN_TOP;
    config->word_wrap = true;
    config->line_spacing = 2;
    config->letter_spacing = 0;
}

/**
 * @brief 绘制文本（高级模式）
 */
void lv_draw_text_advanced(const lv_font_t *font, const char *text,
                           const lv_text_config_t *config,
                           lv_draw_pixel_cb_t draw_pixel, void *user_data)
{
    if (!font || !text || !config || !draw_pixel) return;

    // 简化实现：分行处理
    const char *line_start = text;
    const char *p = text;
    int16_t line_y = config->window.y;

    while (*p) {
        // 查找换行符或测量当前行
        const char *line_end = p;
        int16_t line_width = 0;

        while (*line_end && *line_end != '\n') {
            uint32_t unicode;
            uint8_t len = lv_utf8_decode(line_end, &unicode);
            if (len == 0) break;

            const lv_font_glyph_dsc_t *glyph = lv_font_get_glyph_dsc(font, unicode);
            if (glyph) {
                int16_t char_width = glyph->adv_w / 16 + config->letter_spacing;

                // 检查是否需要换行
                if (config->word_wrap && line_width + char_width > config->window.width && line_end != line_start) {
                    break;
                }

                line_width += char_width;
            }

            line_end += len;
        }

        // 计算行起始X坐标（根据对齐方式）
        int16_t line_x = config->window.x;

        switch (config->h_align) {
            case LV_TEXT_ALIGN_CENTER:
                line_x += (config->window.width - line_width) / 2;
                break;
            case LV_TEXT_ALIGN_RIGHT:
                line_x += config->window.width - line_width;
                break;
            case LV_TEXT_ALIGN_JUSTIFY:
                // 两端对齐（简化实现）
                break;
            default:  // LEFT
                break;
        }

        // 绘制当前行
        int16_t cursor_x = line_x;
        const char *char_p = line_start;

        while (char_p < line_end) {
            uint32_t unicode;
            uint8_t len = lv_utf8_decode(char_p, &unicode);
            if (len == 0) break;

            int16_t char_width = lv_draw_char(font, cursor_x, line_y, unicode, draw_pixel, user_data);
            cursor_x += char_width + config->letter_spacing;

            char_p += len;
        }

        // 移动到下一行
        line_y += font->line_height + config->line_spacing;

        // 检查是否超出窗口
        if (line_y >= config->window.y + config->window.height) {
            break;
        }

        // 跳过换行符
        if (*line_end == '\n') {
            line_end++;
        }

        line_start = line_end;
        p = line_end;
    }
}

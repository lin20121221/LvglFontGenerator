/**
 * @file lvgl_font_render.h
 * @brief 通用LVGL字库渲染引擎 - 适用于任何MCU平台
 *
 * 功能特性：
 * - 支持LVGL 8.x和9.x生成的字库
 * - 支持单字符和字符串渲染
 * - 支持自动换行
 * - 支持多种对齐模式（左、右、居中、拉伸）
 * - 支持窗口裁剪
 * - 无需LVGL库，可独立使用
 */

#ifndef LVGL_FONT_RENDER_H
#define LVGL_FONT_RENDER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 数据结构定义（兼容LVGL字库格式）
 * ========================================================================== */

/** 字形描述结构 */
typedef struct {
    uint32_t bitmap_index;  /**< 位图数据在数组中的起始位置 */
    uint8_t adv_w;          /**< 字符前进宽度（单位：1/16像素） */
    uint8_t box_w;          /**< 字形宽度（像素） */
    uint8_t box_h;          /**< 字形高度（像素） */
    int8_t ofs_x;           /**< X轴偏移（像素） */
    int8_t ofs_y;           /**< Y轴偏移（像素） */
} lv_font_glyph_dsc_t;

/** 字符映射表类型 */
typedef enum {
    LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY = 0,  /**< 连续Unicode范围 */
    LV_FONT_FMT_TXT_CMAP_SPARSE_TINY = 1    /**< 稀疏Unicode映射 */
} lv_font_fmt_txt_cmap_type_t;

/** 字符映射表结构 */
typedef struct {
    uint32_t range_start;           /**< Unicode起始值 */
    uint16_t range_length;          /**< 范围长度 */
    uint16_t glyph_id_start;        /**< 字形ID起始值 */
    const uint16_t *unicode_list;   /**< Unicode列表（稀疏映射时使用） */
    const void *glyph_id_ofs_list;  /**< 字形ID偏移列表 */
    uint16_t list_length;           /**< 列表长度 */
    uint8_t type;                   /**< 映射类型 */
} lv_font_fmt_txt_cmap_t;

/** 字体描述符结构 */
typedef struct {
    const uint8_t *glyph_bitmap;        /**< 位图数据 */
    const lv_font_glyph_dsc_t *glyph_dsc; /**< 字形描述数组 */
    const lv_font_fmt_txt_cmap_t *cmaps;  /**< 字符映射表 */
    const void *kern_dsc;               /**< 字距调整描述（暂不支持） */
    uint16_t kern_scale;                /**< 字距调整比例 */
    uint16_t cmap_num;                  /**< 映射表数量 */
    uint8_t bpp;                        /**< 每像素位数 */
    uint8_t kern_classes;               /**< 字距调整类数量 */
    uint8_t bitmap_format;              /**< 位图格式 */
} lv_font_fmt_txt_dsc_t;

/** 字体结构（简化版） */
typedef struct {
    const lv_font_fmt_txt_dsc_t *dsc;   /**< 字体描述符 */
    uint8_t line_height;                /**< 行高 */
    uint8_t base_line;                  /**< 基线 */
} lv_font_t;

/* ============================================================================
 * 渲染配置
 * ========================================================================== */

/** 文本对齐模式 */
typedef enum {
    LV_TEXT_ALIGN_LEFT = 0,     /**< 左对齐 */
    LV_TEXT_ALIGN_CENTER,       /**< 水平居中 */
    LV_TEXT_ALIGN_RIGHT,        /**< 右对齐 */
    LV_TEXT_ALIGN_TOP,          /**< 顶部对齐 */
    LV_TEXT_ALIGN_MIDDLE,       /**< 垂直居中 */
    LV_TEXT_ALIGN_BOTTOM,       /**< 底部对齐 */
    LV_TEXT_ALIGN_JUSTIFY       /**< 两端对齐（拉伸） */
} lv_text_align_t;

/** 渲染窗口 */
typedef struct {
    int16_t x;          /**< 窗口X坐标 */
    int16_t y;          /**< 窗口Y坐标 */
    int16_t width;      /**< 窗口宽度 */
    int16_t height;     /**< 窗口高度 */
} lv_area_t;

/** 渲染配置 */
typedef struct {
    lv_area_t window;           /**< 渲染窗口 */
    lv_text_align_t h_align;    /**< 水平对齐 */
    lv_text_align_t v_align;    /**< 垂直对齐 */
    bool word_wrap;             /**< 是否自动换行 */
    uint8_t line_spacing;       /**< 行间距（像素） */
    uint8_t letter_spacing;     /**< 字符间距（像素） */
} lv_text_config_t;

/** 像素绘制回调函数类型 */
typedef void (*lv_draw_pixel_cb_t)(int16_t x, int16_t y, uint8_t alpha, void *user_data);

/* ============================================================================
 * API函数
 * ========================================================================== */

/**
 * @brief 初始化默认文本配置
 * @param config 配置结构指针
 */
void lv_text_config_init(lv_text_config_t *config);

/**
 * @brief 根据Unicode查找字形索引
 * @param font 字体指针
 * @param unicode Unicode字符
 * @return 字形索引，-1表示未找到
 */
int lv_font_get_glyph_index(const lv_font_t *font, uint32_t unicode);

/**
 * @brief 获取字形描述
 * @param font 字体指针
 * @param unicode Unicode字符
 * @return 字形描述指针，NULL表示未找到
 */
const lv_font_glyph_dsc_t* lv_font_get_glyph_dsc(const lv_font_t *font, uint32_t unicode);

/**
 * @brief 计算文本宽度
 * @param font 字体指针
 * @param text UTF-8编码的文本
 * @param letter_spacing 字符间距
 * @return 文本宽度（像素）
 */
int16_t lv_text_get_width(const lv_font_t *font, const char *text, uint8_t letter_spacing);

/**
 * @brief 绘制单个字符
 * @param font 字体指针
 * @param x X坐标
 * @param y Y坐标（基线位置）
 * @param unicode Unicode字符
 * @param draw_pixel 像素绘制回调
 * @param user_data 用户数据
 * @return 字符宽度（像素）
 */
int16_t lv_draw_char(const lv_font_t *font, int16_t x, int16_t y, uint32_t unicode,
                     lv_draw_pixel_cb_t draw_pixel, void *user_data);

/**
 * @brief 绘制文本（简单模式，无窗口限制）
 * @param font 字体指针
 * @param x X坐标
 * @param y Y坐标
 * @param text UTF-8编码的文本
 * @param letter_spacing 字符间距
 * @param draw_pixel 像素绘制回调
 * @param user_data 用户数据
 */
void lv_draw_text_simple(const lv_font_t *font, int16_t x, int16_t y, const char *text,
                         uint8_t letter_spacing, lv_draw_pixel_cb_t draw_pixel, void *user_data);

/**
 * @brief 绘制文本（高级模式，支持窗口、对齐、换行）
 * @param font 字体指针
 * @param text UTF-8编码的文本
 * @param config 渲染配置
 * @param draw_pixel 像素绘制回调
 * @param user_data 用户数据
 */
void lv_draw_text_advanced(const lv_font_t *font, const char *text,
                           const lv_text_config_t *config,
                           lv_draw_pixel_cb_t draw_pixel, void *user_data);

/**
 * @brief UTF-8解码
 * @param text UTF-8文本指针
 * @param unicode 输出Unicode字符
 * @return 消耗的字节数
 */
uint8_t lv_utf8_decode(const char *text, uint32_t *unicode);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_FONT_RENDER_H */

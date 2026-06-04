#include "freetyperenderer.h"
#include <QFile>
#include <QDebug>
#include <QtMath>
#include <hb.h>
#include <hb-ft.h>

FreeTypeRenderer::FreeTypeRenderer()
    : m_library(nullptr)
    , m_face(nullptr)
    , m_hb_font(nullptr)
    , m_initialized(false)
    , m_antialiasing(true)  // 默认启用抗锯齿
{
}

FreeTypeRenderer::~FreeTypeRenderer()
{
    cleanup();
}

bool FreeTypeRenderer::loadFont(const QString &fontPath, int fontSize)
{
    cleanup();

    // 初始化 FreeType 库
    FT_Error error = FT_Init_FreeType(&m_library);
    if (error) {
        m_lastError = QString("Failed to initialize FreeType library: error %1").arg(error);
        return false;
    }

    // 读取字体文件到内存
    QFile file(fontPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open font file: " + fontPath;
        FT_Done_FreeType(m_library);
        m_library = nullptr;
        return false;
    }

    m_fontData = file.readAll();
    file.close();

    if (m_fontData.isEmpty()) {
        m_lastError = "Font file is empty";
        FT_Done_FreeType(m_library);
        m_library = nullptr;
        return false;
    }

    // 从内存加载字体
    error = FT_New_Memory_Face(
        m_library,
        reinterpret_cast<const FT_Byte*>(m_fontData.constData()),
        m_fontData.size(),
        0,
        &m_face
    );

    if (error) {
        m_lastError = QString("Failed to load font face: error %1").arg(error);
        FT_Done_FreeType(m_library);
        m_library = nullptr;
        return false;
    }

    // 设置字符大小（与官方工具一致）
    // 官方工具使用 FT_Set_Char_Size 设置 300 DPI，然后再用 FT_Set_Pixel_Sizes
    // freetype/index.js 第101-107行
    error = FT_Set_Char_Size(m_face, 0, fontSize * 64, 300, 300);
    if (error) {
        m_lastError = QString("Failed to set char size: error %1").arg(error);
        cleanup();
        return false;
    }

    error = FT_Set_Pixel_Sizes(m_face, 0, fontSize);
    if (error) {
        m_lastError = QString("Failed to set pixel sizes: error %1").arg(error);
        cleanup();
        return false;
    }

    m_initialized = true;

    // 创建 HarfBuzz font
    m_hb_font = hb_ft_font_create(m_face, NULL);
    if (!m_hb_font) {
        qWarning() << "Failed to create HarfBuzz font, kerning preview may not work correctly";
    }

    qDebug() << "FreeType font loaded successfully:";
    qDebug() << "  units_per_EM:" << m_face->units_per_EM;
    qDebug() << "  ascender:" << m_face->ascender;
    qDebug() << "  descender:" << m_face->descender;
    qDebug() << "  height:" << m_face->height;
    qDebug() << "  num_fixed_sizes:" << m_face->num_fixed_sizes;

    // 显示缩放后的度量值（26.6 格式，除以 64 得到像素）
    qDebug() << "  size->metrics (scaled, in 1/64 pixels):";
    qDebug() << "    ascender:" << m_face->size->metrics.ascender << "(" << (m_face->size->metrics.ascender / 64.0) << "px)";
    qDebug() << "    descender:" << m_face->size->metrics.descender << "(" << (m_face->size->metrics.descender / 64.0) << "px)";
    qDebug() << "    height:" << m_face->size->metrics.height << "(" << (m_face->size->metrics.height / 64.0) << "px)";

    // 显示可用的内嵌位图尺寸
    if (m_face->num_fixed_sizes > 0) {
        qDebug() << "  Available embedded bitmap sizes (ppem):";
        for (int i = 0; i < m_face->num_fixed_sizes; i++) {
            qDebug() << "    Strike" << i << ":"
                     << m_face->available_sizes[i].width << "x"
                     << m_face->available_sizes[i].height;
        }
    }

    return true;
}

bool FreeTypeRenderer::renderGlyph(uint32_t charCode, GlyphData &outGlyph)
{
    if (!m_initialized || !m_face) {
        m_lastError = "FreeType not initialized";
        return false;
    }

    // 获取字形索引
    FT_UInt glyph_index = FT_Get_Char_Index(m_face, charCode);
    if (glyph_index == 0) {
        m_lastError = QString("Glyph does not exist for character U+%1")
                          .arg(charCode, 4, 16, QChar('0'));
        return false;
    }

    // 加载字形 - 根据抗锯齿设置选择渲染模式
    FT_Int32 load_flags;
    if (m_antialiasing) {
        // 使用抗锯齿（与官方工具一致）
        load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT;
    } else {
        // 单色渲染（无抗锯齿，适合 BPP 1）
        load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME;
    }

    FT_Error error = FT_Load_Glyph(m_face, glyph_index, load_flags);
    if (error) {
        m_lastError = QString("Failed to load glyph: error %1").arg(error);
        return false;
    }

    FT_GlyphSlot slot = m_face->glyph;

    // 检查渲染模式
    if (charCode == '!') {
        qDebug() << "Glyph '!' render info:";
        qDebug() << "  bitmap.pixel_mode:" << slot->bitmap.pixel_mode;
        qDebug() << "  bitmap.width:" << slot->bitmap.width;
        qDebug() << "  bitmap.rows:" << slot->bitmap.rows;
        qDebug() << "  bitmap.pitch:" << slot->bitmap.pitch;
        qDebug() << "  bitmap_left:" << slot->bitmap_left;
        qDebug() << "  bitmap_top:" << slot->bitmap_top;
    }

    // 提取字形数据（与官方工具一致）
    outGlyph.width = slot->bitmap.width;
    outGlyph.height = slot->bitmap.rows;
    outGlyph.bitmap_left = slot->bitmap_left;
    outGlyph.bitmap_top = slot->bitmap_top;

    // 使用 linearHoriAdvance（16.16 固定点格式，需要除以 65536）
    // 官方工具 freetype/index.js 第245行使用 linearHoriAdvance
    outGlyph.advance_x = slot->linearHoriAdvance / 65536.0;
    outGlyph.advance_y = slot->linearVertAdvance / 65536.0;

    // 提取位图数据
    outGlyph.pixels.clear();
    outGlyph.pixels.resize(outGlyph.height);

    for (int y = 0; y < outGlyph.height; y++) {
        outGlyph.pixels[y].resize(outGlyph.width);

        for (int x = 0; x < outGlyph.width; x++) {
            unsigned char value;
            int pitch = abs(slot->bitmap.pitch);

            // 根据位图格式提取像素值
            if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                // 单色位图：每个像素占1位
                int byte_index = x / 8;
                int bit_index = 7 - (x % 8);
                unsigned char byte_value = slot->bitmap.buffer[y * pitch + byte_index];
                value = (byte_value & (1 << bit_index)) ? 255 : 0;
            } else if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
                // 灰度位图：每个像素占1字节
                value = slot->bitmap.buffer[y * pitch + x];
            } else {
                // 其他格式，默认为0
                value = 0;
            }

            outGlyph.pixels[y][x] = value;
        }
    }

    return true;
}

void FreeTypeRenderer::cleanup()
{
    if (m_hb_font) {
        hb_font_destroy(m_hb_font);
        m_hb_font = nullptr;
    }

    if (m_face) {
        FT_Done_Face(m_face);
        m_face = nullptr;
    }

    if (m_library) {
        FT_Done_FreeType(m_library);
        m_library = nullptr;
    }

    m_fontData.clear();
    m_kerningCache.clear();
    m_initialized = false;
}

bool FreeTypeRenderer::hasKerning() const
{
    if (!m_initialized || !m_face) {
        return false;
    }

    // 检查字体是否包含kerning信息
    return FT_HAS_KERNING(m_face);
}

int FreeTypeRenderer::getKerning(uint32_t leftChar, uint32_t rightChar)
{
    if (!m_initialized || !m_face) {
        return 0;
    }

    // 检查缓存
    QPair<uint32_t, uint32_t> key(leftChar, rightChar);
    if (m_kerningCache.contains(key)) {
        return m_kerningCache[key];
    }

    int result = 0;

    // 优先使用 HarfBuzz（支持 GPOS）
    if (m_hb_font) {
        result = getKerningHarfBuzz(leftChar, rightChar);
    }

    // 如果 HarfBuzz 失败或返回 0，尝试 FreeType
    if (result == 0) {
        result = getKerningFreeType(leftChar, rightChar);
    }

    // 缓存结果
    m_kerningCache[key] = result;

    return result;
}

int FreeTypeRenderer::getKerningHarfBuzz(uint32_t leftChar, uint32_t rightChar)
{
    if (!m_hb_font) {
        return 0;
    }

    // 创建 buffer 并添加字符对
    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_add_utf32(buffer, &leftChar, 1, 0, 1);
    hb_buffer_add_utf32(buffer, &rightChar, 1, 0, 1);
    hb_buffer_guess_segment_properties(buffer);

    // 进行 shaping（应用 GPOS 等特性）
    hb_shape(m_hb_font, buffer, NULL, 0);

    // 获取位置信息
    unsigned int glyph_count;
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_count);

    int result = 0;
    if (glyph_count >= 2) {
        // 第一个字符的 x_advance 包含了 kerning
        hb_position_t advance_with_kern = glyph_pos[0].x_advance;

        // 获取标准 advance（无 kerning）
        hb_codepoint_t glyph_id;
        if (hb_font_get_nominal_glyph(m_hb_font, leftChar, &glyph_id)) {
            hb_position_t standard_advance = hb_font_get_glyph_h_advance(m_hb_font, glyph_id);

            // 计算 kerning 值（26.6 格式）
            hb_position_t kern_x = advance_with_kern - standard_advance;

            if (kern_x != 0) {
                // 转换为 1/16 像素
                // HarfBuzz 返回 26.6 格式，除以 64 得到像素，再乘以 16
                double kernPixels = static_cast<double>(kern_x) / 64.0;
                result = qRound(kernPixels * 16);
            }
        }
    }

    hb_buffer_destroy(buffer);
    return result;
}

int FreeTypeRenderer::getKerningFreeType(uint32_t leftChar, uint32_t rightChar)
{
    // 获取字形索引
    FT_UInt left_glyph = FT_Get_Char_Index(m_face, leftChar);
    FT_UInt right_glyph = FT_Get_Char_Index(m_face, rightChar);

    if (left_glyph == 0 || right_glyph == 0) {
        return 0;
    }

    // 获取kerning值
    FT_Vector delta;
    FT_Error error = FT_Get_Kerning(m_face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &delta);

    if (error) {
        return 0;
    }

    // delta.x 是以1/64像素为单位的
    // LVGL的kern_scale=16，表示kerning值以1/16像素为单位
    // 转换公式: (delta.x / 64) * 16 = delta.x / 4
    // 四舍五入: (delta.x + 2) / 4
    int result = (delta.x + 2) / 4;

    return result;
}

int FreeTypeRenderer::getUnderlinePosition() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    return m_face->underline_position;
}

int FreeTypeRenderer::getUnderlineThickness() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    return m_face->underline_thickness;
}

int FreeTypeRenderer::getUnitsPerEM() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    return m_face->units_per_EM;
}

int FreeTypeRenderer::getAscender() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    return m_face->ascender;
}

int FreeTypeRenderer::getDescender() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    return m_face->descender;
}

int FreeTypeRenderer::getHeight() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    return m_face->height;
}

int FreeTypeRenderer::getScaledAscender() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    // size->metrics 的值是 26.6 格式（1/64 像素）
    // 使用标准的四舍五入：round(value / 64.0)
    return qRound(m_face->size->metrics.ascender / 64.0);
}

int FreeTypeRenderer::getScaledDescender() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    return qRound(m_face->size->metrics.descender / 64.0);
}

int FreeTypeRenderer::getScaledHeight() const
{
    if (!m_initialized || !m_face) {
        return 0;
    }
    return qRound(m_face->size->metrics.height / 64.0);
}

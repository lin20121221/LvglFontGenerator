#include "freetyperenderer.h"
#include <QFile>
#include <QDebug>

FreeTypeRenderer::FreeTypeRenderer()
    : m_library(nullptr)
    , m_face(nullptr)
    , m_initialized(false)
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

    // 设置字符大小（参照官方工具：FT_Set_Char_Size）
    // size * 64 是因为 FreeType 使用 26.6 固定点格式
    error = FT_Set_Char_Size(m_face, 0, fontSize * 64, 300, 300);
    if (error) {
        m_lastError = QString("Failed to set char size: error %1").arg(error);
        cleanup();
        return false;
    }

    // 设置像素大小（参照官方工具：FT_Set_Pixel_Sizes）
    error = FT_Set_Pixel_Sizes(m_face, 0, fontSize);
    if (error) {
        m_lastError = QString("Failed to set pixel sizes: error %1").arg(error);
        cleanup();
        return false;
    }

    m_initialized = true;

    qDebug() << "FreeType font loaded successfully:";
    qDebug() << "  units_per_EM:" << m_face->units_per_EM;
    qDebug() << "  ascender:" << m_face->ascender;
    qDebug() << "  descender:" << m_face->descender;
    qDebug() << "  height:" << m_face->height;

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

    // 加载字形（参照官方工具的 load_flags）
    // FT_LOAD_RENDER: 直接渲染为位图
    // FT_LOAD_TARGET_LIGHT: 使用轻度 hinting（官方工具默认）
    // FT_LOAD_FORCE_AUTOHINT: 强制使用自动 hinting
    FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT;

    FT_Error error = FT_Load_Glyph(m_face, glyph_index, load_flags);
    if (error) {
        m_lastError = QString("Failed to load glyph: error %1").arg(error);
        return false;
    }

    FT_GlyphSlot slot = m_face->glyph;

    // 提取字形数据（参照官方工具 freetype/index.js）
    outGlyph.width = slot->bitmap.width;
    outGlyph.height = slot->bitmap.rows;
    outGlyph.bitmap_left = slot->bitmap_left;
    outGlyph.bitmap_top = slot->bitmap_top;

    // 使用 linearHoriAdvance（16.16 固定点格式，需要除以 65536）
    outGlyph.advance_x = slot->linearHoriAdvance / 65536.0;
    outGlyph.advance_y = slot->linearVertAdvance / 65536.0;

    // 提取位图数据
    outGlyph.pixels.clear();
    outGlyph.pixels.resize(outGlyph.height);

    for (int y = 0; y < outGlyph.height; y++) {
        outGlyph.pixels[y].resize(outGlyph.width);

        for (int x = 0; x < outGlyph.width; x++) {
            // FreeType 的位图数据按行存储
            int pitch = abs(slot->bitmap.pitch);
            unsigned char value = slot->bitmap.buffer[y * pitch + x];
            outGlyph.pixels[y][x] = value;
        }
    }

    return true;
}

void FreeTypeRenderer::cleanup()
{
    if (m_face) {
        FT_Done_Face(m_face);
        m_face = nullptr;
    }

    if (m_library) {
        FT_Done_FreeType(m_library);
        m_library = nullptr;
    }

    m_fontData.clear();
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

    // 获取字形索引
    FT_UInt left_glyph = FT_Get_Char_Index(m_face, leftChar);
    FT_UInt right_glyph = FT_Get_Char_Index(m_face, rightChar);

    if (left_glyph == 0 || right_glyph == 0) {
        return 0;
    }

    // 获取kerning值
    // 注意：FT_Get_Kerning 可以从 GPOS 表（OpenType）或传统 kern 表中提取 kerning
    // 不需要先检查 FT_HAS_KERNING（它只检查传统 kern 表）
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

    // 调试输出（仅对A-V字符对）
    if ((leftChar == 'A' && rightChar == 'V') || (leftChar == 'T' && rightChar == 'o')) {
        qDebug() << "Kerning for" << QChar(leftChar) << "-" << QChar(rightChar) << ":"
                 << "delta.x=" << delta.x << "(1/64px),"
                 << "result=" << result << "(1/16px)";
    }

    return result;
}

#include "harfbuzzkerning.h"
#include <QFile>
#include <QDebug>
#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

HarfBuzzKerning::HarfBuzzKerning()
{
}

HarfBuzzKerning::~HarfBuzzKerning()
{
}

bool HarfBuzzKerning::extractKerning(const QString &fontPath, int fontSize, const QVector<uint32_t> &characters)
{
    m_kerningPairs.clear();
    m_lastError.clear();

    // 读取字体文件
    QFile file(fontPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open font file: " + fontPath;
        return false;
    }

    QByteArray fontData = file.readAll();
    file.close();

    if (fontData.isEmpty()) {
        m_lastError = "Font file is empty";
        return false;
    }

    // 初始化 FreeType
    FT_Library library;
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        m_lastError = QString("Failed to initialize FreeType: error %1").arg(error);
        return false;
    }

    // 加载字体
    FT_Face face;
    error = FT_New_Memory_Face(
        library,
        reinterpret_cast<const FT_Byte*>(fontData.constData()),
        fontData.size(),
        0,
        &face
    );

    if (error) {
        m_lastError = QString("Failed to load font face: error %1").arg(error);
        FT_Done_FreeType(library);
        return false;
    }

    // 设置字符大小（使用像素大小，与 CLI 工具一致）
    error = FT_Set_Pixel_Sizes(face, 0, fontSize);
    if (error) {
        m_lastError = QString("Failed to set pixel sizes: error %1").arg(error);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    int unitsPerEM = face->units_per_EM;

    qDebug() << "HarfBuzz kerning extraction:";
    qDebug() << "  Font:" << fontPath;
    qDebug() << "  Size:" << fontSize << "px";
    qDebug() << "  Units per EM:" << unitsPerEM;
    qDebug() << "  Characters:" << characters.size();

    // 创建 HarfBuzz font
    hb_font_t *hb_font = hb_ft_font_create(face, NULL);
    if (!hb_font) {
        m_lastError = "Failed to create HarfBuzz font";
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    // 创建字符到索引的映射
    QMap<uint32_t, int> charToIndex;
    QMap<uint32_t, hb_codepoint_t> charToGlyph;

    for (int i = 0; i < characters.size(); i++) {
        uint32_t unicode = characters[i];
        charToIndex[unicode] = i;

        // 获取 glyph index
        hb_codepoint_t glyph;
        if (hb_font_get_nominal_glyph(hb_font, unicode, &glyph)) {
            charToGlyph[unicode] = glyph;
        }
    }

    // 提取 kerning 对
    // 使用 HarfBuzz shaping 来获取 GPOS kerning
    int pairCount = 0;
    for (int i = 0; i < characters.size(); i++) {
        uint32_t leftChar = characters[i];
        if (!charToGlyph.contains(leftChar)) continue;

        for (int j = 0; j < characters.size(); j++) {
            uint32_t rightChar = characters[j];
            if (!charToGlyph.contains(rightChar)) continue;

            // 创建 buffer 并添加字符对
            hb_buffer_t *buffer = hb_buffer_create();
            hb_buffer_add_utf32(buffer, &leftChar, 1, 0, 1);
            hb_buffer_add_utf32(buffer, &rightChar, 1, 0, 1);
            hb_buffer_guess_segment_properties(buffer);

            // 进行 shaping（应用 GPOS 等特性）
            hb_shape(hb_font, buffer, NULL, 0);

            // 获取位置信息
            unsigned int glyph_count;
            hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_count);

            if (glyph_count >= 2) {
                // 第一个字符的 x_advance 包含了 kerning
                hb_position_t advance_with_kern = glyph_pos[0].x_advance;

                // 获取标准 advance（无 kerning）
                hb_position_t standard_advance = hb_font_get_glyph_h_advance(hb_font, charToGlyph[leftChar]);

                // 计算 kerning 值
                hb_position_t kern_x = advance_with_kern - standard_advance;

                if (kern_x != 0) {
                    // HarfBuzz 返回的值已经是缩放后的值（26.6 格式）
                    // 转换为像素
                    double kernPixels = static_cast<double>(kern_x) / 64.0;

                    // 转换为 LVGL 格式（1/16 像素）
                    int kernLVGL = qRound(kernPixels * 16);

                    // 限制在 signed 8-bit 范围 [-128, 127]
                    kernLVGL = qMax(-128, qMin(127, kernLVGL));

                    if (kernLVGL != 0) {
                        KerningPair pair;
                        pair.leftIndex = i;
                        pair.rightIndex = j;
                        pair.valueLVGL = kernLVGL;
                        m_kerningPairs.append(pair);
                        pairCount++;

                        // 调试输出前几个
                        if (pairCount <= 10) {
                            qDebug() << "  " << QChar(leftChar) << "-" << QChar(rightChar) << ":"
                                     << kern_x << "(26.6 format) ="
                                     << QString::number(kernPixels, 'f', 2) << "px ="
                                     << kernLVGL << "(1/16 px)";
                        }
                    }
                }
            }

            hb_buffer_destroy(buffer);
        }
    }

    qDebug() << "Extracted" << pairCount << "non-zero kerning pairs";

    // 清理
    hb_font_destroy(hb_font);
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return true;
}

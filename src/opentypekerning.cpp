#include "opentypekerning.h"
#include <QFile>
#include <QDebug>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

OpenTypeKerning::OpenTypeKerning()
{
}

OpenTypeKerning::~OpenTypeKerning()
{
}

bool OpenTypeKerning::extractKerning(const QString &fontPath, int fontSize, const QVector<uint32_t> &characters)
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

    // 设置字符大小
    error = FT_Set_Char_Size(face, 0, fontSize * 64, 300, 300);
    if (error) {
        m_lastError = QString("Failed to set char size: error %1").arg(error);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    error = FT_Set_Pixel_Sizes(face, 0, fontSize);
    if (error) {
        m_lastError = QString("Failed to set pixel sizes: error %1").arg(error);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    int unitsPerEM = face->units_per_EM;

    qDebug() << "OpenType kerning extraction:";
    qDebug() << "  Font:" << fontPath;
    qDebug() << "  Size:" << fontSize << "px";
    qDebug() << "  Units per EM:" << unitsPerEM;
    qDebug() << "  Characters:" << characters.size();

    // 创建字符到索引的映射
    QMap<uint32_t, int> charToIndex;
    for (int i = 0; i < characters.size(); i++) {
        charToIndex[characters[i]] = i;
    }

    // 提取 kerning 对
    // FT_Get_Kerning 可以从 GPOS 表或传统 kern 表中提取
    int pairCount = 0;
    for (int i = 0; i < characters.size(); i++) {
        uint32_t leftChar = characters[i];
        FT_UInt leftGlyph = FT_Get_Char_Index(face, leftChar);

        if (leftGlyph == 0) continue;

        for (int j = 0; j < characters.size(); j++) {
            uint32_t rightChar = characters[j];
            FT_UInt rightGlyph = FT_Get_Char_Index(face, rightChar);

            if (rightGlyph == 0) continue;

            // 获取 kerning 值（字体单位）
            FT_Vector delta;
            error = FT_Get_Kerning(face, leftGlyph, rightGlyph, FT_KERNING_UNSCALED, &delta);

            if (error || delta.x == 0) {
                continue;
            }

            // 转换为像素
            double kernPixels = static_cast<double>(delta.x) * fontSize / unitsPerEM;

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
                             << delta.x << "font units ="
                             << QString::number(kernPixels, 'f', 2) << "px ="
                             << kernLVGL << "(1/16 px)";
                }
            }
        }
    }

    qDebug() << "Extracted" << pairCount << "non-zero kerning pairs";

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return true;
}

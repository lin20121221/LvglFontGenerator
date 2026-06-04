#include "gposkerning.h"
#include "gpos_reader.h"
#include <QFile>
#include <QDebug>
#include <ft2build.h>
#include FT_FREETYPE_H

GPOSKerning::GPOSKerning()
{
}

GPOSKerning::~GPOSKerning()
{
}

bool GPOSKerning::extractKerning(const QString &fontPath, int fontSize, const QVector<uint32_t> &characters)
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

    qDebug() << "GPOS kerning extraction:";
    qDebug() << "  Font:" << fontPath;
    qDebug() << "  Size:" << fontSize << "px";
    qDebug() << "  Characters:" << characters.size();

    // 转换 QVector 到 std::vector
    std::vector<uint32_t> codes;
    for (uint32_t c : characters) {
        codes.push_back(c);
    }

    // 使用 GPOS Reader 提取 kerning（与 CLI 工具完全一致）
    std::map<uint32_t, std::map<uint32_t, int8_t>> kerning_map;
    if (!lvgl::GPOSReader::extract_kerning(face, codes, kerning_map)) {
        m_lastError = "Failed to extract GPOS kerning data";
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    // 创建字符索引映射
    QMap<uint32_t, int> charToIndex;
    for (int i = 0; i < characters.size(); i++) {
        charToIndex[characters[i]] = i;
    }

    // 转换 kerning_map 到 QVector<KerningPair>
    int pairCount = 0;
    for (const auto& left_entry : kerning_map) {
        uint32_t leftChar = left_entry.first;
        if (!charToIndex.contains(leftChar)) continue;

        for (const auto& right_entry : left_entry.second) {
            uint32_t rightChar = right_entry.first;
            int8_t kernValue = right_entry.second;

            if (!charToIndex.contains(rightChar)) continue;

            KerningPair pair;
            pair.leftIndex = charToIndex[leftChar];
            pair.rightIndex = charToIndex[rightChar];
            pair.valueLVGL = kernValue;
            m_kerningPairs.append(pair);
            pairCount++;

            // 调试输出前几个
            if (pairCount <= 10) {
                qDebug() << "  " << QChar(leftChar) << "-" << QChar(rightChar) << ":"
                         << kernValue << "(FP4.4 format)";
            }
        }
    }

    qDebug() << "Extracted" << pairCount << "non-zero kerning pairs using GPOS";

    // 清理
    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return true;
}

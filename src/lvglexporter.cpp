#include "lvglexporter.h"
#include "freetyperenderer.h"
#include "kerningoptimizer.h"
#include "opentypekerning.h"
#include "harfbuzzkerning.h"
#include "gposkerning.h"
#include "cmapoptimizer.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

LvglExporter::LvglExporter()
    : m_lvglVersion(9)
    , m_isExternal(false)
    , m_bpp(8)
    , m_enableKerning(false)
    , m_hasKerningData(false)
    , m_cmapCount(0)
    , m_fontSize(16)
{
}

void LvglExporter::setConfig(int lvglVersion, bool isExternal, int bpp, bool enableKerning)
{
    m_lvglVersion = lvglVersion;
    m_isExternal = isExternal;
    m_bpp = bpp;
    m_enableKerning = enableKerning;
}

void LvglExporter::setFontName(const QString &name)
{
    m_fontName = name;
}

void LvglExporter::setFontSize(int size)
{
    m_fontSize = size;
}

void LvglExporter::setFontPath(const QString &fontPath, int fontSize)
{
    m_fontPath = fontPath;
    m_fontSize = fontSize;
}

void LvglExporter::addGlyph(const GlyphInfo &glyph)
{
    m_glyphs.append(glyph);
}

bool LvglExporter::exportCFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << generateCFileContent();

    file.close();
    return true;
}

bool LvglExporter::exportBinFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QByteArray data = generateBinaryData();
    file.write(data);
    file.close();

    return true;
}

QString LvglExporter::generateCFileContent()
{
    // m_glyphs 已经在 fontgenerator.cpp 中排序好了，直接使用
    QString content;
    QTextStream out(&content);

    out << "/*******************************************************************************\n";
    out << " * Size: " << m_fontSize << " px\n";
    out << " * Bpp: " << m_bpp << "\n";
    out << " * Opts: --no-compress --no-prefilter --bpp " << m_bpp << " --size " << m_fontSize << "\n";
    out << " * Compatible with LVGL 8.x and 9.x\n";
    out << " ******************************************************************************/\n\n";

    out << "#ifdef __has_include\n";
    out << "    #if __has_include(\"lvgl.h\")\n";
    out << "        #ifndef LV_LVGL_H_INCLUDE_SIMPLE\n";
    out << "            #define LV_LVGL_H_INCLUDE_SIMPLE\n";
    out << "        #endif\n";
    out << "    #endif\n";
    out << "#endif\n\n";

    out << "#ifdef LV_LVGL_H_INCLUDE_SIMPLE\n";
    out << "    #include \"lvgl.h\"\n";
    out << "#else\n";
    out << "    #include \"lvgl/lvgl.h\"\n";
    out << "#endif\n\n";

    // 添加宏保护
    QString fontMacro = m_fontName.toUpper();
    out << "\n\n#ifndef " << fontMacro << "\n";
    out << "#define " << fontMacro << " 1\n";
    out << "#endif\n\n";
    out << "#if " << fontMacro << "\n\n";

    // 生成 bitmaps 部分
    out << "/*-----------------\n";
    out << " *    BITMAPS\n";
    out << " *----------------*/\n\n";

    if (!m_isExternal) {
        out << generateBitmapArray();
    }

    out << generateGlyphDescArray();
    out << generateCmapTables();

    if (m_enableKerning) {
        out << generateKernTables();
    }

    out << generateFontStruct();

    if (m_isExternal) {
        out << "\n/*External font data getter function, needs user implementation*/\n";
        out << "extern const uint8_t *user_font_get_data(uint32_t offset, uint32_t size);\n";
    }

    // 结束宏保护
    out << "\n\n#endif /*#if " << fontMacro << "*/\n";

    return content;
}

QString LvglExporter::generateBitmapArray()
{
    QString result;
    QTextStream out(&result);

    out << "/*Store the image of the glyphs*/\n";
    out << "static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {\n";

    int offset = 0;
    for (int glyphIdx = 0; glyphIdx < m_glyphs.size(); glyphIdx++) {
        const GlyphInfo &glyph = m_glyphs[glyphIdx];
        out << "    /* U+" << QString::number(glyph.unicode, 16).toUpper().rightJustified(4, '0') << " \""
            << QChar(glyph.unicode) << "\" */\n";

        const QImage &img = glyph.bitmap;

        // 生成位图数据（按官方工具格式：逐行存储，大端序打包）
        QVector<uint8_t> bitmapData = generateGlyphBitmap(img);

        // 输出位图数据
        if (bitmapData.isEmpty()) {
            // 空字符（如空格）：只输出空行
            out << "\n";
        } else {
            for (int i = 0; i < bitmapData.size(); i++) {
                if (i % 8 == 0) {
                    out << "   ";
                }

                out << " 0x" << QString::number(bitmapData[i], 16) << ",";

                if ((i + 1) % 8 == 0) {
                    out << "\n";
                }
            }

            if (bitmapData.size() % 8 != 0) {
                out << "\n";
            }
        }

        out << "\n";

        offset += bitmapData.size();
    }

    out << "};\n\n";
    return result;
}

// 生成字形位图数据（匹配官方lv_font_conv的格式）
// 官方工具使用BitStream连续打包像素，不在行边界对齐字节
QVector<uint8_t> LvglExporter::generateGlyphBitmap(const QImage &img)
{
    QVector<uint8_t> result;

    if (img.width() == 0 || img.height() == 0) {
        return result;
    }

    int totalPixels = img.width() * img.height();

    // 根据bpp处理像素数据（连续打包，模拟BitStream行为）
    if (m_bpp == 1) {
        // 1bpp: 每字节存储8个像素，每像素1位(0-1)
        int pixelIndex = 0;
        while (pixelIndex < totalPixels) {
            uint8_t packed = 0;
            for (int bit = 0; bit < 8 && pixelIndex < totalPixels; bit++, pixelIndex++) {
                int y = pixelIndex / img.width();
                int x = pixelIndex % img.width();
                int gray = qGray(img.pixel(x, y));
                int pixel = (gray > 127) ? 1 : 0;
                packed |= (pixel << (7 - bit));
            }
            result.append(packed);
        }
    } else if (m_bpp == 2) {
        // 2bpp: 每字节存储4个像素，每像素2位(0-3)
        int pixelIndex = 0;
        while (pixelIndex < totalPixels) {
            uint8_t packed = 0;
            for (int i = 0; i < 4 && pixelIndex < totalPixels; i++, pixelIndex++) {
                int y = pixelIndex / img.width();
                int x = pixelIndex % img.width();
                int gray = qGray(img.pixel(x, y));
                int pixel = gray >> 6;  // 转换为2位值 (0-3)
                packed |= (pixel << (6 - i * 2));
            }
            result.append(packed);
        }
    } else if (m_bpp == 4) {
        // 4bpp: 每字节存储2个像素，每像素4位(0-15)
        // 大端序：第一个像素在高4位，第二个像素在低4位
        // 连续打包所有像素（不在行边界对齐）
        int pixelIndex = 0;
        while (pixelIndex < totalPixels) {
            int y = pixelIndex / img.width();
            int x = pixelIndex % img.width();
            int gray1 = qGray(img.pixel(x, y));
            int pixel1 = gray1 >> 4;  // 转换为4位值 (0-15)
            pixelIndex++;

            int pixel2 = 0;
            if (pixelIndex < totalPixels) {
                y = pixelIndex / img.width();
                x = pixelIndex % img.width();
                int gray2 = qGray(img.pixel(x, y));
                pixel2 = gray2 >> 4;
                pixelIndex++;
            }

            // 打包：第一个像素在高4位，第二个像素在低4位
            uint8_t packed = (pixel1 << 4) | pixel2;
            result.append(packed);
        }
    } else {
        // 8bpp: 每字节存储1个像素，每像素8位(0-255)
        for (int y = 0; y < img.height(); y++) {
            for (int x = 0; x < img.width(); x++) {
                int gray = qGray(img.pixel(x, y));
                result.append(static_cast<uint8_t>(gray));
            }
        }
    }

    return result;
}

QString LvglExporter::generateGlyphDescArray()
{
    QString result;
    QTextStream out(&result);

    out << "/*Glyph description*/\n";
    out << "static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {\n";

    // 添加id=0的保留元素
    out << "    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,\n";

    int bitmapOffset = 0;
    for (const GlyphInfo &glyph : m_glyphs) {
        // advWidth已经是以1/16像素为单位（在fontgenerator.cpp中已经乘以16）
        int advWidthScaled = glyph.advWidth;

        out << "    {.bitmap_index = " << bitmapOffset
            << ", .adv_w = " << advWidthScaled
            << ", .box_w = " << glyph.boxW
            << ", .box_h = " << glyph.boxH
            << ", .ofs_x = " << glyph.ofsX
            << ", .ofs_y = " << glyph.ofsY << "},\n";

        // 使用实际生成的位图大小计算偏移量
        QVector<uint8_t> bitmapData = generateGlyphBitmap(glyph.bitmap);
        bitmapOffset += bitmapData.size();
    }

    out << "};\n\n";
    return result;
}

QString LvglExporter::generateCmapTables()
{
    QString result;
    QTextStream out(&result);

    if (m_glyphs.isEmpty()) {
        m_cmapCount = 0;
        return result;
    }

    out << "/*---------------------\n";
    out << " *  CHARACTER MAPPING\n";
    out << " *--------------------*/\n\n";

    // 提取所有Unicode码点
    QVector<uint32_t> codepoints;
    for (const GlyphInfo &glyph : m_glyphs) {
        codepoints.append(glyph.unicode);
    }

    // 使用动态规划优化分割
    QVector<CmapOptimizer::SubTable> subTables = CmapOptimizer::optimize(codepoints);

    // 生成 unicode_list 和 glyph_id_ofs_list
    int subTableIdx = 0;
    for (const auto &sub : subTables) {
        if (sub.format == CmapOptimizer::SPARSE_TINY) {
            // 生成 unicode_list（相对于 range_start 的偏移）
            out << "static const uint16_t unicode_list_" << subTableIdx << "[] = {\n    ";
            for (int i = 0; i < sub.codepoints.size(); i++) {
                uint16_t offset = sub.codepoints[i] - sub.rangeStart;
                out << "0x" << QString::number(offset, 16);
                if (i < sub.codepoints.size() - 1) {
                    out << ", ";
                    if ((i + 1) % 12 == 0) out << "\n    ";
                }
            }
            out << "\n};\n\n";
        } else if (sub.format == CmapOptimizer::FORMAT0) {
            // 生成 glyph_id_ofs_list（需要构建完整范围的ID偏移数组）
            out << "static const uint8_t glyph_id_ofs_list_" << subTableIdx << "[] = {\n    ";

            int glyphIdxInSub = 0;
            for (uint32_t code = sub.rangeStart; code <= sub.rangeEnd; code++) {
                uint8_t idOffset = 0;

                // 检查该码点是否存在
                if (glyphIdxInSub < sub.codepoints.size() &&
                    sub.codepoints[glyphIdxInSub] == code) {
                    idOffset = glyphIdxInSub;
                    glyphIdxInSub++;
                }

                out << QString::number(idOffset);
                if (code < sub.rangeEnd) {
                    out << ", ";
                    if ((code - sub.rangeStart + 1) % 16 == 0) out << "\n    ";
                }
            }
            out << "\n};\n\n";
        }
        // FORMAT0_TINY 不需要数据数组

        subTableIdx++;
    }

    // 生成 cmaps 数组
    m_cmapCount = subTables.size();
    out << "/*Collect the unicode lists and glyph_id offsets*/\n";
    out << "static const lv_font_fmt_txt_cmap_t cmaps[] =\n{\n";

    for (int i = 0; i < subTables.size(); i++) {
        const auto &sub = subTables[i];
        uint32_t rangeLength = sub.rangeEnd - sub.rangeStart + 1;

        out << "    {\n";
        out << "        .range_start = " << sub.rangeStart << ", ";
        out << ".range_length = " << rangeLength << ", ";
        out << ".glyph_id_start = " << sub.glyphIdStart << ",\n";
        out << "        .unicode_list = ";

        if (sub.format == CmapOptimizer::FORMAT0_TINY) {
            out << "NULL, ";
        } else if (sub.format == CmapOptimizer::SPARSE_TINY) {
            out << "unicode_list_" << i << ", ";
        } else {  // FORMAT0
            out << "NULL, ";
        }

        out << ".glyph_id_ofs_list = ";
        if (sub.format == CmapOptimizer::FORMAT0) {
            out << "glyph_id_ofs_list_" << i << ", ";
        } else {
            out << "NULL, ";
        }

        // FORMAT0_TINY 的 list_length 应该为0（官方工具的行为）
        int listLength = (sub.format == CmapOptimizer::FORMAT0_TINY) ? 0 : sub.codepoints.size();
        out << ".list_length = " << listLength << ", ";
        out << ".type = ";

        switch (sub.format) {
            case CmapOptimizer::FORMAT0_TINY:
                out << "LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY";
                break;
            case CmapOptimizer::FORMAT0:
                out << "LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL";
                break;
            case CmapOptimizer::SPARSE_TINY:
                out << "LV_FONT_FMT_TXT_CMAP_SPARSE_TINY";
                break;
        }

        out << "\n    }";
        if (i < subTables.size() - 1) out << ",";
        out << "\n";
    }

    out << "};\n\n";

    return result;
}

QString LvglExporter::generateFontStruct()
{
    QString result;
    QTextStream out(&result);

    out << "/*Font descriptor*/\n";
    out << "#if LVGL_VERSION_MAJOR == 8\n";
    out << "static lv_font_fmt_txt_glyph_cache_t cache;\n";
    out << "#endif\n\n";

    out << "#if LVGL_VERSION_MAJOR >= 8\n";
    out << "static const lv_font_fmt_txt_dsc_t font_dsc = {\n";
    out << "#else\n";
    out << "static lv_font_fmt_txt_dsc_t font_dsc = {\n";
    out << "#endif\n";

    out << "    .glyph_bitmap = " << (m_isExternal ? "NULL" : "glyph_bitmap") << ",\n";
    out << "    .glyph_dsc = glyph_dsc,\n";
    out << "    .cmaps = cmaps,\n";
    out << "    .kern_dsc = " << (m_enableKerning && m_hasKerningData ? "&kern_classes" : "NULL") << ",\n";
    out << "    .kern_scale = " << (m_enableKerning && m_hasKerningData ? "16" : "0") << ",\n";
    out << "    .cmap_num = " << m_cmapCount << ",\n";
    out << "    .bpp = " << m_bpp << ",\n";
    out << "    .kern_classes = " << (m_enableKerning && m_hasKerningData ? "1" : "0") << ",\n";
    out << "    .bitmap_format = 0,\n";
    out << "#if LVGL_VERSION_MAJOR == 8\n";
    out << "    .cache = &cache\n";
    out << "#endif\n";
    out << "};\n\n";

    out << "/*Public font structure*/\n";
    out << "#if LVGL_VERSION_MAJOR >= 8\n";
    out << "const lv_font_t " << m_fontName << " = {\n";
    out << "#else\n";
    out << "lv_font_t " << m_fontName << " = {\n";
    out << "#endif\n";
    out << "    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,\n";
    out << "    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,\n";

    // 计算字体度量参数
    // line_height: 使用所有字形的实际最大高度
    // 官方工具计算方式：max(glyph.boxH + glyph.ofsY) - min(glyph.ofsY)
    int maxTop = 0;      // 最高点（box_h + ofs_y 的最大值）
    int minBottom = 0;   // 最低点（ofs_y 的最小值）

    for (const GlyphInfo &glyph : m_glyphs) {
        int top = glyph.boxH + glyph.ofsY;
        int bottom = glyph.ofsY;

        if (top > maxTop) {
            maxTop = top;
        }
        if (bottom < minBottom) {
            minBottom = bottom;
        }
    }

    int lineHeight = maxTop - minBottom;
    int baseLine = -minBottom;  // base_line 是从底部到基线的距离
    int underlinePosition = -m_fontSize / 8;
    int underlineThickness = m_fontSize / 16;

    // 从字体文件获取 underline 参数
    if (!m_fontPath.isEmpty()) {
        FreeTypeRenderer renderer;
        if (renderer.loadFont(m_fontPath, m_fontSize)) {
            int unitsPerEM = renderer.getUnitsPerEM();
            if (unitsPerEM > 0) {
                double scale = static_cast<double>(m_fontSize) / unitsPerEM;
                // underline_position 使用 ceil（向上取整，对负数向零方向）
                // 例如：-2.8 → -2
                underlinePosition = qCeil(renderer.getUnderlinePosition() * scale);
                underlineThickness = qRound(renderer.getUnderlineThickness() * scale);
            }

            qDebug() << "Font metrics calculated from glyphs:";
            qDebug() << "  maxTop (max box_h + ofs_y):" << maxTop;
            qDebug() << "  minBottom (min ofs_y):" << minBottom;
            qDebug() << "  line_height:" << lineHeight << "pixels (maxTop - minBottom)";
            qDebug() << "  base_line:" << baseLine << "pixels (-minBottom)";
            qDebug() << "  underline_position:" << renderer.getUnderlinePosition()
                     << "font units ->" << underlinePosition << "pixels (ceil)";
            qDebug() << "  underline_thickness:" << renderer.getUnderlineThickness()
                     << "font units ->" << underlineThickness << "pixels";
        }
    }

    out << "    .line_height = " << lineHeight << ",\n";
    out << "    .base_line = " << baseLine << ",\n";
    out << "#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)\n";
    out << "    .subpx = LV_FONT_SUBPX_NONE,\n";
    out << "#endif\n";
    out << "#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8\n";
    out << "    .underline_position = " << underlinePosition << ",\n";
    out << "    .underline_thickness = " << underlineThickness << ",\n";
    out << "#endif\n";
    out << "    .static_bitmap = 0,\n";
    out << "    .dsc = &font_dsc,\n";
    out << "#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9\n";
    out << "    .fallback = NULL,\n";
    out << "#endif\n";
    out << "    .user_data = NULL\n";
    out << "};\n";

    return result;
}

QString LvglExporter::generateKernTables()
{
    QString result;
    QTextStream out(&result);

    out << "/*--------------------\n";
    out << " *  KERNING\n";
    out << " *-------------------*/\n\n";

    // m_glyphs 已经在 generateCFileContent() 中排序过了，直接使用
    int glyphCount = m_glyphs.size() + 1; // +1 for reserved glyph 0

    // 如果有字体路径，尝试提取真实kerning数据
    QMap<QPair<int, int>, int> kernPairs;  // (left_class, right_class) -> kern_value
    bool hasRealKerning = false;

    if (!m_fontPath.isEmpty()) {
        qDebug() << "Loading font for kerning extraction:" << m_fontPath;
        qDebug() << "Font size:" << m_fontSize;

        // 构建字符列表
        QVector<uint32_t> characters;
        for (const GlyphInfo &g : m_glyphs) {
            characters.append(g.unicode);
        }

        // 优先使用 GPOS 提取器（与 CLI 工具完全一致）
        GPOSKerning gposExtractor;
        if (gposExtractor.extractKerning(m_fontPath, m_fontSize, characters)) {
            qDebug() << "Successfully extracted kerning using GPOS (CLI-compatible method)";

            QVector<GPOSKerning::KerningPair> pairs = gposExtractor.getKerningPairs();

            for (const auto &pair : pairs) {
                // left_class = leftIdx+1, right_class = rightIdx+1 (因为0是保留的)
                kernPairs[qMakePair(pair.leftIndex + 1, pair.rightIndex + 1)] = pair.valueLVGL;
            }

            if (!kernPairs.isEmpty()) {
                hasRealKerning = true;
                qDebug() << "Loaded" << kernPairs.size() << "non-zero kerning pairs";

                // 输出前10个用于调试
                int count = 0;
                for (auto it = kernPairs.constBegin(); it != kernPairs.constEnd() && count < 10; ++it, ++count) {
                    int leftIdx = it.key().first - 1;
                    int rightIdx = it.key().second - 1;
                    qDebug() << "  " << QChar(m_glyphs[leftIdx].unicode) << "-"
                             << QChar(m_glyphs[rightIdx].unicode)
                             << ": class(" << (leftIdx+1) << "," << (rightIdx+1) << ") = " << it.value();
                }
            } else {
                qDebug() << "No kerning data available, will set kern_dsc = NULL";
            }
        } else {
            qDebug() << "GPOS kerning extraction failed:" << gposExtractor.lastError();

            // 回退到 HarfBuzz 提取器
            qDebug() << "Falling back to HarfBuzz kerning extractor...";
            HarfBuzzKerning hbExtractor;
            if (hbExtractor.extractKerning(m_fontPath, m_fontSize, characters)) {
            qDebug() << "Successfully extracted kerning using HarfBuzz";

            QVector<HarfBuzzKerning::KerningPair> pairs = hbExtractor.getKerningPairs();

            for (const auto &pair : pairs) {
                // left_class = leftIdx+1, right_class = rightIdx+1 (因为0是保留的)
                kernPairs[qMakePair(pair.leftIndex + 1, pair.rightIndex + 1)] = pair.valueLVGL;
            }

            if (!kernPairs.isEmpty()) {
                hasRealKerning = true;
                qDebug() << "Loaded" << kernPairs.size() << "non-zero kerning pairs";

                // 输出前10个用于调试
                int count = 0;
                for (auto it = kernPairs.constBegin(); it != kernPairs.constEnd() && count < 10; ++it, ++count) {
                    int leftIdx = it.key().first - 1;
                    int rightIdx = it.key().second - 1;
                    qDebug() << "  " << QChar(m_glyphs[leftIdx].unicode) << "-"
                             << QChar(m_glyphs[rightIdx].unicode)
                             << ": class(" << (leftIdx+1) << "," << (rightIdx+1) << ") = " << it.value();
                }
            } else {
                qDebug() << "No kerning data available, will set kern_dsc = NULL";
            }
        } else {
            qDebug() << "HarfBuzz kerning extraction failed:" << hbExtractor.lastError();

            // 回退到 FreeType 提取器
            qDebug() << "Falling back to FreeType kerning extractor...";
            OpenTypeKerning ftExtractor;

            if (ftExtractor.extractKerning(m_fontPath, m_fontSize, characters)) {
                qDebug() << "Successfully extracted kerning using FreeType";

                QVector<OpenTypeKerning::KerningPair> pairs = ftExtractor.getKerningPairs();

                for (const auto &pair : pairs) {
                    kernPairs[qMakePair(pair.leftIndex + 1, pair.rightIndex + 1)] = pair.valueLVGL;
                }

                if (!kernPairs.isEmpty()) {
                    hasRealKerning = true;
                    qDebug() << "Loaded" << kernPairs.size() << "non-zero kerning pairs";
                } else {
                    qDebug() << "No kerning data available, will set kern_dsc = NULL";
                }
            } else {
                qDebug() << "FreeType kerning extraction also failed:" << ftExtractor.lastError();
            }
            }
        }
    } else {
        qDebug() << "Font path is empty, cannot extract kerning data";
    }

    // 如果没有提取到任何有效的 kerning 数据，返回空字符串
    // 这样字体结构中会设置 kern_dsc = NULL（与官方工具一致）
    if (!hasRealKerning || kernPairs.isEmpty()) {
        qDebug() << "No kerning data available, will set kern_dsc = NULL";
        m_hasKerningData = false;

        QString result;
        QTextStream out(&result);
        out << "/*--------------------\n";
        out << " *  KERNING\n";
        out << " *-------------------*/\n\n";
        out << "/* No kerning data */\n\n";
        return result;
    }

    // 有有效的 kerning 数据
    m_hasKerningData = true;
    qDebug() << "Has valid kerning data, will generate kern tables";

    // 构建 unicode 列表（按 m_glyphs 的顺序）
    QList<uint32_t> unicodeList;
    for (const GlyphInfo &g : m_glyphs) {
        unicodeList.append(g.unicode);
    }

    // 使用类优化算法（类似官方工具）
    KerningOptimizer::OptimizedKerning optimized =
        KerningOptimizer::optimize(glyphCount, kernPairs, unicodeList);

    // 生成左侧类映射
    out << "/*Map glyph_ids to kern left classes*/\n";
    out << "static const uint8_t kern_left_class_mapping[] =\n{\n";
    for (int i = 0; i < optimized.leftClassMapping.size(); i++) {
        if (i % 8 == 0 && i > 0) {
            out << "\n";
        }
        if (i % 8 == 0) {
            out << "    ";
        }
        out << optimized.leftClassMapping[i];
        if (i < optimized.leftClassMapping.size() - 1) {
            out << ", ";
        }
    }
    out << "\n};\n\n";

    // 生成右侧类映射
    out << "/*Map glyph_ids to kern right classes*/\n";
    out << "static const uint8_t kern_right_class_mapping[] =\n{\n";
    for (int i = 0; i < optimized.rightClassMapping.size(); i++) {
        if (i % 8 == 0 && i > 0) {
            out << "\n";
        }
        if (i % 8 == 0) {
            out << "    ";
        }
        out << optimized.rightClassMapping[i];
        if (i < optimized.rightClassMapping.size() - 1) {
            out << ", ";
        }
    }
    out << "\n};\n\n";

    // 生成类值数组
    out << "/*Kern values between classes*/\n";
    out << "static const int8_t kern_class_values[] =\n{\n";
    for (int i = 0; i < optimized.classValues.size(); i++) {
        if (i % 8 == 0 && i > 0) {
            out << "\n";
        }
        if (i % 8 == 0) {
            out << "    ";
        }
        out << optimized.classValues[i];
        if (i < optimized.classValues.size() - 1) {
            out << ", ";
        }
    }
    out << "\n};\n\n";

    // 生成kern_classes结构
    out << "/*Collect the kern class' data in one place*/\n";
    out << "static const lv_font_fmt_txt_kern_classes_t kern_classes =\n{\n";
    out << "    .class_pair_values   = kern_class_values,\n";
    out << "    .left_class_mapping  = kern_left_class_mapping,\n";
    out << "    .right_class_mapping = kern_right_class_mapping,\n";
    out << "    .left_class_cnt      = " << optimized.leftClassCount << ",\n";
    out << "    .right_class_cnt     = " << optimized.rightClassCount << "\n";
    out << "};\n\n";

    return result;
}

QByteArray LvglExporter::generateBinaryData()
{
    QByteArray data;

    for (const GlyphInfo &glyph : m_glyphs) {
        const QImage &img = glyph.bitmap;
        for (int y = 0; y < img.height(); y++) {
            for (int x = 0; x < img.width(); x++) {
                int gray = qGray(img.pixel(x, y));
                data.append(static_cast<char>(gray));
            }
        }
    }

    return data;
}

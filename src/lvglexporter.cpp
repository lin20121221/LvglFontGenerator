#include "lvglexporter.h"
#include "freetyperenderer.h"
#include "kerningoptimizer.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

LvglExporter::LvglExporter()
    : m_lvglVersion(9)
    , m_isExternal(false)
    , m_bpp(8)
    , m_enableKerning(false)
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

    return content;
}

QString LvglExporter::generateBitmapArray()
{
    QString result;
    QTextStream out(&result);

    out << "/*Store the image of the glyphs*/\n";
    out << "static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {\n";

    int offset = 0;
    for (const GlyphInfo &glyph : m_glyphs) {
        out << "    /* U+" << QString::number(glyph.unicode, 16).toUpper().rightJustified(4, '0') << " \""
            << QChar(glyph.unicode) << "\" */\n";

        const QImage &img = glyph.bitmap;
        int count = 0;

        // 根据bpp处理像素数据
        if (m_bpp == 1) {
            // 1bpp: 每字节存储8个像素，每像素1位(0-1)
            // 连续打包所有像素，不按行分割
            int pixelIndex = 0;
            int totalPixels = img.width() * img.height();

            while (pixelIndex < totalPixels) {
                if (count % 8 == 0) {
                    out << "    ";
                }

                int packed = 0;
                for (int bit = 0; bit < 8 && pixelIndex < totalPixels; bit++, pixelIndex++) {
                    int y = pixelIndex / img.width();
                    int x = pixelIndex % img.width();
                    int gray = qGray(img.pixel(x, y));
                    int pixel = (gray > 127) ? 1 : 0;
                    packed |= (pixel << (7 - bit));
                }

                out << "0x" << QString::number(packed, 16).rightJustified(2, '0') << ", ";
                count++;
                if (count % 8 == 0) {
                    out << "\n";
                }
            }
        } else if (m_bpp == 2) {
            // 2bpp: 每字节存储4个像素，每像素2位(0-3)
            // 连续打包所有像素，不按行分割
            int pixelIndex = 0;
            int totalPixels = img.width() * img.height();

            while (pixelIndex < totalPixels) {
                if (count % 8 == 0) {
                    out << "    ";
                }

                int packed = 0;
                for (int i = 0; i < 4 && pixelIndex < totalPixels; i++, pixelIndex++) {
                    int y = pixelIndex / img.width();
                    int x = pixelIndex % img.width();
                    int gray = qGray(img.pixel(x, y));
                    int pixel = gray >> 6;
                    packed |= (pixel << (6 - i * 2));
                }

                out << "0x" << QString::number(packed, 16).rightJustified(2, '0') << ", ";
                count++;
                if (count % 8 == 0) {
                    out << "\n";
                }
            }
        } else if (m_bpp == 4) {
            // 4bpp: 每字节存储2个像素，每像素4位(0-15)
            // 连续打包所有像素，不按行分割
            int pixelIndex = 0;
            int totalPixels = img.width() * img.height();

            while (pixelIndex < totalPixels) {
                if (count % 8 == 0) {
                    out << "    ";
                }

                // 计算当前像素的坐标
                int y = pixelIndex / img.width();
                int x = pixelIndex % img.width();

                // 获取第一个像素
                int gray1 = qGray(img.pixel(x, y));
                int pixel1 = gray1 >> 4;

                // 获取第二个像素（如果存在）
                int pixel2 = 0;
                if (pixelIndex + 1 < totalPixels) {
                    int y2 = (pixelIndex + 1) / img.width();
                    int x2 = (pixelIndex + 1) % img.width();
                    int gray2 = qGray(img.pixel(x2, y2));
                    pixel2 = gray2 >> 4;
                }

                // 打包：高4位是第一个像素，低4位是第二个像素
                int packed = (pixel1 << 4) | pixel2;

                out << "0x" << QString::number(packed, 16).rightJustified(2, '0') << ", ";
                count++;
                if (count % 8 == 0) {
                    out << "\n";
                }

                pixelIndex += 2;  // 每次处理2个像素
            }
        } else {
            // 8bpp: 每字节存储1个像素，每像素8位(0-255)
            for (int y = 0; y < img.height(); y++) {
                for (int x = 0; x < img.width(); x++) {
                    if (count % 8 == 0) {
                        out << "    ";
                    }

                    int gray = qGray(img.pixel(x, y));

                    out << "0x" << QString::number(gray, 16).rightJustified(2, '0') << ", ";
                    count++;
                    if (count % 8 == 0) {
                        out << "\n";
                    }
                }
            }
        }

        if (count % 8 != 0) {
            out << "\n";
        }
        out << "\n";

        // 更新偏移量
        int totalPixels = img.width() * img.height();
        if (m_bpp == 1) {
            // 1bpp: 每8个像素占1字节
            offset += (totalPixels + 7) / 8;
        } else if (m_bpp == 2) {
            // 2bpp: 每4个像素占1字节
            offset += (totalPixels + 3) / 4;
        } else if (m_bpp == 4) {
            // 4bpp: 每2个像素占1字节
            offset += (totalPixels + 1) / 2;
        } else {
            // 8bpp: 每像素1字节
            offset += totalPixels;
        }
    }

    out << "};\n\n";
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

        // 根据bpp计算偏移量
        int totalPixels = glyph.boxW * glyph.boxH;
        if (m_bpp == 1) {
            // 1bpp: 每8个像素占1字节
            bitmapOffset += (totalPixels + 7) / 8;
        } else if (m_bpp == 2) {
            // 2bpp: 每4个像素占1字节
            bitmapOffset += (totalPixels + 3) / 4;
        } else if (m_bpp == 4) {
            // 4bpp: 每2个像素占1字节
            bitmapOffset += (totalPixels + 1) / 2;
        } else {
            // 8bpp: 每像素1字节
            bitmapOffset += totalPixels;
        }
    }

    out << "};\n\n";
    return result;
}

QString LvglExporter::generateCmapTables()
{
    QString result;
    QTextStream out(&result);

    if (m_glyphs.isEmpty()) {
        return result;
    }

    // 按unicode排序
    QList<GlyphInfo> sortedGlyphs = m_glyphs;
    std::sort(sortedGlyphs.begin(), sortedGlyphs.end(),
              [](const GlyphInfo &a, const GlyphInfo &b) {
                  return a.unicode < b.unicode;
              });

    // 检查是否是连续的unicode范围
    bool isContinuous = true;
    for (int i = 1; i < sortedGlyphs.size(); i++) {
        if (sortedGlyphs[i].unicode != sortedGlyphs[i-1].unicode + 1) {
            isContinuous = false;
            break;
        }
    }

    out << "/*Character mapping table*/\n";
    out << "static const lv_font_fmt_txt_cmap_t cmaps[] = {\n";
    out << "    {\n";
    out << "        .range_start = " << sortedGlyphs.first().unicode << ",\n";
    out << "        .range_length = " << sortedGlyphs.size() << ",\n";
    out << "        .glyph_id_start = 1,\n";  // 从1开始，因为0是保留的

    if (isContinuous) {
        // 连续范围，使用FORMAT0_TINY
        out << "        .unicode_list = NULL,\n";
        out << "        .glyph_id_ofs_list = NULL,\n";
        out << "        .list_length = 0,\n";
        out << "        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY\n";
    } else {
        // 非连续范围，使用SPARSE_TINY
        out << "        .unicode_list = unicode_list,\n";
        out << "        .glyph_id_ofs_list = NULL,\n";
        out << "        .list_length = " << sortedGlyphs.size() << ",\n";
        out << "        .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY\n";

        // 需要生成unicode_list
        QString unicodeList;
        QTextStream listOut(&unicodeList);
        listOut << "static const uint16_t unicode_list[] = {\n    ";
        int count = 0;
        for (const GlyphInfo &glyph : sortedGlyphs) {
            listOut << "0x" << QString::number(glyph.unicode, 16) << ", ";
            if (++count % 8 == 0) {
                listOut << "\n    ";
            }
        }
        listOut << "\n};\n\n";
        result = unicodeList + result;
    }

    out << "    }\n";
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
    out << "    .kern_dsc = " << (m_enableKerning ? "&kern_classes" : "NULL") << ",\n";
    out << "    .kern_scale = " << (m_enableKerning ? "16" : "0") << ",\n";
    out << "    .cmap_num = 1,\n";
    out << "    .bpp = " << m_bpp << ",\n";
    out << "    .kern_classes = " << (m_enableKerning ? "1" : "0") << ",\n";
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

    // 使用最大字形高度作为 line_height
    int maxHeight = 0;
    for (const GlyphInfo &glyph : m_glyphs) {
        if (glyph.boxH > maxHeight) {
            maxHeight = glyph.boxH;
        }
    }

    out << "    .line_height = " << maxHeight << ",\n";
    out << "    .base_line = 0,\n";
    out << "#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)\n";
    out << "    .subpx = LV_FONT_SUBPX_NONE,\n";
    out << "#endif\n";
    out << "#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8\n";
    out << "    .underline_position = " << (-m_fontSize / 8) << ",\n";
    out << "    .underline_thickness = " << (m_fontSize / 16) << ",\n";
    out << "#endif\n";
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

    // 按unicode排序字形
    QList<GlyphInfo> sortedGlyphs = m_glyphs;
    std::sort(sortedGlyphs.begin(), sortedGlyphs.end(),
              [](const GlyphInfo &a, const GlyphInfo &b) {
                  return a.unicode < b.unicode;
              });

    int glyphCount = sortedGlyphs.size() + 1; // +1 for reserved glyph 0

    // 如果有字体路径，尝试提取真实kerning数据
    QMap<QPair<int, int>, int> kernPairs;  // (left_class, right_class) -> kern_value
    bool hasRealKerning = false;

    if (!m_fontPath.isEmpty()) {
        qDebug() << "Loading font for kerning extraction:" << m_fontPath;
        qDebug() << "Font size:" << m_fontSize;

        // 方案1: 尝试使用 Node.js + opentype.js 提取 kerning（支持 GPOS 表）
        QString nodeScriptPath = QCoreApplication::applicationDirPath() + "/extract_kerning.js";
        QFileInfo scriptInfo(nodeScriptPath);

        if (scriptInfo.exists()) {
            qDebug() << "Trying to extract kerning using Node.js + opentype.js...";

            // 构建字符列表
            QString charList;
            for (const GlyphInfo &g : sortedGlyphs) {
                charList += QChar(g.unicode);
            }

            // 调用 Node.js 脚本
            QProcess process;
            QStringList args;
            args << nodeScriptPath << m_fontPath << QString::number(m_fontSize) << charList;

            process.start("node", args);
            if (process.waitForFinished(30000)) { // 30秒超时
                if (process.exitCode() == 0) {
                    // 读取生成的 JSON 文件
                    QString jsonPath = m_fontPath;
                    int lastDot = jsonPath.lastIndexOf('.');
                    if (lastDot != -1) {
                        jsonPath = jsonPath.left(lastDot);
                    }
                    jsonPath += "_kerning.json";

                    QFile jsonFile(jsonPath);
                    if (jsonFile.open(QIODevice::ReadOnly)) {
                        QByteArray jsonData = jsonFile.readAll();
                        jsonFile.close();

                        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
                        QJsonObject root = doc.object();
                        QJsonArray pairs = root["kerningPairs"].toArray();

                        qDebug() << "Successfully extracted" << pairs.size() << "kerning pairs using opentype.js";

                        for (const QJsonValue &pairVal : pairs) {
                            QJsonObject pair = pairVal.toObject();
                            int leftIdx = pair["leftIndex"].toInt();
                            int rightIdx = pair["rightIndex"].toInt();
                            int valueLVGL = pair["valueLVGL"].toInt();

                            // left_class = leftIdx+1, right_class = rightIdx+1 (因为0是保留的)
                            kernPairs[qMakePair(leftIdx + 1, rightIdx + 1)] = valueLVGL;
                        }

                        if (!kernPairs.isEmpty()) {
                            hasRealKerning = true;
                            qDebug() << "Loaded" << kernPairs.size() << "non-zero kerning pairs from JSON";

                            // 输出前10个用于调试
                            int count = 0;
                            for (auto it = kernPairs.constBegin(); it != kernPairs.constEnd() && count < 10; ++it, ++count) {
                                int leftIdx = it.key().first - 1;
                                int rightIdx = it.key().second - 1;
                                qDebug() << "  " << QChar(sortedGlyphs[leftIdx].unicode) << "-"
                                         << QChar(sortedGlyphs[rightIdx].unicode)
                                         << ": class(" << (leftIdx+1) << "," << (rightIdx+1) << ") = " << it.value();
                            }
                        }

                        // 暂时不删除 JSON 文件，用于调试
                        // QFile::remove(jsonPath);
                        qDebug() << "Kerning JSON saved at:" << jsonPath;
                    } else {
                        qDebug() << "Failed to read kerning JSON file:" << jsonPath;
                    }
                } else {
                    qDebug() << "Node.js script failed:" << process.readAllStandardError();
                }
            } else {
                qDebug() << "Node.js script timeout or failed to start";
            }
        }

        // 方案2: 如果 Node.js 方案失败，回退到 FreeType（仅支持传统 kern 表）
        if (!hasRealKerning) {
            qDebug() << "Falling back to FreeType for kerning extraction...";

            FreeTypeRenderer renderer;
            if (renderer.loadFont(m_fontPath, m_fontSize)) {
                qDebug() << "Font loaded successfully";
                qDebug() << "Extracting kerning data (checking all character pairs)...";

                // 提取所有字符对的kerning值
                // 注意：FT_Get_Kerning 可能无法从 GPOS 表中提取 kerning
                int nonZeroCount = 0;
                for (int i = 0; i < sortedGlyphs.size(); i++) {
                    for (int j = 0; j < sortedGlyphs.size(); j++) {
                        int kernValue = renderer.getKerning(
                            sortedGlyphs[i].unicode,
                            sortedGlyphs[j].unicode
                        );

                        if (kernValue != 0) {
                            // 使用简化的类映射：每个字符一个类
                            // left_class = i+1, right_class = j+1 (因为0是保留的)
                            kernPairs[qMakePair(i + 1, j + 1)] = kernValue;
                            nonZeroCount++;

                            // 调试：输出前10个非零kerning对
                            if (nonZeroCount <= 10) {
                                qDebug() << "  " << QChar(sortedGlyphs[i].unicode) << "-" << QChar(sortedGlyphs[j].unicode)
                                         << ": class(" << (i+1) << "," << (j+1) << ") = " << kernValue;
                            }
                        }
                    }
                }

                if (nonZeroCount > 0) {
                    qDebug() << "Extracted" << nonZeroCount << "non-zero kerning pairs";
                    hasRealKerning = true;
                } else {
                    qDebug() << "No kerning data found (all pairs returned 0)";
                }
            } else {
                qDebug() << "Failed to load font:" << renderer.lastError();
            }
        }
    } else {
        qDebug() << "Font path is empty, cannot extract kerning data";
    }

    // 构建 unicode 列表（按 sortedGlyphs 的顺序）
    QList<uint32_t> unicodeList;
    for (const GlyphInfo &g : sortedGlyphs) {
        unicodeList.append(g.unicode);
    }

    // 使用类优化算法（类似官方工具）
    KerningOptimizer::OptimizedKerning optimized =
        KerningOptimizer::optimize(glyphCount, kernPairs, unicodeList);

    // 生成左侧类映射
    out << "/*Map glyph_ids to kern left classes*/\n";
    out << "static const uint8_t kern_left_class_mapping[] =\n{\n";
    for (int i = 0; i < optimized.leftClassMapping.size(); i++) {
        if (i % 16 == 0) {
            out << "    ";
        }
        out << optimized.leftClassMapping[i];
        if (i < optimized.leftClassMapping.size() - 1) {
            out << ", ";
        }
        if ((i + 1) % 16 == 0 && i < optimized.leftClassMapping.size() - 1) {
            out << "\n";
        }
    }
    out << "\n};\n\n";

    // 生成右侧类映射
    out << "/*Map glyph_ids to kern right classes*/\n";
    out << "static const uint8_t kern_right_class_mapping[] =\n{\n";
    for (int i = 0; i < optimized.rightClassMapping.size(); i++) {
        if (i % 16 == 0) {
            out << "    ";
        }
        out << optimized.rightClassMapping[i];
        if (i < optimized.rightClassMapping.size() - 1) {
            out << ", ";
        }
        if ((i + 1) % 16 == 0 && i < optimized.rightClassMapping.size() - 1) {
            out << "\n";
        }
    }
    out << "\n};\n\n";

    // 生成类值数组
    out << "/*Kern values between classes*/\n";
    out << "static const int8_t kern_class_values[] =\n{\n";
    for (int i = 0; i < optimized.classValues.size(); i++) {
        if (i % 16 == 0) {
            out << "    ";
        }
        out << optimized.classValues[i];
        if (i < optimized.classValues.size() - 1) {
            out << ", ";
        }
        if ((i + 1) % 16 == 0 && i < optimized.classValues.size() - 1) {
            out << "\n";
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
